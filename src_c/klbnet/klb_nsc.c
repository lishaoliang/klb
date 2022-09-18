// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_nsc.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf_atom.h"
#include "klbnet/klb_multiplex.h"
#include "klbutil/klb_hlist.h"
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbutil/klb_log.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>


/// @struct klb_nsc_item_t
/// @brief  每一个连接
typedef struct klb_nsc_item_t_
{
    klb_socket_t*               p_socket;           ///< socket
    int                         id;                 ///< ID号

    void*                       ptr;                ///< 扩展的指针
    klb_ncm_ops_t               ops;                ///< 解析接口函数
}klb_nsc_item_t;


/// @struct klb_nsc_t
/// @brief  ncm模块
typedef struct klb_nsc_t_
{
    klb_multiplex_t*            p_multi;            ///< 多路复用

    klb_nsc_item_t*             p_item;             ///< 连接信息

    struct
    {
        klb_ncm_ops_recv_cb     cb_receiver;        ///< 来数据后的接收函数
        void*                   p_obj_receiver;     ///< 用户数据
    };
}klb_nsc_t;

/// @brief 创建nsc(net single connect); 单个网络连接
klb_nsc_t* klb_nsc_create(klb_multiplex_t* p_multi)
{
    assert(NULL != p_multi);

    klb_nsc_t* p_nsc = KLB_MALLOCZ(klb_nsc_t, 1, 0);
    p_nsc->p_multi = p_multi;

    return p_nsc;
}


/// @brief 销毁nsc
void klb_nsc_destroy(klb_nsc_t* p_nsc)
{
    assert(NULL != p_nsc);

    // close
    klb_nsc_close(p_nsc);

    // free
    KLB_FREE(p_nsc);
}


/// @brief 添加数据接收函数
/// @return int 0.成功; 非0.失败
int klb_nsc_set_receiver(klb_nsc_t* p_nsc, klb_ncm_ops_recv_cb cb_recv, void* p_obj)
{
    p_nsc->cb_receiver = cb_recv;
    p_nsc->p_obj_receiver = p_obj;

    return 0;
}

static void free_klb_nsc_item(klb_nsc_item_t* p_item)
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
static int cb_remove_klb_nsc_item(void* p_lparam, void* p_wparam, int id)
{
    //klb_nsc_t* p_nsc = (klb_nsc_t*)p_lparam;
    klb_nsc_item_t* p_item = (klb_nsc_item_t*)p_wparam;

    KLB_FREE_BY(p_item, free_klb_nsc_item);

    return 0;
}

/// @brief 当可以接收数据时
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @param [in] now                 当前系统滴答(毫秒)
/// @return int 接收的数据量
static int on_recv_klb_nsc_item(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_nsc_t* p_nsc = (klb_nsc_t*)p_lparam;
    klb_nsc_item_t* p_item = (klb_nsc_item_t*)p_wparam;

    int recv = p_item->ops.on_recv(p_item->ptr, p_item->p_socket, now);

    return recv;
}

/// @brief 当可以发送数据时
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @param [in] now                 当前系统滴答(毫秒)
/// @return int 发送的数据量
static int on_send_klb_nsc_item(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_nsc_t* p_nsc = (klb_nsc_t*)p_lparam;
    klb_nsc_item_t* p_item = (klb_nsc_item_t*)p_wparam;

    int send = p_item->ops.on_send(p_item->ptr, p_item->p_socket, now);

    return send;
}

static int on_proc_klb_nsc_item(void* p_lparam, void* p_wparam, int msg, int id, klb_socket_t* p_socket, int64_t now)
{
    klb_nsc_t* p_nsc = (klb_nsc_t*)p_lparam;
    klb_nsc_item_t* p_item = (klb_nsc_item_t*)p_wparam;

    int ret = p_item->ops.on_proc(p_item->ptr, p_item->p_socket, msg, now);

    return ret;
}

static int cb_recv_klb_nsc(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data)
{
    klb_nsc_t* p_nsc = (klb_nsc_t*)ptr;

    if (NULL != p_nsc->cb_receiver)
    {
        p_nsc->cb_receiver(p_nsc->p_obj_receiver, protocol, id, code, packtype, p_data);
    }

    return 0;
}

static bool get_ops_klb_nsc(int protocol, klb_ncm_ops_t* p_ops)
{
    switch (protocol)
    {
    case KLB_PROTOCOL_RPC_MNP_LUA:
        klb_ncm_init_ops_rpc_lua_mnp(p_ops);
        return true;
    case KLB_PROTOCOL_RPC_MNP_JSON:
        klb_ncm_init_ops_rpc_json_mnp(p_ops);
        return true;
    default:
        break;
    }

    return false;
}

