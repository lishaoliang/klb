// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_coroutine.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_time.h"



typedef struct klua_ex_coroutine_timeout_t_
{
    int64_t     tick_count;
    int64_t     wait_tc;
    lua_State*  p_co;
}klua_ex_coroutine_timeout_t;


typedef struct klua_ex_coroutine_t_
{
    klua_env_t*     p_env;

    klb_hlist_t*    p_co_hlist;
    klb_nlist_t*     p_wakeup_list;
    klb_nlist_t*     p_timeout_list;
}klua_ex_coroutine_t;


//////////////////////////////////////////////////////////////////////////


static int klua_ex_coroutine_call_auxwrap(lua_State* L, int reg)
{
    if (reg <= 0) return EXIT_FAILURE;
    lua_rawgeti(L, LUA_REGISTRYINDEX, reg);     /* to call 'co_reg' in protected mode */

    int status = lua_pcall(L, 0, 0, 0);         /* do the call */
    klua_env_report_by_L(L, status);

    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

static void* klua_ex_coroutine_create(klua_env_t* p_env)
{
    klua_ex_coroutine_t* p_ex = KLB_MALLOCZ(klua_ex_coroutine_t, 1, 0);
    p_ex->p_env = p_env;

    p_ex->p_co_hlist = klb_hlist_create(0);
    p_ex->p_wakeup_list = klb_nlist_create();
    p_ex->p_timeout_list = klb_nlist_create();

    return p_ex;
}

static void klua_ex_coroutine_destroy(void* ptr)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    KLB_FREE_BY(p_ex->p_timeout_list, klb_nlist_destroy);
    KLB_FREE_BY(p_ex->p_wakeup_list, klb_nlist_destroy);
    KLB_FREE_BY(p_ex->p_co_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_coroutine_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)ptr;

    // wakeup list
    while (0 < klb_nlist_size(p_ex->p_wakeup_list))
    {
        lua_State* p_co = (lua_State*)klb_nlist_pop_head(p_ex->p_wakeup_list);
        klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));

        if (NULL != p_co_env)
        {
            klua_ex_coroutine_call_auxwrap(p_co_env->p_main, p_co_env->co_reg);
        }
    }

    // timeout list
    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ex->p_timeout_list);
    while (NULL != p_iter)
    {
        klb_nlist_iter_t* p_next = klb_nlist_next(p_iter);

        // 
        klua_ex_coroutine_timeout_t* p_co_timeout = (klua_ex_coroutine_timeout_t*)klb_nlist_data(p_iter);

        if (p_co_timeout->tick_count + p_co_timeout->wait_tc <= now)
        {
            // 时间到
            klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co_timeout->p_co, sizeof(lua_State*));

            if (NULL != p_co_env)
            {
                klua_ex_coroutine_call_auxwrap(p_co_env->p_main, p_co_env->co_reg);
            }

            klb_nlist_remove(p_ex->p_timeout_list, p_iter);
            KLB_FREE(p_co_timeout);
        }

        p_iter = p_next;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_coroutine_push(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env)
{
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_co_hlist, p_co_env->p_co, sizeof(lua_State*), p_co_env);
    assert(NULL != p_iter);

    return 0;
}

int klua_ex_coroutine_remove(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env)
{
    klb_hlist_remove_bykey(p_ex->p_co_hlist, p_co_env->p_co, sizeof(lua_State*));
    return 0;
}

int klua_ex_coroutine_wakeup(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klb_nlist_push_tail(p_ex->p_wakeup_list, p_co);

    return 0;
}

int klua_ex_coroutine_wakeup_timeout(klua_ex_coroutine_t* p_ex, lua_State* p_co, int64_t tc)
{
    assert(0 < tc);

    klua_ex_coroutine_timeout_t* p_co_timeout = KLB_MALLOCZ(klua_ex_coroutine_timeout_t, 1, 0);
    p_co_timeout->tick_count = klua_env_get_tick_count(p_ex->p_env);
    p_co_timeout->wait_tc = tc;
    p_co_timeout->p_co = p_co;

    klb_nlist_push_tail(p_ex->p_timeout_list, p_co_timeout);

    return 0;
}

lua_State* klua_ex_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));

    if (NULL != p_co_env)
    {
        lua_rawgeti(p_co_env->p_main, LUA_REGISTRYINDEX, p_co_env->co_reg); /* to call 'co_reg' in protected mode */
        return p_co_env->p_main;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_coroutine(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_coroutine_create;
    ex.cb_destroy = klua_ex_coroutine_destroy;
    ex.cb_loop_once = klua_ex_coroutine_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_COROUTINE_NAME, &ex);

    return 0;
}

klua_ex_coroutine_t* klua_ex_get_coroutine(klua_env_t* p_env)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)klua_env_get_extension(p_env, KLUA_EX_COROUTINE_NAME);
    return p_ex;
}

klua_ex_coroutine_t* klua_ex_get_coroutine_by_L(lua_State* L)
{
    return klua_ex_get_coroutine(klua_env_get_by_L(L));
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

klua_ex_coroutine_t* klua_coroutine_get(klua_env_t* p_env)
{
    return klua_ex_get_coroutine(p_env);
}

klua_ex_coroutine_t* klua_coroutine_get_by_L(lua_State* L)
{
    return klua_ex_get_coroutine(klua_env_get_by_L(L));
}

lua_State* klua_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    return klua_ex_coroutine_rawgeti(p_ex, p_co);
}
