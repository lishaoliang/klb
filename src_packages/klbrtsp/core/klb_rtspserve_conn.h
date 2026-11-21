///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtspserve_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTSP服务连接(klb rtsp serve connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTSPSERVE_CONN_H__
#define __KLB_RTSPSERVE_CONN_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建rtsp连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_rtspserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief 改走 TCP interleaved. rtp_ch 为 RTP 通道, RTCP 为 rtp_ch + 1
void klb_rtspserve_conn_setup_tcp(klb_netconn_t* p_conn, int rtp_ch);


/// @brief 改走 UDP. 向 RTSP 连接对端的 client_rtp_port 发送 RTP
/// @param [out] *p_server_rtp_port  本端偶数 RTP 端口
/// @return int 0.成功; 非0.失败
int klb_rtspserve_conn_setup_udp(klb_netconn_t* p_conn, int client_rtp_port, int* p_server_rtp_port);


/// @brief 设置连接读写/空闲超时, 由定时器(ticker)判定超时后上报 KLB_SOCKET_TIMEOUT
/// @param [in]  read_ms   读超时(毫秒); 0=不启用
/// @param [in]  write_ms  写超时(毫秒); 0=不启用
/// @param [in]  idle_ms   空闲超时(毫秒); 0=不启用
/// @return 无
/// @note 参考 mnp/smp: 基于套接字 last_recv_tc/last_send_tc 判定; 空闲超时用于回收空闲会话
void klb_rtspserve_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms);


/// @brief 设置视频 RTP 载荷类型(H264); 与 SDP 宣告一致
/// @param [in]  pt  载荷类型; <=0 或 >=128 不生效
/// @return 无
void klb_rtspserve_conn_set_video_pt(klb_netconn_t* p_conn, int pt);


/// @brief 设置音频 RTP 载荷类型(AAC); <=0 或 >=128 表示关闭音频
/// @return 无
void klb_rtspserve_conn_set_audio_pt(klb_netconn_t* p_conn, int pt);

/// @brief 设置指定轨的载荷类型与音频属性; track 越界忽略
/// @param [in]  track     轨索引
/// @param [in]  pt        载荷类型; <=0 或 >=128 记为 0
/// @param [in]  is_audio  true.音频轨; false.视频轨
/// @return 无
KLB_API void klb_rtspserve_conn_set_track_pt(klb_netconn_t* p_conn, int track, int pt, bool is_audio);

/// @brief 指定轨改走 TCP interleaved. rtp_ch 为 RTP 通道, RTCP 为 rtp_ch + 1
/// @return int 0.成功; 非0.失败
KLB_API int klb_rtspserve_conn_setup_tcp_track(klb_netconn_t* p_conn, int track, int rtp_ch);

/// @brief 指定轨改走 UDP. 向 RTSP 连接对端的 client_rtp_port 发送 RTP
/// @param [out] *p_server_rtp_port  本端偶数 RTP 端口
/// @return int 0.成功; 非0.失败
KLB_API int klb_rtspserve_conn_setup_udp_track(klb_netconn_t* p_conn, int track, int client_rtp_port, int* p_server_rtp_port);



#ifdef __cplusplus
}
#endif


#endif // __KLB_RTSPSERVE_CONN_H__
//end
