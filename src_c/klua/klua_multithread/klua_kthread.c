// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_thread.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_mutex.h"
#include "klbutil/klb_rand.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klbplatform/klb_thread.h"
#include "klbplatform/klb_atomic.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klbmem/klb_mem.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_KTHREAD_NAME_LEN   8
#define KLUA_KTHREAD_NAME_BUF   12


/// @struct klua_kthread_item_t
/// @brief  线程信息
typedef struct klua_kthread_item_t_
{
    int volatile    wait;               ///< true: 需等待; false: 不需等待

#define KLUA_KTHREAD_OWNER      0       ///< 本模块管理生命周期
#define KLUA_KTHREAD_OTHER      1       ///< 外部模块管理生命周期
    int             flag;               ///< 线程所属标记

    sds             entry_path;         ///< Lua脚本入口
    klua_env_t*     p_env;              ///< Lua环境

    klb_thread_t*   p_thread;           ///< 线程
}klua_kthread_item_t;


/// @struct klua_kthread_t
/// @brief  全局 线程,LPC,LPC模块等信息
typedef struct klua_kthread_t_
{
    klb_atomic_t volatile   module_lock;        ///< p_module_hlist锁
    klb_atomic_t volatile   lpc_lock;           ///< p_lpc_hlist锁

    klb_hlist_t*            p_module_hlist;     ///< 模块(供LPC通信)列表: klua_env_t*
    klb_hlist_t*            p_lpc_hlist;        ///< LPC列表: klua_env_t*

    klb_mutex_t*            p_thread_mutex;     ///< 线程锁
    klb_hlist_t*            p_thread_hlist;     ///< 所有线程: klua_kthread_item_t*

    klua_openlibs_cb        pre_openlibs;       ///< lua环境预加载库
}klua_kthread_t;


static klua_kthread_t* g_klua_kthread = NULL;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klua_kthread_item_destroy(klua_kthread_item_t* p_item);

//////////////////////////////////////////////////////////////////////////
// 初始化/退出

int klua_kthread_init()
{
    assert(NULL == g_klua_kthread);
    g_klua_kthread = KLB_MALLOCZ(klua_kthread_t, 1, 0);

    klb_atomic_set_zero(&g_klua_kthread->module_lock);
    klb_atomic_set_zero(&g_klua_kthread->lpc_lock);

    g_klua_kthread->p_thread_mutex = klb_mutex_create();
    g_klua_kthread->p_thread_hlist = klb_hlist_create(0);
    g_klua_kthread->p_module_hlist = klb_hlist_create(0);
    g_klua_kthread->p_lpc_hlist = klb_hlist_create(0);

    return 0;
}

void klua_kthread_quit()
{
    assert(NULL != g_klua_kthread);

    // 清空线程
    klb_mutex_lock(g_klua_kthread->p_thread_mutex);
    while (0 < klb_hlist_size(g_klua_kthread->p_thread_hlist))
    {
        // 先关闭线程
        klua_kthread_item_t* p_head = (klua_kthread_item_t*)klb_hlist_head(g_klua_kthread->p_thread_hlist);
        KLB_FREE_BY(p_head->p_thread, klb_thread_destroy);

        // 移除,并销毁
        klua_kthread_item_t* p_item = (klua_kthread_item_t*)klb_hlist_pop_head(g_klua_kthread->p_thread_hlist);
        KLB_FREE_BY(p_item, klua_kthread_item_destroy);
    }
    klb_mutex_unlock(g_klua_kthread->p_thread_mutex);

    // 销毁
    KLB_FREE_BY(g_klua_kthread->p_thread_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kthread->p_lpc_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kthread->p_module_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kthread->p_thread_mutex, klb_mutex_destroy);
    KLB_FREE(g_klua_kthread);
}

//////////////////////////////////////////////////////////////////////////
// klua_kthread_t 部分C函数

static int cb_thread_klua_kthread(void* p_obj, volatile int* p_run)
{
    klua_kthread_item_t* p_item = (klua_kthread_item_t*)p_obj;
    klua_env_t* p_env = p_item->p_env;

    if (0 != klua_env_dolibrary(p_env, p_item->entry_path))
    {
        p_item->wait = false;
        return 0;
    }

    // Lua线程 开启完成的标记为 klua_env_dolibrary() 执行完成
    // 即将 线程入口脚本 完全加载完成
    p_item->wait = false;

    while (*p_run && !klua_env_is_exit(p_env))
    {
        int sleep = klua_env_loop_once(p_env);

        if (0 < sleep)
        {
            klb_sleep(sleep);
        }
    }

    klua_env_exit(p_env); // 退出线程时, 将Lua环境置为退出状态
    klua_env_doend(p_env);

    return 0;
}

