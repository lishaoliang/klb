///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTP 结构定义
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


//////////////////////////////////////////////////////////////////////////
// rtp head

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


//////////////////////////////////////////////////////////////////////////
// rtp nalu h264


/// @enum  klb_rtp_nalu_type_e
/// @brief RTP NAL Unit Type 
typedef enum klb_rtp_nalu_type_e_
{
    KLB_RTP_NALU_NULL           = 0,        ///< 未使用
    KLB_RTP_NALU_VCL_1          = 1,        ///< VCL(1) 单元(视频编码层)
    KLB_RTP_NALU_VCL_23         = 23,       ///< VCL(23) 单元(视频编码层)

    KLB_RTP_NALU_STAP_A         = 24,       ///< STAP-A (单一时间聚合包): 聚合多个小 NALU 为一个 RTP 包(减少头部开销), 适合多个小尺寸 NALU(如 SPS PPS)
    KLB_RTP_NALU_STAP_B         = 25,       ///< 类似 STAP-A, 但包含 DTS(解码时间戳)字段,较少使用

    KLB_RTP_NALU_MTAP16         = 26,       ///< MTAP16(多时间聚合包): 聚合不同时间戳的 NALU, 包含 16 比特时间偏移, 用于特殊同步场景
    KLB_RTP_NALU_MTAP24         = 27,       ///< MTAP24: 类似 MTAP16, 但时间偏移为 24 比特

    KLB_RTP_NALU_FU_A           = 28,       ///< FU-A(Fragmentation Unit 分片单元) : 用于将大 NALU 拆分为多个 RTP 包(最常用的分片方式),通过 S(起始)和 E(结束)位标识分片边界
    KLB_RTP_NALU_FU_B           = 29,       ///< 类似 FU-A, 但包含 DTS 字段, 较少使用

    KLB_RTP_NALU_UNDEFINED_30   = 30,       ///< 保留
    KLB_RTP_NALU_UNDEFINED_31   = 31,       ///< 保留
}klb_rtp_nalu_type_e;


/// @struct klb_rtp_nalu_fu_t
/// @brief  NALU FU 分片头部定义
typedef struct klb_rtp_nalu_fu_t_
{
    uint8_t     start : 1;                  ///< 起始位: 1 表示当前分片是原始 NALU 的第一个分片(首片); 0 表示当前分片是中间分片或最后一个分片
    uint8_t     end : 1;                    ///< 结束位: 1 表示当前分片是原始 NALU 的最后一个分片(尾片); 0 表示当前分片是起始分片或中间分片
    uint8_t     reserved : 1;               ///< 保留位: 必须为 0
    uint8_t     nalu_type : 5;              ///< 原始 NALU 的类型(与原始 NALU 头部的 Type 字段一致); 接收端通过该字段还原原始 NALU 的类型
}klb_rtp_nalu_fu_t;


/// @struct klb_rtp_nalu_h264_t
/// @brief  RTP NALU H264 头部定义
///   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
///   +-+-+-+-+-+-+-+-++-+-+-+-+-+-+-+-+
///   | F | NRI    |        Type       |
typedef struct klb_rtp_nalu_h264_t_
{
    uint8_t     forbidden : 1;          ///< F(Forbidden Zero Bit) : 禁止位, 必须为 0; 若为 1, 表明该 NALU 存在不可恢复的错误, 接收端应丢弃
    uint8_t     nri : 2;                ///< NRI(NAL Reference IDC) : 参考重要性指示: 值越大, 该 NALU 对解码的重要性越高(0~3); 关键帧(IDR)的 NRI 通常为 3(最高); 非参考帧的 NRI 可能为 0
    uint8_t     nalu_type : 5;          ///< NAL Unit Type (klb_rtp_nalu_type_e) : NALU 类型. 标识 NALU 载荷的内容(0~31), 决定了 RTP 封装方式和解析逻辑
}klb_rtp_nalu_h264_t;



//////////////////////////////////////////////////////////////////////////
// rtp nalu h265



#pragma pack()


#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTP_H__
//end
