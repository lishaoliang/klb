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
#include "lstate.h"
#include "klua/klua.h"
#include "klua_help.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbplatform/klb_time.h"
#include "klbutil/klb_hlist.h"
#include "klbthird/sds.h"
#include "klua/extension/klua_extension.h"
#include <assert.h>


#if 0
#define KLUA_ENV_PTR            "_KLUA_ENV_PTR_"
#endif

#define KLUA_ENV_GC_TIME_OUT    600000


/// @struct klua_env_extension_activate_t
/// @brief  激活的扩展
typedef struct klua_env_extension_activate_t_
{
    void*                ptr;           ///< 扩展的指针
    sds                  name;          ///< 扩展的名称
    klua_env_extension_t ex;            ///< 扩展的接口函数
}klua_env_extension_activate_t;


/// @struct klua_env_t
/// @brief  Lua环境
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
        klb_hlist_t* p_extension_hlist;             ///< 注册的扩展; klua_env_extension_t*
        klb_hlist_t* p_extension_activate_hlist;    ///< 激活的扩展; klua_env_extension_activate_t*
    };

    struct
    {
        bool            is_exit;        ///< 是否退出
    };

    struct
    {
        int64_t         tc;             ///< 当前时间

        int64_t         gc_tc;          ///< 上次gc时间
        int64_t         gc_interval;    ///< gc间隔
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

    p_env->is_exit = false;
    p_env->p_extension_hlist = klb_hlist_create(0);
    p_env->p_extension_activate_hlist = klb_hlist_create(0);

    p_env->L = luaL_newstate(); // Lua运行环境
    p_env->L->udata = p_env;

    p_env->tc = klb_tick_counti64();

    p_env->gc_interval = KLUA_ENV_GC_TIME_OUT;
    p_env->gc_tc = p_env->tc;

    // 注册标准扩展
    klua_register_extension_std(p_env);

    // lua环境初始化
    klua_env_init(p_env, cb_pre_load);

    return p_env;
}

void klua_env_destroy(klua_env_t* p_env)
{
    assert(NULL != p_env);

    // 销毁注册的扩展
    while (0 < klb_hlist_size(p_env->p_extension_hlist))
    {
        klua_env_extension_t* p_extension = (klua_env_extension_t*)klb_hlist_pop_head(p_env->p_extension_hlist);
        KLB_FREE(p_extension);
    }

    KLB_FREE_BY(p_env->L, lua_close);
    KLB_FREE_BY(p_env->p_extension_activate_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_env->p_extension_hlist, klb_hlist_destroy);
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

int64_t klua_env_get_tc(klua_env_t* p_env)
{
    assert(NULL != p_env);
    return p_env->tc;
}

static int klua_pdofile(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dofile(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        klua_env_call_kin(p_env);   // 调用"kin"

        lua_pushboolean(L, true);
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
        lua_pushboolean(L, false);
    }

    return 1;
}

static int klua_pdolibrary(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dolibrary(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        klua_env_call_kin(p_env);   // 调用"kin"

        lua_pushboolean(L, true);
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
        lua_pushboolean(L, false);
    }

    return 1;
}

int klua_env_dofile(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);

    // 更新时间
    p_env->tc = klb_tick_counti64();
    p_env->gc_tc = p_env->tc;

    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdofile);    /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    int ret = (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    if (EXIT_SUCCESS != ret)
    {
        p_env->is_exit = true;
    }

    return ret;
}

int klua_env_dolibrary(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);

    // 更新时间
    p_env->tc = klb_tick_counti64();
    p_env->gc_tc = p_env->tc;

    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdolibrary); /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    int ret = (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    if (EXIT_SUCCESS != ret)
    {
        p_env->is_exit = true;
    }

    return ret;
}

