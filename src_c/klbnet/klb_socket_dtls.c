// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_dtls.c
/// @brief   DTLS over UDP 会话实现 (mbedTLS datagram 传输 + 重传定时器)
/// @version 0.4
/// @history 修改历史
///  \n 2026 0.1 创建文件(桩)
///  \n [2026-10] 0.2 实现非阻塞握手 / 分片 / cookie / 证书指纹
///  \n [2026-10] 0.3 实现 DTLS-SRTP: use_srtp 协商 + EXTRACTOR-dtls_srtp 密钥导出
///  \n [2026-10] 0.4 增加对端方向密钥导出 klb_socket_dtls_srtp_export_peer
/// @warning 服务端一段会话绑定一个 UDP fd; 未 connect 的 fd 以首个报文源地址为对端
///////////////////////////////////////////////////////////////////////////
#include "klbnet/klb_socket_dtls.h"
#include "klbnet/klb_netcode.h"
#include "klbplatform/klb_time.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <string.h>

#if defined(__KLB_MBEDTLS__)

// 仅本编译单元读取 mbedtls_dtls_srtp_info_t 的私有成员名 (结构布局不变)
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"

#ifndef _WIN32
#include <errno.h>
#endif

/// @def   KLB_DTLS_IO_TID
/// @brief io 模式服务端固定的 client transport id (单会话 P2P)
#define KLB_DTLS_IO_TID             "klb-dtls-io"

/// @def   KLB_DTLS_MTU
/// @brief DTLS 会话 MTU; 对齐 WebRTC 常用 1200
#define KLB_DTLS_MTU                1200

/// @def   KLB_DTLS_FP_PREFIX
/// @brief SDP fingerprint 属性前缀
#define KLB_DTLS_FP_PREFIX         "sha-256 "

/// @def   KLB_DTLS_FP_SHA256_LEN
/// @brief SHA-256 摘要字节数
#define KLB_DTLS_FP_SHA256_LEN      32

/// @def   KLB_DTLS_SRTP_KEY_LEN
/// @brief SRTP 主密钥长度 (AES-128)
#define KLB_DTLS_SRTP_KEY_LEN       16

/// @def   KLB_DTLS_SRTP_SALT_LEN
/// @brief SRTP 主盐长度
#define KLB_DTLS_SRTP_SALT_LEN      14

/// @def   KLB_DTLS_SRTP_MATERIAL_LEN
/// @brief RFC 5764 导出材料长度: client_key + server_key + client_salt + server_salt
#define KLB_DTLS_SRTP_MATERIAL_LEN  (KLB_DTLS_SRTP_KEY_LEN * 2 + KLB_DTLS_SRTP_SALT_LEN * 2)

/// @def   KLB_DTLS_SRTP_LABEL
/// @brief RFC 5764 4.2 节 密钥导出标签
#define KLB_DTLS_SRTP_LABEL         "EXTRACTOR-dtls_srtp"


typedef struct klb_socket_dtls_t_
{
    mbedtls_ssl_context         ssl;
    mbedtls_ssl_config          conf;
    mbedtls_ctr_drbg_context    ctr_drbg;
    mbedtls_entropy_context     entropy;
    mbedtls_x509_crt            ca;
    mbedtls_x509_crt            own_cert;           ///< 本端证书; 须与 conf 同生命周期
    mbedtls_pk_context          own_key;            ///< 本端私钥; 须与 conf 同生命周期
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_ctx      cookie;
#endif
    uint32_t                    timer_start_ms;     ///< 重传定时器起点
    uint32_t                    timer_int_ms;       ///< 中间超时
    uint32_t                    timer_fin_ms;       ///< 最终超时; 0 表示取消
    bool                        handshake_done;
    bool                        is_server;
    bool                        has_peer;           ///< 是否已确定对端地址
    bool                        transport_set;      ///< 服务端是否已设 client transport id
    bool                        keys_exported;      ///< 是否已抓到 TLS1.2 主密钥
    unsigned char               master_secret[48];  ///< TLS1.2 主密钥
    size_t                      master_secret_len;  ///< 主密钥长度
    unsigned char               client_random[32];  ///< client_random
    unsigned char               server_random[32];  ///< server_random
    mbedtls_tls_prf_types       prf_type;           ///< 协商的 PRF 类型
    klb_socket_dtls_io_t        io;                 ///< 抽象传输回调 (io 模式有效)
    bool                        io_mode;            ///< 是否使用抽象传输 (不持有 fd)
}klb_socket_dtls_t;

//////////////////////////////////////////////////////////////////////////

