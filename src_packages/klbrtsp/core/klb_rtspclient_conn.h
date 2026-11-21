///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtspclient_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTSP客户端连接(klb rtsp client connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTSPCLIENT_CONN_H__
#define __KLB_RTSPCLIENT_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建rtsp连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_rtspclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief 连接目标
/// @param [out] *p_code          可选; 失败时 klb_netcode_e; 成功为 0
KLB_API klb_netconn_t* klb_rtspclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, int* p_code);


/// @brief 设置视频 RTP payload type. 默认 96
void klb_rtspclient_conn_set_video_pt(klb_netconn_t* p_conn, int pt);


/// @brief 设置音频 RTP payload type. 默认 97; 0 表示关闭音频解包
void klb_rtspclient_conn_set_audio_pt(klb_netconn_t* p_conn, int pt);


/// @brief 设置音频采样参数, 写入 klb_mnp_audio_t. 默认 44100/2/2
void klb_rtspclient_conn_set_audio_info(klb_netconn_t* p_conn, int samples, int tracks, int bits_per_sample);


/// @brief 设置连接读写/空闲超时, 由定时器(ticker)判定超时后上报 KLB_SOCKET_TIMEOUT
/// @param [in]  read_ms   读超时(毫秒); 0=不启用
/// @param [in]  write_ms  写超时(毫秒); 0=不启用
/// @param [in]  idle_ms   空闲超时(毫秒); 0=不启用
/// @return 无
/// @note 参考 mnp/smp: 基于套接字 last_recv_tc/last_send_tc 判定
void klb_rtspclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms);


/// @brief UDP 数据到达回调. RTP 由 p_data 从 RTP 头开始; RTCP 为整包
typedef void (*klb_rtspudp_rtp_cb)(void* p_udata, char* p_data, int data_len);


/// @brief 绑定一对偶数 RTP / 奇数 RTCP UDP 端口, 并挂进 netmulti
/// @param [in]  cb_rtp          RTP 回调. 可为 NULL
/// @param [in]  cb_rtcp         RTCP 回调; 为 NULL 时只接收丢弃
/// @param [out] *p_out_rtp      RTP 子连接
/// @param [out] *p_out_rtcp     RTCP 子连接
/// @param [out] *p_rtp_port     偶数 RTP 端口
/// @return int 0.成功; 非0.失败
int klb_rtspudp_open_pair(klb_netmulti_t* p_multi, void* p_udata, klb_rtspudp_rtp_cb cb_rtp, klb_rtspudp_rtp_cb cb_rtcp, klb_netconn_t** p_out_rtp, klb_netconn_t** p_out_rtcp, int* p_rtp_port);


/// @brief 从 netmulti 移除并销毁 UDP 端口对. 指针可为 NULL
void klb_rtspudp_close_pair(klb_netmulti_t* p_multi, klb_netconn_t* p_rtp, klb_netconn_t* p_rtcp);


/// @brief 为客户端打开 UDP 接收端口
/// @param [out] *p_rtp_port     偶数 RTP 端口, 即 client_port
/// @return int 0.成功; 非0.失败
int klb_rtspclient_conn_open_udp(klb_netconn_t* p_conn, int* p_rtp_port);


/// @brief 关闭客户端 UDP 接收端口
void klb_rtspclient_conn_close_udp(klb_netconn_t* p_conn);


/// @brief UDP SETUP 成功后, 向对端 RTP 与 RTCP 端口各发一包 RTCP RR, 打开 NAT 映射
/// @param [in] server_rtp_port  对端 server_port 的偶数 RTP 端口
/// @return int 0.已发出; 非0.未发出
int klb_rtspclient_conn_udp_punch(klb_netconn_t* p_conn, const char* p_host, int server_rtp_port);



#ifdef __cplusplus
}
#endif


#endif // __KLB_RTSPCLIENT_CONN_H__
//end
