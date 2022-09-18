///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    kpa_rtsp_client_inter.h
/// @brief   rtsp tcp
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KPA_RTSP_CLIENT_INTER_H__
#define __KPA_RTSP_CLIENT_INTER_H__


#include "klb_type.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_socket.h"
#include "klbmem/klb_buf.h"
#include "klbthird/sds.h"

#include "kpa_rtsp/kpa_rtsp_tcp.h"
#include "kpa_rtsp/kpa_rtsp_udp_rtp.h"
#include "kpa_rtsp/kpa_rtsp_udp_rtcp.h"

typedef enum kpa_rtsp_client_cmd_e_
{
    KPA_RTSP_CLIENT_NULL = 0,
    KPA_RTSP_CLIENT_OPTIONS,
    KPA_RTSP_CLIENT_DESCRIBE,
    KPA_RTSP_CLIENT_TEARDOWN,
    KPA_RTSP_CLIENT_SETUP_VIDEO,
    KPA_RTSP_CLIENT_SETUP_AUDIO,
    KPA_RTSP_CLIENT_PLAY,
    KPA_RTSP_CLIENT_GET_PARAMETER,
}kpa_rtsp_client_cmd_e;


typedef struct kpa_rtsp_client_inter_t_
{
    klb_multiplex_t*            p_multi;    ///< 复用
    kpa_rtsp_tcp_t*             p_tcp;      ///< tcp连接(RTSP协商)
    int                         seq_next;   ///< 下次请求的序列号

    // url等请求信息
    struct
    {
        sds url;            ///< rtsp 请求基础 url
        sds host;           ///< 主机
        int port;           ///< rtsp的tcp端口
        sds path;           ///< 请求路径参数
        sds session;        ///< 会话标示
    };

    struct
    {
        int                     last_seq;   ///< 上一次请求的序列号
        kpa_rtsp_client_cmd_e   last_cmd;   ///< 上一次的命令
    };

    // udp 端口等信息
    struct
    {
        int     video_client_port1;
        int     video_client_port2;
        int     video_server_port1;
        int     video_server_port2;

        int     audio_client_port1;
        int     audio_client_port2;
        int     audio_server_port1;
        int     audio_server_port2;
    };

    struct
    {
        kpa_rtsp_udp_rtp_t*         p_video_rtp;    ///< 从 service 发给 client, RTP视频
        kpa_rtsp_udp_rtcp_t*        p_video_rtcp;   ///< 视频 RTCP反馈包
        kpa_rtsp_udp_rtp_t*         p_audio_rtp;    ///< 从 service 发给 client, RTP音频
        kpa_rtsp_udp_rtcp_t*        p_audio_rtcp;   ///< 音频 RTCP反馈包
    };
}kpa_rtsp_client_inter_t;

kpa_rtsp_client_inter_t* kpa_rtsp_client_inter_create(klb_multiplex_t* p_multi, const char* p_host, int port, const char* p_path);
void kpa_rtsp_client_inter_destroy(kpa_rtsp_client_inter_t* p_inter);


#endif // __KPA_RTSP_CLIENT_INTER_H__
//end