#if defined(MBEDTLS_SSL_DTLS_SRTP)
/// @brief 支持的 SRTP 保护套件; 以 UNSET 结束, 生命周期同 conf
static const mbedtls_ssl_srtp_profile s_dtls_srtp_profiles[] =
{
    MBEDTLS_TLS_SRTP_AES128_CM_HMAC_SHA1_80,
    MBEDTLS_TLS_SRTP_UNSET,
};
#endif

/// @brief mbedTLS 密钥导出回调; 抓取 TLS1.2 主密钥与双方随机数
static void export_keys_klb_socket_dtls(void* p_expkey, mbedtls_ssl_key_export_type type,
                                        const unsigned char* secret, size_t secret_len,
                                        const unsigned char client_random[32],
                                        const unsigned char server_random[32],
                                        mbedtls_tls_prf_types tls_prf_type)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_expkey;

    if ((MBEDTLS_SSL_KEY_EXPORT_TLS12_MASTER_SECRET != type) || (NULL == secret))
    {
        return;
    }

    if (secret_len > sizeof(p_dtls->master_secret))
    {
        return;
    }

    memcpy(p_dtls->master_secret, secret, secret_len);
    p_dtls->master_secret_len = secret_len;
    memcpy(p_dtls->client_random, client_random, sizeof(p_dtls->client_random));
    memcpy(p_dtls->server_random, server_random, sizeof(p_dtls->server_random));
    p_dtls->prf_type = tls_prf_type;
    p_dtls->keys_exported = true;
}

static bool wouldblock_klb_socket_dtls(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();
    if ((WSAEWOULDBLOCK == err) || (WSAEINTR == err))
    {
        return true;
    }
#else
    if ((EAGAIN == errno) || (EWOULDBLOCK == errno) || (EINTR == errno))
    {
        return true;
    }
#endif

    return false;
}

static void set_timer_klb_socket_dtls(void* ctx, uint32_t int_ms, uint32_t fin_ms)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)ctx;

    p_dtls->timer_start_ms = klb_tick_count();
    p_dtls->timer_int_ms = int_ms;
    p_dtls->timer_fin_ms = fin_ms;
}

static int get_timer_klb_socket_dtls(void* ctx)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)ctx;

    if (0 == p_dtls->timer_fin_ms)
    {
        return -1;
    }

    uint32_t elapsed = klb_tick_count() - p_dtls->timer_start_ms;
    if (elapsed >= p_dtls->timer_fin_ms)
    {
        return 2;
    }

    if ((0 != p_dtls->timer_int_ms) && (elapsed >= p_dtls->timer_int_ms))
    {
        return 1;
    }

    return 0;
}

