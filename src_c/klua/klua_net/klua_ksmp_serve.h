///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ksmp_serve.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   Lua-SMP服务处理(klb lua smp serve)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_KSMP_SERVE_H__
#define __KLUA_KSMP_SERVE_H__

#include "klua/klua.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建SMP服务 meta
/// @return void
void klua_ksmpserve_createmeta(lua_State* L);

/// @brief 创建 SMP 监听 服务 meta
/// @return void
void klua_ksmpserve_listen_createmeta(lua_State* L);

/// @brief SMP 服务监听
int klua_ksmpserve_listen(lua_State* L);


/// @brief SMP-RPC 服务 meta
void klua_ksmpserverpc_createmeta(lua_State* L);

/// @brief 创建 SMP-RPC 监听 服务 meta
void klua_ksmpserverpc_listen_createmeta(lua_State* L);

/// @brief SMP-RPC 服务监听
int klua_ksmpserverpc_listen(lua_State* L);



#ifdef __cplusplus
}
#endif

#endif // __KLUA_KSMP_SERVE_H__
//end
