﻿#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_multiplex.h"
#include "klbutil/klb_hlist.h"
#include "klbnet/klbncm_parser/klb_ncm_parser.h"
#include "klbutil/klb_log.h"
#include <assert.h>


/// @struct klb_ncm_item_t
/// @brief  ncm中每一个连接项
typedef struct klb_ncm_item_t_
{
    klb_socket_t*       p_socket;           ///< socket
    int                 id;                 ///< ID号

    void*               ptr;                ///< 扩展的指针
    klb_ncm_parser_t    parser;             ///< 解析接口函数
}klb_ncm_item_t;


/// @struct klb_ncm_t
/// @brief  ncm模块
typedef struct klb_ncm_t_
{
    klb_multiplex_t*    p_multi;            ///< 多路复用
    
    klb_hlist_t*        p_parser_hlist;     ///< 注册的解析器列表: klb_ncm_parser_t*
    klb_hlist_t*        p_item_hlist;       ///< 连接项列表: klb_ncm_item_t*
}klb_ncm_t;


klb_ncm_t* klb_ncm_create(klb_multiplex_t* p_multi)
{
    klb_ncm_t* p_ncm = KLB_MALLOC(klb_ncm_t, 1, 0);
    KLB_MEMSET(p_ncm, 0, sizeof(klb_ncm_t));

    p_ncm->p_multi = p_multi;

    p_ncm->p_parser_hlist = klb_hlist_create(0);
    p_ncm->p_item_hlist = klb_hlist_create(0);

    // 注册标准解析器
    KLB_NCM_REGISTER_PARSER_STD(p_ncm);

    return p_ncm;
}

void klb_ncm_destroy(klb_ncm_t* p_ncm)
{
    assert(NULL != p_ncm);

    // 清空
    while (0 < klb_hlist_size(p_ncm->p_parser_hlist))
    {
        klb_ncm_parser_t* p_parser = (klb_ncm_parser_t*)klb_hlist_pop_head(p_ncm->p_parser_hlist);
        KLB_FREE(p_parser);
    }
    
    //KLB_FREE_BY(p_ncm->p_multi, klb_multiplex_destroy);

    KLB_FREE_BY(p_ncm->p_item_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_ncm->p_parser_hlist, klb_hlist_destroy);

    KLB_FREE(p_ncm);
}

int klb_ncm_register(klb_ncm_t* p_ncm, int protocol, const klb_ncm_parser_t* p_parser)
{
    assert(NULL != p_ncm);
    assert(NULL != p_parser);
    assert(NULL != p_parser->cb_create);
    assert(NULL != p_parser->cb_destroy);

    klb_ncm_parser_t* p_tmp = KLB_MALLOC(klb_ncm_parser_t, 1, 0);
    KLB_MEMSET(p_tmp, 0, sizeof(klb_ncm_parser_t));

    memcpy(p_tmp, p_parser, sizeof(klb_ncm_parser_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ncm->p_parser_hlist, &protocol, sizeof(protocol), p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG("register ncm parser error!protocol:[%d]\n", protocol);
        return 1; // 放入失败, 已存在
    }

    return 0;
}

/// @brief 当移除之后, 可以销毁时机
/// @param [in] *p_lparam           调用者指定的参数1
/// @param [in] *p_wparam           调用者指定的参数2
/// @param [in] id                  ID编号
/// @return int 0
/// @note cb_recv/cb_send回调函数中, 不可以销毁对象
static int cb_remove_klb_ncm_item(void* p_lparam, void* p_wparam, int id)
{
    klb_ncm_t* p_ncm = (klb_ncm_t*)p_lparam;
    klb_ncm_item_t* p_item = (klb_ncm_item_t*)p_wparam;


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

    int recv = p_item->parser.on_recv(p_item->ptr, p_item->p_socket, now);

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

    int send = p_item->parser.on_send(p_item->ptr, p_item->p_socket, now);

    return send;
}

int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_parser_t* p_parser = (klb_ncm_parser_t*)klb_hlist_find(p_ncm->p_parser_hlist, &protocol, sizeof(protocol));
    if (NULL == p_parser)
    {
        return -1;  // 不支持的协议
    }

    klb_ncm_item_t* p_item = KLB_MALLOC(klb_ncm_item_t, 1, 0);
    KLB_MEMSET(p_item, 0, sizeof(klb_ncm_item_t));

    memcpy(&p_item->parser, p_parser, sizeof(klb_ncm_parser_t));
    p_item->p_socket = p_socket;

    klb_multiplex_obj_t o = { 0 };
    o.cb_remove = cb_remove_klb_ncm_item;
    o.cb_recv = on_recv_klb_ncm_item;
    o.cb_send = on_send_klb_ncm_item;

    o.p_lparam = p_ncm;
    o.p_wparam = p_item;

    int id = klb_multiplex_push_socket(p_ncm->p_multi, p_socket, &o);

    p_item->ptr = p_parser->cb_create(p_ncm, protocol, id);
    p_item->id = id;

    // 所有的id从 klb_multiplex_t 中来, 不会冲突 
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ncm->p_item_hlist, &id, sizeof(id), p_item);
    assert(NULL != p_iter);

    klb_socket_set_reading(p_socket, true);

    assert(NULL != p_item->ptr);
    assert(0 < p_item->id);
    return id;
}

int klb_ncm_close(klb_ncm_t* p_ncm, int id)
{
    return 0;
}

int klb_ncm_send(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_item_t* p_item = klb_hlist_find(p_ncm->p_item_hlist, &id, sizeof(id));
    if (NULL == p_item)
    {
        return -1;
    }

    int ret = -1;
    if (NULL != p_item->parser.cb_send)
    {
        ret = p_item->parser.cb_send(p_item->ptr, p_item->p_socket, sequence, uid, p_extra, extra_len, p_data, data_len);
    }

    return ret;
}

int klb_ncm_recv(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, int* p_code, uint32_t* p_sequence, uint32_t* p_uid, klb_buf_t** p_extra, klb_buf_t** p_data)
{
    return 0;
}

int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data)
{
    return 0;
}

int klb_ncm_recv_media(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, klb_buf_t** p_data)
{
    return 0;
}

int klb_ncm_loop_once(klb_ncm_t* p_ncm, int64_t now)
{
    return 0;
}
