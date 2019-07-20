///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua_env.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "klua/klua_env.h"
#include "klua/klua.h"
#include "klua_help.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include <assert.h>


#define KLUA_ENV_STR        "_KLUA_ENV_"


typedef struct klua_env_t
{
    struct
    {
        lua_State*      L;              ///< Lua环境

        int             G;              ///< "G"
        int             kin;            ///< "kin"
        int             kgo;            ///< "kgo"
        int             kexit;          ///< "kexit"
    };
}klua_env_t;


static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load, char* p_main);
static int klua_pquit(lua_State *L);


klua_env_t* klua_env_create(lua_CFunction cb_pre_load, char* p_main)
{
    klua_env_t* p_env = KLB_MALLOC(klua_env_t, 1, 0);
    KLB_MEMSET(p_env, 0, sizeof(klua_env_t));

    p_env->L = luaL_newstate(); // Lua运行环境

    // lua环境初始化
    klua_env_init(p_env, cb_pre_load, p_main);

    //KLB_LOG("*** klua_env_create:%p\n", p_env);
    return p_env;
}

void klua_env_destroy(klua_env_t* p_env)
{
    assert(NULL != p_env);
    //KLB_LOG("*** klua_env_destroy:%p\n", p_env);

    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pquit);      /* to call 'klua_pquit' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    int status = lua_pcall(L, 1, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLB_FREE_BY(p_env->L, lua_close);
    KLB_FREE(p_env);
}

static int klua_env_dofile(klua_env_t* p_env, char* p_name)
{
    assert(NULL != p_env);
    assert(NULL != p_name);

    return klua_help_dofile(p_env->L, p_name);
}

static int klua_env_dolibrary(klua_env_t* p_env, char* p_name)
{
    assert(NULL != p_env);
    assert(NULL != p_name);

    return klua_help_dolibrary(p_env->L, p_name);
}

klua_env_t* klua_env_get_by_L(lua_State* L)
{
    assert(NULL != L);
    lua_getglobal(L, KLUA_ENV_STR);

    if (LUA_TLIGHTUSERDATA == lua_type(L, -1))
    {
        return (klua_env_t*)lua_touserdata(L, -1);
    }

    assert(false);
    return NULL;
}

lua_State* klua_env_get_L(klua_env_t* p_env)
{
    assert(NULL != p_env);
    return p_env->L;
}

static int klua_env_call_kin(klua_env_t* p_env)
{
    assert(NULL != p_env);
    if (p_env->kin <= 0) return EXIT_FAILURE;

    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_env->kin);  /* to call 'kin' in protected mode */
    int status = lua_pcall(L, 0, 1, 0);             /* do the call */
    int result = lua_toboolean(L, -1);              /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int klua_env_call_kgo(klua_env_t* p_env)
{
    assert(NULL != p_env);
    if (p_env->kgo <= 0) return EXIT_FAILURE;

    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_env->kgo);  /* to call 'kgo' in protected mode */
    int status = lua_pcall(L, 0, 1, 0);             /* do the call */
    int result = lua_toboolean(L, -1);              /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int klua_env_call_kexit(klua_env_t* p_env)
{
    assert(NULL != p_env);
    if (p_env->kexit <= 0) return EXIT_FAILURE;

    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_env->kexit);/* to call 'kexit' in protected mode */
    int status = lua_pcall(L, 0, 1, 0);             /* do the call */
    int result = lua_toboolean(L, -1);              /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

static void klua_pref(klua_env_t* p_env, lua_State* L)
{
    assert(NULL != p_env);
    assert(NULL != L);

    int id = lua_getglobal(L, "G");
    if (0 < id) { p_env->G = luaL_ref(L, LUA_REGISTRYINDEX); }

    id = lua_getglobal(L, "kin");
    if (0 < id) { p_env->kin = luaL_ref(L, LUA_REGISTRYINDEX); }

    id = lua_getglobal(L, "kgo");
    if (0 < id) { p_env->kgo = luaL_ref(L, LUA_REGISTRYINDEX); }

    id = lua_getglobal(L, "kexit");
    if (0 < id) { p_env->kexit = luaL_ref(L, LUA_REGISTRYINDEX); }
}

static void klua_punref(klua_env_t* p_env, lua_State* L)
{
    assert(NULL != p_env);
    assert(NULL != L);

    if (0 < p_env->kexit)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->kexit);
        p_env->kexit = 0;
    }

    if (0 < p_env->kgo)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->kgo);
        p_env->kgo = 0;
    }

    if (0 < p_env->kin)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->kin);
        p_env->kin = 0;
    }

    if (0 < p_env->G)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->G);
        p_env->G = 0;
    }
}

static int klua_pmain(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    lua_CFunction cb_pre_load = (lua_CFunction)lua_touserdata(L, 2);
    char* p_main = (char*)lua_tostring(L, 3);

    // 将环境指针保存到全局, 如果有更好的方法则可换掉
    lua_pushlightuserdata(L, p_env);
    lua_setglobal(L, KLUA_ENV_STR);

    // 加载标准库
    luaL_openlibs(L);

    // 加载"k*"系列额外库
    klua_pre_loadlib(L, klua_pre_open_kos, "kos");
    klua_pre_loadlib(L, klua_pre_open_ktime, "ktime");
    klua_pre_loadlib(L, klua_pre_open_kthread, "kthread");


    // 加载自定义额外库
    if (cb_pre_load) { cb_pre_load(L); }

    // 如果定义了main入口, 则执行main
    if (NULL != p_main)
    {
        if (LUA_OK == klua_env_dofile(p_env, p_main))
        {
            klua_pref(p_env, L);        // 保存地址
            klua_env_call_kin(p_env);   // 调用"kin"
        }
        else
        {
            KLB_LOG_E("klua_env dofile error!path:%s\n", p_main);
        }
    }

    return 0;
}

static int klua_pquit(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);

    // exit
    klua_env_call_kexit(p_env);

    // unref
    klua_punref(p_env, L);

    return 0;
}

static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load, char* p_main)
{
    assert(NULL != p_env);
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pmain);      /* to call 'klua_pmain' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushlightuserdata(L, (void*)cb_pre_load);  /* 2st argument */
    lua_pushstring(L, p_main);              /* 3st argument */
    int status = lua_pcall(L, 3, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    //lua_close(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
