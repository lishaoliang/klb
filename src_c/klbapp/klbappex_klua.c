// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbapp/klbappex_klua.h"
#include "klbapp/klbappex_klua_in.h"
#include "klbapp/klb_app_in.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_thread.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_map_serialize.h"
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_mem.h"


/// @def   KLB_APPEX_klua
/// @brief APP的扩展
#define KLB_APPEX_klua       "KLB-APPEX-klua"


//////////////////////////////////////////////////////////////////////////
// 内部: 扩展接口函数


/// @brief 创建扩展
static void* klbappex_klua_create(klb_app_t* p_app)
{
    klbappex_klua_t* p_appex = KLB_MALLOCZ(klbappex_klua_t, 1, 0);

    // env 初始化
    {
        p_appex->is_load_entry = false;

        p_appex->p_preload_rwlock = klb_rwlock_create();
        p_appex->p_preload_nlist = klb_nlist_create();

        p_appex->p_env = klua_env_create(NULL);
    }

    return p_appex;
}

static int on_clear_preload_item_klbappex_klua(void* p_obj, void* p_data)
{
    klbappex_preload_item_t* p_item = (klbappex_preload_item_t*)p_data;

    KLB_FREE(p_item);

    return 0;
}

/// @brief 销毁扩展
static void klbappex_klua_destroy(void* ptr, klb_app_t* p_app)
{
    klbappex_klua_t* p_appex = (klbappex_klua_t*)ptr;

    // 清空
    klb_nlist_clear(p_appex->p_preload_nlist, on_clear_preload_item_klbappex_klua, NULL);

    KLB_FREE_BY(p_appex->p_preload_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_appex->p_preload_rwlock, klb_rwlock_destroy);
    KLB_FREE_BY(p_appex->p_env, klua_env_destroy);

    KLB_FREE(p_appex);
}

/// @brief 控制操作消息
static int klbappex_klua_control(void* ptr, klb_app_t* p_app, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbappex_klua_t* p_appex = (klbappex_klua_t*)ptr;

    return 0;
}

/// @brief 常规调用一次
static int klbappex_klua_loop_once(void* ptr, klb_app_t* p_app, int64_t now)
{
    klbappex_klua_t* p_appex = (klbappex_klua_t*)ptr;

    if ( NULL == p_appex->p_env || !p_appex->is_load_entry)
    {
        return 0; // 未加载 入口 lua 脚本
    }

    return klua_env_loop_once(p_appex->p_env);
}

//////////////////////////////////////////////////////////////////////////
// 内部

// 所有 默认预加载函数
static int on_preload_klualib_klbappex_klua(lua_State* L)
{
    klbappex_klua_t* p_appex = klbappex_get_klua2();

    // 读锁
    klb_rwlock_rdlock(p_appex->p_preload_rwlock);

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_appex->p_preload_nlist);
    while (NULL != p_iter)
    {
        klbappex_preload_item_t* p_item = (klbappex_preload_item_t*)klb_nlist_data(p_iter);

        if (NULL != p_item && NULL != p_item->cb)
        {
            if (NULL != p_item->name)
            {
                // 加载 单个库, 使用库名称
                klua_loadlib(L, p_item->cb, p_item->name);
            }
            else
            {
                // wrapper 回调: 在函数中通过 klua_loadlib() 预加载多个库
                p_item->cb(L);
            }
        }

        p_iter = klb_nlist_next(p_iter);
    }

    klb_rwlock_rdunlock(p_appex->p_preload_rwlock);

    return 0;
}

// 设置 启动初始参数
static void set_startup_args_klbappex_klua(klua_env_t* p_env, int argc, char** argv)
{
    klb_map_t r = { 0 };
    klb_map_init(&r);

    for (int i = 0; i < argc; i++)
    {
        klb_map_set_idx_string(&r, i, argv[i]);
    }

    klb_buf_t* p_args = klb_map_seri_pack(&r);
    klua_env_set_args(p_env, p_args->p_buf + p_args->start, p_args->end - p_args->start);

    KLB_FREE(p_args);
    klb_map_quit(&r);
}


int klbappex_klua_do_preinit(klbappex_klua_t* p_appex, int argc, char** argv)
{
    // 设置所有子线程都使用预加载库
    klua_thread_set_preload(on_preload_klualib_klbappex_klua);

    // 初始化 主lua环境
    {
        // 设置启动参数 args
        set_startup_args_klbappex_klua(p_appex->p_env, argc, argv);

        // 设置主入口线程 预加载库函数
        klua_env_set_preload(p_appex->p_env, on_preload_klualib_klbappex_klua);

        // 初始化lua环境
        klua_env_doinit(p_appex->p_env);
    }

    // 注册标记 本线程为 main
    klua_thread_register("main", p_appex->p_env);        // main

    // 开始加载 lua 入口 脚本
    if (argc < 2 || NULL == argv[1])
    {
        klua_thread_unregister("main");
        return -1;
    }

    if (0 != klua_env_dofile(p_appex->p_env, argv[1]))
    {
        klua_thread_unregister("main");
    }
    else
    {
        p_appex->is_load_entry = true;
    }

    return 0;
}

int klbappex_klua_do_prequit(klbappex_klua_t* p_appex)
{
    if (p_appex->is_load_entry)
    {
        klua_env_exit(p_appex->p_env); // 退出线程时, 将Lua环境置为退出状态
        klua_env_doend(p_appex->p_env);
    }

    p_appex->is_load_entry = false;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数

int klbappex_klua_push_preload(klbappex_klua_t* p_appex, lua_CFunction cb_pre_load, const char* p_name)
{
    klbappex_preload_item_t* p_item = KLB_MALLOCZ(klbappex_preload_item_t, 1, 0);

    p_item->cb = cb_pre_load;
    p_item->name = p_name;

    // 写锁
    klb_rwlock_wrlock(p_appex->p_preload_rwlock);
    klb_nlist_push_tail(p_appex->p_preload_nlist, p_item);
    klb_rwlock_wrunlock(p_appex->p_preload_rwlock);

    return 0;
}

klua_env_t* klbappex_klua_get_klua_env(klbappex_klua_t* p_appex)
{
    return p_appex->p_env;
}

//////////////////////////////////////////////////////////////////////////
// 注册 / 获取

/// @brief 获取 klua
/// @return klbappex_klua_t*
klbappex_klua_t* klbappex_get_klua(klb_app_t* p_app)
{
    return (klbappex_klua_t*)klb_app_get_extension(p_app, KLB_APPEX_klua);
}

klbappex_klua_t* klbappex_get_klua2()
{
    klb_app_t* p_app = klb_app_instance();
    return klbappex_get_klua(p_app);
}

/// @brief 注册 klua 扩展
int klbappex_register_klua(klb_app_t* p_app)
{
    klb_app_extension_t ex = { 0 };

    ex.cb_create = klbappex_klua_create;
    ex.cb_destroy = klbappex_klua_destroy;
    ex.cb_control = klbappex_klua_control;
    ex.cb_loop_once = klbappex_klua_loop_once;

    klb_app_register_extension(p_app, KLB_APPEX_klua, &ex);

    return 0;
}

//end
