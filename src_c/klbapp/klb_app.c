// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbapp/klb_app.h"
#include "klbapp/klb_app_in.h"
#include "klbbase/klb_base.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_thread.h"
#include "klbplatform/klb_time.h"
#include "klbthird/klb_getopt.h"
#include "klbutil/klb_log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#if !defined(_WIN32) && !defined(WIN32)
#include <unistd.h>
#else
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#endif


//////////////////////////////////////////////////////////////////////////
static klb_app_t* g_klb_app = NULL;

#define KLUA_CORE_VER_REL       "klbcore/VERSION"
#define KLUA_SHARE_DEFAULT      "/usr/local/share/klua"
#define KLUA_ENV_SHARE          "KLUA_SHARE_KLUA"
#define KLUA_VER_FALLBACK       "0.0.0.000000"


//////////////////////////////////////////////////////////////////////////
static void on_preload_klb_app(klb_app_t* p_app);


/// @brief 打印用法
static void print_usage_klb_app(const char* p_prog)
{
    const char* name = (NULL != p_prog && '\0' != p_prog[0]) ? p_prog : "klua";

    fprintf(stdout,
        "Usage: %s [options] <entry.lua> [args...]\n"
        "Options:\n"
        "  -h, --help             Show this help\n"
        "  -v, --version          Show version\n"
        "  -p, --plugins PATH     Enable plugins and add search path (repeatable)\n",
        name);
}


/// @brief 取可执行文件名 (无路径)
static const char* prog_basename_klb_app(const char* p_prog)
{
    const char* p = p_prog;
    const char* slash = NULL;

    if (NULL == p || '\0' == p[0])
    {
        return "klua";
    }

#if defined(_WIN32) || defined(WIN32)
    slash = strrchr(p, '\\');
    if (NULL != slash)
    {
        p = slash + 1;
    }
#endif
    slash = strrchr(p, '/');
    if (NULL != slash)
    {
        p = slash + 1;
    }

    return (NULL != p && '\0' != p[0]) ? p : "klua";
}


/// @brief 读一行 VERSION 文件到 out
static bool read_version_file_klb_app(const char* path, char* out, size_t out_size)
{
    FILE* fp = NULL;
    size_t n = 0;

    if (NULL == path || NULL == out || 0 == out_size)
    {
        return false;
    }

    out[0] = '\0';
    fp = fopen(path, "rb");
    if (NULL == fp)
    {
        return false;
    }

    if (NULL == fgets(out, (int)out_size, fp))
    {
        fclose(fp);
        return false;
    }

    fclose(fp);

    n = strlen(out);
    while (0 < n && ('\n' == out[n - 1] || '\r' == out[n - 1] || ' ' == out[n - 1] || '\t' == out[n - 1]))
    {
        out[n - 1] = '\0';
        n -= 1;
    }

    return (0 < n);
}