int klua_env_doend(klua_env_t* p_env)
{
    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_pushcfunction(L, &klua_pquit);      /* to call 'klua_pquit' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    int status = lua_pcall(L, 1, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

klua_env_t* klua_env_get_by_L(lua_State* L)
{
    assert(NULL != L);

    return (klua_env_t*)L->udata;

#if 0
    lua_getglobal(L, KLUA_ENV_PTR);

    if (LUA_TLIGHTUSERDATA == lua_type(L, -1))
    {
        return (klua_env_t*)lua_touserdata(L, -1);
    }

    assert(false);
    return NULL;
#endif
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

static int klua_exit(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_env->is_exit = true;

    return 0;
}

static int klua_pmain(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    lua_CFunction cb_pre_load = (lua_CFunction)lua_touserdata(L, 2);

#if 0
    // 将环境指针保存到全局, 如果有更好的方法则可换掉
    lua_pushlightuserdata(L, p_env);
    lua_setglobal(L, KLUA_ENV_PTR);
#endif

    // 加载标准库
    luaL_openlibs(L);

    // 退出函数
    lua_register(L, "exit", klua_exit);

    // 加载"k*"系列额外库
    //klua_loadlib(L, klua_open_kos, "kos");
    //klua_loadlib(L, klua_open_ktime, "ktime");
    //klua_loadlib(L, klua_open_kthread, "kthread");

    // 加载自定义库
    if (cb_pre_load) { cb_pre_load(L); }

    lua_pushboolean(L, true);
    return 1;
}

static int klua_pquit(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);

    // exit
    klua_env_call_kexit(p_env);

    // 销毁激活的 extension
    while (0 < klb_hlist_size(p_env->p_extension_activate_hlist))
    {
        klua_env_extension_activate_t* p_tmp = (klua_env_extension_activate_t*)klb_hlist_pop_head(p_env->p_extension_activate_hlist);

        KLB_FREE_BY(p_tmp->ptr, p_tmp->ex.cb_destroy);
        KLB_FREE_BY(p_tmp->name, sdsfree);
        KLB_FREE(p_tmp);
    }

    // unref
    klua_punref(p_env, L);

    p_env->is_exit = true;
    lua_pushboolean(L, true);
    return 1;
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

int klua_env_register_extension(klua_env_t* p_env, const char* p_name, const klua_env_extension_t* p_extension)
{
    assert(NULL != p_env);
    assert(NULL != p_name);
    assert(NULL != p_extension);
    assert(NULL != p_extension->cb_create);
    assert(NULL != p_extension->cb_destroy);

    size_t name_len = strlen(p_name);

    klua_env_extension_t* p_tmp = KLB_MALLOC(klua_env_extension_t, 1, 0);
    KLB_MEMSET(p_tmp, 0, sizeof(klua_env_extension_t));

    memcpy(p_tmp, p_extension, sizeof(klua_env_extension_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_env->p_extension_hlist, p_name, name_len, p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG_E("register lua env extension error!name:[%s]\n", p_name);
        return 1; // 放入失败, 名称重复
    }

    return 0;
}

void* klua_env_get_extension(klua_env_t* p_env, const char* p_name)
{
    assert(NULL != p_env);
    assert(NULL != p_name);

    size_t name_len = strlen(p_name);

    klua_env_extension_activate_t* p_activate = (klua_env_extension_activate_t*)klb_hlist_find(p_env->p_extension_activate_hlist, p_name, name_len);
    if (NULL != p_activate)
    {
        assert(NULL != p_activate->ptr);
        return p_activate->ptr;
    }

    klua_env_extension_t* p_extension = (klua_env_extension_t*)klb_hlist_find(p_env->p_extension_hlist, p_name, name_len);
    if (NULL != p_extension)
    {
        klua_env_extension_activate_t* p_tmp = KLB_MALLOC(klua_env_extension_activate_t, 1, 0);
        KLB_MEMSET(p_tmp, 0, sizeof(klua_env_extension_activate_t));

        memcpy(&p_tmp->ex, p_extension, sizeof(klua_env_extension_t));
        p_tmp->name = sdsnewlen(p_name, name_len);

        p_tmp->ptr = p_tmp->ex.cb_create(p_env);
        assert(NULL != p_tmp->ptr);

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_env->p_extension_activate_hlist, p_name, name_len, p_tmp);
        assert(NULL != p_iter);

        return p_tmp->ptr;
    }

    return NULL;
}

int klua_env_loop_once(klua_env_t* p_env)
{
    assert(NULL != p_env);
    int64_t now = klb_tick_counti64();

    //KLB_LOG("klua_env_loop_once now=%lld\n", now);

    // loop once extension
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_env->p_extension_activate_hlist);
    while (NULL != p_iter)
    {
        klua_env_extension_activate_t* p_activate = (klua_env_extension_activate_t*)klb_hlist_data(p_iter);

        if (NULL != p_activate->ex.cb_loop_once)
        {
            p_activate->ex.cb_loop_once(p_activate->ptr, p_env, p_env->tc, now);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    // gc
    if (p_env->gc_interval <= ABS_SUB(now, p_env->gc_tc))
    {
        p_env->gc_tc = now;

        int pre_used_kb = lua_gc(p_env->L, LUA_GCCOUNT, 0);     // 获取lua使用的内存
        lua_gc(p_env->L, LUA_GCCOLLECT, 0);
        int after_used_kb = lua_gc(p_env->L, LUA_GCCOUNT, 0);   // 获取gc之后,lua使用的内存

        //KLB_LOG("klua_env_loop_once gc:[%dKB]\n", pre_used_kb - after_used_kb);
    }

    p_env->tc = now;

    return 0;
}

bool klua_env_is_exit(klua_env_t* p_env)
{
    return p_env->is_exit;
}

