///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smpparser.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SMP协议解析
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMPPARSER_H__
#define __KLB_SMPPARSER_H__

#include "klb_type.h"
#include "klbbase/klb_smp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_smpparser_status_e
/// @brief 解析状态
typedef enum klb_smpparser_status_e_
{
    KLB_SMPPARSER_null      = 0,    ///< null
    KLB_SMPPARSER_head,             ///< 解析 SMP 头部
    KLB_SMPPARSER_body              ///< 解析 SMP 本体
}klb_smpparser_status_e;


/// @struct klb_smpparser_t
/// @brief  解析
typedef struct klb_smpparser_t_
{
    int                 packtype;   ///< 包类型: -1.失败
    int                 pack_len;   ///< 包长度

    int                 head_len;   ///< 头部长度

    klb_smp_t           smp;        ///< SMP协议头部

    union
    {
        klb_mnp_media_t media;      ///< 媒体数据包
        klb_mnp_rpc_t   rpc;        ///< RPC包 头部
    };
}klb_smpparser_t;


/// @brief 解析 SMP 包
/// @return int 0.解析成功; -1. 协议错误; 1. 数据不足
int klb_smpparser_parse(klb_smpparser_t* p_parser, char* p_data, int data_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SMPPARSER_H__
//end
