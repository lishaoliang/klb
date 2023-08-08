// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_nsp.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_ncm.h"
#include "klbbase/klb_mnp.h"
#include "libavutil/avstring.h"
#include "klbthird/sds.h"
#include <assert.h>


/// @brief URL路径 对应的协议
typedef struct klb_nsp_route_t_
{
    int             protocol;
}klb_nsp_route_t;


typedef struct klb_nsp_item_t_
{
    klb_socket_t*   p_socket;               ///< socket 对象
    int             id;                     ///< socket 编号

    klb_buf_t*      p_buf;                  ///< 读取缓存

    int             protocol;               ///< 协议类型: klb_protocol_e/KLB_PROTOCOL_UNKOWN

    struct
    {
        klb_nsp_accept_cb   cb_accept;
        void*               p_udata;
    };
}klb_nsp_item_t;


typedef struct klb_nsp_t_
{
    klb_multiplex_t*        p_multi;                ///< 复用模块
    klb_hlist_t*            p_socket_hlist;         ///< socket列表: klb_nsp_item_t*

    klb_hlist_t*            p_route_hlist;          ///< URL路径对应的协议

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
    p_nsp->p_route_hlist = klb_hlist_create(0);

    return p_nsp;
}


/// @brief 销毁nsp
void klb_nsp_destroy(klb_nsp_t* p_nsp)
{
    assert(NULL != p_nsp);

    // 清空
    while (0 < klb_hlist_size(p_nsp->p_socket_hlist))
    {
        klb_nsp_item_t* p_item = (klb_nsp_item_t*)klb_hlist_pop_head(p_nsp->p_socket_hlist);

        klb_socket_closing(p_item->p_socket);
        klb_multiplex_remove(p_nsp->p_multi, p_item->id);
    }

    // 清空
    while (0 < klb_hlist_size(p_nsp->p_route_hlist))
    {
        klb_nsp_route_t* p_route = (klb_nsp_route_t*)klb_hlist_pop_head(p_nsp->p_route_hlist);
        KLB_FREE(p_route);
    }

    KLB_FREE_BY(p_nsp->p_socket_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_nsp->p_route_hlist, klb_hlist_destroy);
    KLB_FREE(p_nsp);
}

void klb_nsp_route(klb_nsp_t* p_nsp, const char* p_path, int protocol)
{
    klb_nsp_route_t* p_route = (klb_nsp_route_t*)klb_hlist_find(p_nsp->p_route_hlist, p_path, strlen(p_path));
    if (NULL != p_route)
    {
        p_route->protocol = protocol;
    }
    else
    {
        klb_nsp_route_t* p_tmp = KLB_MALLOCZ(klb_nsp_route_t, 1, 0);
        p_tmp->protocol = protocol;

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_nsp->p_route_hlist, p_path, strlen(p_path), p_tmp);
        assert(NULL != p_iter);
    }
}

int klb_nsp_set_accept(klb_nsp_t* p_nsp, klb_nsp_accept_cb cb_accept, void* ptr)
{
    p_nsp->cb_accept = cb_accept;
    p_nsp->p_udata = ptr;

    return 0;
}

static int check_protocol_route_klb_nsp(klb_nsp_t* p_nsp, const char* p_path)
{
    klb_nsp_route_t* p_route = (klb_nsp_route_t*)klb_hlist_find(p_nsp->p_route_hlist, p_path, strlen(p_path));
    if (NULL != p_route)
    {
        return p_route->protocol;
    }

    return KLB_PROTOCOL_UNKOWN;
}

static int klb_nsp_check_protocol(klb_nsp_t* p_nsp, klb_buf_t* p_buf)
{
    char* ptr = p_buf->p_buf;
    int data_len = p_buf->end - p_buf->start;

    if ((sizeof(klb_mnp_t) <= data_len) &&
        (KLB_MNP_MAGIC == *((uint32_t*)(ptr))))
    {
        klb_mnp_t* p_mnp = (klb_mnp_t*)ptr;

        if (KLB_MNP_RPC_LUA == p_mnp->packtype)
        {
            return KLB_PROTOCOL_RPC_MNP_LUA;
        }
        else if(KLB_MNP_RPC_JSON == p_mnp->packtype)
        {
            return KLB_PROTOCOL_RPC_MNP_JSON;
        }

        return KLB_PROTOCOL_MNP;
    }

    char* p_rn = av_strnstr(ptr, "\r\n", data_len);
    if (NULL != p_rn)
    {
        int line_len = p_rn - ptr;
        sds line = sdsnewlen(ptr, line_len); // eg. "GET / HTTP/1.1"

        char path[1024] = { 0 };
        sscanf(line, "%*[^ ] %1000s", path);

        char* p_http = av_strnstr(line, "HTTP", sdslen(line));
        if (NULL != p_http)
        {
            sdsfree(line);

            int protocol = check_protocol_route_klb_nsp(p_nsp, path);
            if (KLB_PROTOCOL_UNKOWN == protocol)
            {
                return KLB_PROTOCOL_HTTP;
            }

            return protocol;
        }

        char* p_rtsp = av_strnstr(line, "RTSP", sdslen(line));
        if (NULL != p_rtsp)
        {
            sdsfree(line);
            return KLB_PROTOCOL_RTSP;
        }

        sdsfree(line);
    }

    return KLB_PROTOCOL_UNKOWN;
}

/// @brief 当移除之后
static int cb_remove_klb_nsp(void* p_lparam, void* p_wparam, int id)
{
    //klb_nsp_t* p_nsp = (klb_nsp_t*)p_lparam;
    klb_nsp_item_t* p_item = (klb_nsp_item_t*)p_wparam;

    bool close = true;

    if (KLB_PROTOCOL_UNKOWN != p_item->protocol && NULL != p_item->cb_accept)
    {
        p_item->cb_accept(p_item->p_udata, p_item->protocol, p_item->p_socket, p_item->p_buf);
        close = false;
    }
    
    if(close)
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
        int r = klb_socket_recv(p_item->p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), len);
        if (0 < r)
        {
            p_buf->end += r;

            p_item->protocol = klb_nsp_check_protocol(p_nsp, p_buf);

            if (KLB_PROTOCOL_UNKOWN != p_item->protocol)
            {
                // 识别到了协议
                p_item->cb_accept = p_nsp->cb_accept;
                p_item->p_udata = p_nsp->p_udata;

                klb_hlist_remove_bykey(p_nsp->p_socket_hlist, &p_item->id, sizeof(p_item->id));
                klb_multiplex_remove(p_nsp->p_multi, id);
            }
        }
    }
    else
    {
        // 数据满了, 还未识别到协议
        klb_hlist_remove_bykey(p_nsp->p_socket_hlist, &p_item->id, sizeof(p_item->id));
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