int klb_nsc_push_by_ops(klb_nsc_t* p_nsc, const klb_ncm_ops_t* p_ops, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    assert(NULL == p_nsc->p_item);

    klb_nsc_item_t* p_item = KLB_MALLOCZ(klb_nsc_item_t, 1, 0);
    memcpy(&p_item->ops, p_ops, sizeof(klb_ncm_ops_t));
    p_item->p_socket = p_socket;

    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klb_nsc_item;
    o.cb_recv = on_recv_klb_nsc_item;
    o.cb_send = on_send_klb_nsc_item;
    o.cb_proc = on_proc_klb_nsc_item;

    o.p_lparam = p_nsc;
    o.p_wparam = p_item;

    int id = klb_multiplex_push_socket(p_nsc->p_multi, p_socket, &o);

    klb_ncm_ops_lparam_t lparam = { 0 };
    lparam.p_ncm = p_nsc;
    lparam.p_socket = p_socket;
    lparam.protocol = protocol;
    lparam.id = id;
    lparam.cb_malloc = klb_buf_atom_malloc;
    lparam.p_pool = NULL;
    lparam.cb_malloc_media = klb_buf_atom_malloc;
    lparam.p_pool_media = NULL;
    lparam.cb_recv = cb_recv_klb_nsc;

    klb_ncm_ops_wparam_t wparam = { 0 };
    wparam.read_buffer_size = 32 * 1024;

    p_item->ptr = p_item->ops.cb_create(&lparam, &wparam);
    p_item->id = id;

    assert(NULL != p_item->ptr);
    assert(0 < p_item->id);

    // 
    p_nsc->p_item = p_item;
    klb_socket_set_reading(p_socket, true);

    assert(NULL != p_item->ptr);
    assert(0 < p_item->id);

    // 新连接进来
    if (klb_socket_is_connected(p_socket))
    {
        cb_recv_klb_nsc(p_nsc, protocol, id, KLB_SOCKET_CONNECT, 0, NULL);
    }

    // 初始 init
    assert(NULL != p_item->ops.cb_init);
    p_item->ops.cb_init(p_item->ptr, p_socket, p_data, data_len);

    return id;
}

/// @brief 向nsc放入一个socket
/// @param [in]  *p_nsc                 nsc模块
/// @return int -1.放入失败; 大于0.为id号
int klb_nsc_push(klb_nsc_t* p_nsc, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_t ops = { 0 };
    if (!get_ops_klb_nsc(protocol, &ops))
    {
        return -1;
    }

    return klb_nsc_push_by_ops(p_nsc, &ops, protocol, p_socket, p_data, data_len);
}


/// @brief 关闭socket
/// @return int 0.成功; 非0.失败
int klb_nsc_close(klb_nsc_t* p_nsc)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    if (NULL != p_item)
    {
        klb_socket_closing(p_item->p_socket);
        klb_multiplex_remove(p_nsc->p_multi, p_item->id);
    }

    p_nsc->p_item = NULL;

    return 0;
}

int klb_nsc_get_status(klb_nsc_t* p_nsc)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;
    if (NULL != p_item)
    {
        return (int)klb_socket_get_status(p_item->p_socket);
    }

    return -1;
}

/// @brief 发送文本数据
/// @return int 0.成功; 非0.失败
int klb_nsc_send_text(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_TEXT, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return ret;
}

/// @brief 发送二进制数据
/// @return int 0.成功; 非0.失败
int klb_nsc_send_binary(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_BINARY, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return ret;
}

/// @brief 发送媒体数据
/// @return int 0.成功; 非0.失败
int klb_nsc_send_media(klb_nsc_t* p_nsc, klb_buf_t* p_data)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_send_media)
    {
        ret = p_item->ops.cb_send_media(p_item->ptr, p_item->p_socket, p_data);
    }

    return ret;
}

/// @brief 发送RPC数据
/// @return int 0.成功; 非0.失败
int klb_nsc_send_rpc(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_LUA, sequence, uid, p_head, head_len, p_body, body_len);
    }
   
    return ret;
}

int klb_nsc_send_rpc_json(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_send_normal)
    {
        ret = p_item->ops.cb_send_normal(p_item->ptr, p_item->p_socket, KLB_MNP_RPC_JSON, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return ret;
}

int klb_nsc_ctrl(klb_nsc_t* p_nsc, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_nsc_item_t* p_item = p_nsc->p_item;

    int ret = -1;
    if (NULL != p_item && NULL != p_item->ops.cb_ctrl)
    {
        ret = p_item->ops.cb_ctrl(p_item->ptr, p_in, p_out);
    }

    return ret;
}
