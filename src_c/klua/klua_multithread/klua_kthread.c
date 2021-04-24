#include "klua/klua_kthread.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_mutex.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_list.h"
#include "klbplatform/klb_thread.h"
#include "klbmem/klb_mem.h"
#include "klbthird/sds.h"
#include <assert.h>


typedef struct klua_kthread_item_t_
{
#define KLUA_KTHREAD_OWNER      0       ///< 本模块管理生命周期
#define KLUA_KTHREAD_OTHER      1       ///< 外部模块管理生命周期
    int             flag;

    klua_env_t*     p_env;              ///< Lua环境

    struct
    {
        sds         name;               ///< 线程名称
        sds         entry_path;         ///< Lua脚本入口

        klb_list_t* p_msg_list;         ///< 暂存的消息队列
    };

    struct
    {
        klb_thread_t*   p_thread;       ///< 线程
    };
}klua_kthread_item_t;


typedef struct klua_kthread_t_
{
    klb_mutex_t*        p_mutex;        ///< 锁
    klb_hlist_t*        p_hlist;        ///< 所有线程: klua_kthread_item_t*
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
    g_klua_kthread = KLB_MALLOC(klua_kthread_t, 1, 0);
    KLB_MEMSET(g_klua_kthread, 0, sizeof(klua_kthread_t));

    g_klua_kthread->p_mutex = klb_mutex_create();
    g_klua_kthread->p_hlist = klb_hlist_create(0);

    return 0;
}

void klua_kthread_quit()
{
    assert(NULL != g_klua_kthread);

    // 清空线程
    klb_mutex_lock(g_klua_kthread->p_mutex);
    while (0 < klb_hlist_size(g_klua_kthread->p_hlist))
    {
        // 先关闭线程
        klua_kthread_item_t* p_head = (klua_kthread_item_t*)klb_hlist_head(g_klua_kthread->p_hlist);
        KLB_FREE_BY(p_head->p_thread, klb_thread_destroy);

        // 移除,并销毁
        klua_kthread_item_t* p_item = (klua_kthread_item_t*)klb_hlist_pop_head(g_klua_kthread->p_hlist);
        KLB_FREE_BY(p_item, klua_kthread_item_destroy);
    }
    klb_mutex_unlock(g_klua_kthread->p_mutex);

    // 销毁
    KLB_FREE_BY(g_klua_kthread->p_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kthread->p_mutex, klb_mutex_destroy);
    KLB_FREE(g_klua_kthread);
}

//////////////////////////////////////////////////////////////////////////
// klua_kthread_t 部分C函数

static int cb_klb_thread_klua_kthread(void* p_obj, volatile int* p_run)
{
    klua_kthread_item_t* p_item = (klua_kthread_item_t*)p_obj;
    klua_env_t* p_env = p_item->p_env;

    if (0 != klua_env_dolibrary(p_env, p_item->entry_path))
    {
        return 0;
    }

    if (klua_env_is_exit(p_env))
    {
        klua_env_doend(p_env);
        return 0;
    }

    while (*p_run)
    {
        klua_env_loop_once(p_env);

        if (klua_env_is_exit(p_env))
        {
            break;
        }
        else
        {
            klb_sleep(10);
        }
    }

    klua_env_doend(p_env);

    return 0;
}

static klua_kthread_item_t* klua_kthread_item_create(const char* p_name, size_t name_len, const char* p_entry)
{
    klua_kthread_item_t* p_item = KLB_MALLOC(klua_kthread_item_t, 1, 0);
    KLB_MEMSET(p_item, 0, sizeof(klua_kthread_item_t));

    p_item->flag = KLUA_KTHREAD_OWNER;
    p_item->name = sdsnewlen(p_name, name_len);
    p_item->entry_path = sdsnew(p_entry);
    p_item->p_msg_list = klb_list_create();

    p_item->p_env = klua_env_create(klua_loadlib_all);
    klua_env_set_name(p_item->p_env, p_name, name_len);

    return p_item;
}

static klua_kthread_item_t* klua_kthread_item_create_other(const char* p_name, size_t name_len, klua_env_t* p_env)
{
    klua_kthread_item_t* p_item = KLB_MALLOC(klua_kthread_item_t, 1, 0);
    KLB_MEMSET(p_item, 0, sizeof(klua_kthread_item_t));

    p_item->flag = KLUA_KTHREAD_OTHER;
    p_item->name = sdsnewlen(p_name, name_len);
    p_item->entry_path = sdsnew("");
    p_item->p_msg_list = klb_list_create();

    p_item->p_env = p_env;

    klua_env_set_name(p_item->p_env, p_name, name_len);

    return p_item;
}

static void klua_kthread_item_destroy(klua_kthread_item_t* p_item)
{
    assert(NULL == p_item->p_thread);

    KLB_FREE_BY(p_item->p_msg_list, klb_list_destroy);

    KLB_FREE_BY(p_item->name, sdsfree);
    KLB_FREE_BY(p_item->entry_path, sdsfree);
    KLB_FREE(p_item);
}

static int klua_kthread_create(klua_kthread_t* p_kthread, const char* p_name, size_t name_len, const char* p_entry)
{
    int ret = 1;
    klb_mutex_lock(p_kthread->p_mutex);

    if (NULL == klb_hlist_find_iter(p_kthread->p_hlist, p_name, name_len))
    {
        // 没有被占用名称

        // 创建, 并加入hlist
        klua_kthread_item_t* p_item = klua_kthread_item_create(p_name, name_len, p_entry);
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_hlist, p_name, name_len, p_item);
        assert(NULL != p_iter);

        // 创建线程, 启动线程
        p_item->p_thread = klb_thread_create(cb_klb_thread_klua_kthread, p_item, -1, p_name);
        assert(NULL != p_item->p_thread);

        ret = 0;
    }

    klb_mutex_unlock(p_kthread->p_mutex);
    return ret;
}

