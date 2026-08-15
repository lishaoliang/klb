// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#ifdef _WIN32

#include "klbnet/klb_iopoll_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_rand.h"
#include <assert.h>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>


#define KLB_IOCP_WAIT_MS            5


/// @struct klb_iocp_closing_t
/// @brief  待关闭的 连接
typedef struct klb_iocp_closing_t_
{
    klb_netconn_t*          p_netconn;                  ///< 连接
    char                    name[KLB_NETCONN_NAME_MAX]; ///< 名称
}klb_iocp_closing_t;


/// @struct klb_iocp_item_t
/// @brief  IOCP 连接项
typedef struct klb_iocp_item_t_
{
    klb_netconn_t*          p_netconn;                  ///< 连接
    OVERLAPPED              read_ovl;                     ///< 读重叠 IO
    WSABUF                  read_wsabuf;                ///< 读缓冲描述
    uint8_t                 read_peek_buf[1];           ///< MSG_PEEK 探测缓冲
    bool                    read_posted;                ///< 读重叠 IO 已投递
}klb_iocp_item_t;


/// @struct klb_iocp_t
/// @brief  多路复用(klb net multiplex)
typedef struct klb_iocp_t_
{
    HANDLE                  h_iocp;                     ///< IOCP 句柄
    klb_hlist_t*            p_conn_hlist;               ///< 连接列表; 存储 klb_iocp_item_t*
    klb_nlist_t*            p_closing_nlist;            ///< 待移除的 连接列表; 存储 klb_iocp_closing_t*

    struct
    {
        int64_t             now_tc;                     ///< 计时: 当前时间
        int64_t             last_tc;                    ///< 上一次定时器时间
    };
}klb_iocp_t;


//////////////////////////////////////////////////////////////////////////
static int klb_iocp_loop_once_do_closing(klb_netmulti_t* p_multi);
static klb_iocp_item_t* klb_iocp_find_item(klb_iocp_t* p_io, klb_netconn_t* p_conn);
static void klb_iocp_item_cancel(klb_iocp_item_t* p_item);
static void klb_iocp_item_free(klb_iocp_item_t* p_item);
static bool klb_iocp_is_listen(klb_socket_fd fd);
static bool klb_iocp_is_tcp_connected(klb_socket_fd fd);
static void klb_iocp_on_connected(klb_netconn_t* p_conn, int64_t now);
static void klb_iocp_dispatch_read(klb_iocp_item_t* p_item, int64_t now);
static void klb_iocp_dispatch_write(klb_iocp_item_t* p_item, int64_t now);
static void klb_iocp_post_read(klb_iocp_item_t* p_item, int64_t now);
static void klb_iocp_handle_completion(klb_iocp_t* p_io, klb_iocp_item_t* p_item, DWORD bytes, bool ok, int64_t now);


static klb_iocp_item_t* klb_iocp_find_item(klb_iocp_t* p_io, klb_netconn_t* p_conn)
{
    const char* p_name = klb_netconn_get_name(p_conn);
    int name_len = (int)strlen(p_name);

    return (klb_iocp_item_t*)klb_hlist_find(p_io->p_conn_hlist, p_name, name_len);
}

static void klb_iocp_item_cancel(klb_iocp_item_t* p_item)
{
    if (NULL == p_item || NULL == p_item->p_netconn || NULL == p_item->p_netconn->p_socket)
    {
        return;
    }

    if (p_item->read_posted)
    {
        CancelIoEx((HANDLE)p_item->p_netconn->p_socket->fd, &p_item->read_ovl);
        p_item->read_posted = false;
    }
}

static void klb_iocp_item_free(klb_iocp_item_t* p_item)
{
    if (NULL == p_item)
    {
        return;
    }

    klb_iocp_item_cancel(p_item);
    KLB_FREE(p_item);
}

static bool klb_iocp_is_listen(klb_socket_fd fd)
{
    int val = 0;
    int len = sizeof(val);

    if (0 != getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, (char*)&val, &len))
    {
        return false;
    }

    return (0 != val);
}

static bool klb_iocp_is_tcp_connected(klb_socket_fd fd)
{
    struct sockaddr_in addr;
    int len = sizeof(addr);

    return (0 == getpeername(fd, (struct sockaddr*)&addr, &len));
}

static void klb_iocp_on_connected(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (0 != p_socket->connected)
    {
        return;
    }

    klb_socket_set_connected(p_socket, true);

    if (NULL != p_conn->vtable.on_msg)
    {
        p_conn->vtable.on_msg(p_conn, KLB_NETCONN_MSG_connected, now);
    }
}