/// @brief 解析 klbcore 脚本根并读 VERSION (base.YYYYMM)
static void read_klbcore_version_klb_app(const char* argv0, char* out, size_t out_size)
{
    char path[PATH_MAX];
    const char* env_share = getenv(KLUA_ENV_SHARE);
    const char* share = (NULL != env_share && '\0' != env_share[0]) ? env_share : KLUA_SHARE_DEFAULT;

    if (NULL == out || 0 == out_size)
    {
        return;
    }

    // step1. 安装 share/klua/klbcore/VERSION
    snprintf(path, sizeof(path), "%s/%s", share, KLUA_CORE_VER_REL);
    if (read_version_file_klb_app(path, out, out_size))
    {
        return;
    }

#if !defined(_WIN32) && !defined(WIN32)
    // step2. 源树: lib/klua -> ../bin/klbcore/VERSION
    {
        char exe_dir[PATH_MAX];
        char* slash = NULL;
        ssize_t n = readlink("/proc/self/exe", exe_dir, sizeof(exe_dir) - 1);

        if (0 <= n)
        {
            exe_dir[n] = '\0';
            slash = strrchr(exe_dir, '/');
            if (NULL != slash)
            {
                *slash = '\0';
            }

            snprintf(path, sizeof(path), "%s/../bin/%s", exe_dir, KLUA_CORE_VER_REL);
            if (read_version_file_klb_app(path, out, out_size))
            {
                return;
            }

            // step3. debug 树: klua 旁 klbcore/VERSION
            snprintf(path, sizeof(path), "%s/%s", exe_dir, KLUA_CORE_VER_REL);
            if (read_version_file_klb_app(path, out, out_size))
            {
                return;
            }
        }
        else if (NULL != argv0 && '\0' != argv0[0])
        {
            (void)argv0;
        }
    }
#else
    // Win32: 与 Linux 分支对称, 经 GetModuleFileNameA 解析 exe 目录
    {
        char exe_path[PATH_MAX];
        char exe_dir[PATH_MAX];
        char* slash = NULL;
        DWORD n = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));

        (void)argv0;

        if (0 < n && n < (DWORD)sizeof(exe_path))
        {
            snprintf(exe_dir, sizeof(exe_dir), "%s", exe_path);
            slash = strrchr(exe_dir, '\\');
            if (NULL == slash)
            {
                slash = strrchr(exe_dir, '/');
            }
            if (NULL != slash)
            {
                *slash = '\0';
            }

            // step2. release 平铺/debug: klua.exe 旁 klbcore/VERSION
            snprintf(path, sizeof(path), "%s/%s", exe_dir, KLUA_CORE_VER_REL);
            if (read_version_file_klb_app(path, out, out_size))
            {
                return;
            }

            // step3. 源树: lib/mingw/klua.exe -> ../../bin/klbcore/VERSION
            snprintf(path, sizeof(path), "%s/../../bin/%s", exe_dir, KLUA_CORE_VER_REL);
            if (read_version_file_klb_app(path, out, out_size))
            {
                return;
            }
        }

        // step4. cwd 源树根: .\bin\klbcore\VERSION (历史行为)
        snprintf(path, sizeof(path), ".\\bin\\%s", KLUA_CORE_VER_REL);
        if (read_version_file_klb_app(path, out, out_size))
        {
            return;
        }
    }
#endif

    snprintf(out, out_size, "%s", KLUA_VER_FALLBACK);
}


/// @brief 打印: <prog> <脚本版本> (klb <核心版本>); 脚本版本来自 klbcore/VERSION
static void print_version_klb_app(const char* p_prog)
{
    char script_ver[64];
    const char* name = prog_basename_klb_app(p_prog);

    read_klbcore_version_klb_app(p_prog, script_ver, sizeof(script_ver));
    fprintf(stdout, "%s %s (klb %s)\n", name, script_ver, klb_version());
}


/// @brief 解析壳层命令行; 须在 plugins_preload 之前调用
/// @param [in]  argc           参数个数
/// @param [in]  **argv         参数数组 (getopt 可能重排)
/// @param [out] *p_entry_idx   入口脚本下标 (klb_optind)
/// @return int 0.继续主流程; 1.已处理(help/version)应返回0; -1.失败应返回非0
static int parse_args_klb_app(int argc, char** argv, int* p_entry_idx)
{
    static const klb_option_t long_opts[] =
    {
        { "help",    KLB_no_argument,       NULL, 'h' },
        { "version", KLB_no_argument,       NULL, 'v' },
        { "plugins", KLB_required_argument, NULL, 'p' },
        { NULL, 0, NULL, 0 }
    };

    const char* p_prog = (0 < argc) ? argv[0] : "klua";
    int opt = 0;

    klb_optind = 1;
    klb_opterr = 1;

    while (true)
    {
        opt = klb_getopt_long(argc, argv, "hvp:", long_opts, NULL);
        if (-1 == opt)
        {
            break;
        }

        if ('h' == opt)
        {
            print_usage_klb_app(p_prog);
            return 1;
        }
        else if ('v' == opt)
        {
            print_version_klb_app(p_prog);
            return 1;
        }
        else if ('p' == opt)
        {
            klb_app_enable_plugins(true);
            klb_app_push_plugins_path(klb_optarg);
        }
        else
        {
            // '?' 等非法选项
            print_usage_klb_app(p_prog);
            return -1;
        }
    }

    if (klb_optind >= argc || NULL == argv[klb_optind] || '\0' == argv[klb_optind][0])
    {
        fprintf(stderr, "%s: missing entry script\n", p_prog);
        print_usage_klb_app(p_prog);
        return -1;
    }

    *p_entry_idx = klb_optind;
    return 0;
}


