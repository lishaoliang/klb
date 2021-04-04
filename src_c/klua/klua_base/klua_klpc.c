#include "klb_type.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klua/extension/klua_ex_lpc.h"
#include <assert.h>


#define KLUA_KLPC_HANDLE     "KLUA_KNCM_HANDLE*"


typedef struct klua_klpc_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L

    klua_ex_lpc_t*          p_ex;           ///< LPC扩展

    int                     reg_on_callback;///< 注册的应答函数
}klua_klpc_t;

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

static klua_klpc_t* new_klua_klpc(lua_State* L)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)lua_newuserdata(L, sizeof(klua_klpc_t));
    KLB_MEMSET(p_klpc, 0, sizeof(klua_klpc_t));
    luaL_setmetatable(L, KLUA_KLPC_HANDLE);
    return p_klpc;
}

static klua_klpc_t* to_klua_klpc(lua_State* L, int index)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)luaL_checkudata(L, index, KLUA_KLPC_HANDLE);
    luaL_argcheck(L, NULL != p_klpc, index, "'klpc' expected");
    return p_klpc;
}

static void free_klua_klpc(klua_klpc_t* p_klpc)
{

}

//////////////////////////////////////////////////////////////////////////


/// @brief 调用(其他线程)函数
static int lib_klua_klpc_call(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    luaL_checktype(L, 2, LUA_TFUNCTION);                ///< @2. 函数
    const char* p_name = luaL_checkstring(L, 3);        ///< @3. 线程名称
    const char* p_func_name = luaL_checkstring(L, 4);   ///< @4. 被调用的远程函数名称

    assert(p_klpc->reg_on_callback <= 0);
    p_klpc->reg_on_callback = klua_ref_registryindex(L, 2);

    //

    ///< #1. true.成功; false.失败
    return 0;
}

/// @brief gc
static int lib_klua_klpc_gc(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);
    free_klua_klpc(p_klpc);

    return 0;
}

static int lib_klua_klpc_tostring(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    lua_pushfstring(L, "klpc:%p", p_klpc);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

static void klua_klpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "call",           lib_klua_klpc_call },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            lib_klua_klpc_gc},
        {"__close",         lib_klua_klpc_gc},
        {"__tostring",      lib_klua_klpc_tostring},
        {NULL,              NULL}
    };

    luaL_newmetatable(L, KLUA_KLPC_HANDLE); /* metatable for KLUA_KLPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


/// @brief 注册被(其他线程)调用函数
static int lib_klua_klpc_register(lua_State* L)
{
    size_t name_len = 0;
    const char* p_func_name = luaL_checklstring(L, 1, &name_len);   ///< @1. 函数名称
    luaL_checktype(L, 2, LUA_TFUNCTION);                            ///< @2. 函数

    klua_env_t* p_env = klua_env_get_by_L(L);
    klua_ex_lpc_t* p_ex = klua_ex_get_lpc(p_env);

    klua_ex_lpc_register_function(p_ex, p_func_name, name_len, 2);

    return 0;
}

/// @brief 新建一个LPC
static int lib_klua_klpc_new(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    klua_ex_lpc_t* p_ex = klua_ex_get_lpc(p_env);

    klua_klpc_t* p_klpc = new_klua_klpc(L);
    p_klpc->p_env = p_env;
    p_klpc->L = L;
    p_klpc->p_ex = p_ex;

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_klpc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "register",           lib_klua_klpc_register },

        { "new",                lib_klua_klpc_new },

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KLPC_HANDLE
    klua_klpc_createmeta(L);

    return 1;
}
