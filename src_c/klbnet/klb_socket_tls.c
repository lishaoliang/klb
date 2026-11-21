// Doc Encode : UTF-8 BOM, Unix(LF)
// TLS socket 封装 (mbedtls); 非阻塞 handshake + set_bio
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

#if defined(__KLB_MBEDTLS__)

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
#include <errno.h>
#endif

typedef struct klb_socket_tls_server_ctx_t_
{
    int                         ref;            ///< 引用计数; create=1, socket +1
    mbedtls_ssl_config          conf;
    mbedtls_ctr_drbg_context    ctr_drbg;
    mbedtls_entropy_context     entropy;
    mbedtls_x509_crt            cert;
    mbedtls_pk_context          key;
}klb_socket_tls_server_ctx_t;

typedef struct klb_socket_tls_t_
{
    mbedtls_ssl_context         ssl;
    mbedtls_ssl_config          conf;
    mbedtls_ctr_drbg_context    ctr_drbg;
    mbedtls_entropy_context     entropy;
    bool                        handshake_done;
    bool                        is_server;
    klb_socket_tls_server_ctx_t* p_server_ctx;
    mbedtls_x509_crt            ca;             ///< 客户端 CA 链 (可选)
    bool                        ca_loaded;
    char*                       p_alpn_buf;     ///< ALPN 协议名存储
    const char**                p_alpn_list;    ///< 指向 p_alpn_buf 的指针表
    int                         alpn_count;
    char                        pin_sha256[65]; ///< 对端证书 SHA256 hex; 空=不校验
}klb_socket_tls_t;

//////////////////////////////////////////////////////////////////////////

static bool wouldblock_klb_socket_tls(void)
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

static int bio_send_klb_socket_tls(void* ctx, const unsigned char* buf, size_t len)
{
    klb_socket_t* p_socket = (klb_socket_t*)ctx;
    int nlen = (int)len;
    if (len > (size_t)0x7FFFFFFF)
    {
        nlen = 0x7FFFFFFF;
    }

#ifdef _WIN32
    int ret = send(p_socket->fd, (const char*)buf, nlen, 0);
#else
#ifndef __APPLE__
    int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
    int ret = send(p_socket->fd, (const char*)buf, nlen, flags);
#else
    int ret = send(p_socket->fd, (const char*)buf, nlen, MSG_DONTWAIT);
#endif
#endif

    if (0 <= ret)
    {
        return ret;
    }

    if (wouldblock_klb_socket_tls())
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    return ret;
}

