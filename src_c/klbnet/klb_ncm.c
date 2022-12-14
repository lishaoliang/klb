// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf_atom.h"
#include "klbnet/klb_multiplex.h"
#include "klbutil/klb_hlist.h"
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbutil/klb_log.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>


/// @struct klb_ncm_item_t
/// @brief  ncm中每一个连接项
typedef struct klb_ncm_item_t_
{
    klb_socket_t*       p_socket;           ///< socket
    int                 id;                 ///< ID号

    void*               ptr;                ///< 扩展的指针
    klb_ncm_ops_t       ops;                ///< 解析接口函数
}klb_ncm_item_t;


/// @struct klb_ncm_t
/// @brief  ncm模块
typedef struct klb_ncm_t_
{
    klb_multiplex_t*            p_multi;            ///< 多路复用
    
    klb_hlist_t*                p_ops_hlist;        ///< 注册的解析器列表: klb_ncm_ops_t*
    klb_hlist_t*                p_item_hlist;       ///< 连接项列表: klb_ncm_item_t*

    struct
    {
        klb_ncm_ops_recv_cb     cb_receiver;        ///< 来数据后的接收函数
        void*                   p_obj_receiver;     ///< 用户数据
    };
}klb_ncm_t;


//////////////////////////////////////////////////////////////////////////

klb_ncm_t* klb_ncm_create(klb_multiplex_t* p_multi, uint32_t preload)
{
    assert(NULL != p_multi);

    klb_ncm_t* p_ncm = KLB_MALLOCZ(klb_ncm_t, 1, 0);

    p_ncm->p_multi = p_multi;

    p_ncm->p_ops_hlist = klb_hlist_create(0);
    p_ncm->p_item_hlist = klb_hlist_create(0);

    // 注册标准解析器
    if (preload & KLB_PROTOCOL_LOAD_STD)
    {
        KLB_NCM_REGISTER_OPS_STD(p_ncm);
    }

    // 注册标准RPC解析器
    if (preload & KLB_PROTOCOL_LOAD_RPC)
    {
        KLB_NCM_REGISTER_OPS_RPC(p_ncm);
    }

    return p_ncm;
}

void klb_ncm_destroy(klb_ncm_t* p_ncm)
{
    assert(NULL != p_ncm);

    // 清空
    while (0 < klb_hlist_size(p_ncm->p_item_hlist))
    {
        klb_ncm_item_t* p_item = (klb_ncm_item_t*)klb_hlist_pop_head(p_ncm->p_item_hlist);

        klb_socket_closing(p_item->p_socket);
        klb_multiplex_remove(p_ncm->p_multi, p_item->id);
    }

    while (0 < klb_hlist_size(p_ncm->p_ops_hlist))
    {
        klb_ncm_ops_t* p_opt = (klb_ncm_ops_t*)klb_hlist_pop_head(p_ncm->p_ops_hlist);
        KLB_FREE(p_opt);
    }

    KLB_FREE_BY(p_ncm->p_item_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_ncm->p_ops_hlist, klb_hlist_destroy);

    KLB_FREE(p_ncm);
}

int klb_ncm_add_receiver(klb_ncm_t* p_ncm, klb_ncm_ops_recv_cb cb_recv, void* p_obj)
{
    p_ncm->cb_receiver = cb_recv;
    p_ncm->p_obj_receiver = p_obj;

    return 0;
}

