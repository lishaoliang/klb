#include "klua/extension/klua_ex_lpc.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_EX_LPC_NAME      "_KLUA_EX_LPC_"


typedef struct klua_ex_lpc_task_t_
{
    sds     func_name;
}klua_ex_lpc_task_t;


typedef struct klua_ex_lpc_function_t_
{
    int     reg_on_callback;
}klua_ex_lpc_function_t;


typedef struct klua_ex_lpc_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    lua_State*          L;              ///< L

    klb_hlist_t*        p_func_hlist;   ///< 注册的函数列表: klua_ex_lpc_function_t*

    klb_hlist_t*        p_request_list; ///< 请求列表: klua_ex_lpc_task_t*
    klb_hlist_t*        p_response_list;///< 响应列表: klua_ex_lpc_task_t*
}klua_ex_lpc_t;


//////////////////////////////////////////////////////////////////////////
static int klua_ex_lpc_loop_once_request(klua_ex_lpc_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now);
static int klua_ex_lpc_loop_once_response(klua_ex_lpc_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now);

static void* klua_ex_lpc_create(klua_env_t* p_env)
{
    klua_ex_lpc_t* p_ex = KLB_MALLOC(klua_ex_lpc_t, 1, 0);
    KLB_MEMSET(p_ex, 0, sizeof(klua_ex_lpc_t));

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    p_ex->p_func_hlist = klb_hlist_create(0);
    p_ex->p_request_list = klb_hlist_create(0);
    p_ex->p_response_list = klb_hlist_create(0);

    return p_ex;
}

static void klua_ex_lpc_destroy(void* ptr)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    // 清空
    while (0 < klb_hlist_size(p_ex->p_func_hlist))
    {
        klua_ex_lpc_function_t* p_func = (klua_ex_lpc_function_t*)klb_hlist_pop_head(p_ex->p_func_hlist);
        klua_unref_registryindex(p_ex->L, p_func->reg_on_callback);

        KLB_FREE(p_func);
    }

    KLB_FREE_BY(p_ex->p_response_list, klb_hlist_destroy);
    KLB_FREE_BY(p_ex->p_request_list, klb_hlist_destroy);
    KLB_FREE_BY(p_ex->p_func_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_lpc_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;

    // 处理请求
    klua_ex_lpc_loop_once_request(p_ex, p_env, last_tc, now);

    // 处理响应
    klua_ex_lpc_loop_once_response(p_ex, p_env, last_tc, now);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
static void free_klua_ex_lpc_task(klua_ex_lpc_task_t* p_task)
{
    KLB_FREE_BY(p_task->func_name, sdsfree);
    KLB_FREE(p_task);
}

static int call_klua_ex_lpc_function(klua_ex_lpc_t* p_ex, klua_ex_lpc_function_t* p_func, klua_ex_lpc_task_t* p_task)
{
    assert(NULL != p_ex);
    if (p_func->reg_on_callback <= 0) return EXIT_FAILURE;

    lua_State* L = p_ex->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_func->reg_on_callback); /* to call reg_on_callback in protected mode */
    lua_pushstring(L, "111");                                   /* 1st argument */
    lua_pushstring(L, "222");                                   /* 2st argument */
    lua_pushstring(L, "333");                                   /* 3st argument */
    int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
    klua_env_report(p_ex->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int klua_ex_lpc_loop_once_request(klua_ex_lpc_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    while (0 < klb_hlist_size(p_ex->p_request_list))
    {
        klua_ex_lpc_task_t* p_task = (klua_ex_lpc_task_t*)klb_hlist_pop_head(p_ex->p_request_list);

        klua_ex_lpc_function_t* p_func = klb_hlist_find(p_ex->p_func_hlist, p_task->func_name, sdslen(p_task->func_name));
        if (NULL != p_func)
        {
            call_klua_ex_lpc_function(p_ex, p_func, p_task);
        }

        KLB_FREE_BY(p_task, free_klua_ex_lpc_task);
    }

    return 0;
}

static int klua_ex_lpc_loop_once_response(klua_ex_lpc_t* p_ex, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    return 0;
}

void klua_ex_lpc_register_function(klua_ex_lpc_t* p_ex, const char* p_func_name, size_t name_len, int func_idx)
{
    klua_ex_lpc_function_t* p_func = (klua_ex_lpc_function_t*)klb_hlist_find(p_ex->p_func_hlist, p_func_name, name_len);
    if (NULL != p_func)
    {
        // 释放原来的函数
        klua_unref_registryindex(p_ex->L, p_func->reg_on_callback);

        // 引用新函数
        p_func->reg_on_callback = klua_ref_registryindex(p_ex->L, func_idx);
    }
    else
    {
        // 新建
        klua_ex_lpc_function_t* p_new_func = KLB_MALLOC(klua_ex_lpc_function_t, 1, 0);
        KLB_MEMSET(p_new_func, 0, sizeof(klua_ex_lpc_function_t));

        // 引用
        p_new_func->reg_on_callback = klua_ref_registryindex(p_ex->L, func_idx);

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_func_hlist, p_func_name, name_len, p_new_func);
        assert(NULL != p_iter);
    }
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_lpc(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_lpc_create;
    ex.cb_destroy = klua_ex_lpc_destroy;
    ex.cb_loop_once = klua_ex_lpc_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_LPC_NAME, &ex);

    return 0;
}

klua_ex_lpc_t* klua_ex_get_lpc(klua_env_t* p_env)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)klua_env_get_extension(p_env, KLUA_EX_LPC_NAME);
    return p_ex;
}
