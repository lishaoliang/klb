///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smprpcer.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMPRPCER_H__
#define __KLB_SMPRPCER_H__

#include "klb_type.h"
#include "klbbase/klb_smp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 转换为RPCTYPE字符串
/// @param [in] rpctype       RPC类型  
/// @return const char*
const char* klb_smprpcer_to_rpctype_string(int rpctype);


/// @brief 转换为method字符串
/// @param [in] method        RPC方法 
/// @return const char*
const char* klb_smprpcer_to_method_string(klb_mnp_rpc_method_e method);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SMPRPCER_H__
//end
