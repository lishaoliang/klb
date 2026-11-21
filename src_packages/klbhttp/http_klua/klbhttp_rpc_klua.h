// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbhttp_rpc_klua.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   khttp RPC 绑定(connect_rpc / listen_rpc)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBHTTP_RPC_KLUA_H__
#define __KLBHTTP_RPC_KLUA_H__

#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建 khttp 客户端 RPC metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_khttpclientrpc_createmeta(lua_State* L);


/// @brief 创建 khttp 服务 RPC metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_khttpserverpc_createmeta(lua_State* L);


/// @brief 创建 khttp RPC 监听 metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_khttpserverpc_listen_createmeta(lua_State* L);


/// @brief khttp.connect_rpc(host, port [, opts])
/// @param [in] *L            Lua 状态
/// @return int 1
int klua_khttp_connect_rpc(lua_State* L);


/// @brief khttp.listen_rpc(port [, opts])
/// @param [in] *L            Lua 状态
/// @return int 1
int klua_khttp_listen_rpc(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBHTTP_RPC_KLUA_H__

// end
