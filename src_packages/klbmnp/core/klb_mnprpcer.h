// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnprpcer.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   MNP RPC 类型与方法字符串
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNPRPCER_H__
#define __KLB_MNPRPCER_H__

#include "klb_type.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 转换为 RPCTYPE 字符串
const char* klb_mnprpcer_to_rpctype_string(int rpctype);


/// @brief 转换为 method 字符串
const char* klb_mnprpcer_to_method_string(klb_mnp_rpc_method_e method);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNPRPCER_H__

// end