static int bio_send_klb_socket_dtls(void* ctx, const unsigned char* buf, size_t len)
{
    klb_socket_t* p_socket = (klb_socket_t*)ctx;
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;

    int nlen = (int)len;
    if (len > (size_t)0x7FFFFFFF)
    {
        nlen = 0x7FFFFFFF;
    }

    if (p_dtls->io_mode)
    {
        int io_ret = p_dtls->io.send(p_dtls->io.p_ctx, (const uint8_t*)buf, nlen);

        return (0 < io_ret) ? io_ret : MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    int ret = -1;
    if (0 != p_socket->connected)
    {
#ifdef _WIN32
        ret = send(p_socket->fd, (const char*)buf, nlen, 0);
#else
        ret = send(p_socket->fd, (const char*)buf, nlen, MSG_DONTWAIT | MSG_NOSIGNAL);
#endif
    }
    else if (p_dtls->has_peer)
    {
#ifdef _WIN32
        ret = sendto(p_socket->fd, (const char*)buf, nlen, 0,
            &p_socket->peer.addr.sa, (int)p_socket->peer.len);
#else
        ret = sendto(p_socket->fd, (const char*)buf, nlen, MSG_DONTWAIT | MSG_NOSIGNAL,
            &p_socket->peer.addr.sa, p_socket->peer.len);
#endif
    }
    else
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    if (0 <= ret)
    {
        return ret;
    }

    if (wouldblock_klb_socket_dtls())
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    return ret;
}

static int bio_recv_klb_socket_dtls(void* ctx, unsigned char* buf, size_t len)
{
    klb_socket_t* p_socket = (klb_socket_t*)ctx;
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;

    int nlen = (int)len;
    if (len > (size_t)0x7FFFFFFF)
    {
        nlen = 0x7FFFFFFF;
    }

    if (p_dtls->io_mode)
    {
        int io_ret = p_dtls->io.recv(p_dtls->io.p_ctx, (uint8_t*)buf, nlen);

        return (0 < io_ret) ? io_ret : MBEDTLS_ERR_SSL_WANT_READ;
    }

    int ret = -1;
    if (0 != p_socket->connected)
    {
#ifdef _WIN32
        ret = recv(p_socket->fd, (char*)buf, nlen, 0);
#else
        ret = recv(p_socket->fd, (char*)buf, nlen, MSG_DONTWAIT);
#endif
    }
    else
    {
        struct sockaddr_storage sa;
        memset(&sa, 0, sizeof(sa));

        socklen_t sa_len = (socklen_t)sizeof(sa);
#ifdef _WIN32
        int from_len = (int)sizeof(sa);
        ret = recvfrom(p_socket->fd, (char*)buf, nlen, 0,
            (struct sockaddr*)&sa, &from_len);
        sa_len = (socklen_t)from_len;
#else
        ret = recvfrom(p_socket->fd, (char*)buf, nlen, MSG_DONTWAIT,
            (struct sockaddr*)&sa, &sa_len);
#endif

        if (0 <= ret)
        {
            memcpy(&p_socket->peer.addr, &sa, (size_t)sa_len);
            p_socket->peer.len = sa_len;
            p_dtls->has_peer = true;
        }
    }

    if (0 <= ret)
    {
        return ret;
    }

    if (wouldblock_klb_socket_dtls())
    {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return ret;
}

static void apply_want_klb_socket_dtls(klb_socket_t* p_socket, int mbedtls_ret)
{
    if (MBEDTLS_ERR_SSL_WANT_READ == mbedtls_ret)
    {
        p_socket->status_rw = KLB_SOCKET_WAIT_READ;
        klb_socket_set_reading(p_socket, true);
    }
    else if (MBEDTLS_ERR_SSL_WANT_WRITE == mbedtls_ret)
    {
        p_socket->status_rw = KLB_SOCKET_WAIT_WRITE;
        klb_socket_set_writing(p_socket, true);
    }
}

/// @brief 服务端: 窥探首个报文以确定对端地址 (不消费数据)
/// @return int 0.已确定对端; 1.暂无报文; 小于 0.klb_netcode_e
static int peek_peer_klb_socket_dtls(klb_socket_t* p_socket)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;

    if (p_dtls->has_peer)
    {
        return 0;
    }

    if (0 != p_socket->connected)
    {
        p_dtls->has_peer = (0 < p_socket->peer.len) ? true : false;
        return p_dtls->has_peer ? 0 : KLB_NETCODE_NOTCONN;
    }

    uint8_t dummy = 0;
    struct sockaddr_storage sa;
    memset(&sa, 0, sizeof(sa));

    socklen_t sa_len = (socklen_t)sizeof(sa);
#ifdef _WIN32
    int from_len = (int)sizeof(sa);
    int ret = recvfrom(p_socket->fd, (char*)&dummy, 1, MSG_PEEK,
        (struct sockaddr*)&sa, &from_len);
    sa_len = (socklen_t)from_len;
#else
    int ret = recvfrom(p_socket->fd, &dummy, 1, MSG_PEEK | MSG_DONTWAIT,
        (struct sockaddr*)&sa, &sa_len);
#endif

    if (0 > ret)
    {
        if (wouldblock_klb_socket_dtls())
        {
            return 1;
        }

        return KLB_NETCODE_IO;
    }

    memcpy(&p_socket->peer.addr, &sa, (size_t)sa_len);
    p_socket->peer.len = sa_len;
    p_dtls->has_peer = true;

    return 0;
}

static int handshake_klb_socket_dtls(klb_socket_t* p_socket)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    const unsigned char* p_tid = (const unsigned char*)&p_socket->peer.addr.sa;
    size_t tid_len = (size_t)p_socket->peer.len;

    if (p_dtls->io_mode)
    {
        p_tid = (const unsigned char*)KLB_DTLS_IO_TID;
        tid_len = sizeof(KLB_DTLS_IO_TID) - 1;
    }

    if (p_dtls->handshake_done)
    {
        p_socket->status_rw = KLB_SOCKET_RW_OK;
        return 0;
    }

    if (p_dtls->is_server && !p_dtls->transport_set)
    {
        if (!p_dtls->io_mode)
        {
            int prc = peek_peer_klb_socket_dtls(p_socket);
            if (1 == prc)
            {
                p_socket->status_rw = KLB_SOCKET_WAIT_READ;
                klb_socket_set_reading(p_socket, true);
                return 1;
            }
            else if (0 != prc)
            {
                p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
                return -1;
            }

            p_tid = (const unsigned char*)&p_socket->peer.addr.sa;
            tid_len = (size_t)p_socket->peer.len;
        }

        int src = mbedtls_ssl_set_client_transport_id(&p_dtls->ssl, p_tid, tid_len);
        if (0 != src)
        {
            p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
            return -1;
        }

        p_dtls->transport_set = true;
    }

    int ret = mbedtls_ssl_handshake(&p_dtls->ssl);
    if (0 == ret)
    {
        p_dtls->handshake_done = true;
        p_dtls->timer_fin_ms = 0;
        p_socket->status_rw = KLB_SOCKET_RW_OK;
        klb_socket_set_connected(p_socket, true);
        return 0;
    }
    else if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        apply_want_klb_socket_dtls(p_socket, ret);
        return 1;
    }
    else if (MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED == ret)
    {
        // 首个 ClientHello 无 cookie: HelloVerifyRequest 已发出; 重置会话等待带 cookie 的 ClientHello
        int rr = mbedtls_ssl_session_reset(&p_dtls->ssl);
        if (0 != rr)
        {
            p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
            return -1;
        }

        rr = mbedtls_ssl_set_client_transport_id(&p_dtls->ssl, p_tid, tid_len);
        if (0 != rr)
        {
            p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
            return -1;
        }

        p_socket->status_rw = KLB_SOCKET_WAIT_READ;
        klb_socket_set_reading(p_socket, true);
        return 1;
    }
    else if ((MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS == ret) || (MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == ret))
    {
        return 1;
    }
    else if (MBEDTLS_ERR_SSL_TIMEOUT == ret)
    {
        p_socket->status = KLB_SOCKET_TIMEOUT;
        return -1;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static void quit_klb_socket_dtls(klb_socket_dtls_t* p_dtls)
{
    mbedtls_ssl_free(&p_dtls->ssl);
    mbedtls_ssl_config_free(&p_dtls->conf);
    mbedtls_ctr_drbg_free(&p_dtls->ctr_drbg);
    mbedtls_entropy_free(&p_dtls->entropy);
    mbedtls_x509_crt_free(&p_dtls->ca);
    mbedtls_x509_crt_free(&p_dtls->own_cert);
    mbedtls_pk_free(&p_dtls->own_key);
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_free(&p_dtls->cookie);
#endif
}

static void destroy_klb_socket_dtls(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;

    if (p_dtls->handshake_done)
    {
        mbedtls_ssl_close_notify(&p_dtls->ssl);
    }

    quit_klb_socket_dtls(p_dtls);

    if (p_dtls->io_mode)
    {
        if (NULL != p_dtls->io.close)
        {
            p_dtls->io.close(p_dtls->io.p_ctx);
        }
    }
    else
    {
        KLB_SOCKET_CLOSE(p_socket->fd);
    }

    KLB_FREE(p_socket);
}

static bool is_dtls_klb_socket_dtls(klb_socket_t* p_socket)
{
    return (destroy_klb_socket_dtls == p_socket->vtable.cb_destroy) ? true : false;
}

static int send_klb_socket_dtls(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    int hs = handshake_klb_socket_dtls(p_socket);
    if (0 != hs)
    {
        if (0 > hs)
        {
            return -1;
        }

        return 0;
    }

    if ((NULL == p_data) || (0 >= len))
    {
        return 0;
    }

    int ret = mbedtls_ssl_write(&p_dtls->ssl, p_data, (size_t)len);
    if (0 < ret)
    {
        return ret;
    }
    else if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        apply_want_klb_socket_dtls(p_socket, ret);
        return 0;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static int recv_klb_socket_dtls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    int hs = handshake_klb_socket_dtls(p_socket);
    if (0 != hs)
    {
        if (0 > hs)
        {
            return -1;
        }

        return 0;
    }

    int ret = mbedtls_ssl_read(&p_dtls->ssl, p_buf, (size_t)buf_len);
    if (0 < ret)
    {
        return ret;
    }
    else if (0 == ret)
    {
        return 0;
    }
    else if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        apply_want_klb_socket_dtls(p_socket, ret);
        return 0;
    }
    else if (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == ret)
    {
        return 0;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static int recv_peek_klb_socket_dtls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, MSG_PEEK);
#else
    int ret = recv(p_socket->fd, p_buf, buf_len, MSG_PEEK | MSG_DONTWAIT);
#endif

    return ret;
}

