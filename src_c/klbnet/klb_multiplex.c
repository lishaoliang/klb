// Doc-Encode UTF8-BOM, Space(4), Unix(LF)

// 引用select之前定义
#define FD_SETSIZE                  1024

#include "klbnet/klb_multiplex.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include <assert.h>

#define KLB_MULTIPLEX_ID_MIN            1000            ///< 分配id的最小值
#define KLB_MULTIPLEX_ID_MAX            0x7FFF0000      ///< 分配id的最大值

#define KLB_MULTIPLEX_TIMER             (2 * 1000)      ///< 定时检查间隔(ms)
#define KLB_MULTIPLEX_connect_timeout   (30 * 1000)     ///< 首次发起connect超时时间


typedef struct klb_multiplex_remove_t_
{
    int                 id;
}klb_multiplex_remove_t;


typedef struct klb_multiplex_item_t_
{
    int                 id;
    klb_socket_t*       p_socket;

    klb_multiplex_ops_t ops;
}klb_multiplex_item_t;


typedef struct klb_multiplex_t_
{
    klb_hlist_t*        p_items_hlist;  ///< 当前所有元素: klb_multiplex_item_t*
    klb_nlist_t*         p_remove_list;  ///< 待移除列表: klb_multiplex_remove_t*

    int                 next_id;        ///< 下一个ID号
    int64_t             tc;             ///< 计时: 当前时间
    int64_t             timer_tc;       ///< 上一次定时器时间
}klb_multiplex_t;


//////////////////////////////////////////////////////////////////////////
static int klb_multiplex_next_id(klb_multiplex_t* p_ex)
{
    int id = p_ex->next_id;

    p_ex->next_id++;
    if (KLB_MULTIPLEX_ID_MAX <= p_ex->next_id)
    {
        p_ex->next_id = KLB_MULTIPLEX_ID_MIN;
    }

    return id;
}

static int klb_multiplex_get_id(klb_multiplex_t* p_ex)
{
    while (true)
    {
        int id = klb_multiplex_next_id(p_ex);
        if (NULL == klb_hlist_find_iter(p_ex->p_items_hlist, &id, sizeof(id)))
        {
            return id;
        }
    }

    assert(false);
    return KLB_MULTIPLEX_ID_MIN;
}

klb_multiplex_t* klb_multiplex_create(int64_t tc)
{
    klb_multiplex_t* p_multi = KLB_MALLOC(klb_multiplex_t, 1, 0);
    KLB_MEMSET(p_multi, 0, sizeof(klb_multiplex_t));

    p_multi->p_items_hlist = klb_hlist_create(0);
    p_multi->p_remove_list = klb_nlist_create();

    p_multi->next_id = KLB_MULTIPLEX_ID_MIN;
    p_multi->tc = tc;
    p_multi->timer_tc = tc;

    return p_multi;
}

void klb_multiplex_destroy(klb_multiplex_t* p_multi)
{
    // 移除
    while (0 < klb_nlist_size(p_multi->p_remove_list))
    {
        klb_multiplex_remove_t* p_remove = (klb_multiplex_remove_t*)klb_nlist_pop_head(p_multi->p_remove_list);
        KLB_FREE(p_remove);
    }

    // 通知移除
    while (0 < klb_hlist_size(p_multi->p_items_hlist))
    {
        klb_multiplex_item_t* p_item = (klb_multiplex_item_t*)klb_hlist_pop_head(p_multi->p_items_hlist);

        if (p_item->ops.cb_remove)
        {
            p_item->ops.cb_remove(p_item->ops.p_lparam, p_item->ops.p_wparam, p_item->id);
        }

        KLB_FREE(p_item);
    }

    // 销毁
    KLB_FREE_BY(p_multi->p_remove_list, klb_nlist_destroy);
    KLB_FREE_BY(p_multi->p_items_hlist, klb_hlist_destroy);
    KLB_FREE(p_multi);
}

