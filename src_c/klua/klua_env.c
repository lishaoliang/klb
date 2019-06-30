///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_env.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "klua/klua_env.h"
#include "klua_help.h"
#include "mem/klb_mem.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <assert.h>


#define KLUA_ENV_STR        "_KLUA_ENV_"


typedef struct klua_env_t
{
    struct
    {
        lua_State*  L;              ///< Lua环境
    };
}klua_env_t;


static int klua_env_init(klua_env_t* p_env);
static int klua_env_quit(klua_env_t* p_env);


klua_env_t* klua_env_create()
{
    klua_env_t* p_env = KLB_MALLOC(klua_env_t, 1, 0);
    KLB_MEMSET(p_env, 0, sizeof(klua_env_t));

    p_env->L = luaL_newstate(); // Lua运行环境

    // lua环境初始化
    klua_env_init(p_env);

    return p_env;
}

void klua_env_destroy(klua_env_t* p_env)
{
    assert(NULL != p_env);

    klua_env_quit(p_env);

    KLB_FREE_BY(p_env->L, lua_close);
    KLB_FREE(p_env);
}

//////////////////////////////////////////////////////////////////////////

static int lua_pmain(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);

    // 将环境指针保存起来
    lua_pushlightuserdata(L, p_env);
    lua_setglobal(L, KLUA_ENV_STR);

    // 加载标准库
    luaL_openlibs(L);


    return 0;
}

static int klua_env_init(klua_env_t* p_env)
{
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &lua_pmain);       /* to call 'pmain' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    int status = lua_pcall(L, 1, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    //lua_close(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int klua_env_quit(klua_env_t* p_env)
{
    return 0;
}
