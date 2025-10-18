// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_netmulti.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_rand.h"
#include <assert.h>


/// @struct klb_netmulti_t
/// @brief  多路复用(klb net multiplex)
typedef struct klb_netmulti_t_
{
    klb_hlist_t*            p_conn_hlist;       ///< 存储 klb_netconn_t*

    struct
    {
        int64_t             now_tc;             ///< 计时: 当前时间
        int64_t             last_tc;            ///< 上一次定时器时间
    };
}klb_netmulti_t;


//////////////////////////////////////////////////////////////////////////


/// @brief 创建
klb_netmulti_t* klb_netmulti_create(int64_t tc)
{
    klb_netmulti_t* p_multi = KLB_MALLOCZ(klb_netmulti_t, 1, 0);

    p_multi->p_conn_hlist = klb_hlist_create(0);

    p_multi->now_tc = tc;       ///< 计时: 当前时间
    p_multi->last_tc = tc;

    return p_multi;
}

/// @brief 销毁
void klb_netmulti_destroy(klb_netmulti_t* p_multi)
{
    KLB_FREE_BY(p_multi->p_conn_hlist, klb_hlist_destroy);
    KLB_FREE(p_multi);
}

/// @brief 放入连接对象
int klb_netmulti_push(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    char name[KLB_NETCONN_NAME_max] = { 0 };

    // 65^3 = 274,625
    // 65^4 = 17,850,625
    int name_len = MIN(KLB_NETCONN_NAME_max, 4);

    while (true)
    {
        klb_rand_string(name, name_len, true);

        if (NULL != klb_hlist_push_tail(p_multi->p_conn_hlist, name, name_len, p_conn))
        {
            // 成功, 修改名称
            klb_netconn_set_name(p_conn, name);

            // 更新计时
            p_conn->p_socket->last_send_tc = p_multi->now_tc;
            p_conn->p_socket->last_recv_tc = p_multi->now_tc;

            break;
        }
    }

    return 0;
}

/// @brief 移除连接对象
/// @return int 0.成功; 非0. 失败
int klb_netmulti_remove(klb_netmulti_t* p_multi, klb_netconn_t* p_conn)
{
    const char* p_name = klb_netconn_get_name(p_conn);

    klb_netconn_t* p_tmp = klb_netmulti_remove_by_name(p_multi, p_name);

    if (NULL != p_tmp)
    {
        assert(p_tmp == p_conn);
        return 0;
    }

    return 1;
}

/// @brief 移除连接对象
/// @return int 0.成功; 非0. 失败
klb_netconn_t* klb_netmulti_remove_by_name(klb_netmulti_t* p_multi, const char* p_name)
{
    int name_len = strlen(p_name);
    klb_netconn_t* p_conn = (klb_netconn_t*)klb_hlist_remove_bykey(p_multi->p_conn_hlist, p_name, name_len);

    return p_conn;
}


static int klb_netmulti_loop_once_do(klb_netmulti_t* p_multi, int64_t now)
{
    klb_socket_fd fd_num = 0, fd_max = 0;

    fd_set r_fds, w_fds;
    FD_ZERO(&r_fds);
    FD_ZERO(&w_fds);

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_multi->p_conn_hlist);
    while (NULL != p_iter)
    {
        klb_netconn_t* p_item = (klb_netconn_t*)klb_hlist_data(p_iter);
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
        p_iter = klb_hlist_begin(p_multi->p_conn_hlist);
        while (NULL != p_iter)
        {
            klb_netconn_t* p_item = (klb_netconn_t*)klb_hlist_data(p_iter);
            klb_socket_t* p_socket = p_item->p_socket;

            if (KLB_SOCKET_OK == p_socket->status)
            {
                // 正常的连接
                if (p_socket->reading && FD_ISSET(p_socket->fd, &r_fds))
                {
                    if (0 == p_socket->connected)
                    {
                        klb_socket_set_connected(p_socket, true);

                        if (NULL != p_item->vtable.on_msg)
                        {
                            p_item->vtable.on_msg(p_item, KLB_NETCONN_MSG_connected, now); // 连接握手完成
                        }
                    }

                    // 注意读取数据前, 须把缓存中的数据处理完毕
                    if (NULL != p_item->vtable.on_recv)
                    {
                        int recv_num = p_item->vtable.on_recv(p_item, now);
                        if (0 < recv_num)
                        {
                            p_socket->last_recv_tc = now; // 更新读时间
                        }
                    }
                }

                if (p_socket->writing && FD_ISSET(p_socket->fd, &w_fds))
                {
                    if (0 == p_socket->connected)
                    {
                        klb_socket_set_connected(p_socket, true);

                        if (NULL != p_item->vtable.on_msg)
                        {
                            p_item->vtable.on_msg(p_item, KLB_NETCONN_MSG_connected, now); // 连接握手完成
                        }
                    }

                    if (NULL != p_item->vtable.on_send)
                    {
                        int send_num = p_item->vtable.on_send(p_item, now);
                        if (0 < send_num)
                        {
                            p_socket->last_send_tc = now;  // 更新写时间
                        }
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

static int klb_netmulti_loop_once_onticker(klb_netmulti_t* p_multi, int64_t now)
{
    // timer 流程, 可用于检查超时等

    klb_hlist_iter_t* p_iter = (klb_hlist_iter_t*)klb_hlist_begin(p_multi->p_conn_hlist);
    while (NULL != p_iter)
    {
        klb_netconn_t* p_item = (klb_netconn_t*)klb_hlist_data(p_iter);

        if (NULL != p_item->vtable.on_msg)
        {
            if (0 == p_item->p_socket->connected &&
                KLB_SOCKET_OK == p_item->p_socket->status &&
                30 * 1000 < ABS_SUB(now, p_item->p_socket->last_recv_tc))
            {
                p_item->vtable.on_msg(p_item, KLB_NETCONN_MSG_connect_timeout, now);
            }
            else if(KLB_SOCKET_OK == p_item->p_socket->status)
            {
                p_item->vtable.on_msg(p_item, KLB_NETCONN_MSG_onticker, now);
            }
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return 0;
}

/// @brief 调用一次; 需要定期调用
int klb_netmulti_loop_once(klb_netmulti_t* p_multi, int64_t now)
{
    // 更新计时
    p_multi->now_tc = now;

    // 执行主体业务
    int ret = klb_netmulti_loop_once_do(p_multi, now);

    // 定时器
    if (2 * 1000 <= ABS_SUB(now, p_multi->last_tc))
    {
        klb_netmulti_loop_once_onticker(p_multi, now);
        p_multi->last_tc = now;
    }

    return ret;
}

//end
