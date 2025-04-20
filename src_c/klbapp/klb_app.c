#include "klbapp/klb_app.h"
#include "klbapp/klb_app_in.h"
#include "klbbase/klb_base.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_thread.h"
#include "klbplatform/klb_time.h"
#include "klbutil/klb_log.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
static klb_app_t* g_klb_app = NULL;


//////////////////////////////////////////////////////////////////////////
static void on_preload_klb_app(klb_app_t* p_app);


int klb_app_main(int argc, char** argv, lua_CFunction cb_pre_load)
{
    // init
    //klb_base_init(NULL);    // klb base

    // step1. app instance
    klb_app_t* p_app = klb_app_instance();
    klbappex_plugins_t* p_appex_plugins = p_app->p_plugins;
    klbappex_klua_t* p_appex_klua = p_app->p_klua;

    klua_env_t* p_klua_env = klbappex_klua_get_klua_env(p_appex_klua);

    // step2. 初始化 klua 预加载库
    {
        klbappex_klua_set_preload(p_appex_klua, cb_pre_load);
    }

    // step3. 初始化
    // 准备初始化 其他扩展/插件 等环境
    {
        // 预加载 APP扩展 流程
        on_preload_klb_app(p_app);

        // 预加载(动态库)插件
        klbappex_plugins_preload(p_appex_plugins);
    }

    // step4. 初始化完成之后, 为 lua 启动做初始等流程
    // 准备完成之后
    {
        klbappex_klua_do_preinit(p_appex_klua, argc, argv);
    }

    // step5. loop
    while (0 == p_app->run_status && !klua_env_is_exit(p_klua_env))
    {
        int sleep = klb_app_loop_once();

        if (0 < sleep)
        {
            klb_sleep(sleep);
        }
        else
        {
            klb_sleep(1);
        }
    }
    
    // stepN. 结束
    {
        klbappex_klua_do_prequit(p_appex_klua);
    }

    // quit
    //klb_base_quit();

    return 0;
}

klb_app_t* klb_app_instance()
{
    assert(NULL != g_klb_app);
    return g_klb_app;
}

int klb_app_init()
{
    assert(NULL == g_klb_app);

    g_klb_app = klb_app_create();

    return 0;
}

void klb_app_quit()
{
    KLB_FREE_BY(g_klb_app, klb_app_destroy);
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建
klb_app_t* klb_app_create()
{
    klb_app_t* p_app = KLB_MALLOCZ(klb_app_t, 1, 0);

    p_app->run_status = 0;

    // 预加载函数
    {
        p_app->p_nlist_preload = klb_nlist_create();
    }
 
    // 扩展 支持
    {
        p_app->p_extension_hlist = klb_hlist_create(0);
        p_app->p_extension_activated_hlist = klb_hlist_create(0);
    }

    // 注册标准扩展
    {
        klbappex_register_plugins(p_app); // plugins 插件扩展
        klbappex_register_klua(p_app); // klua 扩展
    }

    // 初始 激活扩展
    {
        p_app->p_plugins = klbappex_get_plugins(p_app);
        p_app->p_klua = klbappex_get_klua(p_app);
    }

    return p_app;
}

/// @brief 销毁
/// @return 无
void klb_app_destroy(klb_app_t* p_app)
{
    // 清空 已激活的扩展模块
    while (0 < klb_hlist_size(p_app->p_extension_activated_hlist))
    {
        klb_app_extension_activated_t* p_activated = (klb_app_extension_activated_t*)klb_hlist_pop_tail(p_app->p_extension_activated_hlist);

        // 销毁
        p_activated->ex.cb_destroy(p_activated->ptr, p_app);

        KLB_FREE_BY(p_activated->name, sdsfree);
        KLB_FREE(p_activated);
    }

    // 清空 注册列表
    while (0 < klb_hlist_size(p_app->p_extension_hlist))
    {
        klb_app_extension_t* p_tmp = (klb_app_extension_t*)klb_hlist_pop_tail(p_app->p_extension_hlist);
        KLB_FREE(p_tmp);
    }

    // 清空 预加载函数列表
    while (0 < klb_nlist_size(p_app->p_nlist_preload))
    {
        klb_nlist_pop_head(p_app->p_nlist_preload);
    }

    // 释放列表
    KLB_FREE_BY(p_app->p_extension_activated_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_app->p_extension_hlist, klb_hlist_destroy);

    KLB_FREE_BY(p_app->p_nlist_preload, klb_nlist_destroy);

    KLB_FREE(p_app);
}

// 预加载流程
static void on_preload_klb_app(klb_app_t* p_app)
{
    while (0 < klb_nlist_size(p_app->p_nlist_preload))
    {
        klb_app_preload_cb cb_preload = (klb_app_preload_cb)klb_nlist_pop_head(p_app->p_nlist_preload);

        // 调用 预加载函数
        // 预先加载流程, 一般 先注册 各个扩展模块
        cb_preload(p_app);
    }
}

/////////////////////////////////////////

/// @brief 注册app扩展
int klb_app_register_extension(klb_app_t* p_app, const char* p_name, const klb_app_extension_t* p_extension)
{
    assert(NULL != p_app);
    assert(NULL != p_name);
    assert(NULL != p_extension);
    assert(NULL != p_extension->cb_create);
    assert(NULL != p_extension->cb_destroy);

    size_t name_len = strlen(p_name);

    klb_app_extension_t* p_tmp = KLB_MALLOCZ(klb_app_extension_t, 1, 0);
    memcpy(p_tmp, p_extension, sizeof(klb_app_extension_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_app->p_extension_hlist, p_name, name_len, p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG_E("register app extension error!name:[%s]\n", p_name);
        return 1; // 放入失败, 名称重复
    }

    return 0;
}

/// @brief 获取app扩展
static void* get_extension_klb_app(klb_app_t* p_app, const char* p_name, klb_app_extension_activated_t** p_out_ex)
{
    assert(NULL != p_app);
    assert(NULL != p_name);

    // 先从激活的里面找
    size_t name_len = strlen(p_name);
    klb_app_extension_activated_t* p_activated = (klb_app_extension_activated_t*)klb_hlist_find(p_app->p_extension_activated_hlist, p_name, name_len);
    if (NULL != p_activated)
    {
        if (NULL != p_out_ex) { *p_out_ex = p_activated; };

        return p_activated->ptr;
    }

    // 未找到, 则激活
    klb_app_extension_t* p_extension = (klb_app_extension_t*)klb_hlist_find(p_app->p_extension_hlist, p_name, name_len);
    if (NULL != p_extension)
    {
        klb_app_extension_activated_t* p_tmp = KLB_MALLOCZ(klb_app_extension_activated_t, 1, 0);

        // 扩展名称
        memcpy(&p_tmp->ex, p_extension, sizeof(klb_app_extension_t));
        p_tmp->name = sdsnewlen(p_name, name_len);

        // 创建
        p_tmp->ptr = p_tmp->ex.cb_create(p_app);
        assert(NULL != p_tmp->ptr);

        // ioctrl 接口
        {
            // 若存在 ioctrl 接口, 则获取
            if (NULL != p_tmp->ex.cb_get_ioctrl)
            {
                p_tmp->ex.cb_get_ioctrl(p_tmp->ptr, &p_tmp->ioctrl);
            }

            p_tmp->ioctrl.ptr = p_tmp->ptr;
        }

        // 放入链表
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_app->p_extension_activated_hlist, p_name, name_len, p_tmp);
        assert(NULL != p_iter);

        if (NULL != p_out_ex) { *p_out_ex = p_tmp; };
        return p_tmp->ptr;
    }

    return NULL;
}

/// @brief 获取app扩展
void* klb_app_get_extension(klb_app_t* p_app, const char* p_name)
{
    return get_extension_klb_app(p_app, p_name, NULL);
}

//////////////////////////////////////////////////////////////////////////
// app extension ioctrl 扩展接口

/// @brief 注册app扩展的 ioctrl 接口
klbappex_ioctrl_t* klbappex_get_ioctrl(klb_app_t* p_app, const char* p_name)
{
    klb_app_extension_activated_t* p_ex = NULL;
    get_extension_klb_app(p_app, p_name, &p_ex);

    if (NULL != p_ex)
    {
        return &p_ex->ioctrl;
    }

    // 失败
    return NULL;
}

int klbappex_ioctrl_opt0(klbappex_ioctrl_t* p_ioctrl, int opt)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt0)
    {
        return p_ioctrl->ioctrl_opt0(p_ioctrl->ptr, opt);
    }

    return 1;
}

