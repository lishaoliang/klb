// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_coroutine.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_time.h"
#include "klbutil/klb_rand.h"
#include "lstate.h"


typedef struct klua_ex_coroutine_timeout_t_
{
    int64_t     tick_count;
    int64_t     wait_tc;
    lua_State*  p_co;
}klua_ex_coroutine_timeout_t;


typedef struct klua_ex_coroutine_t_
{
    klua_env_t*     p_env;              ///< 环境

    klb_hlist_t*    p_co_hlist;         ///< 所有协程

    klb_nlist_t*    p_wakeup_list;      ///< 待立即唤醒列表
    klb_nlist_t*    p_timeout_list;     ///< 计时唤醒列表: klua_ex_coroutine_timeout_t*
}klua_ex_coroutine_t;


//////////////////////////////////////////////////////////////////////////

//  查找 co env
static klua_coroutine_env_t* klua_ex_coroutine_find_co_env(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    // 使用 额外添加的 lua_State.uname[16] 字段, 作为查找 key
    klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co->uname, KLUA_EX_STATE_NAME_LEN);

    return p_co_env;
}

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

    //while (0 < klb_nlist_size(p_ex->p_timeout_list))
    //{
    //    klua_ex_coroutine_timeout_t* p_co_timeout = klb_nlist_pop_head(p_ex->p_timeout_list);
    //    KLB_FREE(p_co_timeout);
    //}

    KLB_FREE_BY(p_ex->p_timeout_list, klb_nlist_destroy);
    KLB_FREE_BY(p_ex->p_wakeup_list, klb_nlist_destroy);
    KLB_FREE_BY(p_ex->p_co_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int on_exit_klua_ex_coroutine(klua_ex_coroutine_t* p_ex, klua_env_t* p_env)
{
    // 当被通知, 需要退出时, 中断所有当前处于柱塞下的协程    
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_ex->p_co_hlist);
    while (NULL != p_iter)
    {
        // Fixed Bug. [2025] 退出唤醒后, 协程可能在 cb 函数调用后, 被释放掉;
        // 这里先取下一个协程
        klb_hlist_iter_t* p_cur = p_iter;
        p_iter = klb_hlist_next(p_cur);

        klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_data(p_cur);

        if (p_co_env && p_co_env->cb_wakeup)
        {
            klua_coroutine_yield_cb cb = p_co_env->cb_wakeup;
            void* ptr = p_co_env->ptr;

            cb(ptr, p_ex, p_co_env->p_co, KLUA_ENV_EX_quit);
        }
    }

    return 0;
}


/// @brief 对扩展直接控制设置
/// @param [in] *ptr            扩展的指针
/// @param [in] *p_env          Lua环境
/// @param [in] opt             控制消息
/// @param [in] *p_param_in_out 控制参数
/// @param [in] param_size      参数大小
/// @return int 0
static int klua_ex_coroutine_ctrl(void* ptr, klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)ptr;

    switch (opt)
    {
    case KLUA_ENV_EX_quit:
        on_exit_klua_ex_coroutine(p_ex, p_env);
        break;
    default:
        break;
    }


    return 0;
}

static int klua_ex_coroutine_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)ptr;

    // wakeup list
    while (0 < klb_nlist_size(p_ex->p_wakeup_list))
    {
        lua_State* p_co = (lua_State*)klb_nlist_pop_head(p_ex->p_wakeup_list);
        klua_coroutine_env_t* p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co);// (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));

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
            klua_coroutine_env_t* p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co_timeout->p_co); // (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co_timeout->p_co, sizeof(lua_State*));

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

int klua_ex_coroutine_exit(klua_env_t* p_env, int64_t now)
{
    // 环境退出时, 将所有协程退出

    klua_ex_coroutine_t* p_ex = klua_ex_get_coroutine(p_env);

    while (0 < klb_nlist_size(p_ex->p_wakeup_list) ||
        0 < klb_nlist_size(p_ex->p_timeout_list) )
    {
        klua_ex_coroutine_loop_once(p_ex, p_env, now, now);

        now += 1000000000;
    }

    return 0;
}

int klua_ex_coroutine_push(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env)
{
    char uname[16] = { 0 };
    while (true)
    {
        // 随机名称
        klb_rand_string(uname, KLUA_EX_STATE_NAME_LEN, true);

        // 放入
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_co_hlist, uname, KLUA_EX_STATE_NAME_LEN, p_co_env);
        if (NULL != p_iter)
        {
            // 修改名称
            strncpy(p_co_env->p_co->uname, uname, sizeof(p_co_env->p_co->uname));
            p_co_env->p_co->uname[sizeof(p_co_env->p_co->uname) - 1] = '\0';

            break;
        }
    }

    return 0;
}

int klua_ex_coroutine_remove(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env)
{
    klb_hlist_remove_bykey(p_ex->p_co_hlist, p_co_env->p_co->uname, KLUA_EX_STATE_NAME_LEN);
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
    klua_coroutine_env_t* p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co); // (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));

    if (NULL != p_co_env)
    {
        lua_rawgeti(p_co_env->p_main, LUA_REGISTRYINDEX, p_co_env->co_reg); /* to call 'co_reg' in protected mode */

        // 清除中断回调地址函数
        p_co_env->cb_wakeup = NULL;
        p_co_env->ptr = NULL;

        return p_co_env->p_main;
    }

    return NULL;
}

int klua_ex_coroutine_yield(klua_ex_coroutine_t* p_ex, lua_State* p_co, klua_coroutine_yield_cb cb, void* ptr)
{
    klua_coroutine_env_t* p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co); // (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));

    if (NULL != p_co_env)
    {
        assert(p_co_env->p_co == p_co);

        // 记录中断回调地址函数
        p_co_env->cb_wakeup = cb;
        p_co_env->ptr = ptr;
    }

    return lua_yield(p_co, lua_gettop(p_co));
}

int klua_ex_coroutine_debug_check(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klua_coroutine_env_t* p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co); // (klua_coroutine_env_t*)klb_hlist_find(p_ex->p_co_hlist, p_co, sizeof(lua_State*));
    if (NULL != p_co_env)
    {
        assert(p_co_env->p_co == p_co);
        if (p_co_env->p_co == p_co)
        {
            return 0;
        }
    }

    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_ex->p_co_hlist);
    while (NULL != p_iter)
    {
        klua_coroutine_env_t* p_tmp = klb_hlist_data(p_iter);
        if (p_tmp->p_co == p_co)
        {
            klua_ex_coroutine_find_co_env(p_ex, p_co);

            assert(false);
            return 2;
        }

        p_iter = klb_hlist_next(p_iter);
    }

    assert(false);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_coroutine(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_coroutine_create;
    ex.cb_destroy = klua_ex_coroutine_destroy;
    ex.cb_ctrl = klua_ex_coroutine_ctrl;
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

int klua_coroutine_yield(klua_ex_coroutine_t* p_ex, lua_State* p_co, klua_coroutine_yield_cb cb, void* ptr)
{
    return klua_ex_coroutine_yield(p_ex, p_co, cb, ptr);
}

int klua_coroutine_debug_check(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    return klua_ex_coroutine_debug_check(p_ex, p_co);
}

//end