int klb_app_main(int argc, char** argv, lua_CFunction cb_pre_load, bool has_loop)
{
    // init
    //klb_base_init(NULL);    // klb base

    // step1. app instance
    klb_app_t* p_app = klb_app_instance();
    p_app->has_loop_c = has_loop;

    klbappex_plugins_t* p_appex_plugins = p_app->p_plugins;
    klbappex_klua_t* p_appex_klua = p_app->p_klua;
    int entry_idx = 1;
    int parse_rc = 0;

    klua_env_t* p_klua_env = klbappex_klua_get_klua_env(p_appex_klua);
    klua_env_set_has_loop_c(p_klua_env, has_loop);

    // step1.1 解析壳层参数 (-h/-v/-p); 须在 plugins_preload 之前
    {
        parse_rc = parse_args_klb_app(argc, argv, &entry_idx);
        if (1 == parse_rc)
        {
            return 0;
        }

        if (0 != parse_rc)
        {
            return 1;
        }
    }

    // step2. 初始化 klua 预加载库
    {
        klbappex_klua_push_preload(p_appex_klua, cb_pre_load, NULL);
    }

    // step3. 初始化
    // 准备初始化 其他扩展/插件 等环境
    {
        // 预加载 APP扩展 流程
        on_preload_klb_app(p_app);

        // 预加载(动态库)插件
        klbappex_plugins_preload(p_appex_plugins);
    }

    // step3.5 优雅退出: 须在脚本 dofile 之前装上
    // 脚本内长循环(如 alua_web serve)即主循环, 装晚了永远到不了
    klua_env_install_quit_signal();

    // step4. 初始化完成之后, 为 lua 启动做初始等流程
    // 准备完成之后
    {
        if (0 != klbappex_klua_do_preinit(p_appex_klua, argc, argv, entry_idx))
        {
            return 1;
        }
    }

    // step5. loop, has_loop == true 才 loop
    while (has_loop && 0 == p_app->run_status && !klua_env_is_exit(p_klua_env))
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

bool klb_app_has_loop_c()
{
    if (NULL == g_klb_app)
    {
        return false;
    }

    return g_klb_app->has_loop_c;
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
        p_app->p_preload_mutex = klb_mutex_create();
        p_app->p_preload_nlist = klb_nlist_create();
    }
 
    // 扩展 支持
    {
        p_app->p_extension_rwlock = klb_rwlock_create();
        p_app->p_extension_hlist = klb_hlist_create(0);
        p_app->p_extension_activated_hlist = klb_hlist_create(0);

        p_app->p_loop_rwlock = klb_rwlock_create();
        p_app->p_loop_nlist = klb_nlist_create();
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


        // 初始化
        klbappex_plugins_set_kluaex(p_app->p_plugins, p_app->p_klua);
    }

    return p_app;
}

/// @brief 销毁
/// @return 无
void klb_app_destroy(klb_app_t* p_app)
{
    // 清空 loop 模块
    klb_nlist_clear(p_app->p_loop_nlist, NULL, NULL);

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
    while (0 < klb_nlist_size(p_app->p_preload_nlist))
    {
        klb_nlist_pop_head(p_app->p_preload_nlist);
    }

    // 释放列表
    KLB_FREE_BY(p_app->p_extension_activated_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_app->p_extension_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_app->p_loop_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_app->p_preload_nlist, klb_nlist_destroy);

    KLB_FREE_BY(p_app->p_preload_mutex, klb_mutex_destroy);
    KLB_FREE_BY(p_app->p_extension_rwlock, klb_rwlock_destroy);
    KLB_FREE_BY(p_app->p_loop_rwlock, klb_rwlock_destroy);

    KLB_FREE(p_app);
}