static void klb_iocp_dispatch_read(klb_iocp_item_t* p_item, int64_t now)
{
    klb_netconn_t* p_conn = p_item->p_netconn;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != p_socket->status)
    {
        return;
    }

    if (0 == p_socket->connected && klb_iocp_is_tcp_connected(p_socket->fd))
    {
        klb_iocp_on_connected(p_conn, now);
    }

    if (NULL != p_conn->vtable.on_recv)
    {
        int recv_num = p_conn->vtable.on_recv(p_conn, now);
        if (0 < recv_num)
        {
            p_socket->last_recv_tc = now;
        }
    }
}

static void klb_iocp_dispatch_write(klb_iocp_item_t* p_item, int64_t now)
{
    klb_netconn_t* p_conn = p_item->p_netconn;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != p_socket->status || 0 == p_socket->writing)
    {
        return;
    }

    if (0 == p_socket->connected && klb_iocp_is_tcp_connected(p_socket->fd))
    {
        klb_iocp_on_connected(p_conn, now);
    }

    if (NULL != p_conn->vtable.on_send)
    {
        int send_num = p_conn->vtable.on_send(p_conn, now);
        if (0 < send_num)
        {
            p_socket->last_send_tc = now;
        }
    }
}

static void klb_iocp_post_read(klb_iocp_item_t* p_item, int64_t now)
{
    klb_netconn_t* p_conn = p_item->p_netconn;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (p_item->read_posted || 0 == p_socket->reading || KLB_SOCKET_OK != p_socket->status)
    {
        return;
    }

    if (klb_iocp_is_listen(p_socket->fd))
    {
        return;
    }

    if (0 == p_socket->connected && !klb_iocp_is_tcp_connected(p_socket->fd))
    {
        return;
    }

    memset(&p_item->read_ovl, 0, sizeof(OVERLAPPED));
    p_item->read_wsabuf.buf = (char*)p_item->read_peek_buf;
    p_item->read_wsabuf.len = sizeof(p_item->read_peek_buf);

    DWORD flags = MSG_PEEK;
    DWORD bytes = 0;

    int ret = WSARecv(p_socket->fd, &p_item->read_wsabuf, 1, &bytes, &flags, &p_item->read_ovl, NULL);
    if (0 == ret)
    {
        p_item->read_posted = false;
        klb_iocp_dispatch_read(p_item, now);
    }
    else
    {
        int err = WSAGetLastError();
        if (WSA_IO_PENDING == err)
        {
            p_item->read_posted = true;
        }
        else if (WSAECONNRESET == err || WSAENETRESET == err || WSAESHUTDOWN == err)
        {
            klb_iocp_dispatch_read(p_item, now);
        }
        else
        {
            // overlapped MSG_PEEK 不可用时, 退化为直接读
            klb_iocp_dispatch_read(p_item, now);
        }
    }
}

static void klb_iocp_handle_completion(klb_iocp_t* p_io, klb_iocp_item_t* p_item, DWORD bytes, bool ok, int64_t now)
{
    (void)p_io;
    (void)bytes;

    if (NULL == p_item)
    {
        return;
    }

    p_item->read_posted = false;

    if (!ok)
    {
        int err = (int)GetLastError();
        if (ERROR_OPERATION_ABORTED == err)
        {
            return;
        }
    }

    klb_iocp_dispatch_read(p_item, now);
}


/// @brief 创建
static klb_netmulti_t* klb_iocp_create(int64_t tc)
{
    klb_netmulti_t* p_multi = KLB_MALLOCZ(klb_netmulti_t, 1, sizeof(klb_iocp_t));
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    p_io->h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    assert(NULL != p_io->h_iocp);

    p_io->p_conn_hlist = klb_hlist_create(0);
    p_io->p_closing_nlist = klb_nlist_create();

    p_io->now_tc = tc;
    p_io->last_tc = tc;

    return p_multi;
}

/// @brief 销毁
static void klb_iocp_destroy(klb_netmulti_t* p_multi)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    klb_iocp_loop_once_do_closing(p_multi);

    while (0 < klb_hlist_size(p_io->p_conn_hlist))
    {
        klb_iocp_item_t* p_item = (klb_iocp_item_t*)klb_hlist_pop_head(p_io->p_conn_hlist);
        klb_iocp_item_free(p_item);
    }

    KLB_FREE_BY(p_io->p_closing_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_io->p_conn_hlist, klb_hlist_destroy);

    if (NULL != p_io->h_iocp)
    {
        CloseHandle(p_io->h_iocp);
        p_io->h_iocp = NULL;
    }

    KLB_FREE(p_multi);
}