static int bio_recv_klb_socket_tls(void* ctx, unsigned char* buf, size_t len)
{
    klb_socket_t* p_socket = (klb_socket_t*)ctx;
    int nlen = (int)len;
    if (len > (size_t)0x7FFFFFFF)
    {
        nlen = 0x7FFFFFFF;
    }

#ifdef _WIN32
    int ret = recv(p_socket->fd, (char*)buf, nlen, 0);
#else
    int ret = recv(p_socket->fd, (char*)buf, nlen, MSG_DONTWAIT);
#endif

    if (0 <= ret)
    {
        return ret;
    }

    if (wouldblock_klb_socket_tls())
    {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return ret;
}

static void apply_want_klb_socket_tls(klb_socket_t* p_socket, int mbedtls_ret)
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

static int handshake_klb_socket_tls(klb_socket_t* p_socket)
{
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (p_tls->handshake_done)
    {
        p_socket->status_rw = KLB_SOCKET_RW_OK;
        return 0;
    }

    int ret = mbedtls_ssl_handshake(&p_tls->ssl);
    if (0 == ret)
    {
        p_tls->handshake_done = true;
        p_socket->status_rw = KLB_SOCKET_RW_OK;
        klb_socket_set_connected(p_socket, true);
        return 0;
    }
    else if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        apply_want_klb_socket_tls(p_socket, ret);
        return 1;
    }
    else if ((MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS == ret) || (MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == ret))
    {
        return 1;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static void quit_klb_socket_tls(klb_socket_tls_t* p_tls)
{
    mbedtls_ssl_free(&p_tls->ssl);
    mbedtls_ssl_config_free(&p_tls->conf);
    mbedtls_ctr_drbg_free(&p_tls->ctr_drbg);
    mbedtls_entropy_free(&p_tls->entropy);

    if (p_tls->ca_loaded)
    {
        mbedtls_x509_crt_free(&p_tls->ca);
        p_tls->ca_loaded = false;
    }

    if (NULL != p_tls->p_alpn_list)
    {
        KLB_FREE(p_tls->p_alpn_list);
        p_tls->p_alpn_list = NULL;
    }

    if (NULL != p_tls->p_alpn_buf)
    {
        KLB_FREE(p_tls->p_alpn_buf);
        p_tls->p_alpn_buf = NULL;
    }

    p_tls->alpn_count = 0;
    p_tls->pin_sha256[0] = '\0';
}

static void quit_klb_socket_tls_server_ctx(klb_socket_tls_server_ctx_t* p_ctx)
{
    mbedtls_pk_free(&p_ctx->key);
    mbedtls_x509_crt_free(&p_ctx->cert);
    mbedtls_ssl_config_free(&p_ctx->conf);
    mbedtls_ctr_drbg_free(&p_ctx->ctr_drbg);
    mbedtls_entropy_free(&p_ctx->entropy);
}

static size_t pem_buflen_klb_socket_tls(const char* p_pem, int len)
{
    size_t buflen = (size_t)len;
    if ((0 < len) && ('\0' != p_pem[len - 1]))
    {
        buflen = (size_t)len + 1;
    }

    return buflen;
}

static int init_klb_socket_tls(klb_socket_t* p_socket)
{
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;

    mbedtls_ssl_init(&p_tls->ssl);
    mbedtls_ssl_config_init(&p_tls->conf);
    mbedtls_ctr_drbg_init(&p_tls->ctr_drbg);
    mbedtls_entropy_init(&p_tls->entropy);

    static const unsigned char pers[] = "klb";
    int ret = mbedtls_ctr_drbg_seed(&p_tls->ctr_drbg, mbedtls_entropy_func, &p_tls->entropy, pers, sizeof(pers) - 1);
    if (0 != ret)
    {
        return -1;
    }

    ret = mbedtls_ssl_config_defaults(&p_tls->conf,
        MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (0 != ret)
    {
        return -1;
    }

    // 对齐历史 OpenSSL 门面: 未接 CA 校验; 设备侧默认不验对端证书
    mbedtls_ssl_conf_authmode(&p_tls->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&p_tls->conf, mbedtls_ctr_drbg_random, &p_tls->ctr_drbg);

    ret = mbedtls_ssl_setup(&p_tls->ssl, &p_tls->conf);
    if (0 != ret)
    {
        return -1;
    }

    mbedtls_ssl_set_bio(&p_tls->ssl, p_socket, bio_send_klb_socket_tls, bio_recv_klb_socket_tls, NULL);
    return 0;
}

//////////////////////////////////////////////////////////////////////////

static void destroy_klb_socket_tls(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;

    if (p_tls->handshake_done)
    {
        mbedtls_ssl_close_notify(&p_tls->ssl);
    }

    if (p_tls->is_server)
    {
        mbedtls_ssl_free(&p_tls->ssl);
        klb_socket_tls_server_ctx_destroy(p_tls->p_server_ctx);
        p_tls->p_server_ctx = NULL;
    }
    else
    {
        quit_klb_socket_tls(p_tls);
    }

    KLB_SOCKET_CLOSE(p_socket->fd);
    KLB_FREE(p_socket);
}

static int send_klb_socket_tls(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    int hs = handshake_klb_socket_tls(p_socket);
    if (0 != hs)
    {
        if (hs < 0)
        {
            return -1;
        }

        return 0;
    }

    if ((NULL == p_data) || (len <= 0))
    {
        return 0;
    }

    int ret = mbedtls_ssl_write(&p_tls->ssl, p_data, (size_t)len);
    if (0 < ret)
    {
        return ret;
    }
    else if ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret))
    {
        apply_want_klb_socket_tls(p_socket, ret);
        return 0;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static int recv_klb_socket_tls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    int hs = handshake_klb_socket_tls(p_socket);
    if (0 != hs)
    {
        if (hs < 0)
        {
            return -1;
        }

        return 0;
    }

    int ret = mbedtls_ssl_read(&p_tls->ssl, p_buf, (size_t)buf_len);
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
        apply_want_klb_socket_tls(p_socket, ret);
        return 0;
    }
    else if (MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY == ret)
    {
        return 0;
    }

    p_socket->status = KLB_SOCKET_ERR_PROTOCOL;
    return -1;
}

static int recv_peek_klb_socket_tls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, MSG_PEEK);
#else
    int flags = MSG_PEEK;
    if (0 != p_socket->nonblock)
    {
        flags |= MSG_DONTWAIT;
    }
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, flags);
#endif

    return ret;
}

