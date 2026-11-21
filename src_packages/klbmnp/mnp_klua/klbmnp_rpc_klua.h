// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbmnp_rpc_klua.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   kmnp RPC 绑定(klb lua mnp rpc)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBMNP_RPC_KLUA_H__
#define __KLBMNP_RPC_KLUA_H__

#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 客户端 RPC meta
void klua_kmnpclientrpc_createmeta(lua_State* L);


/// @brief 客户端 RPC 连接
int klua_kmnpclient_connect_rpc(lua_State* L);


/// @brief 服务 RPC meta
void klua_kmnpserverpc_createmeta(lua_State* L);


/// @brief 服务 RPC 监听 meta
void klua_kmnpserverpc_listen_createmeta(lua_State* L);


/// @brief 服务 RPC 监听
int klua_kmnpserverpc_listen(lua_State* L);

/// @brief 由已探测 sock 创建 MNP-RPC 服务连接 (供 klisten / klblisten mux)
/// @note 参数1 lightuserdata=klb_netlisten_socket_t* / klb_netmux_socket_t*; 取得 fd 并释放该结构
int klua_kmnpserverpc_new_serve(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBMNP_RPC_KLUA_H__

// end
