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


void klua_loadlib(lua_State* L, lua_CFunction openlib, const char* p_name)
{
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");

    lua_pushcfunction(L, openlib);
    lua_setfield(L, -2, p_name);

    lua_pop(L, 1);  // remove _PRELOAD table
}

bool luaL_checkboolean(lua_State* L, int arg)
{
    luaL_checktype(L, arg, LUA_TBOOLEAN);

    return lua_toboolean(L, arg);
}

void* luaL_checklightuserdata(lua_State* L, int arg)
{
    luaL_checktype(L, arg, LUA_TLIGHTUSERDATA);

    return lua_touserdata(L, arg);
}

//////////////////////////////////////////////////////////////////////////

int klua_ref_registryindex(lua_State* L, int arg)
{
    lua_pushvalue(L, arg);
    int reg = luaL_ref(L, LUA_REGISTRYINDEX);
    assert(0 < reg);

    return reg;
}

void klua_unref_registryindex(lua_State* L, int reg)
{
    if (0 < reg)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, reg);
    }
}

//////////////////////////////////////////////////////////////////////////

// from ./klb/third/lua-cjson-2.1.0/lua_cjson.c
extern int luaopen_cjson(lua_State *l);
extern int luaopen_cjson_safe(lua_State *l);

int klua_open_cjson(lua_State* L)
{
    return luaopen_cjson(L);
}

int klua_open_cjson_safe(lua_State* L)
{
    return luaopen_cjson_safe(L);
}

// from ./klb/third/lpeg-1.0.2/lptree.c
extern int luaopen_lpeg(lua_State *L);

int klua_open_lpeg(lua_State *L)
{
    return luaopen_lpeg(L);
}

// from ./klb/third/luafilesystem-2.0/src/lfs.c
extern int luaopen_lfs(lua_State *L);

int klua_open_lfs(lua_State *L)
{
    return luaopen_lfs(L);
}
