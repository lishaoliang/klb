///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "klua/klua.h"
#include "mem/klb_mem.h"
#include <assert.h>


void klua_pre_loadlib(lua_State* L, lua_CFunction openlib, const char* p_name)
{
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");

    lua_pushcfunction(L, openlib);
    lua_setfield(L, -2, p_name);

    lua_pop(L, 1);  // remove _PRELOAD table
}
