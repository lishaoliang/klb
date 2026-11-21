// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
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

    // 清空协程表: 错误退出路径可能残留未 remove 的协程; 数据由 Lua 侧协程生命周期管理
    while (0 < klb_hlist_size(p_ex->p_co_hlist))
    {
        klb_hlist_pop_head(p_ex->p_co_hlist);
    }

    // 清空计时唤醒表: 元素为本模块分配, 需释放
    while (0 < klb_nlist_size(p_ex->p_timeout_list))
    {
        klua_ex_coroutine_timeout_t* p_co_timeout = (klua_ex_coroutine_timeout_t*)klb_nlist_pop_head(p_ex->p_timeout_list);
        KLB_FREE(p_co_timeout);
    }

    // 清空唤醒表: 元素为 lua_State* (非本模块所有), 只移除
    while (0 < klb_nlist_size(p_ex->p_wakeup_list))
    {
        klb_nlist_pop_head(p_ex->p_wakeup_list);
    }

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

int klua_ex_coroutine_count(klua_ex_coroutine_t* p_ex)
{
    if (NULL == p_ex)
    {
        return 0;
    }

    return klb_hlist_size(p_ex->p_co_hlist);
}

static void mark_done_handle_klua_ex_coroutine(klua_coroutine_env_t* p_co_env)
{
    klua_coroutine_handle_t* p_h = NULL;

    if (NULL == p_co_env->p_handle)
    {
        return;
    }

    // dead == 1; 与 klua_kcoro.c COS_DEAD 一致
    p_h = (klua_coroutine_handle_t*)p_co_env->p_handle;
    p_h->done = true;
    p_h->stat = 1;
    p_h->p_co_env = NULL;
    p_co_env->p_handle = NULL;
}


static void force_free_env_klua_ex_coroutine(klua_ex_coroutine_t* p_ex, klua_coroutine_env_t* p_co_env)
{
    lua_State* p_co = NULL;
    lua_State* p_main = NULL;

    if (NULL == p_co_env)
    {
        return;
    }

    p_co = p_co_env->p_co;
    p_main = p_co_env->p_main;

    // 摘本协程残留调度挂接 (hlist 可能已 detach)
    if (NULL != p_co)
    {
        klua_ex_coroutine_close_one(p_ex, p_co);
        lua_resetthread(p_co);
    }

    mark_done_handle_klua_ex_coroutine(p_co_env);

    if (NULL != p_main && 0 < p_co_env->co_reg)
    {
        klua_unref_registryindex(p_main, p_co_env->co_reg);
        p_co_env->co_reg = 0;
    }

    KLB_FREE(p_co_env);
}


int klua_ex_coroutine_close_all(klua_ex_coroutine_t* p_ex)
{
    if (NULL == p_ex)
    {
        return 0;
    }

    // step1. 强制唤醒处于 yield 的协程 (同 env quit)
    on_exit_klua_ex_coroutine(p_ex, p_ex->p_env);

    // step2. 丢弃计时唤醒 (关闭=中止, 不再到期触发)
    while (0 < klb_nlist_size(p_ex->p_timeout_list))
    {
        klua_ex_coroutine_timeout_t* p_co_timeout = (klua_ex_coroutine_timeout_t*)klb_nlist_pop_head(p_ex->p_timeout_list);
        KLB_FREE(p_co_timeout);
    }

    // step3. 排空立即唤醒队列 (quit 回调可能已入队; 部分协程可正常结束并 remove)
    {
        int64_t now = klua_env_get_tick_count(p_ex->p_env);

        while (0 < klb_nlist_size(p_ex->p_wakeup_list))
        {
            klua_ex_coroutine_loop_once(p_ex, p_ex->p_env, now, now);
            now += 1000000000;
        }
    }

    // step4. 强制收尸残留 (纯 co_sleep 等无 cb_wakeup 的僵尸; 使 count 可归零)
    while (0 < klb_hlist_size(p_ex->p_co_hlist))
    {
        klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_pop_head(p_ex->p_co_hlist);

        force_free_env_klua_ex_coroutine(p_ex, p_co_env);
    }

    return 0;
}

int klua_ex_coroutine_close_one(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klua_coroutine_env_t* p_co_env = NULL;

    if (NULL == p_ex || NULL == p_co)
    {
        return 0;
    }

    p_co_env = klua_ex_coroutine_find_co_env(p_ex, p_co);

    // step1. quit 回调 (网络等 yield 点清理); 随后不再经 wakeup 恢复本协程
    if (NULL != p_co_env && NULL != p_co_env->cb_wakeup)
    {
        klua_coroutine_yield_cb cb = p_co_env->cb_wakeup;
        void* ptr = p_co_env->ptr;

        p_co_env->cb_wakeup = NULL;
        p_co_env->ptr = NULL;
        cb(ptr, p_ex, p_co, KLUA_ENV_EX_quit);
    }

    // step2. 摘超时队列
    {
        klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ex->p_timeout_list);

        while (NULL != p_iter)
        {
            klb_nlist_iter_t* p_next = klb_nlist_next(p_iter);
            klua_ex_coroutine_timeout_t* p_co_timeout = (klua_ex_coroutine_timeout_t*)klb_nlist_data(p_iter);

            if (p_co == p_co_timeout->p_co)
            {
                klb_nlist_remove(p_ex->p_timeout_list, p_iter);
                KLB_FREE(p_co_timeout);
            }

            p_iter = p_next;
        }
    }

    // step3. 摘立即唤醒队列 (含 quit 回调可能入队的项)
    {
        klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ex->p_wakeup_list);

        while (NULL != p_iter)
        {
            klb_nlist_iter_t* p_next = klb_nlist_next(p_iter);
            lua_State* p_cur = (lua_State*)klb_nlist_data(p_iter);

            if (p_co == p_cur)
            {
                klb_nlist_remove(p_ex->p_wakeup_list, p_iter);
            }

            p_iter = p_next;
        }
    }

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

