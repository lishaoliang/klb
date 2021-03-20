#include "klua/extension/klua_ex_time.h"
#include "klua/klua.h"
#include "mem/klb_mem.h"
#include "list/klb_list.h"
#include "hash/klb_hlist.h"
#include "log/klb_log.h"
#include <assert.h>


#define KLUA_EX_TIME_REG_RUN        0
#define KLUA_EX_TIME_REG_DIE        1


typedef struct klua_ex_time_reg_t_
{
    int     status;         ///< 运行状态: KLUA_EX_TIME_REG_RUN / KLUA_EX_TIME_REG_DIE
    int     reg;            ///< Lua脚本函数引用
    int64_t tc;             ///< 注册时间
    int64_t interval;       ///< 等待间隔
}klua_ex_time_reg_t;


typedef struct klua_ex_time_t_
{
    klua_env_t*     p_env;              ///< Lua环境

    klb_list_t*     p_timer_list;       ///< 单次运行, 使用go语言命名习惯
    klb_hlist_t*    p_ticker_hlist;     ///< 长期运行ticker列表
}klua_ex_time_t;


static void* klua_ex_time_create(klua_env_t* p_env)
{
    klua_ex_time_t* p_time = KLB_MALLOC(klua_ex_time_t, 1, 0);
    KLB_MEMSET(p_time, 0, sizeof(klua_ex_time_t));

    p_time->p_env = p_env;

    p_time->p_timer_list = klb_list_create();
    p_time->p_ticker_hlist = klb_hlist_create(0);

    return p_time;
}

static void free_klua_ex_time_reg(klua_ex_time_reg_t* p_once, klua_env_t* p_env)
{
    assert(NULL != p_once);
    klua_unref_registryindex(klua_env_get_L(p_env), p_once->reg);
    KLB_FREE(p_once);
}

static void klua_ex_time_destroy(void* ptr)
{
    klua_ex_time_t* p_time = (klua_ex_time_t*)ptr;
    klua_env_t* p_env = p_time->p_env;

    // 清空 timer list
    while (0 < klb_list_size(p_time->p_timer_list))
    {
        klua_ex_time_reg_t* p_once = (klua_ex_time_reg_t*)klb_list_pop_head(p_time->p_timer_list);
        free_klua_ex_time_reg(p_once, p_env);
    }

    // 清空 ticker hlist
    while (0 < klb_hlist_size(p_time->p_ticker_hlist))
    {
        klua_ex_time_reg_t* p_ticker = (klua_ex_time_reg_t*)klb_hlist_pop_head(p_time->p_ticker_hlist);
        free_klua_ex_time_reg(p_ticker, p_env);
    }

    KLB_FREE_BY(p_time->p_timer_list, klb_list_destroy);
    KLB_FREE_BY(p_time->p_ticker_hlist, klb_hlist_destroy);
    KLB_FREE(p_time);
}

static int call_klua_ex_time_reg(klua_env_t* p_env, int reg, int64_t now)
{
    assert(NULL != p_env);
    if (reg <= 0) return EXIT_FAILURE;

    lua_State* L = klua_env_get_L(p_env);
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, reg);         /* to call 'function' in protected mode */
    lua_pushinteger(L, now);                        /* 1st argument */
    int status = lua_pcall(L, 1, 0, 0);             /* do the call */
    klua_env_report(p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int klua_ex_time_loop_once_timer(klua_ex_time_t* p_time, klua_env_t* p_env, int64_t tc, int64_t now)
{
    // timer once
    klb_list_iter_t* p_iter = klb_list_begin(p_time->p_timer_list);
    while (NULL != p_iter)
    {
        klb_list_iter_t* p_cur = p_iter;
        p_iter = klb_list_next(p_iter);

        klua_ex_time_reg_t* p_once = (klua_ex_time_reg_t*)klb_list_data(p_cur);

        if (p_once->interval <= ABS_SUB(now, p_once->tc))
        {
            // 时间到了, 调用脚本
            call_klua_ex_time_reg(p_env, p_once->reg, now);

            klb_list_remove(p_time->p_timer_list, p_cur);   // 移除
            free_klua_ex_time_reg(p_once, p_env);           // 销毁
        }
    }

    return 0;
}

static int klua_ex_time_loop_once_ticker(klua_ex_time_t* p_time, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    // ticker
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_time->p_ticker_hlist);
    while (NULL != p_iter)
    {
        klb_hlist_iter_t* p_cur = p_iter;
        p_iter = klb_hlist_next(p_iter);

        klua_ex_time_reg_t* p_ticker = (klua_ex_time_reg_t*)klb_hlist_data(p_cur);

        if (KLUA_EX_TIME_REG_RUN == p_ticker->status && p_ticker->interval <= ABS_SUB(now, p_ticker->tc))
        {
            p_ticker->tc = now;

            // 时间到了, 调用脚本
            // 调用脚本后, 本ticker可能被关闭
            call_klua_ex_time_reg(p_env, p_ticker->reg, now);
        }

        if (KLUA_EX_TIME_REG_RUN != p_ticker->status)
        {
            klb_hlist_remove(p_time->p_ticker_hlist, p_cur);    // 移除
            free_klua_ex_time_reg(p_ticker, p_env);             // 销毁
        }
    }

    return 0;
}

