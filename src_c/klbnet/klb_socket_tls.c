// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_mem.h"
#include <klbutil/klb_log.h>
#include <assert.h>

#if defined(__KLB_OPENSSL__)

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/conf.h"
#include "openssl/x509v3.h"

typedef struct klb_socket_openssl_t_
{
    SSL_CTX*        p_ssl_ctx;          ///< ctx
    SSL*            p_ssl;              ///< ssl

    int             need_handshake;       ///< 握手阶段; true.握手阶段; false.已经完成握手
}klb_socket_openssl_t;


//////////////////////////////////////////////////////////////////////////
int klb_socket_tls_init()
{
    SSL_load_error_strings();
    SSL_library_init();

    return 0;
}

void klb_socket_tls_quit()
{

}

//////////////////////////////////////////////////////////////////////////
static void klb_socket_openssl_async_init(klb_socket_openssl_t* p_ssl, klb_socket_fd fd, bool service)
{
    // 先需要握手
    p_ssl->need_handshake = true;

    // ssl方法
    const SSL_METHOD* p_method = SSLv23_client_method();
    //const SSL_METHOD* p_method = TLSv1_2_client_method();

    //环境
    p_ssl->p_ssl_ctx = SSL_CTX_new(p_method);
    assert(NULL != p_ssl->p_ssl_ctx);

    // 新建一个
    p_ssl->p_ssl = SSL_new(p_ssl->p_ssl_ctx);

    // 设置可读写
    int set_fd = SSL_set_fd(p_ssl->p_ssl, fd);
    assert(1 == set_fd);

    if (service)
    {
        // 服务方式
        SSL_accept(p_ssl->p_ssl);
        SSL_set_accept_state(p_ssl->p_ssl);
    }
    else
    {
        // connect方式
        SSL_set_connect_state(p_ssl->p_ssl);
    }
}

static void klb_socket_openssl_async_quit(klb_socket_openssl_t* p_ssl)
{
    SSL_shutdown(p_ssl->p_ssl);

    KLB_FREE_BY(p_ssl->p_ssl, SSL_free);
    KLB_FREE_BY(p_ssl->p_ssl_ctx, SSL_CTX_free);
}

//////////////////////////////////////////////////////////////////////////
// openssl socket

/// @brief 销毁
static void klb_socket_openssl_async_destroy(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    klb_socket_openssl_t* p_ssl = (klb_socket_openssl_t*)p_socket->extra;

    klb_socket_openssl_async_quit(p_ssl);

    // 关闭fd
    KLB_SOCKET_CLOSE(p_socket->fd);

    // 销毁
    KLB_FREE(p_socket);
}

/// @brief TCP发送数据
static int klb_socket_openssl_async_send(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
    klb_socket_openssl_t* p_ssl = (klb_socket_openssl_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    if (p_ssl->need_handshake)
    {
        // ssl握手阶段
        int handshake = SSL_do_handshake(p_ssl->p_ssl);
        if (1 == handshake)
        {
            p_ssl->need_handshake = false;
        }

        p_socket->status_rw = KLB_SOCKET_WAIT_WRITE;
        return 0; // 需要等待
    }
    
    int send = SSL_write(p_ssl->p_ssl, p_data, len);
    if (send < 0)
    {
        // ssl握手阶段
        int err = SSL_get_error(p_ssl->p_ssl, send);
        if (SSL_ERROR_WANT_WRITE == err)
        {
            p_socket->status_rw = KLB_SOCKET_WAIT_WRITE;
            send = 0; // 需要等待
        }
    }

    //KLB_LOG("SSL_write=%d\n", send);
    return send;
}

/// @brief TCP接收数据
static int klb_socket_openssl_async_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
    klb_socket_openssl_t* p_ssl = (klb_socket_openssl_t*)p_socket->extra;
    p_socket->status_rw = KLB_SOCKET_RW_OK;

    if (p_ssl->need_handshake)
    {
        int handshake = SSL_do_handshake(p_ssl->p_ssl);
        if (1 == handshake)
        {
            p_ssl->need_handshake = false;
        }
        
        p_socket->status_rw = KLB_SOCKET_WAIT_READ;
        return 0; // 需要等待
    }
    
    int recv = SSL_read(p_ssl->p_ssl, p_buf, buf_len);
    if (recv < 0)
    {
        int err = SSL_get_error(p_ssl->p_ssl, recv);
        if (SSL_ERROR_WANT_READ == err)
        {
            p_socket->status_rw = KLB_SOCKET_WAIT_READ;
            recv = 0; // 需要等待
        }
    }

    //KLB_LOG("SSL_read=%d\n", recv); 
    return recv;
}

/// @brief UDP发送数据
static int klb_socket_openssl_async_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
{
    assert(false);
    return 0;
}

/// @brief UDP接收数据
static int klb_socket_openssl_async_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
    assert(false);
    return 0;
}

klb_socket_t* klb_socket_tls_async_create(klb_socket_fd fd, bool service, const klb_socket_tls_param_t* p_tls)
{
    assert(INVALID_SOCKET != fd);
    klb_socket_t* p_socket = KLB_MALLOC(klb_socket_t, 1, sizeof(klb_socket_openssl_t));
    KLB_MEMSET(p_socket, 0, sizeof(klb_socket_t) + sizeof(klb_socket_openssl_t));

    klb_socket_openssl_t* p_ssl = (klb_socket_openssl_t*)p_socket->extra;

    p_socket->vtable.cb_destroy = klb_socket_openssl_async_destroy;
    p_socket->vtable.cb_send = klb_socket_openssl_async_send;
    p_socket->vtable.cb_recv = klb_socket_openssl_async_recv;
    p_socket->vtable.cb_sendto = klb_socket_openssl_async_sendto;
    p_socket->vtable.cb_recvfrom = klb_socket_openssl_async_recvfrom;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = fd;

    // 设置为非阻塞, 异步模式
    klb_socket_set_block(fd, false);

    // 设置TLS标记
    klb_socket_set_tls(p_socket, true);

    // 初始化 openssl
    klb_socket_openssl_async_init(p_ssl, fd, service);

    return p_socket;
}

#else

/// @brief openssl库初始化
int klb_socket_tls_init()
{
    return 0;
}


/// @brief openssl库退出
void klb_socket_tls_quit()
{

}

/// @brief 创建一个加密异步socket
/// @param [in]  fd             socket fd
/// @return klb_socket_t* NULL.创建失败; 非NULL
klb_socket_t* klb_socket_tls_async_create(klb_socket_fd fd, bool service, const klb_socket_tls_param_t* p_tls)
{
    assert(false);
    return NULL;
}

#endif