static int sendto_klb_socket_tls(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
{
    (void)p_socket;
    (void)p_data;
    (void)len;
    (void)p_addr;
    (void)addr_len;
    assert(false);
    return -1;
}

static int recvfrom_klb_socket_tls(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
    (void)p_socket;
    (void)p_buf;
    (void)buf_len;
    (void)p_addr;
    (void)p_addr_len;
    assert(false);
    return -1;
}

int klb_socket_tls_handshake(klb_socket_t* p_socket)
{
    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return -1;
    }

    return handshake_klb_socket_tls(p_socket);
}


int klb_socket_tls_set_hostname(klb_socket_t* p_socket, const char* p_hostname)
{
    klb_socket_tls_t* p_tls = NULL;
    int ret = 0;

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server || p_tls->handshake_done)
    {
        return 1;
    }

    if (NULL == p_hostname || '\0' == p_hostname[0])
    {
        ret = mbedtls_ssl_set_hostname(&p_tls->ssl, NULL);
    }
    else
    {
        ret = mbedtls_ssl_set_hostname(&p_tls->ssl, p_hostname);
    }

    return (0 == ret) ? 0 : 1;
}


int klb_socket_tls_set_verify(klb_socket_t* p_socket, int verify)
{
    klb_socket_tls_t* p_tls = NULL;
    int mode = MBEDTLS_SSL_VERIFY_NONE;

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server || p_tls->handshake_done)
    {
        return 1;
    }

    if (1 == verify)
    {
        mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
    }
    else if (2 <= verify)
    {
        mode = MBEDTLS_SSL_VERIFY_REQUIRED;
    }
    else
    {
        mode = MBEDTLS_SSL_VERIFY_NONE;
    }

    mbedtls_ssl_conf_authmode(&p_tls->conf, mode);
    return 0;
}


int klb_socket_tls_set_ca_pem(klb_socket_t* p_socket, const char* p_ca_pem, int ca_len)
{
    klb_socket_tls_t* p_tls = NULL;
    size_t buflen = 0;
    int ret = 0;

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server || p_tls->handshake_done)
    {
        return 1;
    }

    if (NULL == p_ca_pem || 0 >= ca_len)
    {
        return 1;
    }

    if (p_tls->ca_loaded)
    {
        mbedtls_x509_crt_free(&p_tls->ca);
        p_tls->ca_loaded = false;
    }

    mbedtls_x509_crt_init(&p_tls->ca);
    buflen = pem_buflen_klb_socket_tls(p_ca_pem, ca_len);
    ret = mbedtls_x509_crt_parse(&p_tls->ca, (const unsigned char*)p_ca_pem, buflen);
    if (0 != ret)
    {
        mbedtls_x509_crt_free(&p_tls->ca);
        return 1;
    }

    mbedtls_ssl_conf_ca_chain(&p_tls->conf, &p_tls->ca, NULL);
    p_tls->ca_loaded = true;
    return 0;
}


