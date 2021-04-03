#include "klua/klua.h"
#include "klbutil/klb_rand.h"
#include "klbmem/klb_mem.h"
#include <stdlib.h>
#include <assert.h>


/// @def   KLUA_KRAND_STACK_BUF
/// @brief 使用栈的最大尺寸
#define KLUA_KRAND_STACK_BUF            1024


/// @def   KLUA_KRAND_LEN
/// @brief 默认长度
#define KLUA_KRAND_LEN                  8


static int klua_krand_rand(lua_State* L)
{
    if (lua_isinteger(L, 1))
    {
        int r_max = (int)lua_tointeger(L, 1);
        if (RAND_MAX < r_max)
        {
            r_max = RAND_MAX;
        }

        if (0 < r_max)
        {
            lua_pushinteger(L, rand() % r_max);
            lua_pushinteger(L, RAND_MAX);
        }
        else
        {
            lua_pushinteger(L, 0);
            lua_pushinteger(L, 0);
        }
    }
    else
    {
        lua_pushinteger(L, rand());
        lua_pushinteger(L, RAND_MAX);
    }

    return 2;
}


static int klua_krand_rand_string(lua_State* L)
{
    size_t len = KLUA_KRAND_LEN;
    if (lua_isinteger(L, 1))
    {
        len = (size_t)lua_tointeger(L, 1);
    }

    char buf[KLUA_KRAND_STACK_BUF] = { 0 };
    char* p_buf = NULL;
    char* p_str = buf;

    if (KLUA_KRAND_STACK_BUF < len)
    {
        p_buf = KLB_MALLOC(char, len, 0);
        p_str = p_buf;
    }

    klb_rand_string(p_str, len, false);
    lua_pushlstring(L, p_str, len);

    KLB_FREE(p_buf);
    return 1;
}

static int klua_krand_rand_integer(lua_State* L)
{
    size_t len = KLUA_KRAND_LEN;
    if (lua_isinteger(L, 1))
    {
        len = (size_t)lua_tointeger(L, 1);
    }

    char buf[KLUA_KRAND_STACK_BUF] = { 0 };
    char* p_buf = NULL;
    char* p_str = buf;

    if (KLUA_KRAND_STACK_BUF < len)
    {
        p_buf = KLB_MALLOC(char, len, 0);
        p_str = p_buf;
    }

    klb_rand_integer(p_str, len, false);
    lua_pushlstring(L, p_str, len);

    KLB_FREE(p_buf);
    return 1;
}

int klua_open_krand(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "rand",           klua_krand_rand },

        { "rand_string",    klua_krand_rand_string },
        { "rand_integer",   klua_krand_rand_integer },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