static klua_kthread_item_t* klua_kthread_item_create(klua_openlibs_cb cb, const char* p_name, size_t name_len, const char* p_entry, const char* p_arg, int arg_size)
{
    klua_kthread_item_t* p_item = KLB_MALLOCZ(klua_kthread_item_t, 1, 0);

    p_item->wait = true;
    p_item->flag = KLUA_KTHREAD_OWNER;
    p_item->entry_path = sdsnew(p_entry);

    p_item->p_env = klua_env_create(cb);
    klua_env_set_name(p_item->p_env, p_name, name_len);
    klua_env_set_args(p_item->p_env, p_arg, arg_size);

    return p_item;
}

static klua_kthread_item_t* klua_kthread_item_create_other(const char* p_name, size_t name_len, klua_env_t* p_env)
{
    klua_kthread_item_t* p_item = KLB_MALLOCZ(klua_kthread_item_t, 1, 0);

    p_item->wait = false;
    p_item->flag = KLUA_KTHREAD_OTHER;
    p_item->entry_path = sdsnew("");

    p_item->p_env = p_env;

    klua_env_set_name(p_item->p_env, p_name, name_len);

    return p_item;
}

static void klua_kthread_item_destroy(klua_kthread_item_t* p_item)
{
    assert(NULL == p_item->p_thread);

    if (KLUA_KTHREAD_OWNER == p_item->flag)
    {
        KLB_FREE_BY(p_item->p_env, klua_env_destroy);
    }

    KLB_FREE_BY(p_item->entry_path, sdsfree);
    KLB_FREE(p_item);
}

static void wait_klua_kthread(klua_kthread_t* p_kthread, char* p_names[], int num)
{
    while (true)
    {
        bool wait = false;

        klb_mutex_lock(p_kthread->p_thread_mutex);

        for (int i = 0; i < num; i++)
        {
            if (NULL != p_names[i])
            {
                klua_kthread_item_t* p_item = (klua_kthread_item_t*)klb_hlist_find(p_kthread->p_thread_hlist, p_names[i], strlen(p_names[i]));
                if (NULL != p_item && p_item->wait)
                {
                    wait = true;
                }
                else
                {
                    p_names[i] = NULL;
                }
            }
        }

        klb_mutex_unlock(p_kthread->p_thread_mutex);

        if (wait)
        {
            klb_sleep(1);
        }
        else
        {
            break;
        }
    }
}

static sds klua_kthread_create(klua_kthread_t* p_kthread, const char* p_entry, bool wait, int cpu_idx, const char* p_arg, int arg_size)
{
    sds ret = NULL;
    klb_thread_t* p_thread = NULL;

    klb_mutex_lock(p_kthread->p_thread_mutex);

    while (true)
    {
        char name[KLUA_KTHREAD_NAME_BUF] = { 0 };
        klb_rand_string(name, KLUA_KTHREAD_NAME_LEN, true);

        if (NULL == klb_hlist_find_iter(p_kthread->p_thread_hlist, name, KLUA_KTHREAD_NAME_LEN))
        {
            // 创建, 并加入hlist
            klua_kthread_item_t* p_item = klua_kthread_item_create(p_kthread->pre_openlibs, name, KLUA_KTHREAD_NAME_LEN, p_entry, p_arg, arg_size);
            klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_thread_hlist, name, KLUA_KTHREAD_NAME_LEN, p_item);
            assert(NULL != p_iter);

            // 创建线程, 启动线程
            p_item->p_thread = klb_thread_create(cb_thread_klua_kthread, p_item, cpu_idx, name);
            assert(NULL != p_item->p_thread);

            ret = sdsnewlen(name, KLUA_KTHREAD_NAME_LEN);

            break;
        }
    }

    klb_mutex_unlock(p_kthread->p_thread_mutex);

    if (wait)
    {
        char* name[1] = { 0 };
        name[0] = ret;

        wait_klua_kthread(g_klua_kthread, name, 1);
    }

    return ret;
}

