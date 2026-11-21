// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbws_rpc_klua.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   kws RPC 绑定(connect_rpc / listen_rpc)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWS_RPC_KLUA_H__
#define __KLBWS_RPC_KLUA_H__

#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建 kws 客户端 RPC metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_kwsclientrpc_createmeta(lua_State* L);


/// @brief 创建 kws 服务 RPC metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_kwsserverpc_createmeta(lua_State* L);


/// @brief 创建 kws RPC 监听 metatable
/// @param [in] *L            Lua 状态
/// @return 无
void klua_kwsserverpc_listen_createmeta(lua_State* L);


/// @brief kws.connect_rpc(host, port [, opts])
/// @param [in] *L            Lua 状态
/// @return int 1
int klua_kws_connect_rpc(lua_State* L);


/// @brief kws.listen_rpc(port [, opts])
/// @param [in] *L            Lua 状态
/// @return int 1
int klua_kws_listen_rpc(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBWS_RPC_KLUA_H__

// end