static int klb_multiplex_loop_once_do(klb_multiplex_t* p_multi, int64_t now)
{
    klb_socket_fd fd_num = 0, fd_max = 0;

    fd_set r_fds, w_fds;
    FD_ZERO(&r_fds);
    FD_ZERO(&w_fds);

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_multi->p_items_hlist);
    while (NULL != p_iter)
    {
        klb_multiplex_item_t* p_item = (klb_multiplex_item_t*)klb_hlist_data(p_iter);
        klb_socket_t* p_socket = p_item->p_socket;

        if (KLB_SOCKET_OK == p_socket->status)
        {
            bool ok = false;

            // 设置读标记
            if (0 != p_socket->reading)
            {
                FD_SET(p_socket->fd, &r_fds);
                ok = true;
            }

            // 设置写标记
            if (0 != p_socket->writing)
            {
                FD_SET(p_socket->fd, &w_fds);
                ok = true;
            }

            if (ok)
            {
                if (fd_max < p_socket->fd)
                {
                    fd_max = p_socket->fd;
                }

                fd_num++;
            }
        }

        p_iter = klb_hlist_next(p_iter);
    }

    if (fd_num <= 0)
    {
        return 0;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;  // 5ms

    int ret = select(fd_max + 1, &r_fds, &w_fds, NULL, &tv);

    if (0 < ret)
    {
        // 遍历列表, 判定读写
        p_iter = klb_hlist_begin(p_multi->p_items_hlist);
        while (NULL != p_iter)
        {
            klb_multiplex_item_t* p_item = (klb_multiplex_item_t*)klb_hlist_data(p_iter);
            klb_socket_t* p_socket = p_item->p_socket;

            if (KLB_SOCKET_OK == p_socket->status)
            {
                // 正常的连接
                if (p_socket->reading && FD_ISSET(p_socket->fd, &r_fds))
                {
                    if (0 == p_socket->connected)
                    {
                        klb_socket_set_connected(p_socket, true);

                        if (p_item->ops.cb_proc)
                        {
                            p_item->ops.cb_proc(p_item->ops.p_lparam, p_item->ops.p_wparam, KLB_MULTIPLEX_MSG_connect, p_item->id, p_item->p_socket, now);
                        }
                    }

                    // 注意读取数据前, 须把缓存中的数据处理完毕
                    int recv = p_item->ops.cb_recv(p_item->ops.p_lparam, p_item->ops.p_wparam, p_item->id, now);

                    if (0 < recv)
                    {
                        p_socket->last_recv_tc = now; // 更新读时间
                    }
                }

                if (p_socket->writing && FD_ISSET(p_socket->fd, &w_fds))
                {
                    if (0 == p_socket->connected)
                    {
                        klb_socket_set_connected(p_socket, true);

                        if (p_item->ops.cb_proc)
                        {
                            p_item->ops.cb_proc(p_item->ops.p_lparam, p_item->ops.p_wparam, KLB_MULTIPLEX_MSG_connect, p_item->id, p_item->p_socket, now);
                        }
                    }

                    int send = p_item->ops.cb_send(p_item->ops.p_lparam, p_item->ops.p_wparam, p_item->id, now);

                    if (0 < send)
                    {
                        p_socket->last_send_tc = now;  // 更新写时间
                    }
                }
            }

            p_iter = klb_hlist_next(p_iter);
        }

    }
    else if (0 == ret)
    {
        // 没有可读写的socket, 还需要等待
    }
    else
    {
        // select失败, 没有数据可读
        //assert(false);
    }

    return 4;
}

static int klb_multiplex_loop_once_remove(klb_multiplex_t* p_multi, int64_t now)
{
    // 处理所有待移除
    while (0 < klb_nlist_size(p_multi->p_remove_list))
    {
        klb_multiplex_remove_t* p_remove = (klb_multiplex_remove_t*)klb_nlist_pop_head(p_multi->p_remove_list);

        int id = p_remove->id;

        klb_multiplex_item_t* p_item = (klb_multiplex_item_t*)klb_hlist_remove_bykey(p_multi->p_items_hlist, &id, sizeof(id));
        if (NULL != p_item && NULL != p_item->ops.cb_remove)
        {
            assert(id == p_item->id);

            // 通知 已经被移除了
            p_item->ops.cb_remove(p_item->ops.p_lparam, p_item->ops.p_wparam, p_item->id);
        }

        KLB_FREE(p_remove);
        KLB_FREE(p_item);
    }

    return 0;
}

static int klb_multiplex_loop_once_timer(klb_multiplex_t* p_multi, int64_t now)
{
    // timer 流程, 可用于检查超时等

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_multi->p_items_hlist);
    while (NULL != p_iter)
    {
        klb_multiplex_item_t* p_item = (klb_multiplex_item_t*)klb_hlist_data(p_iter);
        assert(NULL != p_item);

        if (0 == p_item->p_socket->connected && 
            p_item->ops.cb_proc &&
            KLB_SOCKET_OK == p_item->p_socket->status &&
            KLB_MULTIPLEX_connect_timeout < ABS_SUB(now, p_item->p_socket->last_recv_tc))
        {
            p_item->ops.cb_proc(p_item->ops.p_lparam, p_item->ops.p_wparam, KLB_MULTIPLEX_MSG_timeout, p_item->id, p_item->p_socket, now);
        }

        if (p_item->ops.cb_timer && KLB_SOCKET_OK == p_item->p_socket->status)
        {            
            p_item->ops.cb_timer(p_item->ops.p_lparam, p_item->ops.p_wparam, p_item->id, now);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return 0;
}

int klb_multiplex_loop_once(klb_multiplex_t* p_multi, int64_t now)
{
    // 更新计时
    p_multi->tc = now;

    // 执行主体业务
    int ret = klb_multiplex_loop_once_do(p_multi, now);

    // 可移除对象
    klb_multiplex_loop_once_remove(p_multi, now);

    // 定时器
    if (KLB_MULTIPLEX_TIMER <= ABS_SUB(now, p_multi->timer_tc))
    {
        klb_multiplex_loop_once_timer(p_multi, now);
        p_multi->timer_tc = now;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////

int klb_multiplex_push_socket(klb_multiplex_t* p_multi, klb_socket_t* p_socket, const klb_multiplex_ops_t* p_ops)
{
    int id = klb_multiplex_get_id(p_multi);

    klb_multiplex_item_t* p_item = KLB_MALLOCZ(klb_multiplex_item_t, 1, 0);

    p_item->id = id;
    p_item->p_socket = p_socket;

    // 更新计时
    p_socket->last_send_tc = p_multi->tc;
    p_socket->last_recv_tc = p_multi->tc;

    memcpy(&p_item->ops, p_ops, sizeof(klb_multiplex_ops_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_multi->p_items_hlist, &id, sizeof(id), p_item);
    assert(NULL != p_iter);

    return id;
}

int klb_multiplex_remove(klb_multiplex_t* p_multi, int id)
{
    klb_multiplex_remove_t* p_remove = KLB_MALLOCZ(klb_multiplex_remove_t, 1, 0);

    p_remove->id = id;

    klb_nlist_push_tail(p_multi->p_remove_list, p_remove);

    return 0;
}