static int klua_kthread_destroy(klua_kthread_t* p_kthread, const char* p_name, size_t name_len)
{
    int ret = 1;
    klb_mutex_lock(p_kthread->p_thread_mutex);

    klb_hlist_iter_t* p_iter = klb_hlist_find_iter(p_kthread->p_thread_hlist, p_name, name_len);
    if (NULL != p_iter)
    {
        klua_kthread_item_t* p_item = (klua_kthread_item_t*)klb_hlist_data(p_iter);

        if (KLUA_KTHREAD_OWNER == p_item->flag)
        {
            // 关闭线程
            KLB_FREE_BY(p_item->p_thread, klb_thread_destroy);
        }

        // 移除
        klb_hlist_remove(p_kthread->p_thread_hlist, p_iter);

        // 销毁
        KLB_FREE_BY(p_item, klua_kthread_item_destroy);
        
        ret = 0;
    }

    klb_mutex_unlock(p_kthread->p_thread_mutex);
    return ret;
}


static int klua_kthread_register_in(klua_kthread_t* p_kthread, const char* p_name, size_t name_len, klua_env_t* p_env)
{
    int ret = 1;
    klb_mutex_lock(p_kthread->p_thread_mutex);

    if (NULL == klb_hlist_find_iter(p_kthread->p_thread_hlist, p_name, name_len))
    {
        // 没有被占用名称

        // 创建, 并加入hlist
        klua_kthread_item_t* p_item = klua_kthread_item_create_other(p_name, name_len, p_env);
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_thread_hlist, p_name, name_len, p_item);
        assert(NULL != p_iter);

        ret = 0;
    }

    klb_mutex_unlock(p_kthread->p_thread_mutex);
    return ret;
}

//////////////////////////////////////////////////////////////////////////
// 公共接口函数

void klua_thread_set_preload(klua_openlibs_cb cb)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    klb_mutex_lock(p_kthread->p_thread_mutex);
    p_kthread->pre_openlibs = cb;
    klb_mutex_unlock(p_kthread->p_thread_mutex);
}

int klua_thread_register(const char* p_name, klua_env_t* p_env)
{
    klua_kthread_register_in(g_klua_kthread, p_name, strlen(p_name), p_env);

    return 0;
}

int klua_thread_unregister(const char* p_name)
{
    klua_kthread_destroy(g_klua_kthread, p_name, strlen(p_name));
    return 0;
}

int klua_thread_register_lpc_module(const sds name, klua_env_t* p_env)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    int ret = 1;
    klb_atomic_lock(&p_kthread->module_lock);

    if (NULL == klb_hlist_find_iter(p_kthread->p_module_hlist, name, sdslen(name)))
    {
        // 没有被占用名称
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_module_hlist, name, sdslen(name), p_env);
        assert(NULL != p_iter);

        ret = 0;
    }

    klb_atomic_unlock(&p_kthread->module_lock);
    return ret;
}

int klua_thread_unregister_lpc_module(const sds name)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    int ret = 1;
    klb_atomic_lock(&p_kthread->module_lock);

    klua_env_t* p_env = (klua_env_t*)klb_hlist_remove_bykey(p_kthread->p_module_hlist, name, sdslen(name));
    if (NULL != p_env)
    {
        ret = 0;
    }

    klb_atomic_unlock(&p_kthread->module_lock);
    return ret;
}

sds klua_thread_register_lpc(klua_env_t* p_env)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    sds ret = NULL;

    klb_atomic_lock(&p_kthread->lpc_lock);

    while (true)
    {
        char name[KLUA_LPC_NAME_BUF] = { 0 };
        klb_rand_string(name, KLUA_LPC_NAME_LEN, true);

        if (NULL == klb_hlist_find_iter(p_kthread->p_lpc_hlist, name, KLUA_LPC_NAME_LEN))
        {
            klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_lpc_hlist, name, KLUA_LPC_NAME_LEN, p_env);
            assert(NULL != p_iter);

            ret = sdsnewlen(name, KLUA_LPC_NAME_LEN);
            break;
        }
    }

    klb_atomic_unlock(&p_kthread->lpc_lock);

    return ret;
}

int klua_thread_unregister_lpc(const sds name)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    int ret = 1;
    klb_atomic_lock(&p_kthread->lpc_lock);

    klua_env_t* p_env = (klua_env_t*)klb_hlist_remove_bykey(p_kthread->p_lpc_hlist, name, sdslen(name));
    if (NULL != p_env)
    {
        ret = 0;
    }

    klb_atomic_unlock(&p_kthread->lpc_lock);
    return ret;
}

