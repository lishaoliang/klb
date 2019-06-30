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

#if defined(__cplusplus)
extern "C" {
#endif

#include "klua/klua_cfg.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


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
KLB_API void klua_pre_loadlib(lua_State* L, lua_CFunction openlib, const char* p_name);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_H__
//end