static int sendto_klb_socket_dtls(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
{
    (void)p_socket;
    (void)p_data;
    (void)len;
    (void)p_addr;
    (void)addr_len;
    assert(false);
    return -1;
}

static int recvfrom_klb_socket_dtls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
    (void)p_socket;
    (void)p_buf;
    (void)buf_len;
    (void)p_addr;
    (void)p_addr_len;
    assert(false);
    return -1;
}

static size_t pem_buflen_klb_socket_dtls(const char* p_pem)
{
    return strlen(p_pem) + 1;
}

/// @brief 初始化 mbedTLS DTLS 上下文
/// @return int 0.成功; 小于 0.klb_netcode_e
static int init_klb_socket_dtls(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, bool is_server)
{
    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    p_dtls->is_server = is_server;

    mbedtls_ssl_init(&p_dtls->ssl);
    mbedtls_ssl_config_init(&p_dtls->conf);
    mbedtls_ctr_drbg_init(&p_dtls->ctr_drbg);
    mbedtls_entropy_init(&p_dtls->entropy);
    mbedtls_x509_crt_init(&p_dtls->ca);
    mbedtls_x509_crt_init(&p_dtls->own_cert);
    mbedtls_pk_init(&p_dtls->own_key);
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_init(&p_dtls->cookie);
#endif

    static const unsigned char pers[] = "klb-dtls";
    int ret = mbedtls_ctr_drbg_seed(&p_dtls->ctr_drbg, mbedtls_entropy_func,
        &p_dtls->entropy, pers, sizeof(pers) - 1);
    if (0 != ret)
    {
        return KLB_NETCODE_IO;
    }

    ret = mbedtls_ssl_config_defaults(&p_dtls->conf,
        is_server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
        MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (0 != ret)
    {
        return KLB_NETCODE_IO;
    }

    mbedtls_ssl_conf_rng(&p_dtls->conf, mbedtls_ctr_drbg_random, &p_dtls->ctr_drbg);

    // step1. 对端校验策略; 未提供 CA 时沿用设备侧不校验
    if ((NULL != p_cfg) && (0 != p_cfg->verify_peer) && (NULL != p_cfg->p_ca_pem))
    {
        ret = mbedtls_x509_crt_parse(&p_dtls->ca, (const unsigned char*)p_cfg->p_ca_pem,
            pem_buflen_klb_socket_dtls(p_cfg->p_ca_pem));
        if (0 != ret)
        {
            return KLB_NETCODE_TLS_CERT;
        }

        mbedtls_ssl_conf_ca_chain(&p_dtls->conf, &p_dtls->ca, NULL);
        mbedtls_ssl_conf_authmode(&p_dtls->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    }
    else
    {
        mbedtls_ssl_conf_authmode(&p_dtls->conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    // step2. 本端证书/私钥 (服务端必需); 存于 extra 与 conf 同生命周期
    bool has_own_cert = false;
    if ((NULL != p_cfg) && (NULL != p_cfg->p_cert_pem) && (NULL != p_cfg->p_key_pem))
    {
        ret = mbedtls_x509_crt_parse(&p_dtls->own_cert, (const unsigned char*)p_cfg->p_cert_pem,
            pem_buflen_klb_socket_dtls(p_cfg->p_cert_pem));
        if (0 != ret)
        {
            return KLB_NETCODE_TLS_CERT;
        }

        ret = mbedtls_pk_parse_key(&p_dtls->own_key, (const unsigned char*)p_cfg->p_key_pem,
            pem_buflen_klb_socket_dtls(p_cfg->p_key_pem), NULL, 0,
            mbedtls_ctr_drbg_random, &p_dtls->ctr_drbg);
        if (0 != ret)
        {
            return KLB_NETCODE_TLS_CERT;
        }

        ret = mbedtls_ssl_conf_own_cert(&p_dtls->conf, &p_dtls->own_cert, &p_dtls->own_key);
        if (0 != ret)
        {
            return KLB_NETCODE_IO;
        }

        has_own_cert = true;
    }

    if (is_server && !has_own_cert)
    {
        return KLB_NETCODE_INVAL;
    }

    // step3. cookie (服务端 HelloVerify); 默认 config 的回调为 dummy
#if defined(MBEDTLS_SSL_COOKIE_C)
    if (is_server)
    {
        ret = mbedtls_ssl_cookie_setup(&p_dtls->cookie, mbedtls_ctr_drbg_random, &p_dtls->ctr_drbg);
        if (0 != ret)
        {
            return KLB_NETCODE_IO;
        }

        mbedtls_ssl_conf_dtls_cookies(&p_dtls->conf, mbedtls_ssl_cookie_write,
            mbedtls_ssl_cookie_check, &p_dtls->cookie);
    }
#endif

#if defined(MBEDTLS_SSL_DTLS_SRTP)
    // step4. DTLS-SRTP 保护套件; 双方都配置后 use_srtp 才会协商成功
    ret = mbedtls_ssl_conf_dtls_srtp_protection_profiles(&p_dtls->conf, s_dtls_srtp_profiles);
    if (0 != ret)
    {
        return KLB_NETCODE_NOTSUP;
    }
#endif

    ret = mbedtls_ssl_setup(&p_dtls->ssl, &p_dtls->conf);
    if (0 != ret)
    {
        return KLB_NETCODE_IO;
    }

    mbedtls_ssl_set_mtu(&p_dtls->ssl, KLB_DTLS_MTU);
    mbedtls_ssl_set_bio(&p_dtls->ssl, p_socket, bio_send_klb_socket_dtls, bio_recv_klb_socket_dtls, NULL);
    mbedtls_ssl_set_timer_cb(&p_dtls->ssl, p_dtls, set_timer_klb_socket_dtls, get_timer_klb_socket_dtls);
    mbedtls_ssl_set_export_keys_cb(&p_dtls->ssl, export_keys_klb_socket_dtls, p_dtls);

    return 0;
}

static klb_socket_t* open_klb_socket_dtls(klb_socket_t* p_udp, const klb_socket_dtls_cfg_t* p_cfg,
                                          bool is_server, int* p_code)
{
    assert(NULL != p_udp);
    if ((NULL == p_udp) || (INVALID_SOCKET == p_udp->fd))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    if (is_server && (NULL == p_cfg))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    // step1. 分配新 socket + DTLS extra; 共享 fd 直至 init 成功
    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, sizeof(klb_socket_dtls_t));

    p_socket->vtable.cb_destroy = destroy_klb_socket_dtls;
    p_socket->vtable.cb_send = send_klb_socket_dtls;
    p_socket->vtable.cb_recv = recv_klb_socket_dtls;
    p_socket->vtable.cb_recv_peek = recv_peek_klb_socket_dtls;
    p_socket->vtable.cb_sendto = sendto_klb_socket_dtls;
    p_socket->vtable.cb_recvfrom = recvfrom_klb_socket_dtls;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = p_udp->fd;
    p_socket->nonblock = 0x1;
    p_socket->connected = p_udp->connected;
    p_socket->family = p_udp->family;
    p_socket->type = p_udp->type;
    memcpy(&p_socket->local, &p_udp->local, sizeof(klb_socket_addr_t));
    memcpy(&p_socket->peer, &p_udp->peer, sizeof(klb_socket_addr_t));
    klb_socket_set_tls(p_socket, true);

    // step2. mbedTLS DTLS 初始化; 失败不接管 fd
    int rc = init_klb_socket_dtls(p_socket, p_cfg, is_server);
    if (0 != rc)
    {
        klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
        quit_klb_socket_dtls(p_dtls);
        KLB_FREE(p_socket);

        if (NULL != p_code)
        {
            *p_code = rc;
        }

        return NULL;
    }

    // step3. 接管 fd 并释放原 UDP socket 包装
    klb_socket_set_block(p_socket->fd, false);
    klb_socket_detach_fd(p_udp);            // 置 p_udp->fd = INVALID; 避免 destroy 关闭已接管 fd
    klb_socket_destroy(p_udp);

    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    if ((0 != p_socket->connected) && (0 < p_socket->peer.len))
    {
        p_dtls->has_peer = true;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_socket;
}

//////////////////////////////////////////////////////////////////////////

static klb_socket_t* open_io_klb_socket_dtls(bool is_server, const klb_socket_dtls_io_t* p_io,
                                             const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    if ((NULL == p_io) || (NULL == p_io->send) || (NULL == p_io->recv))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    if (is_server && (NULL == p_cfg))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, sizeof(klb_socket_dtls_t));
    if (NULL == p_socket)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_NOMEM;
        }

        return NULL;
    }

    p_socket->vtable.cb_destroy = destroy_klb_socket_dtls;
    p_socket->vtable.cb_send = send_klb_socket_dtls;
    p_socket->vtable.cb_recv = recv_klb_socket_dtls;
    p_socket->vtable.cb_recv_peek = recv_peek_klb_socket_dtls;
    p_socket->vtable.cb_sendto = sendto_klb_socket_dtls;
    p_socket->vtable.cb_recvfrom = recvfrom_klb_socket_dtls;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = INVALID_SOCKET;
    p_socket->nonblock = 0x1;
    p_socket->connected = 0x1;
    p_socket->family = AF_INET;
    p_socket->type = SOCK_DGRAM;
    klb_socket_set_tls(p_socket, true);

    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    p_dtls->io = *p_io;
    p_dtls->io_mode = true;
    p_dtls->has_peer = true;

    int rc = init_klb_socket_dtls(p_socket, p_cfg, is_server);
    if (0 != rc)
    {
        quit_klb_socket_dtls(p_dtls);
        KLB_FREE(p_socket);

        if (NULL != p_code)
        {
            *p_code = rc;
        }

        return NULL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_socket;
}

//////////////////////////////////////////////////////////////////////////

klb_socket_t* klb_socket_dtls_client_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    return open_klb_socket_dtls(p_socket, p_cfg, false, p_code);
}


klb_socket_t* klb_socket_dtls_server_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    return open_klb_socket_dtls(p_socket, p_cfg, true, p_code);
}