/// @brief 放入连接对象
static int klb_iocp_push(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    char name[KLB_NETCONN_NAME_MAX] = { 0 };
    int name_len = MIN(KLB_NETCONN_NAME_MAX, KLB_NETCONN_NAME_LEN);

    while (true)
    {
        klb_rand_string(name, name_len, true);

        klb_iocp_item_t* p_item = KLB_MALLOCZ(klb_iocp_item_t, 1, 0);
        p_item->p_netconn = p_conn;

        if (NULL != klb_hlist_push_tail(p_io->p_conn_hlist, name, name_len, p_item))
        {
            klb_netconn_set_name(p_conn, name);

            p_conn->p_socket->last_send_tc = p_io->now_tc;
            p_conn->p_socket->last_recv_tc = p_io->now_tc;

            CreateIoCompletionPort((HANDLE)p_conn->p_socket->fd, p_io->h_iocp, (ULONG_PTR)p_item, 0);
            break;
        }

        KLB_FREE(p_item);
    }

    return 0;
}

/// @brief 移除连接对象
static klb_netconn_t* klb_iocp_remove_by_name(klb_netmulti_t* p_multi, const char* p_name)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    int name_len = (int)strlen(p_name);
    klb_iocp_item_t* p_item = (klb_iocp_item_t*)klb_hlist_remove_bykey(p_io->p_conn_hlist, p_name, name_len);

    if (NULL == p_item)
    {
        return NULL;
    }

    klb_netconn_t* p_conn = p_item->p_netconn;
    klb_iocp_item_free(p_item);

    return p_conn;
}

/// @brief 移除连接对象
static int klb_iocp_remove(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    const char* p_name = klb_netconn_get_name(p_conn);
    klb_netconn_t* p_tmp = klb_iocp_remove_by_name(p_multi, p_name);

    if (NULL != p_tmp)
    {
        assert(p_tmp == p_conn);
        return 0;
    }

    return 1;
}

/// @brief 托管关闭连接
static int klb_iocp_closing(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;
    klb_iocp_item_t* p_item = klb_iocp_find_item(p_io, p_conn);

    {
        klb_netconn_bind_recv_data(p_conn, NULL);
        klb_socket_closing(p_conn->p_socket);
    }

    if (NULL != p_item)
    {
        klb_iocp_item_cancel(p_item);
    }

    {
        klb_iocp_closing_t* p_closing = KLB_MALLOCZ(klb_iocp_closing_t, 1, 0);
        p_closing->p_netconn = p_conn;
        strncpy(p_closing->name, p_conn->name, KLB_NETCONN_NAME_MAX);
        p_closing->name[KLB_NETCONN_NAME_MAX - 1] = '\0';

        klb_nlist_push_tail(p_io->p_closing_nlist, p_closing);
    }

    return 0;
}

