// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_help.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef struct klb_ncm_ops_rpc_lua_mnp_t_
{
    // ncm 参数
    struct
    {
        klb_ncm_ops_lparam_t lparam;    ///< ncm参数1
        klb_ncm_ops_wparam_t wparam;    ///< ncm参数2

        klb_ncm_t*           p_ncm;     ///< ncm模块
        int                  protocol;  ///< 协议编号
        int                  id;        ///< id编号
    };

    // send发送相关
    struct
    {
        klb_nlist_t*     p_w_list;       ///< 待发送列表: klb_buf_t*
        klb_buf_t*      p_w_cur;        ///< 当前正在发送的缓存
        int             w_start;        ///< 当前发送的起始点
    };

    // recv 接收/解析
    struct
    {
        klb_buf_t*      p_r_buf;        ///< 临时读取缓存

        int             status_parse;   ///< 解析状态
#define KNORLM_PARSE_HEAD 0             ///< 解析mnp头
#define KNORLM_PARSE_BODY 1             ///< 解析mnp数据体

        klb_mnp_t       mnp;            ///< mnp头
        int             remain_len;     ///< 剩下的数据长度

        klb_buffer_t*   p_rpc_lua;      ///< 缓存rpc/lua数据
    };
}klb_ncm_ops_rpc_lua_mnp_t;


//////////////////////////////////////////////////////////////////////////

static int parse_klb_ncm_ops_rpc_lua_mnp(klb_ncm_ops_rpc_lua_mnp_t* p_ops, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_ops->p_r_buf;

    char* p_data = p_buf->p_buf + p_buf->start;
    int data_len = p_buf->end - p_buf->start;

    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    if (KNORLM_PARSE_HEAD ==  p_ops->status_parse)
    {
        // 解析头
        if (data_len < sizeof(klb_mnp_t))
        {
            return 1; // 数据长度不足
        }

        klb_mnp_t mnp = { 0 };
        memcpy(&mnp, p_data, sizeof(klb_mnp_t));
        
        p_ops->mnp = mnp;
        p_ops->remain_len = mnp.size - sizeof(klb_mnp_t);

        p_ops->status_parse = KNORLM_PARSE_BODY;
        p_buf->start += sizeof(klb_mnp_t);
    }
    else
    {
        int r_len = MIN(p_ops->remain_len, data_len);
        p_ops->remain_len -= r_len;

        if(KLB_MNP_PING == p_ops->mnp.packtype)
        {

        }
        else if(KLB_MNP_RPC_LUA == p_ops->mnp.packtype)
        {
            // 加入缓存
            klb_buffer_write(p_ops->p_rpc_lua, p_data, r_len);

            if (p_ops->remain_len <= 0 &&(KLB_MNP_FULL == p_ops->mnp.opt || KLB_MNP_END == p_ops->mnp.opt))
            {
                // 数据完整, 且为结束包
                klb_buf_t* p_bin = klb_buffer_join(p_ops->p_rpc_lua, p_ops->lparam.cb_malloc, p_ops->lparam.p_pool);

                klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_bin->p_buf + p_bin->start);
                assert(p_bin->end - p_bin->start == p_com->size);

                p_ops->lparam.cb_recv(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_RPC_LUA, p_bin);
                klb_buf_unref_next(p_bin);

                klb_buffer_reset(p_ops->p_rpc_lua);
            }
        }

        // 数据解析完成, 变更为解析头部
        if (p_ops->remain_len <= 0)
        {
            assert(0 == p_ops->remain_len);
            p_ops->status_parse = KNORLM_PARSE_HEAD;
        }

        p_buf->start += r_len;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建连接
/// @param [in] *p_ncm          ncm模块
/// @return void* 连接的指针
static void* create_klb_ncm_ops_rpc_lua_mnp(klb_ncm_ops_lparam_t* p_lparam, klb_ncm_ops_wparam_t* p_wparam)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = KLB_MALLOCZ(klb_ncm_ops_rpc_lua_mnp_t, 1, 0);

    memcpy(&p_ops->lparam, p_lparam, sizeof(klb_ncm_ops_lparam_t));
    memcpy(&p_ops->wparam, p_wparam, sizeof(klb_ncm_ops_wparam_t));

    p_ops->p_ncm = (klb_ncm_t*)p_lparam->p_ncm;
    p_ops->protocol = p_lparam->protocol;
    p_ops->id = p_lparam->id;

    // send发送相关
    p_ops->p_w_list = klb_nlist_create();

    // recv接收相关
    p_ops->p_r_buf = klb_buf_malloc(KLB_MNP_BLOCK_SIZE_MAX, false);

    // 缓存
    p_ops->p_rpc_lua = klb_buffer_create(KLB_MNP_BLOCK_SIZE_MAX);

    return p_ops;
}

/// @brief 销毁连接
/// @param [in] *ptr            连接的指针
/// @return 无
static void destroy_klb_ncm_ops_rpc_lua_mnp(void* ptr)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_opt = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    // w
    while (0 < klb_nlist_size(p_opt->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_opt->p_w_list);
        klb_buf_unref_next(p_tmp);
    }

    if (NULL != p_opt->p_w_cur)
    {
        klb_buf_unref_next(p_opt->p_w_cur);
        p_opt->p_w_cur = NULL;
    }

    // 销毁
    KLB_FREE_BY(p_opt->p_rpc_lua, klb_buffer_destroy);
    KLB_FREE_BY(p_opt->p_w_list, klb_nlist_destroy);
    KLB_FREE_BY(p_opt->p_r_buf, free);

    KLB_FREE(p_opt);
}


