// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_listen.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>


/// @struct klb_listen_t
/// @brief  单个监听
typedef struct klb_listen_t_
{
    klb_multiplex_t*            p_multi;        ///< 多路复用模块

    struct
    {
        klb_socket_fd           fd_listen;      ///< 操作系统分配的原始 fd
        klb_socket_t*           p_socket;       ///< 监听 socket
        int                     id;             ///< 分配的ID,用于 klb_multiplex_t 模块
        int                     port;           ///< 监听端口
    };

    // 回调相关
    struct 
    {
        klb_listen_accept_cb    cb_accept;      ///< 监听到 socket 之后的回调函数
        void*                   ptr;            ///< 回调函数参数
    };

    // TLS相关
    struct
    {
        bool                    tls;            ///< TLS
        klb_socket_tls_param_t  tls_param;      ///< TLS参数
    };
}klb_listen_t;


/// @brief 创建listen
klb_listen_t* klb_listen_create(klb_multiplex_t* p_multi)
{
    klb_listen_t* p_listen = KLB_MALLOCZ(klb_listen_t, 1, 0);

    p_listen->p_multi = p_multi;
    p_listen->fd_listen = INVALID_SOCKET;

    p_listen->tls = false;

    return p_listen;
}

/// @brief 销毁listen
void klb_listen_destroy(klb_listen_t* p_listen)
{
    assert(NULL != p_listen);

    KLB_FREE(p_listen);
}

static int klb_listen_do_accept(klb_listen_t* p_listen)
{
    int num = 0;

    while (true)
    {
        struct sockaddr_in addr = { 0 };
        klb_socket_fd fd = klb_socket_accept(p_listen->fd_listen, &addr);

        if (INVALID_SOCKET != fd)
        {
            num++;

            if (p_listen->cb_accept)
            {
                p_listen->cb_accept(p_listen->ptr, fd, &addr, p_listen->tls, &p_listen->tls_param);
            }
            else
            {
                KLB_SOCKET_CLOSE(fd);
            }
        }
        else
        {
            break; // 都取完了
        }
    }

    return num;
}

static int cb_remove_klb_listen(void* p_lparam, void* p_wparam, int id)
{
    //klb_listen_t* p_listen = p_lparam; ///< 销毁时, klb_listen_t 可能已经被提前销毁了
    klb_socket_t* p_socket = (klb_socket_t*)p_wparam;

    // 销毁 socket
    klb_socket_destroy(p_socket);

    return 0;
}

static int on_recv_klb_listen(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_listen_t* p_listen = (klb_listen_t*)p_lparam;
    klb_socket_t* p_socket = (klb_socket_t*)p_wparam;

    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    return klb_listen_do_accept(p_listen);
}

static int on_send_klb_listen(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_listen_t* p_listen = (klb_listen_t*)p_lparam;
    klb_socket_t* p_socket = (klb_socket_t*)p_wparam;

    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    return klb_listen_do_accept(p_listen);
}

static int on_proc_klb_listen(void* p_lparam, void* p_wparam, int msg, int id, klb_socket_t* p_socket, int64_t now)
{
    klb_listen_t* p_listen = (klb_listen_t*)p_lparam;

    switch (msg)
    {
    case KLB_MULTIPLEX_MSG_connect:
        break;
    case KLB_MULTIPLEX_MSG_timeout:
        break;
    default:
        break;
    }

    return 0;
}

int klb_listen_open(klb_listen_t* p_listen, int port, int max_connect)
{
    klb_socket_fd fd = klb_socket_listen(port, max_connect);
    assert(INVALID_SOCKET != fd);

#if 1
    klb_socket_t* p_socket = klb_socket_async_create(fd);
#else
    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, 0);

    p_socket->fd = fd;
    p_socket->status = KLB_SOCKET_OK;
#endif

    p_listen->fd_listen = fd;
    p_listen->p_socket = p_socket;

    klb_multiplex_ops_t ops = { 0 };
    ops.cb_remove = cb_remove_klb_listen;
    ops.cb_recv = on_recv_klb_listen;
    ops.cb_send = on_send_klb_listen;
    ops.cb_proc = on_proc_klb_listen;

    ops.p_lparam = p_listen;
    ops.p_wparam = p_listen->p_socket;

    p_listen->id = klb_multiplex_push_socket(p_listen->p_multi, p_socket, &ops);

    klb_socket_set_connected(p_socket, true);
    klb_socket_set_reading(p_socket, true);

    return 0;
}

int klb_listen_open_unix(klb_listen_t* p_listen, const char* p_path, int max_connect)
{
    return 0;
}

int klb_listen_set_tls(klb_listen_t* p_listen, const klb_socket_tls_param_t* p_tls_param)
{
    p_listen->tls = true;

    if (NULL != p_tls_param)
    {
        memcpy(&p_listen->tls_param, p_tls_param, sizeof(klb_socket_tls_param_t));
    }
    else 
    {
        memset(&p_listen->tls_param, 0, sizeof(klb_socket_tls_param_t));
    }

    return 0;
}

void klb_listen_close(klb_listen_t* p_listen)
{
    if (NULL != p_listen->p_socket)
    {
        // 采用延时销毁机制

        // socket 正常, 则设置为主动关闭: 防止关闭后, 还有数据产生
        klb_socket_closing(p_listen->p_socket);

        // socket 交给 multiplex 流程销毁
        klb_multiplex_remove(p_listen->p_multi, p_listen->id);

        p_listen->p_socket = NULL;
    }
}

int klb_listen_port(klb_listen_t* p_listen)
{
    return p_listen->port;
}

int klb_listen_set_accept(klb_listen_t* p_listen, klb_listen_accept_cb cb_accept, void* ptr)
{
    p_listen->cb_accept = cb_accept;
    p_listen->ptr = ptr;

    return 0;
}