static int klb_iocp_loop_once_do(klb_netmulti_t* p_multi, int64_t now)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    int fd_num = 0;

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_io->p_conn_hlist);
    while (NULL != p_iter)
    {
        klb_iocp_item_t* p_item = (klb_iocp_item_t*)klb_hlist_data(p_iter);
        klb_socket_t* p_socket = p_item->p_netconn->p_socket;

        if (KLB_SOCKET_OK == p_socket->status)
        {
            if (0 != p_socket->reading || 0 != p_socket->writing)
            {
                fd_num++;
            }

            if (0 == p_socket->connected && 0 != p_socket->reading)
            {
                if (klb_iocp_is_tcp_connected(p_socket->fd))
                {
                    klb_iocp_on_connected(p_item->p_netconn, now);
                }
            }

            if (klb_iocp_is_listen(p_socket->fd))
            {
                if (0 != p_socket->reading)
                {
                    klb_iocp_dispatch_read(p_item, now);
                }
            }
            else
            {
                klb_iocp_post_read(p_item, now);
            }
        }

        p_iter = klb_hlist_next(p_iter);
    }

    if (fd_num <= 0)
    {
        return 0;
    }

    while (true)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED p_ovl = NULL;

        BOOL ok = GetQueuedCompletionStatus(p_io->h_iocp, &bytes, &key, &p_ovl, 0);
        if (!ok && NULL == p_ovl)
        {
            break;
        }

        klb_iocp_item_t* p_item = (klb_iocp_item_t*)key;
        klb_iocp_handle_completion(p_io, p_item, bytes, ok ? true : false, now);

        if (NULL != p_item && KLB_SOCKET_OK == p_item->p_netconn->p_socket->status)
        {
            klb_iocp_post_read(p_item, now);
        }
    }

    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED p_ovl = NULL;

        BOOL ok = GetQueuedCompletionStatus(p_io->h_iocp, &bytes, &key, &p_ovl, KLB_IOCP_WAIT_MS);
        if (ok || NULL != p_ovl)
        {
            klb_iocp_item_t* p_item = (klb_iocp_item_t*)key;
            klb_iocp_handle_completion(p_io, p_item, bytes, ok ? true : false, now);

            if (NULL != p_item && KLB_SOCKET_OK == p_item->p_netconn->p_socket->status)
            {
                klb_iocp_post_read(p_item, now);
            }

            while (true)
            {
                bytes = 0;
                key = 0;
                p_ovl = NULL;

                ok = GetQueuedCompletionStatus(p_io->h_iocp, &bytes, &key, &p_ovl, 0);
                if (!ok && NULL == p_ovl)
                {
                    break;
                }

                p_item = (klb_iocp_item_t*)key;
                klb_iocp_handle_completion(p_io, p_item, bytes, ok ? true : false, now);

                if (NULL != p_item && KLB_SOCKET_OK == p_item->p_netconn->p_socket->status)
                {
                    klb_iocp_post_read(p_item, now);
                }
            }
        }
    }

    p_iter = klb_hlist_begin(p_io->p_conn_hlist);
    while (NULL != p_iter)
    {
        klb_iocp_item_t* p_item = (klb_iocp_item_t*)klb_hlist_data(p_iter);
        klb_socket_t* p_socket = p_item->p_netconn->p_socket;

        if (KLB_SOCKET_OK == p_socket->status && 0 != p_socket->writing)
        {
            klb_iocp_dispatch_write(p_item, now);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return 4;
}

static int klb_iocp_loop_once_do_closing(klb_netmulti_t* p_multi)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    while (0 < klb_nlist_size(p_io->p_closing_nlist))
    {
        klb_iocp_closing_t* p_remove = klb_nlist_pop_head(p_io->p_closing_nlist);
        klb_netconn_t* p_conn = klb_iocp_remove_by_name(p_multi, p_remove->name);

        KLB_FREE_BY(p_conn, klb_netconn_destroy);
        KLB_FREE(p_remove);
    }

    return 0;
}

static int klb_iocp_loop_once_onticker(klb_netmulti_t* p_multi, int64_t now)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_io->p_conn_hlist);
    while (NULL != p_iter)
    {
        klb_iocp_item_t* p_item = (klb_iocp_item_t*)klb_hlist_data(p_iter);
        klb_netconn_t* p_conn = p_item->p_netconn;

        if (NULL != p_conn->vtable.on_msg)
        {
            if (0 == p_conn->p_socket->connected &&
                KLB_SOCKET_OK == p_conn->p_socket->status &&
                30 * 1000 < ABS_SUB(now, p_conn->p_socket->last_recv_tc))
            {
                p_conn->vtable.on_msg(p_conn, KLB_NETCONN_MSG_connect_timeout, now);
            }
            else if (KLB_SOCKET_OK == p_conn->p_socket->status)
            {
                p_conn->vtable.on_msg(p_conn, KLB_NETCONN_MSG_onticker, now);
            }
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return 0;
}

static int klb_iocp_loop_once(klb_netmulti_t* p_multi, int64_t now)
{
    klb_iocp_t* p_io = (klb_iocp_t*)p_multi->extra;

    p_io->now_tc = now;

    klb_iocp_loop_once_do_closing(p_multi);

    int ret = klb_iocp_loop_once_do(p_multi, now);

    klb_iocp_loop_once_do_closing(p_multi);

    if (2 * 1000 <= ABS_SUB(now, p_io->last_tc))
    {
        klb_iocp_loop_once_onticker(p_multi, now);
        p_io->last_tc = now;
    }

    return ret;
}


//////////////////////////////////////////////////////////////////////////


klb_iopoll_ops_t g_klb_iopoll_ops = {
    .create = klb_iocp_create,
    .destroy = klb_iocp_destroy,

    .push = klb_iocp_push,

    .remove = klb_iocp_remove,
    .remove_by_name = klb_iocp_remove_by_name,

    .closing = klb_iocp_closing,
    .loop_once = klb_iocp_loop_once,
};

#endif // _WIN32
//end
