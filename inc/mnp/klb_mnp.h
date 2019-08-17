﻿///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/07/06
//
/// @file    klb_mnp.h
/// @brief   media net protocol, 媒体网络协议
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/07/06 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_H__
#define __KLB_MNP_H__

#include "klb_type.h"
#include "mnp/klb_mnp_txt.h"
#include "mnp/klb_mnp_bin.h"
#include "mnp/klb_mnp_media.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_mnp_t
/// @brief  网络封包头
///  \n 缓存大小:
///  \n  媒体包缓存: [4K, 8K, 16K, 32K]
typedef struct klb_mnp_t_
{
    uint16_t magic;             ///< 魔数: KLB_MNP_MAGIC
    uint16_t magic_user;        ///< magic random, 自定义魔数: KLB_MNP_MAGIC_USER
    //- 4 Byte

    uint16_t size;              ///< 单个数据包大小(包含本结构体): <= 60K (超过60K,协议错误)

    uint8_t  subtype;           ///< 子协议类型: klb_mnp_subtype_e
    uint8_t  opt : 2;           ///< 包组合方式: klb_mnp_opt_e
    uint8_t  packtype : 3;      ///< pack type包类型
    uint8_t  resv1 : 3;         ///< 0 
    //- 4 + 4 = 8 Byte

    uint32_t resv2;             ///< 0
    //- 8 + 4 = 12 Byte
}klb_mnp_t;

#pragma pack()


/// @def   KLB_MNP_MAGIC
/// @brief media net protocol魔数
#define KLB_MNP_MAGIC           0xDCE6


/// @def   KLB_MNP_MAGIC_USER
/// @brief media net protocol user魔数
#define KLB_MNP_MAGIC_USER      0x6ECD


/// @struct klb_mnp_opt_e
/// @brief  包组合方式
typedef enum klb_mnp_opt_e_
{
    KLB_MNP_BEGIN    = 0x0,     ///< Begin包(B包)
    KLB_MNP_CONTINUE = 0x1,     ///< Continue包(C包)
    KLB_MNP_END      = 0x2,     ///< End包(E包)
    KLB_MNP_FULL     = 0x3,     ///< Full包(F包)

    KLB_MNP_OPT_MAX  = 0x3      ///< MAX
}klb_mnp_opt_e;


/// @struct klb_mnp_packtype_e
/// @brief  包类型
typedef enum klb_mnp_packtype_e_
{
    KLB_MNP_HEART        = 0x0, ///< 心跳包: 附加数据为0
    KLB_MNP_TXT          = 0x1, ///< 文本数据
    KLB_MNP_BIN          = 0x2, ///< 二进制数据
    KLB_MNP_MEDIA        = 0x3, ///< 媒体数据
    KLB_MNP_PACKTYPE_MAX = 0x7  ///< MAX
}klb_mnp_packtype_e;


/// @struct klb_mnp_subtype_e
/// @brief  子协议类型
typedef enum klb_mnp_subtype_e_
{
    KLB_MNP_SUB_MEDIA   = 0x00, ///< 媒体协议
    KLB_MNP_SUBTYPE_MAX = 0xFF  ///< MAX
}klb_mnp_subtype_e;


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_H__
//end