int klb_ncm_register(klb_ncm_t* p_ncm, int protocol, const klb_ncm_ops_t* p_ops)
{
    assert(NULL != p_ncm);
    assert(NULL != p_ops);
    assert(NULL != p_ops->cb_create);
    assert(NULL != p_ops->cb_destroy);

    klb_ncm_ops_t* p_tmp = KLB_MALLOC(klb_ncm_ops_t, 1, 0);
    KLB_MEMSET(p_tmp, 0, sizeof(klb_ncm_ops_t));

    memcpy(p_tmp, p_ops, sizeof(klb_ncm_ops_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ncm->p_ops_hlist, &protocol, sizeof(protocol), p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG("register ncm parser error!protocol:[%d]\n", protocol);
        return 1; // 放入失败, 已存在
    }

    return 0;
}

static klb_ncm_item_t* get_item_klb_ncm(klb_ncm_t* p_ncm, int id)
{
    return (klb_ncm_item_t*)klb_hlist_find(p_ncm->p_item_hlist, &id, sizeof(id));
}

static void free_klb_ncm_item(klb_ncm_item_t* p_item)
{
    assert(NULL != p_item);

    // 销毁连接对象
    p_item->ops.cb_destroy(p_item->ptr);

    // 销毁 socket
    KLB_FREE_BY(p_item->p_socket, klb_socket_destroy);
    KLB_FREE(p_item);
}

/// @brief 当移除之后, 可以销毁时机
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @return int 0
/// @note cb_recv/cb_send回调函数中, 不可以销毁对象
static int cb_remove_klb_ncm_item(void* p_lparam, void* p_wparam, int id)
{
    //klb_ncm_t* p_ncm = (klb_ncm_t*)p_lparam;
    klb_ncm_item_t* p_item = (klb_ncm_item_t*)p_wparam;

    KLB_FREE_BY(p_item, free_klb_ncm_item);

    return 0;
}

/// @brief 当可以接收数据时
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @param [in] now                 当前系统滴答(毫秒)
/// @return int 接收的数据量
static int on_recv_klb_ncm_item(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_ncm_t* p_ncm = (klb_ncm_t*)p_lparam;
    klb_ncm_item_t* p_item = (klb_ncm_item_t*)p_wparam;

    int recv = p_item->ops.on_recv(p_item->ptr, p_item->p_socket, now);

    return recv;
}

/// @brief 当可以发送数据时
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @param [in] now                 当前系统滴答(毫秒)
/// @return int 发送的数据量
static int on_send_klb_ncm_item(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_ncm_t* p_ncm = (klb_ncm_t*)p_lparam;
    klb_ncm_item_t* p_item = (klb_ncm_item_t*)p_wparam;

    int send = p_item->ops.on_send(p_item->ptr, p_item->p_socket, now);

    return send;
}

static int on_proc_klb_ncm_item(void* p_lparam, void* p_wparam, int msg, int id, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_t* p_ncm = (klb_ncm_t*)p_lparam;
    klb_ncm_item_t* p_item = (klb_ncm_item_t*)p_wparam;

    int send = p_item->ops.on_proc(p_item->ptr, p_item->p_socket, msg, now);

    return send;
}

/// @brief 连接解析收到数据时,回调
/// @return int 0
static int cb_klb_ncm_opt_recv(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data)
{
    klb_ncm_t* p_ncm = (klb_ncm_t*)ptr;

    if (NULL != p_ncm->cb_receiver)
    {
        p_ncm->cb_receiver(p_ncm->p_obj_receiver, protocol, id, code, packtype, p_data);
    }

    return 0;
}

int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_t* p_ops = (klb_ncm_ops_t*)klb_hlist_find(p_ncm->p_ops_hlist, &protocol, sizeof(protocol));
    if (NULL == p_ops)
    {
        return -1;  // 不支持的协议
    }

    klb_ncm_item_t* p_item = KLB_MALLOCZ(klb_ncm_item_t, 1, 0);

    memcpy(&p_item->ops, p_ops, sizeof(klb_ncm_ops_t));
    p_item->p_socket = p_socket;

    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klb_ncm_item;
    o.cb_recv = on_recv_klb_ncm_item;
    o.cb_send = on_send_klb_ncm_item;

    o.p_lparam = p_ncm;
    o.p_wparam = p_item;

    int id = klb_multiplex_push_socket(p_ncm->p_multi, p_socket, &o);

    klb_ncm_ops_lparam_t lparam = { 0 };
    lparam.p_ncm = p_ncm;
    lparam.p_socket = p_socket;
    lparam.protocol = protocol;
    lparam.id = id;
    lparam.cb_malloc = klb_buf_atom_malloc;
    lparam.p_pool = NULL;
    lparam.cb_malloc_media = klb_buf_atom_malloc;
    lparam.p_pool_media = NULL;
    lparam.cb_recv = cb_klb_ncm_opt_recv;

    klb_ncm_ops_wparam_t wparam = { 0 };
    wparam.read_buffer_size = 32 * 1024;

    p_item->ptr = p_ops->cb_create(&lparam, &wparam);
    p_item->id = id;

    assert(NULL != p_item->ptr);
    assert(0 < p_item->id);

    // 所有的id从 klb_multiplex_t 中来, 不会冲突 
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ncm->p_item_hlist, &id, sizeof(id), p_item);
    assert(NULL != p_iter);

    klb_socket_set_reading(p_socket, true);

    assert(NULL != p_item->ptr);
    assert(0 < p_item->id);

    // 新连接进来
    if (klb_socket_is_connected(p_socket))
    {
        cb_klb_ncm_opt_recv(p_ncm, protocol, id, KLB_SOCKET_CONNECT, 0, NULL);
    }

    // 初始
    assert(NULL != p_ops->cb_init);
    p_ops->cb_init(p_item->ptr, p_socket, p_data, data_len);

    return id;
}

