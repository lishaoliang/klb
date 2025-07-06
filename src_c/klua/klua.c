// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "lstate.h"
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

bool klua_check_coroutine(lua_State* L, const char* p_tip_msg)
{
    if (!klua_is_coroutine(L))
    {
        luaL_error(L, p_tip_msg);
        return false;
    }

    return true;
}

bool klua_check_option_boolean(lua_State* L, int idx, bool v_default)
{
    bool v = v_default;

    if (LUA_TBOOLEAN == lua_type(L, idx))
    {
        v = lua_toboolean(L, idx);
    }

    return v;
}

lua_Integer klua_check_option_integer(lua_State* L, int idx, lua_Integer v_default)
{
    lua_Integer v = v_default;

    if (LUA_TNUMBER == lua_type(L, idx) && lua_isinteger(L, idx))
    {
        v = lua_tointeger(L, idx);
    }

    return v;
}

lua_Number klua_check_option_number(lua_State* L, int idx, lua_Number v_default)
{
    lua_Number v = v_default;

    if (LUA_TNUMBER == lua_type(L, idx) && !lua_isinteger(L, idx))
    {
        v = lua_tonumber(L, idx);
    }

    return v;
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

void klua_setfield_number(lua_State* L, const char* p_key, lua_Number num)
{
    assert(NULL != p_key);
    lua_pushnumber(L, num);
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
    int reg = 0;
    lua_pushvalue(L, arg);

    if (klua_is_coroutine(L))
    {
        lua_xmove(L, G(L)->mainthread, 1);    /* move function from L to mainthread */
    }

    reg = luaL_ref(G(L)->mainthread, LUA_REGISTRYINDEX);
    assert(0 < reg);

    return reg;
}

void klua_unref_registryindex(lua_State* L, int reg)
{
    if (0 < reg)
    {
        luaL_unref(G(L)->mainthread, LUA_REGISTRYINDEX, reg);
    }
}

bool klua_is_coroutine(lua_State* L)
{
    return (G(L)->mainthread == L) ? false : true;
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

#if !defined(__KLB_NO_LPEG__)
// from ./klb/src_c/klua/lpeg-1.0.2/lptree.c
extern int luaopen_lpeg(lua_State *L);

int klua_open_lpeg(lua_State *L)
{
    return luaopen_lpeg(L);
}
#endif

// from ./klb/src_c/klua/luafilesystem-2.0/src/lfs.c
extern int luaopen_lfs(lua_State *L);

int klua_open_lfs(lua_State *L)
{
    return luaopen_lfs(L);
}


#if !defined(__KLB_NO_ZLIB__)
// from ./klb/src_c/klua/lua-zlib-1.2/lua_zlib.c
extern int luaopen_zlib(lua_State * const L);

int klua_open_zlib(lua_State *L)
{
    return luaopen_zlib(L);
}
#endif

#if !defined(__KLB_NO_SQLITE__)
// from ./klb/src_c/klua/lsqlite3/src/lsqlite3.c
extern int luaopen_lsqlite3(lua_State *L);

int klua_open_lsqlite3(lua_State* L)
{
    return luaopen_lsqlite3(L);
}
#endif

// from ./klb/src_c/klua/LuaXML_130610/LuaXML_lib.c
extern int luaopen_LuaXML_lib(lua_State *L);

int klua_open_LuaXML_lib(lua_State* L)
{
    return luaopen_LuaXML_lib(L);
}


//////////////////////////////////////////////////////////////////////////

int klua_loadlib_all(lua_State* L)
{
    // 这里加载所有klb内置包
    // 若需要裁剪, 可自行定义相关函数

    // 预加载基础包
    {
        klua_loadlib(L, klua_open_cjson, "cjson");
        klua_loadlib(L, klua_open_cjson_safe, "cjson.safe");

#if !defined(__KLB_NO_LPEG__)
        klua_loadlib(L, klua_open_lpeg, "lpeg");
#endif

        klua_loadlib(L, klua_open_lfs, "lfs");

#if !defined(__KLB_NO_ZLIB__)
        klua_loadlib(L, klua_open_zlib, "zlib");
#endif

#if !defined(__KLB_NO_SQLITE__)
        klua_loadlib(L, klua_open_lsqlite3, "lsqlite3");
#endif

        klua_loadlib(L, klua_open_LuaXML_lib, "LuaXML_lib");

        klua_loadlib(L, klua_open_kco, "kco");
        klua_loadlib(L, klua_open_kos, "kos");
        klua_loadlib(L, klua_open_ksys, "ksys");
        klua_loadlib(L, klua_open_kenv, "kenv");
        klua_loadlib(L, klua_open_krand, "krand");
        klua_loadlib(L, klua_open_ktime, "ktime");
        klua_loadlib(L, klua_open_kmcache, "kmcache");
        klua_loadlib(L, klua_open_klist, "klist");
        klua_loadlib(L, klua_open_kthread, "kthread");
        klua_loadlib(L, klua_open_kkpa, "kkpa");
        klua_loadlib(L, klua_open_klpc, "klpc");
        klua_loadlib(L, klua_open_kgui, "kgui");
        klua_loadlib(L, klua_open_kwnd, "kwnd");
        klua_loadlib(L, klua_open_kurl, "kurl");
        klua_loadlib(L, klua_open_ktcp, "ktcp");
        klua_loadlib(L, klua_open_kudp, "kudp");
        klua_loadlib(L, klua_open_khttp, "khttp"); 
        klua_loadlib(L, klua_open_kws, "kws");
        klua_loadlib(L, klua_open_kmnp, "kmnp");
        klua_loadlib(L, klua_open_kncm, "kncm");
        klua_loadlib(L, klua_open_krtsp, "krtsp");
        klua_loadlib(L, klua_open_khttp_flv, "khttp_flv");
        klua_loadlib(L, klua_open_khttp_mnp, "khttp_mnp");
        klua_loadlib(L, klua_open_kws_flv, "kws_flv");
        klua_loadlib(L, klua_open_kws_mnp, "kws_mnp");
        klua_loadlib(L, klua_open_krpc, "krpc");
        klua_loadlib(L, klua_open_kh26x, "kh26x");
    }

    // 预加载扩展包
    {
#if !defined(__KLB_NO_PACKAGES__)
        klua_loadlib(L, klua_open_kpa_mgui, "kpa_mgui");
        klua_loadlib(L, klua_open_kpa_http, "kpa_http");
        klua_loadlib(L, klua_open_kpa_ws, "kpa_ws");
        klua_loadlib(L, klua_open_kpa_mnp, "kpa_mnp");
        klua_loadlib(L, klua_open_kpa_rtsp, "kpa_rtsp");
        klua_loadlib(L, klua_open_kpa_flv, "kpa_flv");
        klua_loadlib(L, klua_open_kpa_sip, "kpa_sip");
#endif
    }

    return 0;
}