int klua_thread_push_lpc_msg(const char* p_name, klua_msg_t* p_msg)
{
    klua_kthread_t* p_kthread = g_klua_kthread;

    int ret = 1;

    if (KLUA_LPC_POST == p_msg->type || KLUA_LPC_REQUEST == p_msg->type)
    {
        klb_atomic_lock(&p_kthread->module_lock);
        klua_env_t* p_env = (klua_env_t*)klb_hlist_find(p_kthread->p_module_hlist, p_name, strlen(p_name));
        if (NULL != p_env)
        {
            klua_env_push_lpc_msg(p_env, p_msg);
            ret = 0;
        }
        klb_atomic_unlock(&p_kthread->module_lock);
    }
    else if(KLUA_LPC_RESPONSE == p_msg->type || KLUA_LPC_NOTIFY == p_msg->type)
    {
        klb_atomic_lock(&p_kthread->lpc_lock);
        klua_env_t* p_env = (klua_env_t*)klb_hlist_find(p_kthread->p_lpc_hlist, p_name, strlen(p_name));
        if (NULL != p_env)
        {
            klua_env_push_lpc_msg(p_env, p_msg);
            ret = 0;
        }
        klb_atomic_unlock(&p_kthread->lpc_lock);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
// Lua导出接口


/// @brief 创建
static int lib_klua_kthread_start(lua_State* L)
{
    const char* p_entry_path = luaL_checkstring(L, 1);          ///< @1. 入口Lua脚本路径: eg."aaa.bbb"
    bool wait = klua_check_option_boolean(L, 2, true);          ///< @2. [可选](默认 true): 是否等待线程启动完成 
    int cpu_idx = (int)klua_check_option_integer(L, 3, -1);     ///< @3. [可选](默认 -1): 是否指定CPU核心

    // pack buffer, 启动参数
    klb_buf_t* p_buf = luaseri_map_binary_pack(L, 3);           ///< @4 - @N. [可选] 传递给线程的初始化参数

    sds name = klua_kthread_create(g_klua_kthread, p_entry_path, wait, cpu_idx, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

    lua_pushlstring(L, name, sdslen(name));                     ///< #1. string: 线程名称
    
    KLB_FREE_BY(name, sdsfree);
    KLB_FREE(p_buf);

    return 1;
}

/// @brief 销毁
static int lib_klua_kthread_stop(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);    ///< @1. 线程名称: eg. "aaa"

    int ret = klua_kthread_destroy(g_klua_kthread, p_name, name_len);

    lua_pushboolean(L, (0 == ret) ? true : false);              ///< #1. true.成功; false.失败
    return 1;
}

#define KLUA_KTHREAD_WAIT_MAX   1024


/// @brief 批量等待线程(Lua)启动完成
///  Lua线程启动完成标志为: klua_env_dolibrary() 函数执行完成
static int lib_klua_kthread_wait(lua_State* L)
{
    int idx = 0;
    char* names[KLUA_KTHREAD_WAIT_MAX] = { 0 };

    if (LUA_TTABLE == lua_type(L, 1))
    {
        int absindex = lua_absindex(L, 1);                  ///< @1. [可选] table, 值为线程名称
        lua_pushnil(L);
        while (lua_next(L, absindex) != 0)
        {
            if (LUA_TSTRING == lua_type(L, -1) && idx < KLUA_KTHREAD_WAIT_MAX)
            {
                names[idx] = (char*)lua_tostring(L, -1);
                idx++;
            }

            lua_pop(L, 1);
        }
    }
    else
    {
        int n = lua_gettop(L);                              ///< @1 - @N. [可选] string 线程名称

        for (int i = 1; i <= n; i++)
        {
            if (LUA_TSTRING == lua_type(L, i))
            {
                names[idx] = (char*)lua_tostring(L, i);
                idx++;

                if (KLUA_KTHREAD_WAIT_MAX <= idx)
                {
                    break;
                }
            }
        }
    }

    wait_klua_kthread(g_klua_kthread, names, idx);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// Lua导出库

int klua_open_kthread(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "start",      lib_klua_kthread_start },
        { "stop",       lib_klua_kthread_stop },

        { "wait",       lib_klua_kthread_wait },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