int klb_socket_tls_set_alpn(klb_socket_t* p_socket, const char* p_alpn_csv)
{
    klb_socket_tls_t* p_tls = NULL;
    char* p_buf = NULL;
    const char** p_list = NULL;
    int count = 0;
    int i = 0;
    size_t len = 0;
    char* p = NULL;
    char* p_tok = NULL;

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server || p_tls->handshake_done)
    {
        return 1;
    }

    if (NULL != p_tls->p_alpn_list)
    {
        KLB_FREE(p_tls->p_alpn_list);
        p_tls->p_alpn_list = NULL;
    }

    if (NULL != p_tls->p_alpn_buf)
    {
        KLB_FREE(p_tls->p_alpn_buf);
        p_tls->p_alpn_buf = NULL;
    }

    p_tls->alpn_count = 0;

    if (NULL == p_alpn_csv || '\0' == p_alpn_csv[0])
    {
        return 0;
    }

    len = strlen(p_alpn_csv);
    p_buf = KLB_MALLOC(char, len + 1, 0);
    if (NULL == p_buf)
    {
        return 1;
    }

    memcpy(p_buf, p_alpn_csv, len + 1);

    // 计段: 非空 token
    p = p_buf;
    while ('\0' != *p)
    {
        while (',' == *p || isspace((unsigned char)*p))
        {
            p++;
        }

        if ('\0' == *p)
        {
            break;
        }

        count++;
        while ('\0' != *p && ',' != *p)
        {
            p++;
        }

        if (',' == *p)
        {
            p++;
        }
    }

    if (0 >= count)
    {
        KLB_FREE(p_buf);
        return 0;
    }

    // +1 NULL 终结 (mbedtls 要求)
    p_list = (const char**)KLB_MALLOC(char*, (size_t)count + 1, 0);
    if (NULL == p_list)
    {
        KLB_FREE(p_buf);
        return 1;
    }

    memcpy(p_buf, p_alpn_csv, len + 1);
    i = 0;
    p_tok = p_buf;
    while ('\0' != *p_tok && i < count)
    {
        while (',' == *p_tok || isspace((unsigned char)*p_tok))
        {
            p_tok++;
        }

        if ('\0' == *p_tok)
        {
            break;
        }

        p_list[i] = p_tok;
        i++;
        while ('\0' != *p_tok && ',' != *p_tok)
        {
            p_tok++;
        }

        if (',' == *p_tok)
        {
            *p_tok = '\0';
            p_tok++;
        }

        // trim trailing space in token
        {
            size_t tlen = strlen(p_list[i - 1]);
            while (0 < tlen && isspace((unsigned char)p_list[i - 1][tlen - 1]))
            {
                ((char*)p_list[i - 1])[tlen - 1] = '\0';
                tlen--;
            }
        }
    }

    p_list[i] = NULL;
    if (0 != mbedtls_ssl_conf_alpn_protocols(&p_tls->conf, p_list))
    {
        KLB_FREE(p_list);
        KLB_FREE(p_buf);
        return 1;
    }

    p_tls->p_alpn_buf = p_buf;
    p_tls->p_alpn_list = p_list;
    p_tls->alpn_count = i;
    return 0;
}


int klb_socket_tls_set_pin_sha256(klb_socket_t* p_socket, const char* p_hex)
{
    klb_socket_tls_t* p_tls = NULL;
    size_t n = 0;
    size_t i = 0;

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server)
    {
        return 1;
    }

    p_tls->pin_sha256[0] = '\0';
    if (NULL == p_hex || '\0' == p_hex[0])
    {
        return 0;
    }

    n = strlen(p_hex);
    if (64 != n)
    {
        return 1;
    }

    for (i = 0; i < n; i++)
    {
        char c = p_hex[i];
        if (!isxdigit((unsigned char)c))
        {
            return 1;
        }

        if ('A' <= c && 'F' >= c)
        {
            c = (char)(c - 'A' + 'a');
        }

        p_tls->pin_sha256[i] = c;
    }

    p_tls->pin_sha256[64] = '\0';
    return 0;
}