klb_socket_t* klb_socket_dtls_io_open(bool is_server, const klb_socket_dtls_io_t* p_io,
    const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    return open_io_klb_socket_dtls(is_server, p_io, p_cfg, p_code);
}

int klb_socket_dtls_handshake(klb_socket_t* p_socket)
{
    if ((NULL == p_socket) || !is_dtls_klb_socket_dtls(p_socket))
    {
        return KLB_NETCODE_INVAL;
    }

    return handshake_klb_socket_dtls(p_socket);
}

/// @brief 导出 DTLS-SRTP 密钥材料 (RFC 5764 4.2 节)
/// @param [in]  *p_socket    DTLS socket
/// @param [in]  peer         false.本端发送方向; true.对端发送方向 (本端接收方向)
/// @param [out] *p_key       输出主密钥
/// @param [in]  key_cap      密钥缓存大小
/// @param [out] *p_salt      输出主盐
/// @param [in]  salt_cap     盐缓存大小
/// @return int 0.成功; 小于 0.klb_netcode_e
static int export_klb_socket_dtls(klb_socket_t* p_socket, bool peer, uint8_t* p_key, int key_cap,
                                  uint8_t* p_salt, int salt_cap)
{
    if ((NULL == p_socket) || (NULL == p_key) || (NULL == p_salt) || !is_dtls_klb_socket_dtls(p_socket))
    {
        return KLB_NETCODE_INVAL;
    }

    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    if (!p_dtls->handshake_done)
    {
        return KLB_NETCODE_NOTCONN;
    }

    if ((key_cap < KLB_DTLS_SRTP_KEY_LEN) || (salt_cap < KLB_DTLS_SRTP_SALT_LEN))
    {
        return KLB_NETCODE_MSGSIZE;
    }

#if defined(MBEDTLS_SSL_DTLS_SRTP)
    // step1. 双方未协商出 use_srtp 保护套件时不导出
    mbedtls_dtls_srtp_info info;
    memset(&info, 0, sizeof(info));
    mbedtls_ssl_get_dtls_srtp_negotiation_result(&p_dtls->ssl, &info);
    if (MBEDTLS_TLS_SRTP_UNSET == info.chosen_dtls_srtp_profile)
    {
        return KLB_NETCODE_NOTSUP;
    }
#endif

    if (!p_dtls->keys_exported)
    {
        return KLB_NETCODE_NOTSUP;
    }

    // step2. RFC 5764 4.2 节: PRF(master_secret, label, client_random||server_random, 60)
    unsigned char seed[64];
    unsigned char material[KLB_DTLS_SRTP_MATERIAL_LEN];
    memcpy(seed, p_dtls->client_random, sizeof(p_dtls->client_random));
    memcpy(seed + sizeof(p_dtls->client_random), p_dtls->server_random, sizeof(p_dtls->server_random));

    int ret = mbedtls_ssl_tls_prf(p_dtls->prf_type, p_dtls->master_secret, p_dtls->master_secret_len,
        KLB_DTLS_SRTP_LABEL, seed, sizeof(seed), material, sizeof(material));
    if (0 != ret)
    {
        return KLB_NETCODE_TLS_HANDSHAKE;
    }

    // step3. 方向选择: 客户端发送取 client_*; 服务端发送取 server_*
    bool use_server = p_dtls->is_server;
    if (peer)
    {
        use_server = !use_server;
    }

    int off_key = use_server ? KLB_DTLS_SRTP_KEY_LEN : 0;
    int off_salt = KLB_DTLS_SRTP_KEY_LEN * 2 + (use_server ? KLB_DTLS_SRTP_SALT_LEN : 0);

    memcpy(p_key, material + off_key, KLB_DTLS_SRTP_KEY_LEN);
    memcpy(p_salt, material + off_salt, KLB_DTLS_SRTP_SALT_LEN);

    return 0;
}


