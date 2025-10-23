///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_krtsp_serve.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   Lua-RTSP服务处理(klb lua krtsp serve)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_KRTSP_SERVE_H__
#define __KLUA_KRTSP_SERVE_H__

#include "klua/klua.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建rtsp服务 meta
/// @return void
void klua_krtspserve_createmeta(lua_State* L);


/// @brief rtsp 服务连接 服务
/// @return void
int klua_krtspserve_new_serve(lua_State* L);


/// @brief 创建rtsp 监听 服务 meta
/// @return void
void klua_krtspserve_listen_createmeta(lua_State* L);


/// @brief 服务监听
int klua_krtspserve_listen(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_KRTSP_SERVE_H__
//end