int klbappex_ioctrl_opt1(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt1)
    {
        return p_ioctrl->ioctrl_opt1(p_ioctrl->ptr, opt, ptr1);
    }

    return 1;
}

int klbappex_ioctrl_opt2(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt2)
    {
        return p_ioctrl->ioctrl_opt2(p_ioctrl->ptr, opt, ptr1, ptr2);
    }

    return 1;
}

int klbappex_ioctrl_opt3(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt3)
    {
        return p_ioctrl->ioctrl_opt3(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3);
    }

    return 1;
}

int klbappex_ioctrl_opt4(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt4)
    {
        return p_ioctrl->ioctrl_opt4(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3, ptr4);
    }

    return 1;
}

int klbappex_ioctrl_opt5(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt5)
    {
        return p_ioctrl->ioctrl_opt5(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3, ptr4, ptr5);
    }

    return 1;
}

int klbappex_ioctrl_opt6(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt6)
    {
        return p_ioctrl->ioctrl_opt6(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6);
    }

    return 1;
}

int klbappex_ioctrl_opt7(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt7)
    {
        return p_ioctrl->ioctrl_opt7(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7);
    }

    return 1;
}

int klbappex_ioctrl_opt8(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8)
{
    if (NULL != p_ioctrl && NULL != p_ioctrl->ioctrl_opt8)
    {
        return p_ioctrl->ioctrl_opt8(p_ioctrl->ptr, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////

void klb_app_push_preload(klb_app_preload_cb cb_preload)
{
    klb_app_t* p_app = klb_app_instance();

    klb_nlist_push_tail(p_app->p_nlist_preload, cb_preload);
}

void klb_app_enable_plugins(bool enable)
{
    klb_app_t* p_app = klb_app_instance();
    klbappex_plugins_enable(p_app->p_plugins, enable);
}

void klb_app_push_plugins_path(const char* p_path_plugins)
{
    klb_app_t* p_app = klb_app_instance();
    klbappex_plugins_push_path(p_app->p_plugins, p_path_plugins);
}

int klb_app_loop_once()
{
    klb_app_t* p_app = klb_app_instance();
    int64_t tc = klb_tick_counti64();

    int sleep = 0;

    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_app->p_extension_activated_hlist);
    while (NULL != p_iter)
    {
        klb_app_extension_activated_t* p_extension = (klb_app_extension_activated_t*)klb_hlist_data(p_iter);

        if (p_extension && p_extension->ex.cb_loop_once)
        {
            sleep += p_extension->ex.cb_loop_once(p_extension->ptr, p_app, tc);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return 10 - sleep;
}