int klb_socket_dtls_srtp_export(klb_socket_t* p_socket, uint8_t* p_key, int key_cap,
                                uint8_t* p_salt, int salt_cap)
{
    return export_klb_socket_dtls(p_socket, false, p_key, key_cap, p_salt, salt_cap);
}


int klb_socket_dtls_srtp_export_peer(klb_socket_t* p_socket, uint8_t* p_key, int key_cap,
                                     uint8_t* p_salt, int salt_cap)
{
    return export_klb_socket_dtls(p_socket, true, p_key, key_cap, p_salt, salt_cap);
}


/// @brief 将 SHA-256 摘要格式化为 SDP fingerprint 文本 ("sha-256 AB:CD:...")
/// @param [in]  digest     32 字节摘要
/// @param [out] *p_text    输出文本
/// @param [in]  cap        缓冲容量
/// @return int 大于 0.文本长度; 小于 0.klb_netcode_e
static int format_fp_klb_socket_dtls(const unsigned char* digest, char* p_text, int cap)
{
    int need = (int)(sizeof(KLB_DTLS_FP_PREFIX) - 1) + KLB_DTLS_FP_SHA256_LEN * 3;
    if (cap < need)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    static const char hex[] = "0123456789ABCDEF";
    int pos = (int)(sizeof(KLB_DTLS_FP_PREFIX) - 1);
    memcpy(p_text, KLB_DTLS_FP_PREFIX, (size_t)pos);

    for (int i = 0; i < KLB_DTLS_FP_SHA256_LEN; ++i)
    {
        if (0 != i)
        {
            p_text[pos++] = ':';
        }

        p_text[pos++] = hex[(digest[i] >> 4) & 0x0F];
        p_text[pos++] = hex[digest[i] & 0x0F];
    }

    p_text[pos] = '\0';
    return pos;
}


