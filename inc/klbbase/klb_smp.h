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

    uint32_t size;              ///< 数据包长度(包含本结构体)

    uint32_t packtype : 5;      ///< 包类型: klb_mnp_packtype_e
    uint32_t resv1 : 3;
    uint32_t resv2 : 24;

    //- 4 + 4 + 4 = 12 Byte
}klb_smp_t;

#pragma pack()


// 包类型定义: 
// (KLB_MNP_PONG)心跳包(回应) : [klb_smp_t]
// (KLB_MNP_TEXT)文本数据 : [klb_smp_t] + [klb_mnp_common_t] + [head] + [body] + [padding]
// (KLB_MNP_BINARY)二进制数据 : [klb_smp_t] + [klb_mnp_common_t] + [head] + [body] + [padding]
// (KLB_MNP_MEDIA)媒体数据 : [klb_smp_t] + [klb_mnp_media_t] + [H264] + [padding]
// (KLB_MNP_PING)心跳包(发起) : [klb_smp_t]
// (KLB_MNP_RPC_LUA)RPC Lua包 : [klb_smp_t] + [klb_mnp_common_t] + [rpc-lua] + [padding]
// (KLB_MNP_RPC_JSON)RPC Json包 : [klb_smp_t] + [klb_mnp_common_t] + [rpc-json] + [padding]


#ifdef __cplusplus
}
#endif


#endif // __KLB_SMP_H__
//end
