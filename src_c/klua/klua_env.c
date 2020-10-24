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


#define KLUA_ENV_PTR        "_KLUA_ENV_PTR_"


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

    struct
    {
        void*           p_udata;        ///< user data
    };
}klua_env_t;


static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load);
static int klua_pquit(lua_State *L);
static void klua_pref(klua_env_t* p_env, lua_State* L);
static int klua_env_call_kin(klua_env_t* p_env);


klua_env_t* klua_env_create(lua_CFunction cb_pre_load)
{
    klua_env_t* p_env = KLB_MALLOC(klua_env_t, 1, 0);
    KLB_MEMSET(p_env, 0, sizeof(klua_env_t));

    p_env->L = luaL_newstate(); // Lua运行环境

    // lua环境初始化
    klua_env_init(p_env, cb_pre_load);

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

void klua_env_set_udata(klua_env_t* p_env, void* p_udata)
{
    assert(NULL != p_env);
    p_env->p_udata = p_udata;
}

void* klua_env_get_udata(klua_env_t* p_env)
{
    assert(NULL != p_env);

    return p_env->p_udata;
}

static int klua_pdofile(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dofile(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        klua_env_call_kin(p_env);   // 调用"kin"
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
    }

    return 0;
}

static int klua_pdolibrary(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dolibrary(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        klua_env_call_kin(p_env);   // 调用"kin"
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
    }

    return 0;
}

int klua_env_dofile(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdofile);    /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int klua_env_dolibrary(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdolibrary); /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

klua_env_t* klua_env_get_by_L(lua_State* L)
{
    assert(NULL != L);
    lua_getglobal(L, KLUA_ENV_PTR);

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

int klua_env_has_kgo(klua_env_t* p_env)
{
    assert(NULL != p_env);

    if (0 < p_env->kgo)
    {
        return 0;
    }

    return 1;
}

int klua_env_kgo(klua_env_t* p_env)
{
    assert(NULL != p_env);

    return p_env->kgo;
}

int klua_env_call_kgo(klua_env_t* p_env, const char* p_msg, const char* p_msgex, const char* p_lparam, const char* p_wparam, void* ptr)
{
    assert(NULL != p_env);
    if (p_env->kgo <= 0) return EXIT_FAILURE;

    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_env->kgo);  /* to call 'kgo' in protected mode */
    lua_pushstring(L, p_msg);                       /* 1st argument */
    lua_pushstring(L, p_msgex);                     /* 2st argument */
    lua_pushstring(L, p_lparam);                    /* 3st argument */
    lua_pushstring(L, p_wparam);                    /* 4st argument */
    lua_pushlightuserdata(L, ptr);                  /* 5st argument */
    int status = lua_pcall(L, 5, 1, 0);             /* do the call */
    int result = lua_toboolean(L, -1);              /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int klua_env_report(klua_env_t* p_env, int status)
{
    assert(NULL != p_env);
    return klua_help_report(p_env->L, status);
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

    // 将环境指针保存到全局, 如果有更好的方法则可换掉
    lua_pushlightuserdata(L, p_env);
    lua_setglobal(L, KLUA_ENV_PTR);

    // 加载标准库
    luaL_openlibs(L);

    // 加载"k*"系列额外库
    //klua_loadlib(L, klua_open_kos, "kos");
    //klua_loadlib(L, klua_open_ktime, "ktime");
    //klua_loadlib(L, klua_open_kthread, "kthread");

    // 加载自定义库
    if (cb_pre_load) { cb_pre_load(L); }

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

static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load)
{
    assert(NULL != p_env);
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pmain);      /* to call 'klua_pmain' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushlightuserdata(L, (void*)cb_pre_load);  /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    //lua_close(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}