int klb_socket_dtls_fingerprint(klb_socket_t* p_socket, char* p_text, int cap)
{
    if ((NULL == p_socket) || (NULL == p_text) || !is_dtls_klb_socket_dtls(p_socket))
    {
        return KLB_NETCODE_INVAL;
    }

    klb_socket_dtls_t* p_dtls = (klb_socket_dtls_t*)p_socket->extra;
    if (!p_dtls->handshake_done)
    {
        return KLB_NETCODE_NOTCONN;
    }

    const mbedtls_x509_crt* p_crt = mbedtls_ssl_get_peer_cert(&p_dtls->ssl);
    if (NULL == p_crt)
    {
        return KLB_NETCODE_NOENT;
    }

    const mbedtls_md_info_t* p_md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (NULL == p_md)
    {
        return KLB_NETCODE_NOTSUP;
    }

    unsigned char digest[KLB_DTLS_FP_SHA256_LEN];
    int ret = mbedtls_md(p_md, p_crt->raw.p, p_crt->raw.len, digest);
    if (0 != ret)
    {
        return KLB_NETCODE_IO;
    }

    // "sha-256 " + 32*2 hex + 31 ':' + '\0'
    return format_fp_klb_socket_dtls(digest, p_text, cap);
}


int klb_socket_dtls_cert_fingerprint(const char* p_cert_pem, char* p_text, int cap)
{
    if ((NULL == p_cert_pem) || (NULL == p_text))
    {
        return KLB_NETCODE_INVAL;
    }

    mbedtls_x509_crt crt;
    mbedtls_x509_crt_init(&crt);

    int ret = mbedtls_x509_crt_parse(&crt, (const unsigned char*)p_cert_pem,
        pem_buflen_klb_socket_dtls(p_cert_pem));
    if (0 != ret)
    {
        mbedtls_x509_crt_free(&crt);
        return KLB_NETCODE_TLS_CERT;
    }

    const mbedtls_md_info_t* p_md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (NULL == p_md)
    {
        mbedtls_x509_crt_free(&crt);
        return KLB_NETCODE_NOTSUP;
    }

    unsigned char digest[KLB_DTLS_FP_SHA256_LEN];
    ret = mbedtls_md(p_md, crt.raw.p, crt.raw.len, digest);
    mbedtls_x509_crt_free(&crt);

    if (0 != ret)
    {
        return KLB_NETCODE_IO;
    }

    return format_fp_klb_socket_dtls(digest, p_text, cap);
}

