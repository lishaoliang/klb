///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua_help.h
/// @brief   修改自lua.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/30 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_HELP_H__
#define __KLUA_HELP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <assert.h>


#define KLUA_HELP_TOP_B(L_)     int klua_tb_ = lua_gettop(L_);
#define KLUA_HELP_TOP_E(L_)     int klua_te_ = lua_gettop(L_); assert(klua_tb_ == klua_te_);


int klua_help_report(lua_State *L, int status);

int klua_help_dofile(lua_State *L, const char *name);
int klua_help_dolibrary(lua_State *L, const char *name);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_HELP_H__
//end