static bool co_in_timeout_list_klua_ex(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ex->p_timeout_list);

    while (NULL != p_iter)
    {
        klua_ex_coroutine_timeout_t* p_co_timeout = (klua_ex_coroutine_timeout_t*)klb_nlist_data(p_iter);

        if (p_co == p_co_timeout->p_co)
        {
            return true;
        }

        p_iter = klb_nlist_next(p_iter);
    }

    return false;
}


static bool co_in_wakeup_list_klua_ex(klua_ex_coroutine_t* p_ex, lua_State* p_co)
{
    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ex->p_wakeup_list);

    while (NULL != p_iter)
    {
        lua_State* p_cur = (lua_State*)klb_nlist_data(p_iter);

        if (p_co == p_cur)
        {
            return true;
        }

        p_iter = klb_nlist_next(p_iter);
    }

    return false;
}


static const char* status_name_klua_ex(lua_State* L, lua_State* p_co)
{
    if (L == p_co)
    {
        return "running";
    }

    switch (lua_status(p_co))
    {
    case LUA_YIELD:
        return "suspended";
    case LUA_OK:
    {
        lua_Debug ar;

        if (lua_getstack(p_co, 0, &ar))
        {
            return "normal";
        }
        else if (0 == lua_gettop(p_co))
        {
            return "dead";
        }
        else
        {
            return "suspended";
        }
    }
    default:
        return "dead";
    }
}


int klua_ex_coroutine_snapshot(klua_ex_coroutine_t* p_ex, lua_State* L)
{
    klb_hlist_iter_t* p_iter = NULL;
    int idx = 1;
    int n = 0;

    lua_newtable(L); /* result */

    lua_newtable(L); /* list */

    if (NULL != p_ex)
    {
        p_iter = klb_hlist_begin(p_ex->p_co_hlist);

        while (NULL != p_iter)
        {
            klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)klb_hlist_data(p_iter);
            lua_State* p_co = p_co_env->p_co;

            lua_newtable(L); /* item */

            lua_pushstring(L, (NULL != p_co) ? p_co->uname : "");
            lua_setfield(L, -2, "id");

            lua_pushstring(L, (NULL != p_co) ? status_name_klua_ex(L, p_co) : "dead");
            lua_setfield(L, -2, "status");

            lua_pushboolean(L, (NULL != p_co) ? co_in_timeout_list_klua_ex(p_ex, p_co) : false);
            lua_setfield(L, -2, "in_timeout");

            lua_pushboolean(L, (NULL != p_co) ? co_in_wakeup_list_klua_ex(p_ex, p_co) : false);
            lua_setfield(L, -2, "in_wakeup");

            lua_pushboolean(L, NULL != p_co_env->cb_wakeup);
            lua_setfield(L, -2, "has_cb");

            lua_rawseti(L, -2, idx);
            idx++;
            n++;

            p_iter = klb_hlist_next(p_iter);
        }
    }

    lua_setfield(L, -2, "list");

    lua_pushinteger(L, n);
    lua_setfield(L, -2, "count");

    return 1;
}


int klua_ex_register_coroutine(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_coroutine_create;
    ex.cb_destroy = klua_ex_coroutine_destroy;
    ex.cb_ctrl = klua_ex_coroutine_ctrl;
    ex.cb_loop_once = klua_ex_coroutine_loop_once;

    klua_env_register_extension(p_env, KLUAEX_coroutine, &ex);

    return 0;
}

klua_ex_coroutine_t* klua_ex_get_coroutine(klua_env_t* p_env)
{
    klua_ex_coroutine_t* p_ex = (klua_ex_coroutine_t*)klua_env_get_extension(p_env, KLUAEX_coroutine);
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

int klua_coroutine_count(klua_ex_coroutine_t* p_ex)
{
    return klua_ex_coroutine_count(p_ex);
}

int klua_coroutine_close_all(klua_ex_coroutine_t* p_ex)
{
    return klua_ex_coroutine_close_all(p_ex);
}

int klua_coroutine_snapshot(klua_ex_coroutine_t* p_ex, lua_State* L)
{
    return klua_ex_coroutine_snapshot(p_ex, L);
}

//end

