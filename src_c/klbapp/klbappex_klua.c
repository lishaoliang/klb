// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbapp/klbappex_klua.h"
#include "klbapp/klbappex_klua_in.h"
#include "klbapp/klb_app_in.h"
#include "klua/klua_env.h"
#include "klua/klua_thread.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_map_serialize.h"
#include "klbmem/klb_mem.h"


/// @def   KLBAPPEX_KLUA
/// @brief 扩展名称
#define KLBAPPEX_KLUA       "KLBAPPEX-klua"


//////////////////////////////////////////////////////////////////////////
// 内部: 扩展接口函数


/// @brief 创建扩展
static void* klbappex_klua_create(klb_app_t* p_app)
{
    klbappex_klua_t* p_appex = KLB_MALLOCZ(klbappex_klua_t, 1, 0);

    // env 初始化
    {
        p_appex->is_load_entry = false;
        p_appex->cb_pre_load = NULL;

        p_appex->p_env = klua_env_create(NULL);
    }

    return p_appex;
}

/// @brief 销毁扩展
static void klbappex_klua_destroy(void* ptr, klb_app_t* p_app)
{
    klbappex_klua_t* p_appex = (klbappex_klua_t*)ptr;

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
    klua_thread_set_preload(p_appex->cb_pre_load);

    // 初始化 主lua环境
    {
        // 设置启动参数 args
        set_startup_args_klbappex_klua(p_appex->p_env, argc, argv);

        // 设置预加载库函数
        klua_env_set_preload(p_appex->p_env, p_appex->cb_pre_load);

        // 初始化lua环境
        klua_env_doinit(p_appex->p_env);
    }

    // 注册标记 本线程为 main
    klua_thread_register("main", p_appex->p_env);        // main

    // 开始加载 lua 入口 脚本
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

int klbappex_klua_set_preload(klbappex_klua_t* p_appex, lua_CFunction cb_pre_load)
{
    p_appex->cb_pre_load = cb_pre_load;
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
    return (klbappex_klua_t*)klb_app_get_extension(p_app, KLBAPPEX_KLUA);
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

    klb_app_register_extension(p_app, KLBAPPEX_KLUA, &ex);

    return 0;
}

//end
