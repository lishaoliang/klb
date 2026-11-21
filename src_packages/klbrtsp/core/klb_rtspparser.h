///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtspparser.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTSP解析(rtsp parser)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTSPPARSER_H__
#define __KLB_RTSPPARSER_H__

#include "klb_type.h"
#include "klbnet/klbrtp/klb_rtp.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_rtsptype_e
/// @brief 解析出来的rtsp包类型
typedef enum klb_rtsptype_e_
{
    KLB_RTSPTYPE_null = 0,              ///< NULL
    KLB_RTSPTYPE_rtsp,                  ///< RTSP包
    KLB_RTSPTYPE_rtp,                   ///< RTP包
    KLB_RTSPTYPE_rtcp,                  ///< RTCP包
}klb_rtsptype_e;


/// @struct klb_rtspparser_t
/// @brief  RTSP数据解析
typedef struct klb_rtspparser_t_
{
    int                 rtsptype;       ///< rtsp 包类型(klb_rtsptype_e); eg. KLB_RTSPTYPE_rtp
    int                 pack_len;       ///< packet length; 数据包长度

    // 初步解析得到的信息
    struct
    {
        klb_rtp_head_t  rtp_head;       ///< RTP固定头(12字节)

        int             nalu_pos;       ///< NALU起始位置
        int             nalu_len;       ///< NALU数据长度
    };

    // 第二步解析得到的信息
    struct
    {
        int             opt;            ///< 封包类型: klb_mnp_opt_e
        uint8_t         nalu_value;     ///< NALU值

        uint8_t*        p_data;         ///< 有效数据
        int             data_len;       ///< 数据长度
    };
}klb_rtspparser_t;



/// @brief 解析 RTSP 包
/// @return int 0.解析成功; -1. 协议错误; 1. 数据不足
int klb_rtspparser_parse(klb_rtspparser_t* p_parser, char* p_data, int data_len);


/// @brief 解析 UDP 上的单个 RTP 包. 缓冲从 RTP 头开始, 不含 '$' 交织头
/// @return int 0.成功; 非0.错误
int klb_rtspparser_parse_rtp(klb_rtspparser_t* p_parser, char* p_data, int data_len);


/// @brief 解析 RTP H264
int klb_rtspparser_parse_rtp_h264(klb_rtspparser_t* p_parser, char* p_nalu, int nalu_len);


/// @brief 逐步解析 RTP H264. STAP-A 可依次产出多个 NALU
/// @param [in,out] *p_pos  首次传入 0, 成功后为下一偏移
/// @return int 0.产出一个 NALU; 1.没有更多; 负数.错误
int klb_rtspparser_parse_rtp_h264_step(klb_rtspparser_t* p_parser, char* p_nalu, int nalu_len, int* p_pos);


/// @brief 解析 RTP AAC (RFC3640 mpeg4-generic AAC-hbr, sizelength=13)
/// @note  产出首个 AU; p_data/data_len 指向裸 AAC 帧; opt=KLB_MNP_FULL
/// @return int 0.成功; 非0.失败
int klb_rtspparser_parse_rtp_aac(klb_rtspparser_t* p_parser, char* p_payload, int payload_len);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTSPPARSER_H__
//end
