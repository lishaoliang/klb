// 引用select之前定义
#define FD_SETSIZE                  1024

#include "klua/extension/klua_ex_multiplex.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_list.h"
#include <assert.h>


#define KLUA_EX_MULTIPLEX_NAME      "_KLUA_EX_MULTIPLEX_"

#define KLUA_EX_MULTIPLEX_ID_MIN    1000
#define KLUA_EX_MULTIPLEX_ID_MAX    0x7FFF0000


typedef struct klua_ex_multiplex_remove_t_
{
    int                     id;
}klua_ex_multiplex_remove_t;


typedef struct klua_ex_multiplex_item_t_
{
    int                     id;
    klb_socket_t*           p_socket;
    klua_ex_multiplex_obj_t obj;
}klua_ex_multiplex_item_t;


typedef struct klua_ex_multiplex_t_
{
    klua_env_t*     p_env;          ///< Lua环境

    klb_hlist_t*    p_items_hlist;  ///< 当前所有元素: klua_ex_multiplex_item_t*
    klb_list_t*     p_remove_list;  ///< 待移除列表: klua_ex_multiplex_remove_t*

    int             next_id;        ///< 下一个ID号
}klua_ex_multiplex_t;


//////////////////////////////////////////////////////////////////////////
static int klua_ex_multiplex_next_id(klua_ex_multiplex_t* p_ex)
{
    int id = p_ex->next_id;

    p_ex->next_id++;
    if (KLUA_EX_MULTIPLEX_ID_MAX <= p_ex->next_id)
    {
        p_ex->next_id = KLUA_EX_MULTIPLEX_ID_MIN;
    }

    return id;
}

static int klua_ex_multiplex_get_id(klua_ex_multiplex_t* p_ex)
{
    while (true)
    {
        int id = klua_ex_multiplex_next_id(p_ex);
        if (NULL == klb_hlist_find_iter(p_ex->p_items_hlist, &id, sizeof(id)))
        {
            return id;
        }
    }

    assert(false);
    return KLUA_EX_MULTIPLEX_ID_MIN;
}

static void* klua_ex_multiplex_create(klua_env_t* p_env)
{
    klua_ex_multiplex_t* p_ex = KLB_MALLOC(klua_ex_multiplex_t, 1, 0);
    KLB_MEMSET(p_ex, 0, sizeof(klua_ex_multiplex_t));

    p_ex->p_env = p_env;

    p_ex->p_items_hlist = klb_hlist_create(0);
    p_ex->p_remove_list = klb_list_create();

    p_ex->next_id = KLUA_EX_MULTIPLEX_ID_MIN;

    return p_ex;
}