#else

int klb_socket_dtls_cert_fingerprint(const char* p_cert_pem, char* p_text, int cap)
{
    (void)p_cert_pem;
    (void)p_text;
    (void)cap;

    return KLB_NETCODE_NOTSUP;
}

klb_socket_t* klb_socket_dtls_client_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    (void)p_socket;
    (void)p_cfg;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return NULL;
}

klb_socket_t* klb_socket_dtls_server_open(klb_socket_t* p_socket, const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    (void)p_socket;
    (void)p_cfg;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return NULL;
}

klb_socket_t* klb_socket_dtls_io_open(bool is_server, const klb_socket_dtls_io_t* p_io,
    const klb_socket_dtls_cfg_t* p_cfg, int* p_code)
{
    (void)is_server;
    (void)p_io;
    (void)p_cfg;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_NOTSUP;
    }

    return NULL;
}

int klb_socket_dtls_handshake(klb_socket_t* p_socket)
{
    (void)p_socket;

    return KLB_NETCODE_NOTSUP;
}

int klb_socket_dtls_srtp_export(klb_socket_t* p_socket, uint8_t* p_key, int key_cap,
                                uint8_t* p_salt, int salt_cap)
{
    (void)p_socket;
    (void)p_key;
    (void)key_cap;
    (void)p_salt;
    (void)salt_cap;

    return KLB_NETCODE_NOTSUP;
}

int klb_socket_dtls_fingerprint(klb_socket_t* p_socket, char* p_text, int cap)
{
    (void)p_socket;
    (void)p_text;
    (void)cap;

    return KLB_NETCODE_NOTSUP;
}

#endif

// end