int klb_socket_tls_check_pin(klb_socket_t* p_socket)
{
    klb_socket_tls_t* p_tls = NULL;
    const mbedtls_x509_crt* p_crt = NULL;
    unsigned char hash[32];
    char hex[65];
    size_t i = 0;
    static const char* hexdig = "0123456789abcdef";

    if (NULL == p_socket || !klb_socket_is_tls(p_socket))
    {
        return 1;
    }

    p_tls = (klb_socket_tls_t*)p_socket->extra;
    if (NULL == p_tls || p_tls->is_server)
    {
        return 1;
    }

    if ('\0' == p_tls->pin_sha256[0])
    {
        return 0;
    }

    if (!p_tls->handshake_done)
    {
        return 1;
    }

    p_crt = mbedtls_ssl_get_peer_cert(&p_tls->ssl);
    if (NULL == p_crt || NULL == p_crt->raw.p || 0 == p_crt->raw.len)
    {
        return 1;
    }

    if (0 != mbedtls_sha256(p_crt->raw.p, p_crt->raw.len, hash, 0))
    {
        return 1;
    }

    for (i = 0; i < 32; i++)
    {
        hex[i * 2] = hexdig[(hash[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = hexdig[hash[i] & 0x0F];
    }

    hex[64] = '\0';
    if (0 != strcmp(hex, p_tls->pin_sha256))
    {
        return 1;
    }

    return 0;
}


klb_socket_t* klb_socket_async_create_tls(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

    // step1. 分配 socket + mbedtls extra
    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, sizeof(klb_socket_tls_t));

    // step2. 填 vtable, 非阻塞 fd
    p_socket->vtable.cb_destroy = destroy_klb_socket_tls;
    p_socket->vtable.cb_send = send_klb_socket_tls;
    p_socket->vtable.cb_recv = recv_klb_socket_tls;
    p_socket->vtable.cb_recv_peek = recv_peek_klb_socket_tls;
    p_socket->vtable.cb_sendto = sendto_klb_socket_tls;
    p_socket->vtable.cb_recvfrom = recvfrom_klb_socket_tls;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = fd;
    p_socket->nonblock = 0x1;
    klb_socket_set_block(fd, false);
    klb_socket_set_tls(p_socket, true);

    // step3. mbedtls ctx + set_bio (失败不关 fd, 调用方仍持有)
    if (0 != init_klb_socket_tls(p_socket))
    {
        quit_klb_socket_tls((klb_socket_tls_t*)p_socket->extra);
        KLB_FREE(p_socket);
        return NULL;
    }

    return p_socket;
}

static int init_klb_socket_tls_server(klb_socket_t* p_socket, klb_socket_tls_server_ctx_t* p_ctx)
{
    klb_socket_tls_t* p_tls = (klb_socket_tls_t*)p_socket->extra;
    p_tls->is_server = true;
    p_tls->p_server_ctx = p_ctx;

    mbedtls_ssl_init(&p_tls->ssl);

    int ret = mbedtls_ssl_setup(&p_tls->ssl, &p_ctx->conf);
    if (0 != ret)
    {
        mbedtls_ssl_free(&p_tls->ssl);
        p_tls->p_server_ctx = NULL;
        return -1;
    }

    mbedtls_ssl_set_bio(&p_tls->ssl, p_socket, bio_send_klb_socket_tls, bio_recv_klb_socket_tls, NULL);
    return 0;
}

klb_socket_tls_server_ctx_t* klb_socket_tls_server_ctx_create(const char* p_cert_pem, int cert_len, const char* p_key_pem, int key_len)
{
    if ((NULL == p_cert_pem) || (NULL == p_key_pem) || (cert_len <= 0) || (key_len <= 0))
    {
        return NULL;
    }

    // step1. 分配并 init mbedtls 对象
    klb_socket_tls_server_ctx_t* p_ctx = KLB_MALLOCZ(klb_socket_tls_server_ctx_t, 1, 0);
    p_ctx->ref = 1;

    mbedtls_ssl_config_init(&p_ctx->conf);
    mbedtls_ctr_drbg_init(&p_ctx->ctr_drbg);
    mbedtls_entropy_init(&p_ctx->entropy);
    mbedtls_x509_crt_init(&p_ctx->cert);
    mbedtls_pk_init(&p_ctx->key);

    static const unsigned char pers[] = "klb";
    int ret = mbedtls_ctr_drbg_seed(&p_ctx->ctr_drbg, mbedtls_entropy_func, &p_ctx->entropy, pers, sizeof(pers) - 1);
    if (0 != ret)
    {
        goto err;
    }

    ret = mbedtls_ssl_config_defaults(&p_ctx->conf,
        MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (0 != ret)
    {
        goto err;
    }

    // step2. 解析 PEM 证书/私钥
    ret = mbedtls_x509_crt_parse(&p_ctx->cert, (const unsigned char*)p_cert_pem,
        pem_buflen_klb_socket_tls(p_cert_pem, cert_len));
    if (0 != ret)
    {
        goto err;
    }

    ret = mbedtls_pk_parse_key(&p_ctx->key, (const unsigned char*)p_key_pem,
        pem_buflen_klb_socket_tls(p_key_pem, key_len), NULL, 0,
        mbedtls_ctr_drbg_random, &p_ctx->ctr_drbg);
    if (0 != ret)
    {
        goto err;
    }

    // step3. 绑定证书; 不验客户端证书
    mbedtls_ssl_conf_authmode(&p_ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&p_ctx->conf, mbedtls_ctr_drbg_random, &p_ctx->ctr_drbg);

    ret = mbedtls_ssl_conf_own_cert(&p_ctx->conf, &p_ctx->cert, &p_ctx->key);
    if (0 != ret)
    {
        goto err;
    }

    return p_ctx;

err:
    quit_klb_socket_tls_server_ctx(p_ctx);
    KLB_FREE(p_ctx);
    return NULL;
}

void klb_socket_tls_server_ctx_destroy(klb_socket_tls_server_ctx_t* p_ctx)
{
    if (NULL == p_ctx)
    {
        return;
    }

    assert(0 < p_ctx->ref);
    p_ctx->ref--;
    if (0 < p_ctx->ref)
    {
        return;
    }

    quit_klb_socket_tls_server_ctx(p_ctx);
    KLB_FREE(p_ctx);
}

klb_socket_t* klb_socket_async_create_tls_server(klb_socket_fd fd, klb_socket_tls_server_ctx_t* p_ctx)
{
    assert(INVALID_SOCKET != fd);
    if (NULL == p_ctx)
    {
        return NULL;
    }

    // step1. 分配 socket + mbedtls extra
    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, sizeof(klb_socket_tls_t));

    // step2. 填 vtable, 非阻塞 fd
    p_socket->vtable.cb_destroy = destroy_klb_socket_tls;
    p_socket->vtable.cb_send = send_klb_socket_tls;
    p_socket->vtable.cb_recv = recv_klb_socket_tls;
    p_socket->vtable.cb_recv_peek = recv_peek_klb_socket_tls;
    p_socket->vtable.cb_sendto = sendto_klb_socket_tls;
    p_socket->vtable.cb_recvfrom = recvfrom_klb_socket_tls;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = fd;
    p_socket->nonblock = 0x1;
    klb_socket_set_block(fd, false);
    klb_socket_set_tls(p_socket, true);

    // step3. ssl_setup 共用服务端 conf (失败不关 fd)
    if (0 != init_klb_socket_tls_server(p_socket, p_ctx))
    {
        KLB_FREE(p_socket);
        return NULL;
    }

    p_ctx->ref++;
    return p_socket;
}

#else

klb_socket_t* klb_socket_async_create_tls(klb_socket_fd fd)
{
    (void)fd;
    return NULL;
}

int klb_socket_tls_handshake(klb_socket_t* p_socket)
{
    (void)p_socket;
    return -1;
}

int klb_socket_tls_set_hostname(klb_socket_t* p_socket, const char* p_hostname)
{
    (void)p_socket;
    (void)p_hostname;
    return 1;
}

int klb_socket_tls_set_verify(klb_socket_t* p_socket, int verify)
{
    (void)p_socket;
    (void)verify;
    return 1;
}

int klb_socket_tls_set_ca_pem(klb_socket_t* p_socket, const char* p_ca_pem, int ca_len)
{
    (void)p_socket;
    (void)p_ca_pem;
    (void)ca_len;
    return 1;
}

int klb_socket_tls_set_alpn(klb_socket_t* p_socket, const char* p_alpn_csv)
{
    (void)p_socket;
    (void)p_alpn_csv;
    return 1;
}

int klb_socket_tls_set_pin_sha256(klb_socket_t* p_socket, const char* p_hex)
{
    (void)p_socket;
    (void)p_hex;
    return 1;
}

int klb_socket_tls_check_pin(klb_socket_t* p_socket)
{
    (void)p_socket;
    return 1;
}

klb_socket_tls_server_ctx_t* klb_socket_tls_server_ctx_create(const char* p_cert_pem, int cert_len, const char* p_key_pem, int key_len)
{
    (void)p_cert_pem;
    (void)cert_len;
    (void)p_key_pem;
    (void)key_len;
    return NULL;
}

void klb_socket_tls_server_ctx_destroy(klb_socket_tls_server_ctx_t* p_ctx)
{
    (void)p_ctx;
}

klb_socket_t* klb_socket_async_create_tls_server(klb_socket_fd fd, klb_socket_tls_server_ctx_t* p_ctx)
{
    (void)fd;
    (void)p_ctx;
    return NULL;
}

#endif

// end
