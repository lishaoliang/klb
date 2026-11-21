// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc_sctp.h
/// @brief   WebRTC DataChannel 的 SCTP/DCEP 最小子集 (内部接口, 仅供 core 使用)
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件: SCTP 关联四向握手 + DATA + DCEP OPEN/ACK
/// @warning 多流 (8 条) + DATA 分片/重组 + SACK + RTO 重传; CRC32c 按 RFC 4960 小端存储
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WEBRTC_SCTP_H__
#define __KLB_WEBRTC_SCTP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_WEBRTC_DC_STREAMS
/// @brief 单关联协商的出/入站流数上限
#define KLB_WEBRTC_DC_STREAMS          8

/// @def   KLB_WEBRTC_DC_MSG_MAX
/// @brief 单条应用消息上限 (分片重组缓冲)
#define KLB_WEBRTC_DC_MSG_MAX          32768

/// @def   KLB_WEBRTC_DC_PPID_DCEP
/// @brief DCEP 载荷协议号
#define KLB_WEBRTC_DC_PPID_DCEP         50

/// @def   KLB_WEBRTC_DC_PPID_STRING
/// @brief UTF-8 文本消息
#define KLB_WEBRTC_DC_PPID_STRING       51

/// @def   KLB_WEBRTC_DC_PPID_BINARY
/// @brief 二进制消息
#define KLB_WEBRTC_DC_PPID_BINARY       53


/// @brief DataChannel 会话 (不透明)
typedef struct klb_webrtc_dc_t_ klb_webrtc_dc_t;


/// @brief 创建 DataChannel 会话
/// @param [in] is_client       1(true).SCTP 客户端; 0(false).服务端
/// @param [out] *p_code        可选; 失败时 klb_netcode_e
/// @return klb_webrtc_dc_t* NULL.失败; 非 NULL.会话
klb_webrtc_dc_t* klb_webrtc_dc_create(bool is_client, int* p_code);


/// @brief 销毁会话
/// @param [in] *p_dc           会话; 可为 NULL
/// @return 无
void klb_webrtc_dc_destroy(klb_webrtc_dc_t* p_dc);


/// @brief 通道是否已打开 (DCEP OPEN/ACK 完成)
/// @return int 1.已打开; 0.未打开
int klb_webrtc_dc_is_open(klb_webrtc_dc_t* p_dc);


/// @brief 指定流上的通道是否已打开
/// @param [in] *p_dc           会话
/// @param [in] stream_id       流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @return int 1.已打开; 0.未打开
int klb_webrtc_dc_is_open_stream(klb_webrtc_dc_t* p_dc, int stream_id);


/// @brief 发起通道打开 (客户端发送 INIT 并排队 DCEP OPEN)
/// @return int 0.成功; 小于 0.klb_netcode_e
int klb_webrtc_dc_open(klb_webrtc_dc_t* p_dc);


/// @brief 在指定流上打开一条 DataChannel
/// @param [in] *p_dc           会话
/// @param [in] stream_id       流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 关联建立前调用会记入待打开集合, COOKIE_ACK 后统一发 DCEP OPEN
int klb_webrtc_dc_open_stream(klb_webrtc_dc_t* p_dc, int stream_id);


/// @brief 封装一条应用消息为 SCTP 包
/// @param [in]  *p_dc          会话
/// @param [in]  *p_msg         消息内容
/// @param [in]  len            消息长度
/// @param [in]  ppid           载荷协议号
/// @param [out] *p_out         输出报文
/// @param [in]  cap            输出容量
/// @param [out] *p_out_len     输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
int klb_webrtc_dc_output(klb_webrtc_dc_t* p_dc, const uint8_t* p_msg, int len, int ppid,
                         uint8_t* p_out, int cap, int* p_out_len);


/// @brief 按指定流封装一条应用消息 (超过 MTU 自动分片, 余片入队)
/// @param [in]  *p_dc          会话
/// @param [in]  stream_id      流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @param [in]  *p_msg         消息内容
/// @param [in]  len            消息长度; 0..KLB_WEBRTC_DC_MSG_MAX
/// @param [in]  ppid           载荷协议号
/// @param [out] *p_out         首片输出报文
/// @param [in]  cap            输出容量
/// @param [out] *p_out_len     首片长度
/// @return int 0.成功; 小于 0.klb_netcode_e
int klb_webrtc_dc_output_stream(klb_webrtc_dc_t* p_dc, int stream_id,
                                const uint8_t* p_msg, int len, int ppid,
                                uint8_t* p_out, int cap, int* p_out_len);


/// @brief 处理一个入站 SCTP 包
/// @param [in]  *p_dc          会话
/// @param [in]  *p_data        报文
/// @param [in]  len            报文长度
/// @param [out] *p_msg         应用消息输出
/// @param [in]  cap            输出容量
/// @param [out] *p_msg_len     输出长度; 无消息为 0
/// @return int 0.已处理; 小于 0.klb_netcode_e
int klb_webrtc_dc_input(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len,
                        uint8_t* p_msg, int cap, int* p_msg_len);


/// @brief 处理一个入站 SCTP 包 (同时输出消息所属流号)
/// @param [in]  *p_dc          会话
/// @param [in]  *p_data        报文
/// @param [in]  len            报文长度
/// @param [out] *p_msg         应用消息输出
/// @param [in]  cap            输出容量
/// @param [out] *p_msg_len     输出长度; 无消息为 0
/// @param [out] *p_stream_id   消息所属流号; 可为 NULL
/// @return int 0.已处理; 小于 0.klb_netcode_e
int klb_webrtc_dc_input_ex(klb_webrtc_dc_t* p_dc, const uint8_t* p_data, int len,
                           uint8_t* p_msg, int cap, int* p_msg_len, int* p_stream_id);


/// @brief 取一个待发控制包 (INIT/INIT_ACK/COOKIE_ECHO/COOKIE_ACK/DCEP)
/// @param [in]  *p_dc          会话
/// @param [out] *p_out         输出报文
/// @param [in]  cap            输出容量
/// @param [out] *p_out_len     输出长度; 无待发为 0
/// @return int 0.已处理; 小于 0.klb_netcode_e
int klb_webrtc_dc_poll(klb_webrtc_dc_t* p_dc, uint8_t* p_out, int cap, int* p_out_len);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_WEBRTC_SCTP_H__

// end
