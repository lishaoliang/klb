#include "lua_t_klb.h"
#include "mem/klb_mem.h"
#include "t_klb.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define LUA_T_KLB_ARGC_MAX      64

static char* g_t_klb_argv[LUA_T_KLB_ARGC_MAX] = { 0 };


/* 示例
static int lua_t_klb_list_main(lua_State* L)
{
    int idx = 0;
    int argc = lua_gettop(L);
    for (int i = 1; i <= argc; i++)
    {
        if (idx < LUA_T_KLB_ARGC_MAX && LUA_TSTRING == lua_type(L, i))
        {
            g_t_klb_argv[idx] = (char*)lua_tostring(L, i);
            idx += 1;
        }
    }

    char* p_print = NULL;
    int ret = t_klb_list_main(idx, g_t_klb_argv, &p_print);

    lua_pushstring(L, (NULL != p_print) ? p_print : "");    // out @1. string
    lua_pushinteger(L, ret);                                // out @2. integer

    KLB_FREE(p_print);
    return 2;
}
*/

#define LUA_T_KLB_STATIC_FUNC(FUNC_NAME_, FUNC_) \
static int FUNC_NAME_(lua_State* L) \
{ \
    int idx = 0; \
    int argc = lua_gettop(L); \
    for (int i = 1; i <= argc; i++) \
    { \
        if (idx < LUA_T_KLB_ARGC_MAX && LUA_TSTRING == lua_type(L, i)) \
        { \
            g_t_klb_argv[idx] = (char*)lua_tostring(L, i); \
            idx += 1; \
        } \
    } \
    char* p_print = NULL; \
    int ret = FUNC_(idx, g_t_klb_argv, &p_print); \
    lua_pushstring(L, (NULL != p_print) ? p_print : ""); \
    lua_pushinteger(L, ret); \
    KLB_FREE(p_print); \
    return 2; \
}

LUA_T_KLB_STATIC_FUNC(lua_t_klb_hlist_main,         t_klb_hlist_main)
LUA_T_KLB_STATIC_FUNC(lua_t_klb_htab_main,          t_klb_htab_main)
LUA_T_KLB_STATIC_FUNC(lua_t_klb_list_main,          t_klb_list_main)


int luaopen_t_klb(lua_State* L)
{
    static luaL_Reg t_klb_lib[] =
    {
        {"t_klb_hlist",     lua_t_klb_hlist_main },
        {"t_klb_htab",      lua_t_klb_htab_main },
        {"t_klb_list",      lua_t_klb_list_main },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, t_klb_lib);

    return 1;
}
