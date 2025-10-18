///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtspparser.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   文件简要描述
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
    int             rtsptype;           ///< rtsp 包类型(klb_rtsptype_e); eg. KLB_RTSPTYPE_rtp
    int             pack_len;           ///< packet length; 数据包长度

    klb_rtp_head_t  rtp_head;           ///< RTP固定头(12字节)

    int             nalu_pos;           ///< NALU起始位置
    int             nalu_len;           ///< NALU数据长度
}klb_rtspparser_t;



/// @brief 解析 RTSP 包
/// @return int 0.解析成功; -1. 协议错误; 1. 数据不足
KLB_API int klb_rtspparser_parse(klb_rtspparser_t* p_parser, char* p_data, int data_len);




#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTSPPARSER_H__
//end
