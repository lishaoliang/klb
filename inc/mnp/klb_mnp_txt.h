///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/07/06
//
/// @file    klb_mnp_txt.h
/// @brief   media net protocol, txt定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/07/06 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_TXT_H__
#define __KLB_MNP_TXT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#pragma pack(4)

/// @struct klb_mnp_txth_t
/// @brief  media net protocol, text head
typedef struct klb_mnp_txth_t_
{
    uint32_t    size;       ///< 文本数据大小(包含本结构体)
    uint32_t    ttype;      ///< 类型(text type)
    uint32_t    sequence;   ///< 序列号
    uint32_t    uid;        ///< 用户自定义ID(user defined id)
    // - 4 + 4 + 4 + 4 = 12 Byte
}klb_mnp_txth_t;

#pragma pack()


/// @enum  klb_mnp_ttype_e
/// @brief 文本类型
typedef enum klb_mnp_ttype_e_
{
    KLB_MNP_TJSON     = 0,      ///< JSON
    KLB_MNP_TXML      = 1,      ///< XML
    KLB_MNP_TTYPE_MAX = 0x7FFF  ///< MAX
}klb_mnp_ttype_e;


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNP_TXT_H__
//end
