#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_mem.h"
#include <klbutil/klb_log.h>
#include <assert.h>

#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/conf.h"
#include "openssl/x509v3.h"


typedef struct klb_socket_openssl_t_
{
    SSL_CTX*        p_ssl_ctx;          ///< ctx
    SSL*            p_ssl;              ///< ssl

    bool            is_handshake;       ///< 握手阶段
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
static void klb_socket_openssl_async_init(klb_socket_openssl_t* p_ssl, klb_socket_fd fd)
{
    // 先需要握手
    p_ssl->is_handshake = true;

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

    // connect方式
    SSL_set_connect_state(p_ssl->p_ssl);
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
    
    if (p_ssl->is_handshake)
    {
        // ssl握手阶段
        int handshake = SSL_do_handshake(p_ssl->p_ssl);
        if (1 == handshake)
        {
            p_ssl->is_handshake = false;
        }

        return 0;
    }
    
    int send = SSL_write(p_ssl->p_ssl, p_data, len);
    if (send < 0)
    {
        // ssl握手阶段
        int err = SSL_get_error(p_ssl->p_ssl, send);
        if (SSL_ERROR_WANT_WRITE == err)
        {
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
    
    if (p_ssl->is_handshake)
    {
        int handshake = SSL_do_handshake(p_ssl->p_ssl);
        if (1 == handshake)
        {
            p_ssl->is_handshake = false;
        }
        
        return 0;
    }
    
    int recv = SSL_read(p_ssl->p_ssl, p_buf, buf_len);
    if (recv < 0)
    {
        int err = SSL_get_error(p_ssl->p_ssl, recv);
        if (SSL_ERROR_WANT_READ == err)
        {
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

klb_socket_t* klb_socket_tls_async_create(klb_socket_fd fd)
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

    // 初始化 openssl
    klb_socket_openssl_async_init(p_ssl, fd);

    return p_socket;
}
