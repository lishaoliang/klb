///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   simplify media protocol 简易媒体协议
///          对mnp协议, 精简设计 : 一块数据, 不再进行切分
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
///               定义协议封包头部
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMP_H__
#define __KLB_SMP_H__

#include "klb_type.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(4)


/// @struct klb_smp_t
/// @brief  网络封包头
///  \n 固定12字节
///  \n 小端序:
typedef struct klb_smp_t_
{
    uint32_t magic;             ///< 魔数: KLB_SMP_MAGIC
#define KLB_SMP_MAGIC           0x504D53EB  ///< "*SMP"
    //- 4 Byte

    uint32_t size : 27;         ///< 数据包长度(包含本结构体); max=2^27=128M
    uint32_t packtype : 5;      ///< 包类型: klb_mnp_packtype_e
    //- 4 + 4 = 8 Byte
}klb_smp_t;

#pragma pack()


#define KLB_SMP_HEAD_MAX        (64)    //< SMP 头部最大长度


// 包类型定义: 
// (KLB_MNP_PONG)心跳包(回应) : [klb_smp_t]
// (KLB_MNP_TEXT)文本数据 : [klb_smp_t] + [klb_mnp_text_t] + [head] + [body]
// (KLB_MNP_BINARY)二进制数据 : [klb_smp_t] + [klb_mnp_binary_t] + [head] + [body]
// (KLB_MNP_MEDIA)媒体数据 : [klb_smp_t] + [klb_mnp_media_t] + [H264]
// (KLB_MNP_PING)心跳包(发起) : [klb_smp_t]
// (KLB_MNP_RPC_LUA)RPC Lua包 : [klb_smp_t] + [klb_mnp_rpc_t] + [rpc-lua]
// (KLB_MNP_RPC_JSON)RPC Json包 : [klb_smp_t] + [klb_mnp_rpc_t] + [rpc-json]


#ifdef __cplusplus
}
#endif


#endif // __KLB_SMP_H__
//end
