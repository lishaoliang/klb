#include "klbnet/klb_nsp.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_ncm.h"
#include "klbbase/klb_mnp.h"
#include "libavutil/avstring.h"
#include <assert.h>


typedef struct klb_nsp_item_t_
{
    klb_socket_t*   p_socket;               ///< socket 对象
    int             id;                     ///< socket 编号

    klb_buf_t*      p_buf;                  ///< 读取缓存

    int             protocol;               ///< 协议类型: klb_protocol_e/KLB_PROTOCOL_UNKOWN
}klb_nsp_item_t;


typedef struct klb_nsp_t_
{
    klb_multiplex_t*        p_multi;            ///< 复用模块
    klb_hlist_t*            p_socket_hlist;     ///< socket列表: klb_nsp_socket_t*

    struct
    {
        klb_nsp_accept_cb   cb_accept;
        void*               p_udata;
    };
}klb_nsp_t;


/// @brief 创建nsp(net server preprocess); 网络连接预处理模块
klb_nsp_t* klb_nsp_create(klb_multiplex_t* p_multi)
{
    klb_nsp_t* p_nsp = KLB_MALLOCZ(klb_nsp_t, 1, 0);

    p_nsp->p_multi = p_multi;

    p_nsp->p_socket_hlist = klb_hlist_create(0);

    return p_nsp;
}


/// @brief 销毁nsp
void klb_nsp_destroy(klb_nsp_t* p_nsp)
{
    assert(NULL != p_nsp);

    // 清空


    KLB_FREE_BY(p_nsp->p_socket_hlist, klb_hlist_destroy);
    KLB_FREE(p_nsp);
}

int klb_nsp_set_accept(klb_nsp_t* p_nsp, klb_nsp_accept_cb cb_accept, void* ptr)
{
    p_nsp->cb_accept = cb_accept;
    p_nsp->p_udata = ptr;

    return 0;
}

static int klb_nsp_check_protocol(klb_buf_t* p_buf, bool tls)
{
    char* ptr = p_buf->p_buf;
    int data_len = p_buf->end - p_buf->start;

    if ((sizeof(uint32_t) <= data_len) && 
        (KLB_MNP_MAGIC == *((uint32_t*)(ptr))))
    {
        return tls ? KLB_PROTOCOL_MNPS : KLB_PROTOCOL_MNP;
    }

    char* p_rn = av_strnstr(ptr, "\r\n", data_len);
    if (NULL != p_rn)
    {
        int line_len = p_rn - ptr;
        char* p_http = av_strnstr(ptr, "HTTP", line_len);
        if (NULL != p_http)
        {
            return tls ? KLB_PROTOCOL_HTTPS : KLB_PROTOCOL_HTTP;
        }

        char* p_rtsp = av_strnstr(ptr, "RTSP", line_len);
        if (NULL != p_rtsp)
        {
            return KLB_PROTOCOL_RTSP;
        }
    }

    return KLB_PROTOCOL_UNKOWN;
}

/// @brief 当移除之后
static int cb_remove_klb_nsp(void* p_lparam, void* p_wparam, int id)
{
    klb_nsp_t* p_nsp = (klb_nsp_t*)p_lparam;
    klb_nsp_item_t* p_item = (klb_nsp_item_t*)p_wparam;

    // 从 klb_multiplex_remove 移除之后, 可交给对应处理模块

    // 移除
    void* ptr = klb_hlist_remove_bykey(p_nsp->p_socket_hlist, &id, sizeof(id));
    assert(ptr == p_item);

    if (NULL != p_nsp->cb_accept)
    {
        p_nsp->cb_accept(p_nsp->p_udata, p_item->protocol, p_item->p_socket, p_item->p_buf);
    }
    else
    {
        KLB_FREE_BY(p_item->p_socket, klb_socket_destroy);
    }

    KLB_FREE(p_item->p_buf);
    KLB_FREE(p_item);

    return 0;
}

/// @brief 当可以接收数据时
static int cb_recv_klb_nsp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klb_nsp_t* p_nsp = (klb_nsp_t*)p_lparam;
    klb_nsp_item_t* p_item = (klb_nsp_item_t*)p_wparam;

    // 从连接中读取一定量的数据, 并识别协议类型

    klb_buf_t* p_buf = p_item->p_buf;

    int len = p_buf->buf_len - p_buf->end;
    if (0 < len)
    {
        int r = klb_socket_recv(p_item->p_socket, p_buf->p_buf + p_buf->end, len);
        if (0 < r)
        {
            p_buf->end += r;

            p_item->protocol = klb_nsp_check_protocol(p_buf, false);

            if (KLB_PROTOCOL_UNKOWN != p_item->protocol)
            {
                klb_multiplex_remove(p_nsp->p_multi, id);
            }
        }
    }
    else
    {
        // 已经满了
        klb_multiplex_remove(p_nsp->p_multi, id);
    }

    return 0;
}

/// @brief 当可以发送数据时
static int cb_send_klb_nsp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    assert(false);
    return 0;
}

void klb_nsp_push(klb_nsp_t* p_nsp, klb_socket_t* p_socket)
{
    klb_nsp_item_t* p_item = KLB_MALLOCZ(klb_nsp_item_t, 1, 0);
    p_item->p_socket = p_socket;

    p_item->p_buf = klb_buf_malloc(1024 * 4, false);

    klb_multiplex_ops_t ops = { 0 };
    ops.cb_remove = cb_remove_klb_nsp;
    ops.cb_recv = cb_recv_klb_nsp;
    ops.cb_send = cb_send_klb_nsp;

    ops.p_lparam = p_nsp;
    ops.p_wparam = p_item;

    int id = klb_multiplex_push_socket(p_nsp->p_multi, p_socket, &ops);
    p_item->id = id;

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_nsp->p_socket_hlist, &id, sizeof(id), p_item);
    assert(NULL != p_iter);

    klb_socket_set_reading(p_socket, true);
}
