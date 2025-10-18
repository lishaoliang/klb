///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTP_H__
#define __KLB_RTP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_rtp_head_t
/// @brief  RTP 头部最小长度为12字节(固定部分), (RFC 3550)
///    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
///  + -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
///  | V = 2 | P = 0 | X = 0 | CC = 0 | M = 0 | PT = 0 | sequence number |
///  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
///  | timestamp |
///  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
///  | SSRC |
///  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
typedef struct klb_rtp_head_t_
{
    uint8_t     version : 2;            ///< version : 2; 版本号,当前 RTP 版本为 2(RFC 3550 规定), 历史版本 0 和 1 已废弃
    uint8_t     padding : 1;            ///< padding flag : 填充位, 若为 1,报文末尾包含填充字节(用于对齐,最后一字节表示填充长度)
    uint8_t     extension : 1;          ///< extension flag : 扩展位, 若为 1, 固定头部后紧跟一个扩展头部
    uint8_t     csrc_count : 4;         ///< CSRC Count : CSRC(贡献源标识符)数量, 指示固定头部后跟随的 CSRC 列表长度(0~15)

    uint8_t     marker : 1;             ///< marker: 标记位,含义由具体负载类型(如音频 / 视频)定义, 通常用于标记帧边界(如视频关键帧)
    uint8_t     payload_type : 7;       ///< payload type : 负载类型, 指示 RTP 负载的格式(如 PCM 音频 H.264 视频等),值由协商确定(动态范围 96~127)

    uint16_t    sequence;               ///< sequence number : 序列号, 每发送一个 RTP 包递增 1,用于接收端检测丢包和重排序
    uint32_t    timestamp;              ///< timestamp : 时间戳, 反映负载数据的采样时刻(单位由负载类型定义,如音频采样率), 用于同步和抖动补偿
    uint32_t    ssrc;                   ///< synchronization source : 同步源标识符, 唯一标识 RTP 流的发送端(随机生成, 确保碰撞概率极低)
}klb_rtp_head_t;


/// @struct klb_rtp_headex_t
/// @brief  RTP 头部扩展: 若 klb_rtp_head_t.extension = 1 时生效
typedef struct klb_rtp_headex_t_
{
    uint16_t    profile_id;             ///< 扩展类型
    uint16_t    length;                 ///< 扩展数据实际长度 = length * 4; 不包含本结构体
}klb_rtp_headex_t;


#pragma pack()


#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTP_H__
//end
