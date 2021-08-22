#include "klbnet/klb_listen.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef struct klb_listen_t_
{
    klb_multiplex_t*        p_multi;

    klb_socket_fd           fd_listen;
    klb_socket_t*           p_socket;
    int                     id;

    klb_listen_accept_cb    cb_accept;
    void*                   ptr;
}klb_listen_t;


/// @brief 创建listen
klb_listen_t* klb_listen_create(klb_multiplex_t* p_multi)
{
    klb_listen_t* p_listen = KLB_MALLOC(klb_listen_t, 1, 0);
    KLB_MEMSET(p_listen, 0, sizeof(klb_listen_t));

    p_listen->p_multi = p_multi;
    p_listen->fd_listen = INVALID_SOCKET;

    return p_listen;
}

/// @brief 销毁listen
void klb_listen_destroy(klb_listen_t* p_listen)
{
    assert(NULL != p_listen);

    KLB_FREE(p_listen);
}

static void klb_listen_do_accept(klb_listen_t* p_listen)
{
    while (true)
    {
        struct sockaddr_in addr = { 0 };
        klb_socket_fd fd = klb_socket_accept(p_listen->fd_listen, &addr);

        if (INVALID_SOCKET != fd)
        {
            if (p_listen->cb_accept)
            {
                p_listen->cb_accept(p_listen->ptr, fd, &addr);
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
}

static int cb_remove_klb_listen(void* p_lparam, void* p_wparam, int id)
{

    return 0;
}

static int on_recv_klb_listen(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_listen_t* p_listen = p_lparam;

    klb_listen_do_accept(p_listen);

    return 0;
}

static int on_send_klb_listen(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_listen_t* p_listen = p_lparam;

    klb_listen_do_accept(p_listen);

    return 0;
}

int klb_listen_open(klb_listen_t* p_listen, int port, int max_connect)
{
    klb_socket_fd fd = klb_socket_listen(port, max_connect);
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = KLB_MALLOC(klb_socket_t, 1, 0);
    KLB_MEMSET(p_socket, 0, sizeof(klb_socket_t));

    p_socket->fd = fd;
    p_socket->status = KLB_SOCKET_OK;

    p_listen->fd_listen = fd;
    p_listen->p_socket = p_socket;

    klb_multiplex_obj_t ops = { 0 };
    ops.cb_remove = cb_remove_klb_listen;
    ops.cb_recv = on_recv_klb_listen;
    ops.cb_send = on_send_klb_listen;

    ops.p_lparam = p_listen;
    ops.p_wparam = NULL;

    p_listen->id = klb_multiplex_push_socket(p_listen->p_multi, p_socket, &ops);

    klb_socket_set_reading(p_socket, true);

    return 0;
}

int klb_listen_open_unix(klb_listen_t* p_listen, int port, int max_connect)
{
    return 0;
}

int klb_listen_close(klb_listen_t* p_listen)
{
    klb_multiplex_remove(p_listen->p_multi, p_listen->id);

    return 0;
}

int klb_listen_set_accept(klb_listen_t* p_listen, klb_listen_accept_cb cb_accept, void* ptr)
{
    p_listen->cb_accept = cb_accept;
    p_listen->ptr = ptr;

    return 0;
}
