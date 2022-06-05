// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbbase/klb_package.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
// 写

#define KLUA_KKPA_W_HANDLE     "KLUA_KKPA_W_HANDLE*"


/// @struct klua_kkpa_r_t
/// @brief  写
typedef struct klua_kkpa_w_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L

    klb_package_w_t*        p_package;      ///< 写包
}klua_kkpa_w_t;


static klua_kkpa_w_t* new_klua_kkpa_w(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = (klua_kkpa_w_t*)lua_newuserdata(L, sizeof(klua_kkpa_w_t));
    KLB_MEMSET(p_kkpa, 0, sizeof(klua_kkpa_w_t));
    luaL_setmetatable(L, KLUA_KKPA_W_HANDLE);

    p_kkpa->L = L;
    p_kkpa->p_env = klua_env_get_by_L(L);

    return p_kkpa;
}

static klua_kkpa_w_t* to_klua_kkpa_w(lua_State* L, int index)
{
    klua_kkpa_w_t* p_kkpa = (klua_kkpa_w_t*)luaL_checkudata(L, index, KLUA_KKPA_W_HANDLE);
    luaL_argcheck(L, NULL != p_kkpa, index, "'kkpa_w' expected");
    return p_kkpa;
}

static void close_klua_kkpa_w(klua_kkpa_w_t* p_kkpa)
{
    KLB_FREE_BY(p_kkpa->p_package, klb_package_w_close);
}

static int klua_kkpa_w_open(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);

    klb_package_w_t* p_package = klb_package_w_open(p_path);
    if (NULL != p_package)
    {
        klua_kkpa_w_t* p_kkpa = new_klua_kkpa_w(L);
        p_kkpa->p_package = p_package;
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int klua_kkpa_w_close(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = to_klua_kkpa_w(L, 1);
    close_klua_kkpa_w(p_kkpa);
    return 0;
}

static int klua_kkpa_w_write(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = to_klua_kkpa_w(L, 1);

    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 2, &key_len);

    size_t value_len = 0;
    const char* p_value = luaL_checklstring(L, 3, &value_len);

    int ret = 1;
    if (NULL != p_kkpa->p_package)
    {
        ret = klb_package_w_write(p_kkpa->p_package, p_key, key_len, p_value, value_len);
    }

    lua_pushboolean(L, (0 == ret) ? 1 : 0);
    return 1;
}

static int klua_kkpa_w_write_file(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = to_klua_kkpa_w(L, 1);

    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 2, &key_len);
    const char* p_filepath = luaL_checkstring(L, 3);

    int ret = 1;
    if (NULL != p_kkpa->p_package)
    {
        ret = klb_package_w_write_file(p_kkpa->p_package, p_key, key_len, p_filepath);
    }

    lua_pushboolean(L, (0 == ret) ? 1 : 0);
    return 1;
}

static int klua_kkpa_w_gc(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = to_klua_kkpa_w(L, 1);
    close_klua_kkpa_w(p_kkpa);
    return 0;
}

static int klua_kkpa_w_tostring(lua_State* L)
{
    klua_kkpa_w_t* p_kkpa = to_klua_kkpa_w(L, 1);
    lua_pushfstring(L, "kkpa_w:%p", p_kkpa);
    return 1;
}

static void klua_kkpa_w_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kkpa_w_close },
        { "write",          klua_kkpa_w_write },
        { "write_file",     klua_kkpa_w_write_file },

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


/// @struct klua_kkpa_r_t
/// @brief  读
typedef struct klua_kkpa_r_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L

    klb_package_r_t*        p_package;      ///< 读
}klua_kkpa_r_t;


static klua_kkpa_r_t* new_klua_kkpa_r(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = (klua_kkpa_r_t*)lua_newuserdata(L, sizeof(klua_kkpa_r_t));
    KLB_MEMSET(p_kkpa, 0, sizeof(klua_kkpa_r_t));
    luaL_setmetatable(L, KLUA_KKPA_R_HANDLE);

    p_kkpa->L = L;
    p_kkpa->p_env = klua_env_get_by_L(L);

    return p_kkpa;
}

static klua_kkpa_r_t* to_klua_kkpa_r(lua_State* L, int index)
{
    klua_kkpa_r_t* p_kkpa = (klua_kkpa_r_t*)luaL_checkudata(L, index, KLUA_KKPA_R_HANDLE);
    luaL_argcheck(L, NULL != p_kkpa, index, "'kkpa_r' expected");
    return p_kkpa;
}

static void close_klua_kkpa_r(klua_kkpa_r_t* p_kkpa)
{
    KLB_FREE_BY(p_kkpa->p_package, klb_package_r_close);
}

static int klua_kkpa_r_open(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);

    klb_package_r_t* p_package = klb_package_r_open(p_path);
    if (NULL != p_package)
    {
        klua_kkpa_r_t* p_kkpa = new_klua_kkpa_r(L);
        p_kkpa->p_package = p_package;
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int klua_kkpa_r_close(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = to_klua_kkpa_r(L, 1);
    close_klua_kkpa_r(p_kkpa);
    return 0;
}

static int klua_kkpa_r_size(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = to_klua_kkpa_r(L, 1);

    lua_pushinteger(L, klb_package_r_size(p_kkpa->p_package));
    return 1;
}

static int klua_kkpa_r_read(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = to_klua_kkpa_r(L, 1);
    int64_t idx = luaL_checkinteger(L, 2);

    char* p_key = NULL;
    klb_buf_t* p_value = NULL;

    if (0 == klb_package_r_read(p_kkpa->p_package, idx - 1, &p_key, &p_value))
    {
        lua_pushstring(L, p_key);

        if (NULL != p_value)
        {
            lua_pushlstring(L, p_value->p_buf + p_value->start, p_value->end - p_value->start);
        }
        else
        {
            lua_pushnil(L);
        }

        KLB_FREE(p_key);
        KLB_FREE(p_value);
    }
    else
    {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

static int klua_kkpa_r_gc(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = to_klua_kkpa_r(L, 1);
    close_klua_kkpa_r(p_kkpa);
    return 0;
}

static int klua_kkpa_r_tostring(lua_State* L)
{
    klua_kkpa_r_t* p_kkpa = to_klua_kkpa_r(L, 1);
    lua_pushfstring(L, "kkpa_r:%p", p_kkpa);
    return 1;
}

static void klua_kkpa_r_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kkpa_r_close },

        { "size",           klua_kkpa_r_size },
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
//


//////////////////////////////////////////////////////////////////////////

int klua_open_kkpa(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "open_w",             klua_kkpa_w_open },
        { "open_r",             klua_kkpa_r_open },

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