static int init_klb_ncm_ops_rpc_lua_mnp(void* ptr, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    klb_buf_t* p_buf = p_ops->p_r_buf;
    assert(data_len <= p_buf->buf_len);

    memcpy(p_buf->p_buf, p_data, data_len);
    p_buf->end = data_len;

    while (true)
    {
        if (0 != parse_klb_ncm_ops_rpc_lua_mnp(p_ops, p_socket))
        {
            break;
        }
    }

    return 0;
}

/// @brief 主动发送RPC二进制数据
/// @param [in] *ptr            ops对象
/// @return int
static int send_rpc_klb_ncm_ops_rpc_lua_mnp(void* ptr, klb_socket_t* p_socket, int packtype, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    if (KLB_MNP_RPC_LUA != packtype)
    {
        return 0;
    }

    klb_buf_t* p_buf = klb_mnp_pack_data(KLB_MNP_RPC_LUA, sequence, uid, p_head, head_len, p_body, body_len);
    if (NULL != p_buf)
    {
        klb_nlist_push_tail(p_ops->p_w_list, p_buf);
        klb_socket_set_writing(p_socket, true);
    }

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_send_klb_ncm_ops_rpc_lua_mnp(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;
    bool first = true;

    while (true)
    {
        if (NULL == p_ops->p_w_cur && 0 < klb_nlist_size(p_ops->p_w_list))
        {
            p_ops->p_w_cur = (klb_buf_t*)klb_nlist_pop_head(p_ops->p_w_list);
            p_ops->w_start = p_ops->p_w_cur->start;
        }

        klb_buf_t* p_buf = p_ops->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, (const uint8_t*)(p_buf->p_buf + p_ops->w_start), p_buf->end - p_ops->w_start);

        if (0 < w)
        {
            send += w;
            p_ops->w_start += w;

            if (p_buf->end <= p_ops->w_start)
            {
                p_ops->p_w_cur = p_buf->p_next;
                p_ops->w_start = (NULL != p_ops->p_w_cur) ? p_ops->p_w_cur->start : 0;

                klb_buf_unref(p_buf);
            }
        }
        else
        {
            if (first && KLB_SOCKET_WANT_WRITE != p_socket->status_rw)
            {
                // select成功, 却第一次无法写, 出现错误
                err = KLB_SOCKET_DISCONNECT;
            }

            break; // 无法再发送
        }

        first = false;
    }

    if (NULL == p_ops->p_w_cur && klb_nlist_size(p_ops->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        p_ops->lparam.cb_recv(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return send;
}

/// @brief 当网络上可以接收数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_recv_klb_ncm_ops_rpc_lua_mnp(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int recv = 0;
    bool first = true;

    while (true)
    {
        klb_buf_t* p_buf = p_ops->p_r_buf;

        int r = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            while (true)
            {
                if (KLB_SOCKET_OK != klb_socket_get_status(p_socket) ||
                    0 != parse_klb_ncm_ops_rpc_lua_mnp(p_ops, p_socket))
                {
                    break;
                }
            }

            if (p_buf->end <= p_buf->start)
            {
                p_buf->start = 0;
                p_buf->end = 0;
            }
            else if (0 < p_buf->start)
            {
                memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
                p_buf->end = p_buf->end - p_buf->start;
                p_buf->start = 0;
            }
        }
        else
        {
            if (first && KLB_SOCKET_WANT_READ != p_socket->status_rw)
            {
                // select成功, 却第一次读不到数据, 表明对方已断开 
                err = KLB_SOCKET_DISCONNECT;
            }
            break; // 无法再接收
        }

        first = false;
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        p_ops->lparam.cb_recv(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return recv;
}

static int on_proc_klb_ncm_ops_rpc_lua_mnp(void* ptr, klb_socket_t* p_socket, int msg, int64_t now)
{
    klb_ncm_ops_rpc_lua_mnp_t* p_ops = (klb_ncm_ops_rpc_lua_mnp_t*)ptr;

    switch (msg)
    {
    case KLB_MULTIPLEX_MSG_connect:
        {
            p_ops->lparam.cb_recv(p_ops->p_ncm, p_ops->protocol, p_ops->id, KLB_SOCKET_CONNECT, 0, NULL);
        }
        break;
    case KLB_MULTIPLEX_MSG_timeout:
        {
            klb_socket_set_status(p_socket, KLB_SOCKET_TIMEOUT);
            p_ops->lparam.cb_recv(p_ops->p_ncm, p_ops->protocol, p_ops->id, KLB_SOCKET_TIMEOUT, 0, NULL);
        }
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klb_ncm_init_ops_rpc_lua_mnp(klb_ncm_ops_t* p_ops)
{
    p_ops->cb_create = create_klb_ncm_ops_rpc_lua_mnp;
    p_ops->cb_destroy = destroy_klb_ncm_ops_rpc_lua_mnp;

    p_ops->cb_init = init_klb_ncm_ops_rpc_lua_mnp;
    p_ops->cb_ctrl = NULL;

    p_ops->cb_send_media = NULL;
    p_ops->cb_send_normal = send_rpc_klb_ncm_ops_rpc_lua_mnp;

    p_ops->on_send = on_send_klb_ncm_ops_rpc_lua_mnp;
    p_ops->on_recv = on_recv_klb_ncm_ops_rpc_lua_mnp;
    p_ops->on_proc = on_proc_klb_ncm_ops_rpc_lua_mnp;

    return 0;
}

int klb_ncm_register_ops_rpc_lua_mnp(klb_ncm_t* p_ncm, int protocol)
{
    klb_ncm_ops_t ops = { 0 };
    klb_ncm_init_ops_rpc_lua_mnp(&ops);

    int ret = klb_ncm_register(p_ncm, protocol, &ops);
    assert(0 == ret);

    return ret;
}