static int klua_kthread_destroy(klua_kthread_t* p_kthread, const char* p_name, size_t name_len)
{
    int ret = 1;
    klb_mutex_lock(p_kthread->p_mutex);

    klb_hlist_iter_t* p_iter = klb_hlist_find_iter(p_kthread->p_hlist, p_name, name_len);
    if (NULL != p_iter)
    {
        klua_kthread_item_t* p_item = (klua_kthread_item_t*)klb_hlist_data(p_iter);

        if (KLUA_KTHREAD_OWNER == p_item->flag)
        {
            // 关闭线程
            KLB_FREE_BY(p_item->p_thread, klb_thread_destroy);
        }

        // 移除
        klb_hlist_remove(p_kthread->p_hlist, p_iter);

        // 销毁
        KLB_FREE_BY(p_item, klua_kthread_item_destroy);
        
        ret = 0;
    }

    klb_mutex_unlock(p_kthread->p_mutex);
    return ret;
}


static int klua_kthread_register_in(klua_kthread_t* p_kthread, const char* p_name, size_t name_len, klua_env_t* p_env)
{
    int ret = 1;
    klb_mutex_lock(p_kthread->p_mutex);

    if (NULL == klb_hlist_find_iter(p_kthread->p_hlist, p_name, name_len))
    {
        // 没有被占用名称

        // 创建, 并加入hlist
        klua_kthread_item_t* p_item = klua_kthread_item_create_other(p_name, name_len, p_env);
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kthread->p_hlist, p_name, name_len, p_item);
        assert(NULL != p_iter);

        ret = 0;
    }

    klb_mutex_unlock(p_kthread->p_mutex);
    return ret;
}

//////////////////////////////////////////////////////////////////////////
// 公共接口函数

int klua_kthread_register(const char* p_name, klua_env_t* p_env)
{
    klua_kthread_register_in(g_klua_kthread, p_name, strlen(p_name), p_env);

    return 0;
}

int klua_kthread_unregister(const char* p_name)
{
    klua_kthread_destroy(g_klua_kthread, p_name, strlen(p_name));
    return 0;
}

int klua_kthread_push_msg(const char* p_name, klua_msg_t* p_msg)
{
    int ret = 1;
    klb_mutex_lock(g_klua_kthread->p_mutex);

    klua_kthread_item_t* p_item = klb_hlist_find(g_klua_kthread->p_hlist, p_name, strlen(p_name));
    if (NULL != p_item)
    {
        klb_list_push_tail(p_item->p_msg_list, p_msg);
        klua_env_set_msg_flag(p_item->p_env);
        ret = 0;
    }

    klb_mutex_unlock(g_klua_kthread->p_mutex);
    return ret;
}

int klua_kthread_get_msg(const char* p_name, klb_list_t* p_list)
{
    int ret = 1;
    klb_mutex_lock(g_klua_kthread->p_mutex);

    klua_kthread_item_t* p_item = klb_hlist_find(g_klua_kthread->p_hlist, p_name, strlen(p_name));
    if (NULL != p_item)
    {
        while (0 < klb_list_size(p_item->p_msg_list))
        {
            // 取出
            klua_msg_t* p_msg = (klua_msg_t*)klb_list_pop_head(p_item->p_msg_list);

            // 移到输出列表
            klb_list_push_tail(p_list, p_msg);
        }

        ret = 0;
    }

    klb_mutex_unlock(g_klua_kthread->p_mutex);
    return ret;
}


//////////////////////////////////////////////////////////////////////////
// Lua导出接口

/// @brief 创建
static int lib_klua_kthread_create(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);    ///< @1. 线程名称: eg. "aaa"
    const char* p_entry_path = luaL_checkstring(L, 2);          ///< @2. 入口Lua脚本路径: eg."aaa.bbb"

    int ret = klua_kthread_create(g_klua_kthread, p_name, name_len, p_entry_path);

    lua_pushboolean(L, (0 == ret) ? true : false);              ///< #1. true.成功; false.失败
    return 1;
}

/// @brief 销毁
static int lib_klua_kthread_destroy(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);    ///< @1. 线程名称: eg. "aaa"

    int ret = klua_kthread_destroy(g_klua_kthread, p_name, name_len);

    lua_pushboolean(L, (0 == ret) ? true : false);              ///< #1. true.成功; false.失败
    return 1;
}

/// @brief 发送消息
static int lib_klua_kthread_post(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);    ///< @1. 线程名称: eg. "aaa"

    //klua_thread_msg_t* p_msg = KLB_MALLOC(klua_thread_msg_t, 1, 0);
    //KLB_MEMSET(p_msg, 0, sizeof(klua_thread_msg_t));

    //klua_kthread_push_msg(p_name, p_msg);

    lua_pushboolean(L, true);                                   ///< #1. true.成功; false.失败
    return 1;
}

//////////////////////////////////////////////////////////////////////////
// Lua导出库

int klua_open_kthread(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "create",     lib_klua_kthread_create },
        { "destroy",    lib_klua_kthread_destroy },

        { "post",       lib_klua_kthread_post },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
