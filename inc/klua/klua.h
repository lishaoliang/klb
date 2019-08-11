///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/30 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_H__
#define __KLUA_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 自定义预加载Lua库
/// @param [in] *L              Lua状态
/// @return int 0
typedef int(*klua_openlibs_cb)(lua_State* L);


/// @brief 对lua.c中main简易修改
/// @param [in] argc        命令行参数个数
/// @param [in] **argv      命令行参数
/// @param [in] cb          预加载库函数
/// @return int 同main函数
KLB_API int klua_main(int argc, char** argv, klua_openlibs_cb cb);


/// @brief 预加载库函数
/// @param [in] *L          Lua状态
/// @param [in] openlib     库函数列表
/// @param [in] p_name      库名称
/// @return 无
KLB_API void klua_loadlib(lua_State* L, lua_CFunction openlib, const char* p_name);


/// @brief check boolean
/// @return int 0.false; 1.true
KLB_API int luaL_checkboolean(lua_State* L, int arg);


/// @brief check lightuserdata
/// @return void*
KLB_API void* luaL_checklightuserdata(lua_State* L, int arg);


/// @brief 扩展库"kos"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kos(lua_State* L);


/// @brief 扩展库"ktime"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_ktime(lua_State* L);


/// @brief 扩展库"kthread"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kthread(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_H__
//end