static void klua_ex_multiplex_destroy(void* ptr)
{
    klua_ex_multiplex_t* p_ex = (klua_ex_multiplex_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    // 移除
    while (0 < klb_list_size(p_ex->p_remove_list))
    {
        klua_ex_multiplex_remove_t* p_remove = (klua_ex_multiplex_remove_t*)klb_list_pop_head(p_ex->p_remove_list);
        KLB_FREE(p_remove);
    }

    // 通知移除
    while (0 < klb_hlist_size(p_ex->p_items_hlist))
    {
        klua_ex_multiplex_item_t* p_item = (klua_ex_multiplex_item_t*)klb_hlist_pop_head(p_ex->p_items_hlist);

        if (p_item->obj.cb_remove)
        {
            p_item->obj.cb_remove(p_env, p_item->obj.p_lparam, p_item->obj.p_wparam, p_item->id);
        }

        KLB_FREE(p_item);
    }

    // 销毁
    KLB_FREE_BY(p_ex->p_remove_list, klb_list_destroy);
    KLB_FREE_BY(p_ex->p_items_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_multiplex_loop_once_do(klua_ex_multiplex_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_fd fd_num = 0, fd_max = 0;

    fd_set r_fds, w_fds;
    FD_ZERO(&r_fds);
    FD_ZERO(&w_fds);

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_ex->p_items_hlist);
    while (NULL != p_iter)
    {
        klua_ex_multiplex_item_t* p_item = (klua_ex_multiplex_item_t*)klb_hlist_data(p_iter);
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
            if (0 != p_socket->sending)
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
        p_iter = klb_hlist_begin(p_ex->p_items_hlist);
        while (NULL != p_iter)
        {
            klua_ex_multiplex_item_t* p_item = (klua_ex_multiplex_item_t*)klb_hlist_data(p_iter);
            klb_socket_t* p_socket = p_item->p_socket;

            if (KLB_SOCKET_OK == p_socket->status)
            {
                // 正常的连接
                if (FD_ISSET(p_socket->fd, &r_fds))
                {
                    // 注意读取数据前, 须把缓存中的数据处理完毕
                    int recv = p_item->obj.cb_recv(p_env, p_item->obj.p_lparam, p_item->obj.p_wparam, p_item->id, now);

                    if (0 < recv)
                    {
                        p_socket->last_recv_tc = now; // 更新读时间
                    }
                }

                if (FD_ISSET(p_socket->fd, &w_fds))
                {
                    int send = p_item->obj.cb_send(p_env, p_item->obj.p_lparam, p_item->obj.p_wparam, p_item->id, now);

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

    return 0;
}

static int klua_ex_multiplex_loop_once_remove(klua_ex_multiplex_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    // 处理所有待移除
    while (0 < klb_list_size(p_ex->p_remove_list))
    {
        klua_ex_multiplex_remove_t* p_remove = (klua_ex_multiplex_remove_t*)klb_list_pop_head(p_ex->p_remove_list);

        int id = p_remove->id;

        klua_ex_multiplex_item_t* p_item = (klua_ex_multiplex_item_t*)klb_hlist_remove_bykey(p_ex->p_items_hlist, &id, sizeof(id));
        if (NULL != p_item && NULL != p_item->obj.cb_remove)
        {
            assert(id == p_item->id);

            // 通知 已经被移除了
            p_item->obj.cb_remove(p_env, p_item->obj.p_lparam, p_item->obj.p_wparam, p_item->id);
        }

        KLB_FREE(p_remove);
        KLB_FREE(p_item);
    }

    return 0;
}

static int klua_ex_multiplex_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_multiplex_t* p_ex = (klua_ex_multiplex_t*)ptr;

    // 执行主体业务
    klua_ex_multiplex_loop_once_do(p_ex, p_env, last_tc, now);

    // 可移除对象
    klua_ex_multiplex_loop_once_remove(p_ex, p_env, last_tc, now);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_multiplex_push_socket(klua_ex_multiplex_t* p_ex, klb_socket_t* p_socket, const klua_ex_multiplex_obj_t* p_obj)
{
    int id = klua_ex_multiplex_get_id(p_ex);

    klua_ex_multiplex_item_t* p_item = KLB_MALLOC(klua_ex_multiplex_item_t, 1, 0);
    KLB_MEMSET(p_item, 0, sizeof(klua_ex_multiplex_item_t));

    p_item->id = id;
    p_item->p_socket = p_socket;

    memcpy(&p_item->obj, p_obj, sizeof(klua_ex_multiplex_obj_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_items_hlist, &id, sizeof(id), p_item);
    assert(NULL != p_iter);

    return id;
}

int klua_ex_multiplex_remove(klua_ex_multiplex_t* p_ex, int id)
{
    klua_ex_multiplex_remove_t* p_remove = KLB_MALLOC(klua_ex_multiplex_remove_t, 1, 0);
    KLB_MEMSET(p_remove, 0, sizeof(klua_ex_multiplex_remove_t));

    p_remove->id = id;

    klb_list_push_tail(p_ex->p_remove_list, p_remove);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_multiplex(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_multiplex_create;
    ex.cb_destroy = klua_ex_multiplex_destroy;
    ex.cb_loop_once = klua_ex_multiplex_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_MULTIPLEX_NAME, &ex);

    return 0;
}

klua_ex_multiplex_t* klua_ex_get_multiplex(klua_env_t* p_env)
{
    klua_ex_multiplex_t* p_ex = (klua_ex_multiplex_t*)klua_env_get_extension(p_env, KLUA_EX_MULTIPLEX_NAME);
    return p_ex;
}
