///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    kpa_rtsp_udp_rtp.h
/// @brief   rtp udp
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KPA_RTSP_UDP_RTP_H__
#define __KPA_RTSP_UDP_RTP_H__

#include "klb_type.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_socket.h"
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_buf.h"


typedef struct kpa_rtsp_udp_rtp_t_
{
    klb_multiplex_t*    p_multi;        ///< multi
    klb_socket_t*       p_socket;       ///< socket

    klb_socket_fd       fd;             ///< fd
    int                 id;             ///< socket id
    int                 local_port;     ///< 本地端口
    int                 remote_port;    ///< 对方端口

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
}kpa_rtsp_udp_rtp_t;


kpa_rtsp_udp_rtp_t* kpa_rtsp_udp_rtp_create(klb_multiplex_t* p_multi, int local_port, int remote_port);

void kpa_rtsp_udp_rtp_close(kpa_rtsp_udp_rtp_t* p_udp);


#endif // __KPA_RTSP_UDP_RTP_H__
//end
