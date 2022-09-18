///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    kpa_rtsp_tcp.h
/// @brief   rtsp tcp
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KPA_RTSP_TCP_H__
#define __KPA_RTSP_TCP_H__


#include "klb_type.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_socket.h"
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_buf.h"


typedef int(*kpa_rtsp_tcp_recv_cb)(void* p_obj, int type, const char* p_head, int head_len, const char* p_body, int body_len);


typedef struct kpa_rtsp_tcp_t_
{
    klb_multiplex_t*    p_multi;
    klb_socket_t*       p_socket;

    klb_socket_fd       fd;
    int                 id;

    // send发送相关
    struct
    {
        klb_nlist_t*     p_w_list;       ///< 待发送列表: klb_buf_t*
        klb_buf_t*      p_w_cur;        ///< 当前正在发送的缓存
        int             w_start;        ///< 当前发送的起始点
    };

    // recv接收相关
    struct
    {
        klb_buf_t*      p_r_buf;        ///< 临时读取缓存
    };

    struct
    {
        kpa_rtsp_tcp_recv_cb    cb_recv;
        void*                   p_obj;
    };
}kpa_rtsp_tcp_t;


kpa_rtsp_tcp_t* kpa_rtsp_tcp_create(klb_multiplex_t* p_multi, klb_socket_fd fd, kpa_rtsp_tcp_recv_cb cb_recv, void* p_obj);

void kpa_rtsp_tcp_close(kpa_rtsp_tcp_t* p_tcp);

int kpa_rtsp_tcp_send(kpa_rtsp_tcp_t* p_tcp, const char* p_data, int data_len);

int kpa_rtsp_tcp_send_media(kpa_rtsp_tcp_t* p_tcp, klb_buf_t* p_data);


#endif // __KPA_RTSP_TCP_H__
//end
