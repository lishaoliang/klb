// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wsrpcer.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WebSocket RPC 类型与方法名(klb WS rpcer)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
/// @note 握手后每条 RPC 为一个 WS BINARY 帧.
///   帧载荷 = [klb_mnp_rpc_t] + 载荷.
///   JSON 载荷为 JSON 文本, LUA 载荷为 klua_seri 二进制.
///   默认路径 /rpc.
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WSRPCER_H__
#define __KLB_WSRPCER_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 转换为 RPCTYPE 字符串
/// @param [in] rpctype       RPC 类型: KLB_MNP_RPC_LUA / KLB_MNP_RPC_JSON
/// @return const char* "LUA" / "JSON"; 未知为空串
const char* klb_wsrpcer_to_rpctype_string(int rpctype);


/// @brief 转换为 method 字符串
/// @param [in] method        RPC 方法: klb_mnp_rpc_method_e
/// @return const char* "POST" / "NOTIFY" / "REQUEST" / "RESPONSE"; 未知为空串
const char* klb_wsrpcer_to_method_string(int method);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WSRPCER_H__

// end