int klb_ncm_close(klb_ncm_t* p_ncm, int id)
{
    assert(NULL != p_ncm);

    klb_ncm_item_t* p_item = (klb_ncm_item_t*)klb_hlist_remove_bykey(p_ncm->p_item_hlist, &id, sizeof(id));

    if (NULL != p_item)
    {
        klb_socket_closing(p_item->p_socket);
        klb_multiplex_remove(p_ncm->p_multi, p_item->id);
    }

    return 0;
}

int klb_ncm_send_text(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
    if (NULL == p_item)
    {
        return -1;
    }

    int ret = -1;
    if (NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_TEXT, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return ret;
}

int klb_ncm_send_binary(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
    if (NULL == p_item)
    {
        return -1;
    }

    int ret = -1;
    if (NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_BINARY, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return ret;
}

int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data)
{
    klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
    if (NULL == p_item)
    {
        return -1;
    }

    int ret = -1;
    if (NULL != p_item->ops.cb_send_media)
    {
        ret = p_item->ops.cb_send_media(p_item->ptr, p_item->p_socket, p_data);
    }

    return ret;
}

int klb_ncm_send_rpc(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    int ret = -1;

    if (id <= 0)
    {
        // 发送给所有连接
        klb_hlist_iter_t* p_iter = klb_hlist_begin(p_ncm->p_item_hlist);
        while (NULL != p_iter)
        {
            klb_ncm_item_t* p_item = (klb_ncm_item_t*)klb_hlist_data(p_iter);
            if (NULL != p_item->ops.cb_send_normal)
            {
                ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_LUA, sequence, uid, p_head, head_len, p_body, body_len);
            }

            p_iter = klb_hlist_next(p_iter);
        }
    }
    else
    {
        // 单个发送
        klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
        if (NULL == p_item)
        {
            return -1;
        }

        if (NULL != p_item->ops.cb_send_normal)
        {
            ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_LUA, sequence, uid, p_head, head_len, p_body, body_len);
        }
    }

    return ret;
}

int klb_ncm_send_rpc_json(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    int ret = -1;

    if (id <= 0)
    {
        // 发送给所有连接
        klb_hlist_iter_t* p_iter = klb_hlist_begin(p_ncm->p_item_hlist);
        while (NULL != p_iter)
        {
            klb_ncm_item_t* p_item = (klb_ncm_item_t*)klb_hlist_data(p_iter);
            if (NULL != p_item->ops.cb_send_normal)
            {
                ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_JSON, sequence, uid, p_head, head_len, p_body, body_len);
            }

            p_iter = klb_hlist_next(p_iter);
        }
    }
    else
    {
        // 单个发送
        klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
        if (NULL == p_item)
        {
            return -1;
        }

        if (NULL != p_item->ops.cb_send_normal)
        {
            ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_JSON, sequence, uid, p_head, head_len, p_body, body_len);
        }
    }

    return ret;
}

int klb_ncm_ctrl(klb_ncm_t* p_ncm, int id, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_ncm_item_t* p_item = get_item_klb_ncm(p_ncm, id);
    if (NULL == p_item)
    {
        return -1;
    }

    int ret = -1;
    if (NULL != p_item->ops.cb_ctrl)
    {
        ret = p_item->ops.cb_ctrl(p_item->ptr, p_in, p_out);
    }

    return ret;
}