// 预加载流程
static void on_preload_klb_app(klb_app_t* p_app)
{
    // 依次 加载
    while (true)
    {
        klb_mutex_lock(p_app->p_preload_mutex);
        klb_app_preload_cb cb_preload = (klb_app_preload_cb)klb_nlist_pop_head(p_app->p_preload_nlist);
        klb_mutex_unlock(p_app->p_preload_mutex);

        if (NULL != cb_preload)
        {
            cb_preload(p_app);
        }
        else
        {
            break;
        }
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

    klb_rwlock_wrlock(p_app->p_extension_rwlock);
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_app->p_extension_hlist, p_name, name_len, p_tmp);
    if (NULL == p_iter)
    {
        klb_rwlock_wrunlock(p_app->p_extension_rwlock);

        KLB_FREE(p_tmp);

        KLB_LOG_E("register app extension error!name:[%s]\n", p_name);
        return 1; // 放入失败, 名称重复
    }

    klb_rwlock_wrunlock(p_app->p_extension_rwlock);
    return 0;
}

/// @brief 获取app扩展
static void* get_extension_klb_app(klb_app_t* p_app, const char* p_name, klb_app_extension_activated_t** p_out_ex)
{
    assert(NULL != p_app);
    assert(NULL != p_name);

    // 先从激活的里面找
    size_t name_len = strlen(p_name);

    klb_rwlock_rdlock(p_app->p_extension_rwlock);
    klb_app_extension_activated_t* p_activated = (klb_app_extension_activated_t*)klb_hlist_find(p_app->p_extension_activated_hlist, p_name, name_len);
    klb_rwlock_rdunlock(p_app->p_extension_rwlock);

    if (NULL != p_activated)
    {
        if (NULL != p_out_ex) { *p_out_ex = p_activated; };

        return p_activated->ptr;
    }

    // 未找到, 则激活
    klb_rwlock_rdlock(p_app->p_extension_rwlock);
    klb_app_extension_t* p_extension = (klb_app_extension_t*)klb_hlist_find(p_app->p_extension_hlist, p_name, name_len);
    klb_rwlock_rdunlock(p_app->p_extension_rwlock);

    if (NULL != p_extension)
    {
        klb_app_extension_activated_t* p_tmp = KLB_MALLOCZ(klb_app_extension_activated_t, 1, 0);

        // 初始化等
        {
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
        }

        // 放入已激活 链表
        {
            klb_rwlock_wrlock(p_app->p_extension_rwlock);
            klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_app->p_extension_activated_hlist, p_name, name_len, p_tmp);
            assert(NULL != p_iter);
            klb_rwlock_wrunlock(p_app->p_extension_rwlock);
        }

        // 若存在loop, 则放入 loop 链表
        {
            if (NULL != p_tmp->ex.cb_loop_once)
            {
                klb_rwlock_wrlock(p_app->p_loop_rwlock);
                klb_nlist_push_tail(p_app->p_loop_nlist, p_tmp);
                klb_rwlock_wrunlock(p_app->p_loop_rwlock);
            }
        }

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

    klb_mutex_lock(p_app->p_preload_mutex);
    klb_nlist_push_tail(p_app->p_preload_nlist, cb_preload);
    klb_mutex_unlock(p_app->p_preload_mutex);
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

    int sleep_max = klua_env_get_loop_sleep(klbappex_klua_get_klua_env(p_app->p_klua));
    int sleep = 0;

    klb_rwlock_rdlock(p_app->p_loop_rwlock);

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_app->p_loop_nlist);
    while (NULL != p_iter)
    {
        klb_app_extension_activated_t* p_extension = (klb_app_extension_activated_t*)klb_nlist_data(p_iter);

        if (p_extension && p_extension->ex.cb_loop_once)
        {
            sleep += p_extension->ex.cb_loop_once(p_extension->ptr, p_app, tc);
        }

        p_iter = klb_nlist_next(p_iter);
    }

    klb_rwlock_rdunlock(p_app->p_loop_rwlock);

    return sleep_max - sleep;
}

//end
