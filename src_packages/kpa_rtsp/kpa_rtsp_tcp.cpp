#include "kpa_rtsp/kpa_rtsp_tcp.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

extern "C" {
#include "libavutil/avstring.h"
}

static void kpa_rtsp_tcp_destroy(kpa_rtsp_tcp_t* p_tcp)
{
    KLB_FREE_BY(p_tcp->p_socket, klb_socket_destroy);
    KLB_FREE(p_tcp);
}

//////////////////////////////////////////////////////////////////////////


static int parse_rnrn_size_kpa_rtsp_tcp(char* p_data, int data_len, int* p_out_head_len, int* p_out_body_len)
{
    static char s_rnrn[] = "\r\n\r\n";

    if (data_len < sizeof(s_rnrn))
    {
        return 1; // 数据不足
    }

    char* p_rnrn = av_strnstr(p_data, s_rnrn, data_len);
    if (NULL == p_rnrn)
    {
        return 1; // 未找到结束
    }

    int head_len = p_rnrn - p_data + sizeof(s_rnrn) - 1;

    static char s_Content_Length[] = "Content-Length";
    static char s_Content_length[] = "Content-length";
    static char s_content_length[] = "content-length";

    char* p_body_len = av_strnstr(p_data, s_Content_Length, head_len);
    if (NULL == p_body_len) p_body_len = av_strnstr(p_data, s_Content_length, head_len);
    if (NULL == p_body_len) p_body_len = av_strnstr(p_data, s_content_length, head_len);

    int body_len = 0;
    if (NULL != p_body_len)
    {
        av_sscanf(p_body_len, "%*[^:]:%d", &body_len);
    }

    if (data_len < head_len + body_len)
    {
        return 1; // 数据不足
    }

    *p_out_head_len = head_len;
    *p_out_body_len = body_len;

    return 0;
}

static int kpa_rtsp_tcp_parse(kpa_rtsp_tcp_t* p_tcp, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_tcp->p_r_buf;

    char* p_data = p_buf->p_buf + p_buf->start;
    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    int head_len = 0, body_len = 0;
    if (parse_rnrn_size_kpa_rtsp_tcp(p_data, data_len, &head_len, &body_len) != 0)
    {
        return 1; // 数据不足
    }

    p_tcp->cb_recv(p_tcp->p_obj, 0, p_data, head_len, p_data + head_len, body_len);

    p_buf->start += head_len + body_len;

    return 0;
}

static int on_remove_kpa_rtsp_tcp(void* p_lparam, void* p_wparam, int id)
{
    kpa_rtsp_tcp_t* p_tcp = (kpa_rtsp_tcp_t*)p_lparam;

    kpa_rtsp_tcp_destroy(p_tcp);

    return 0;
}

static int on_recv_kpa_rtsp_tcp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    kpa_rtsp_tcp_t* p_tcp = (kpa_rtsp_tcp_t*)p_lparam;
    klb_socket_t* p_socket = p_tcp->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int recv = 0;
    bool first = true;

    while (true)
    {
        klb_buf_t* p_buf = p_tcp->p_r_buf;

        int r = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            //
            while (true)
            {
                if (0 != kpa_rtsp_tcp_parse(p_tcp, p_socket))
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
            if (first)
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
        //p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return recv;
}

static int on_send_kpa_rtsp_tcp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    kpa_rtsp_tcp_t* p_tcp = (kpa_rtsp_tcp_t*)p_lparam;
    klb_socket_t* p_socket = p_tcp->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;
    bool first = true;

    while (true)
    {
        if (NULL == p_tcp->p_w_cur && 0 < klb_nlist_size(p_tcp->p_w_list))
        {
            p_tcp->p_w_cur = (klb_buf_t*)klb_nlist_pop_head(p_tcp->p_w_list);
            if (NULL != p_tcp->p_w_cur)
            {
                p_tcp->w_start = p_tcp->p_w_cur->start;
            }
        }

        klb_buf_t* p_buf = p_tcp->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, (const uint8_t*)(p_buf->p_buf + p_tcp->w_start), p_buf->end - p_tcp->w_start);
        if (0 < w)
        {
            p_tcp->w_start += w;
            send += w;

            if (p_buf->end <= p_tcp->w_start)
            {
                p_tcp->p_w_cur = p_buf->p_next;
                if (NULL != p_tcp->p_w_cur)
                {
                    p_tcp->w_start = p_tcp->p_w_cur->start;
                }

                klb_buf_unref(p_buf);
            }
        }
        else
        {
            if (first)
            {
                // select成功, 却第一次无法写, 表明对方已经断开
                err = KLB_SOCKET_DISCONNECT;
            }

            break; // 无法再发送
        }

        first = false;
    }

    if (NULL == p_tcp->p_w_cur && klb_nlist_size(p_tcp->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        //p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return send;
}

//////////////////////////////////////////////////////////////////////////

void kpa_rtsp_tcp_close(kpa_rtsp_tcp_t* p_tcp)
{
    klb_multiplex_remove(p_tcp->p_multi, p_tcp->id);
}

int kpa_rtsp_tcp_send(kpa_rtsp_tcp_t* p_tcp, const char* p_data, int data_len)
{
    klb_buf_t* p_buf = klb_buf_malloc(data_len, false);
    memcpy(p_buf->p_buf, p_data, data_len);
    p_buf->end = data_len;

    klb_nlist_push_tail(p_tcp->p_w_list, p_buf);
    klb_socket_set_writing(p_tcp->p_socket, true);

    return 0;
}

int kpa_rtsp_tcp_send_media(kpa_rtsp_tcp_t* p_tcp, klb_buf_t* p_data)
{
    return 0;
}

kpa_rtsp_tcp_t* kpa_rtsp_tcp_create(klb_multiplex_t* p_multi, klb_socket_fd fd, kpa_rtsp_tcp_recv_cb cb_recv, void* p_obj)
{
    kpa_rtsp_tcp_t* p_tcp = KLB_MALLOCZ(kpa_rtsp_tcp_t, 1, 0);

    p_tcp->p_multi = p_multi;
    p_tcp->fd = fd;
    p_tcp->cb_recv = cb_recv;
    p_tcp->p_obj = p_obj;
    p_tcp->p_socket = klb_socket_async_create(fd);

    p_tcp->p_w_list = klb_nlist_create();
    p_tcp->p_r_buf = klb_buf_malloc(32 * 1024, false);

    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = on_remove_kpa_rtsp_tcp;
    o.cb_recv = on_recv_kpa_rtsp_tcp;
    o.cb_send = on_send_kpa_rtsp_tcp;

    o.p_lparam = p_tcp;
    o.p_wparam = NULL;

    p_tcp->id = klb_multiplex_push_socket(p_tcp->p_multi, p_tcp->p_socket, &o);

    klb_socket_set_reading(p_tcp->p_socket, true);

    return p_tcp;
}
