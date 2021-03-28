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
#include "klbmem/klb_mem.h"
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

void klua_setfield_boolean(lua_State* L, const char* p_key, bool b)
{
    assert(NULL != p_key);
    lua_pushboolean(L, b);
    lua_setfield(L, -2, p_key);
}

void klua_setfield_integer(lua_State* L, const char* p_key, lua_Integer n)
{
    assert(NULL != p_key);
    lua_pushinteger(L, n);
    lua_setfield(L, -2, p_key);
}

void klua_setfield_string(lua_State* L, const char* p_key, const char* p_value)
{
    assert(NULL != p_key);
    assert(NULL != p_value);
    lua_pushstring(L, p_value);
    lua_setfield(L, -2, p_key);
}

void klua_setfield_lstring(lua_State* L, const char* p_key, const char* p_value, size_t v_len)
{
    assert(NULL != p_key);
    assert(NULL != p_value);
    lua_pushlstring(L, p_value, v_len);
    lua_setfield(L, -2, p_key);
}

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

// from ./klb/src_c/klua/lua-cjson-2.1.0/lua_cjson.c
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

// from ./klb/src_c/klua/lpeg-1.0.2/lptree.c
extern int luaopen_lpeg(lua_State *L);

int klua_open_lpeg(lua_State *L)
{
    return luaopen_lpeg(L);
}

// from ./klb/src_c/klua/luafilesystem-2.0/src/lfs.c
extern int luaopen_lfs(lua_State *L);

int klua_open_lfs(lua_State *L)
{
    return luaopen_lfs(L);
}


//////////////////////////////////////////////////////////////////////////

int klua_loadlib_all(lua_State* L)
{
    KLUA_LOADLIBS(L);

    return 0;
}
