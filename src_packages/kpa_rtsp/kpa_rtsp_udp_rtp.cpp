#include "kpa_rtsp/kpa_rtsp_udp_rtp.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

#ifndef _WIN32
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#endif // !_WIN32


void kpa_rtsp_udp_rtp_destroy(kpa_rtsp_udp_rtp_t* p_udp)
{

}

//////////////////////////////////////////////////////////////////////////



int parse_kpa_rtsp_udp_rtp(kpa_rtsp_udp_rtp_t* p_tcp, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_tcp->p_r_buf;

    char* p_data = p_buf->p_buf + p_buf->start;
    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    //int head_len = 0, body_len = 0;
    //if (parse_rnrn_size_kpa_rtsp_tcp(p_data, data_len, &head_len, &body_len) != 0)
    //{
    //    return 1; // 数据不足
    //}

    //p_tcp->cb_recv(p_tcp->p_obj, 0, p_data, head_len, p_data + head_len, body_len);

    p_buf->start += data_len;

    return 0;
}


//////////////////////////////////////////////////////////////////////////


static int on_remove_kpa_rtsp_udp_rtp(void* p_lparam, void* p_wparam, int id)
{
    kpa_rtsp_udp_rtp_t* p_tcp = (kpa_rtsp_udp_rtp_t*)p_lparam;

    kpa_rtsp_udp_rtp_destroy(p_tcp);

    return 0;
}

static int on_recv_kpa_rtsp_udp_rtp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    kpa_rtsp_udp_rtp_t* p_tcp = (kpa_rtsp_udp_rtp_t*)p_lparam;
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

        struct sockaddr_in addr = { 0 };
        int add_len = sizeof(struct sockaddr_in);
        int r = klb_socket_recvfrom(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end, (struct sockaddr*)&addr, &add_len);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            //
            while (true)
            {
                if (0 != parse_kpa_rtsp_udp_rtp(p_tcp, p_socket))
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

static int on_send_kpa_rtsp_udp_rtp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    kpa_rtsp_udp_rtp_t* p_tcp = (kpa_rtsp_udp_rtp_t*)p_lparam;
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

kpa_rtsp_udp_rtp_t* kpa_rtsp_udp_rtp_create(klb_multiplex_t* p_multi, int local_port, int remote_port)
{
    kpa_rtsp_udp_rtp_t* p_udp = KLB_MALLOCZ(kpa_rtsp_udp_rtp_t, 1, 0);

    p_udp->p_multi = p_multi;
    p_udp->local_port = local_port;
    p_udp->remote_port = remote_port;

    p_udp->p_w_list = klb_nlist_create();
    p_udp->p_r_buf = klb_buf_malloc(32 * 1024, false);

    // 创建UDP, 绑定本地端口
    klb_socket_fd fd = klb_socket_udp();

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // "127.0.0.1"
    addr.sin_port = htons(local_port);
    klb_socket_bind(fd, (const struct sockaddr*)&addr, sizeof(addr));

    p_udp->p_socket = klb_socket_async_create(fd);

    // 
    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = on_remove_kpa_rtsp_udp_rtp;
    o.cb_recv = on_recv_kpa_rtsp_udp_rtp;
    o.cb_send = on_send_kpa_rtsp_udp_rtp;

    o.p_lparam = p_udp;
    o.p_wparam = NULL;

    p_udp->id = klb_multiplex_push_socket(p_udp->p_multi, p_udp->p_socket, &o);

    klb_socket_set_connected(p_udp->p_socket, true);
    klb_socket_set_reading(p_udp->p_socket, true);

    return p_udp;
}

void kpa_rtsp_udp_rtp_close(kpa_rtsp_udp_rtp_t* p_udp)
{
    klb_multiplex_remove(p_udp->p_multi, p_udp->id);
}