static int klua_ex_time_loop_once(void* ptr, klua_env_t* p_env, int64_t tc, int64_t now)
{
    klua_ex_time_t* p_time = (klua_ex_time_t*)ptr;
    assert(NULL != p_time);
    assert(p_env == p_time->p_env);

    // timer once
    klua_ex_time_loop_once_timer(p_time, p_env, tc, now);

    // ticker
    klua_ex_time_loop_once_ticker(p_time, p_env, tc, now);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_time_new_timer_once(klua_ex_time_t* p_time, lua_State* L, lua_Integer wait, int index)
{
    int reg = klua_ref_registryindex(L, index);
    if (reg <= 0)
    {
        assert(false);
        return 1;
    }

    klua_ex_time_reg_t* p_once = KLB_MALLOC(klua_ex_time_reg_t, 1, 0);
    KLB_MEMSET(p_once, 0, sizeof(klua_ex_time_reg_t));

    p_once->status = KLUA_EX_TIME_REG_RUN;
    p_once->tc = klua_env_get_tc(p_time->p_env);
    p_once->interval = wait;
    p_once->reg = reg;

    klb_list_push_tail(p_time->p_timer_list, p_once);

    return 0;
}

int klua_ex_time_new_ticker(klua_ex_time_t* p_time, lua_State* L, const char* p_name, size_t name_len, lua_Integer interval, int index)
{
    int reg = klua_ref_registryindex(L, index);
    if (reg <= 0)
    {
        assert(false);
        return 1;
    }

    klua_ex_time_reg_t* p_ticker = KLB_MALLOC(klua_ex_time_reg_t, 1, 0);
    KLB_MEMSET(p_ticker, 0, sizeof(klua_ex_time_reg_t));

    p_ticker->status = KLUA_EX_TIME_REG_RUN;
    p_ticker->tc = klua_env_get_tc(p_time->p_env);
    p_ticker->interval = interval;
    p_ticker->reg = reg;

    if (NULL == klb_hlist_push_tail(p_time->p_ticker_hlist, p_name, name_len, p_ticker))
    {
        free_klua_ex_time_reg(p_ticker, p_time->p_env);

        assert(false);
        return 1;
    }

    return 0;
}

int klua_ex_time_stop_ticker(klua_ex_time_t* p_time, const char* p_name, size_t name_len)
{
    klua_ex_time_reg_t* p_ticker = (klua_ex_time_reg_t*)klb_hlist_find(p_time->p_ticker_hlist, p_name, name_len);
    if (NULL == p_ticker)
    {
        return 1;
    }

    p_ticker->status = KLUA_EX_TIME_REG_DIE;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_time(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_time_create;
    ex.cb_destroy = klua_ex_time_destroy;
    ex.cb_loop_once = klua_ex_time_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_TIME_NAME, &ex);

    return 0;
}

klua_ex_time_t* klua_ex_get_time(klua_env_t* p_env)
{
    klua_ex_time_t* p_ex = (klua_ex_time_t*)klua_env_get_extension(p_env, KLUA_EX_TIME_NAME);
    return p_ex;
}
