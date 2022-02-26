#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbbase/klb_package.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
// 写

#define KLUA_KKPA_W_HANDLE     "KLUA_KKPA_W_HANDLE*"

typedef struct klua_kkpa_w_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L
}klua_kkpa_w_t;

static klua_kkpa_w_t* new_klua_kkpa_w(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = (klua_kkpa_w_t*)lua_newuserdata(L, sizeof(klua_kkpa_w_t));
    KLB_MEMSET(p_kkpa, 0, sizeof(klua_kkpa_w_t));
    luaL_setmetatable(L, KLUA_KKPA_W_HANDLE);
    return p_kkpa;
}

static klua_kkpa_w_t* to_klua_kkpa_w(lua_State* L, int index)
{
    klua_kkpa_w_t* p_kkpa = (klua_kkpa_w_t*)luaL_checkudata(L, index, KLUA_KKPA_W_HANDLE);
    luaL_argcheck(L, NULL != p_kkpa, index, "'kkpa_w' expected");
    return p_kkpa;
}

static int klua_kkpa_w_open(lua_State* L)
{
    return 0;
}

static int klua_kkpa_w_close(lua_State* L)
{
    return 0;
}

static int klua_kkpa_w_write(lua_State* L)
{
    return 0;
}

static int klua_kkpa_w_gc(lua_State* L)
{
    return 0;
}

static int klua_kkpa_w_tostring(lua_State* L)
{
    return 1;
}

static void klua_kkpa_w_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kkpa_w_close },
        { "write",          klua_kkpa_w_write },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kkpa_w_gc },
        { "__close",         klua_kkpa_w_gc },
        { "__tostring",      klua_kkpa_w_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KKPA_W_HANDLE);   /* metatable for KLUA_KKPA_W_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);              /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                  /* create method table */
    luaL_setfuncs(L, meth, 0);                  /* add file methods to method table */
    lua_setfield(L, -2, "__index");             /* metatable.__index = method table */
    lua_pop(L, 1);                              /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////
// 读


#define KLUA_KKPA_R_HANDLE     "KLUA_KKPA_R_HANDLE*"

typedef struct klua_kkpa_r_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L
}klua_kkpa_r_t;


static klua_kkpa_r_t* new_klua_kkpa_r(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = (klua_kkpa_r_t*)lua_newuserdata(L, sizeof(klua_kkpa_r_t));
    KLB_MEMSET(p_kkpa, 0, sizeof(klua_kkpa_r_t));
    luaL_setmetatable(L, KLUA_KKPA_R_HANDLE);
    return p_kkpa;
}

static klua_kkpa_r_t* to_klua_kkpa_r(lua_State* L, int index)
{
    klua_kkpa_r_t* p_kkpa = (klua_kkpa_r_t*)luaL_checkudata(L, index, KLUA_KKPA_R_HANDLE);
    luaL_argcheck(L, NULL != p_kkpa, index, "'kkpa_r' expected");
    return p_kkpa;
}


static int klua_kkpa_r_open(lua_State* L)
{
    return 0;
}

static int klua_kkpa_r_close(lua_State* L)
{
    return 0;
}

static int klua_kkpa_r_read(lua_State* L)
{
    return 0;
}

static int klua_kkpa_r_gc(lua_State* L)
{
    return 0;
}

static int klua_kkpa_r_tostring(lua_State* L)
{
    return 1;
}

static void klua_kkpa_r_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kkpa_r_close },
        { "read",           klua_kkpa_r_read },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kkpa_r_gc },
        { "__close",         klua_kkpa_r_gc },
        { "__tostring",      klua_kkpa_r_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KKPA_R_HANDLE);   /* metatable for KLUA_KKPA_R_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);              /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                  /* create method table */
    luaL_setfuncs(L, meth, 0);                  /* add file methods to method table */
    lua_setfield(L, -2, "__index");             /* metatable.__index = method table */
    lua_pop(L, 1);                              /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


static int lib_klua_kkpa_open(lua_State* L)
{
    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_kkpa(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "open",               lib_klua_kkpa_open },

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KKPA_W_HANDLE
    klua_kkpa_w_createmeta(L);

    // KLUA_KKPA_R_HANDLE
    klua_kkpa_r_createmeta(L);

    return 1;
}
