///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/03/28
//
/// @file    klb_ncm_ops.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/03/28 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NCM_OPS_H__
#define __KLB_NCM_OPS_H__

#include "klb_type.h"
#include "klbnet/klb_ncm.h"

#if defined(__cplusplus)
extern "C" {
#endif


int klb_ncm_register_ops_mnp(klb_ncm_t* p_ncm, int protocol);


/// @def   KLB_NCM_REGISTER_PARSER_STD
/// @brief ncm预注册的解析器
#define KLB_NCM_REGISTER_OPS_STD(NCM_) {                            \
    klb_ncm_register_ops_mnp((NCM_), KLB_PROTOCOL_MNP);             \
    klb_ncm_register_ops_mnp((NCM_), KLB_PROTOCOL_MNPS);            \
}

#ifdef __cplusplus
}
#endif

#endif // __KLB_NCM_OPS_H__
//end
