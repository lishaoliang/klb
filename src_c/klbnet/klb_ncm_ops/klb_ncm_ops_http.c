// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_list.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_rbuf.h"
#include "klbnet/klb_http.h"
#include <assert.h>


typedef struct klb_ncm_ops_http_t_
{
    klb_ncm_t*          p_ncm;          ///< ncm模块
    int                 protocol;       ///< 协议编号
    int                 id;             ///< id编号

    klb_ncm_ops_recv_cb cb_recv_to_ncm; ///< 收到数据后, 交给ncm

    klb_http_t*         p_http;         ///< http
}klb_ncm_ops_http_t;

//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////

/// @brief 创建连接
/// @param [in] *p_ncm          ncm模块
/// @return void* 连接的指针
static void* cb_create_klb_ncm_ops_http(klb_ncm_ops_lparam_t* p_lparam, klb_ncm_ops_wparam_t* p_wparam)
{
    klb_ncm_ops_http_t* p_ops = KLB_MALLOCZ(klb_ncm_ops_http_t, 1, 0);

    p_ops->p_ncm = p_lparam->p_ncm;
    p_ops->protocol = p_lparam->protocol;
    p_ops->id = p_lparam->id;
    p_ops->cb_recv_to_ncm = p_lparam->cb_recv;

    p_ops->p_http = klb_http_create(HTTP_REQUEST);

    return p_ops;
}

/// @brief 销毁连接
/// @param [in] *ptr            连接的指针
/// @return 无
static void cb_destroy_klb_ncm_ops_http(void* ptr)
{
    klb_ncm_ops_http_t* p_opt = (klb_ncm_ops_http_t*)ptr;


    // 销毁
    KLB_FREE_BY(p_opt->p_http, klb_http_destroy);
    KLB_FREE(p_opt);
}


/// @brief 初始数据等
/// @param [in] *p_ncm          ncm模块
/// @param [in] *p_data         初始已经读取的数据
/// @param [in] data_len        数据长度
/// @return int 0.成功; 非0.失败
static int cb_init_klb_ncm_ops_http(void* ptr, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_http_t* p_ops = (klb_ncm_ops_http_t*)ptr;

    klb_http_init(p_ops->p_http, p_data, data_len);

    while (true)
    {
        klb_buf_t* p_data = NULL;
        if (0 == klb_http_get(p_ops->p_http, &p_data))
        {
            p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_TEXT, p_data);
        }
        else
        {
            break;
        }
    }

    return 0;
}

/// @brief 主动发送数据(非媒体数据)
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_text_klb_ncm_ops_http(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_ncm_ops_http_t* p_ops = (klb_ncm_ops_http_t*)ptr;

    klb_http_push(p_ops->p_http, p_socket, p_head, head_len, p_body, body_len);

    return 0;
}

/// @brief 主动发送媒体数据
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_media_klb_ncm_ops_http(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data)
{
    klb_ncm_ops_http_t* p_ops = (klb_ncm_ops_http_t*)ptr;

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_send_klb_ncm_ops_http(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_http_t* p_ops = (klb_ncm_ops_http_t*)ptr;


    int s = klb_http_send(p_ops->p_http, p_socket);

    return s;
}

/// @brief 当网络上可以接收数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_recv_klb_ncm_ops_http(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_http_t* p_ops = (klb_ncm_ops_http_t*)ptr;

    int r = klb_http_recv(p_ops->p_http, p_socket);

    while (true)
    {
        klb_buf_t* p_data = NULL;
        if (0 == klb_http_get(p_ops->p_http, &p_data))
        {
            p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_TEXT, p_data);
        }
        else
        {
            break;
        }
    }

    return r;
}

//////////////////////////////////////////////////////////////////////////

int klb_ncm_register_ops_http(klb_ncm_t* p_ncm, int protocol)
{
    klb_ncm_ops_t ops = { 0 };

    // 创建/销毁
    ops.cb_create = cb_create_klb_ncm_ops_http;
    ops.cb_destroy = cb_destroy_klb_ncm_ops_http;

    ops.cb_init = cb_init_klb_ncm_ops_http;

    ops.cb_send_text = cb_send_text_klb_ncm_ops_http;
    ops.cb_send_media = cb_send_media_klb_ncm_ops_http;

    ops.on_send = on_send_klb_ncm_ops_http;
    ops.on_recv = on_recv_klb_ncm_ops_http;

    int ret = klb_ncm_register(p_ncm, protocol, &ops);
    assert(0 == ret);

    return ret;
}
