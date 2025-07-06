#include "klbapp/klbappex_plugins.h"
#include "klbapp/klbappex_plugins_in.h"
#include "klbapp/klb_app.h"
#include "klbmem/klb_mem.h"
#include "klbthird/sds.h"
#include "klbplatform/klb_dynamic_link.h"
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <io.h>
#include <sys/locking.h>
#include <sys/utime.h>
#include <fcntl.h>
/* MAX_PATH seems to be 260. Seems kind of small. Is there a better one? */
#define MAXPATHLEN MAX_PATH
#else
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/param.h> /* for MAXPATHLEN */
#define MAXPATHLEN MAXPATHLEN
#endif


#ifdef _WIN32
#ifndef S_ISDIR
#define S_ISDIR(mode)  (mode&_S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode)  (mode&_S_IFREG)
#endif
#ifndef S_ISLNK
#define S_ISLNK(mode)  (0)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(mode)  (0)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode)  (0)
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode)  (mode&_S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(mode)  (0)
#endif
#endif


/// @def   KLBAPPEX_PLUGINS
/// @brief 扩展名称
#define KLBAPPEX_PLUGINS        "KLBAPPEX-plugins"


//////////////////////////////////////////////////////////////////////////
// 内部: 扩展接口函数


/// @brief 创建扩展
static void* klbappex_plugins_create(klb_app_t* p_app)
{
    klbappex_plugins_t* p_appex = KLB_MALLOCZ(klbappex_plugins_t, 1, 0);

    p_appex->enable = false;    // 默认: 不启用 加载动态库插件

    p_appex->p_app = p_app;
    p_appex->p_kluaex = NULL;

    p_appex->p_path_mutex = klb_mutex_create();
    p_appex->p_path_nlist = klb_nlist_create();

    p_appex->p_dl_nlist = klb_nlist_create();

    return p_appex;
}

/// @brief 销毁扩展
static void klbappex_plugins_destroy(void* ptr, klb_app_t* p_app)
{
    klbappex_plugins_t* p_appex = (klbappex_plugins_t*)ptr;

    // 清空 动态库列表
    while (0 < klb_nlist_size(p_appex->p_dl_nlist))
    {
        klbappex_plugins_dl_t* p_dl = (klbappex_plugins_dl_t*)klb_nlist_pop_tail(p_appex->p_dl_nlist);

        // 动态库的 退出函数
        if (NULL != p_dl->cb_quit)
        {
            p_dl->cb_quit();
        }

        KLB_FREE_BY(p_dl->p_dl, klb_dlclose);

        KLB_FREE_BY(p_dl->path, sdsfree);
        KLB_FREE(p_dl);
    }


    // 清空 路径链表
    while (0 < klb_nlist_size(p_appex->p_path_nlist))
    {
        sds path = (sds)klb_nlist_pop_head(p_appex->p_path_nlist);
        KLB_FREE_BY(path, sdsfree);
    }

    KLB_FREE_BY(p_appex->p_dl_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_appex->p_path_nlist, klb_nlist_destroy);

    KLB_FREE_BY(p_appex->p_path_mutex, klb_mutex_destroy);

    KLB_FREE(p_appex);
}

/// @brief 控制操作消息
static int klbappex_plugins_control(void* ptr, klb_app_t* p_app, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbappex_plugins_t* p_appex = (klbappex_plugins_t*)ptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 内部

void klbappex_plugins_set_kluaex(klbappex_plugins_t* p_appex, klbappex_klua_t* p_kluaex)
{
    p_appex->p_kluaex = p_kluaex;
}

/// @brief 设置是否启用 (动态库)插件plugins
///   默认: 未启用
void klbappex_plugins_enable(klbappex_plugins_t* p_appex, bool enable)
{
    p_appex->enable = enable;
}

/// @brief 若启用(动态库)插件, 加载动态库插件的路径
void klbappex_plugins_push_path(klbappex_plugins_t* p_appex, const char* p_path_plugins)
{
    if (NULL == p_path_plugins)
    {
        return;
    }

    // 放入链表
    sds path = sdsnew(p_path_plugins);

    klb_mutex_lock(p_appex->p_path_mutex);
    klb_nlist_push_tail(p_appex->p_path_nlist, path);
    klb_mutex_unlock(p_appex->p_path_mutex);
}

// 尝试 打开插件
static bool try_open_klbappex_plugins(klbappex_plugins_t* p_appex, sds filepath, klb_dl_t* p_dl)
{
    // 动态库 入口函数
    klbapp_init_extension_cb init_extension = (klbapp_init_extension_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_init_extension);
    klbapp_quit_extension_cb quit_extension = (klbapp_quit_extension_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_quit_extension);
    klbapp_extension_count_cb extension_count = (klbapp_extension_count_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_extension_count);
    klbapp_open_extension_cb open_extension = (klbapp_open_extension_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_open_extension);
    klbapp_kluaprelib_count_cb kluaprelib_count = (klbapp_kluaprelib_count_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_kluaprelib_count);
    klbapp_open_kluaprelib_cb open_kluaprelib = (klbapp_open_kluaprelib_cb)klb_dlsym(p_dl, KLBAPPEX_DLSYM_open_kluaprelib);

    // 打开 APP 扩展
    int open_count = 0;

    // 要求: 
    // 1. extension_count / open_extension 这两个函数 必须存在
    // 2. 可以 打开扩展, 并 至少 注册 成功 一个
    if (NULL != extension_count && NULL != open_extension)
    {
        char name[KLBAPPEX_DLSYM_name_max + 4] = { 0 };

        // 获取扩展数目; 并依次打开
        int count = extension_count();
        for (int i = 0; i < count; i++)
        {
            klb_app_extension_t ex = { 0 };
            if (0 == open_extension(i, &ex, name, KLBAPPEX_DLSYM_name_max))
            {
                // 打开成功 且 必须有函数 cb_create / cb_destroy
                if (NULL != ex.cb_create && NULL != ex.cb_destroy)
                {
                    // 注册 扩展
                    if (0 == klb_app_register_extension(p_appex->p_app, name, &ex))
                    {
                        open_count += 1;    // 注册成功
                    }
                }
            }
        }
    }

    // 打开 klua 预加载库
    int prelib_count = 0;

    if (NULL != kluaprelib_count && NULL != open_kluaprelib)
    {
        int num = kluaprelib_count();

        for (int k = 0; k < num; k++)
        {
            lua_CFunction cb_prelib = NULL;
            if (0 == open_kluaprelib(k, &cb_prelib) && NULL != cb_prelib)
            {
                // 打开 klua 扩展之后, 直接放入预加载列表中 
                klbappex_klua_push_preload(p_appex->p_kluaex, cb_prelib);

                prelib_count += 1; // 获取成功
            }
        }
    }

    if (0 < open_count || 0 < prelib_count)
    {
        // 至少 打开一个扩展 或 一个 lua预加载库

        // 放入链表
        klbappex_plugins_dl_t* p_plugins = KLB_MALLOCZ(klbappex_plugins_dl_t, 1, 0);

        p_plugins->p_dl = p_dl;
        p_plugins->path = sdsnew(filepath);
        p_plugins->open_count = open_count;
        p_plugins->prelib_count = prelib_count;

        p_plugins->cb_init = init_extension;
        p_plugins->cb_quit = quit_extension;
        p_plugins->cb_count = extension_count;
        p_plugins->cb_open = open_extension;
        p_plugins->cb_prelib_count = kluaprelib_count;
        p_plugins->cb_open_prelib = open_kluaprelib;

        klb_nlist_push_tail(p_appex->p_dl_nlist, p_plugins);

        // 调用 动态库的初始化 函数
        if (NULL != init_extension)
        {
            init_extension();
        }

        return true;
    }

    return false;
}

// 加载单个 动态库文件
static void load_file_klbappex_plugins(klbappex_plugins_t* p_appex, sds filepath)
{
    klb_dl_t* p_dl = klb_dlopen(filepath);
    if (NULL == p_dl)
    {
        return; // 打开动态库失败
    }

    bool is_open = try_open_klbappex_plugins(p_appex, filepath, p_dl);

    if (!is_open)
    {
        klb_dlclose(p_dl);
    }
}

#ifdef _WIN32

// 按路径 加载 目录下的 所有动态库
static void load_by_path_klbappex_plugins(klbappex_plugins_t* p_appex, sds dir)
{
    if (NULL == dir)
    {
        return;
    }

    sds path = sdsnew(dir);
    path = sdscat(path, "/*.dll");

    struct _finddata_t c_file = { 0 };
    intptr_t fd = _findfirst(path, &c_file);

    if (-1L == fd)
    {
        KLB_FREE_BY(path, sdsfree);
        return ; // 未打开
    }

    sds filepath = sdsempty();

    while (true)
    {
        if (_findnext(fd, &c_file) == -1L)
        {
            break;
        }
        else
        {
            if (0 == strcmp(c_file.name, ".") || 0 == strcmp(c_file.name, ".."))
            {
                // 自身
                continue;
            }
            else
            {
                // 路径名
                filepath = sdscpy(filepath, dir);
                filepath = sdscat(filepath, "/");
                filepath = sdscat(filepath, c_file.name);

                struct _stat buf;
                int result = _stat(filepath, &buf);

                if (0 == result)
                {
                    if (S_ISDIR(buf.st_mode))
                    {
                        // 目录
                    }
                    else if (S_ISREG(buf.st_mode))
                    {
                        // 文件
                        load_file_klbappex_plugins(p_appex, filepath);
                    }
                }
            }
        }
    }

    // 关闭
    _findclose(fd);

    KLB_FREE_BY(filepath, sdsfree);
    KLB_FREE_BY(path, sdsfree);
}

#else

// 检查文件扩展名是否匹配
static int check_extension_klbappex_plugins(const char* p_filename, const char* p_extension)
{
    const char* p_dot = strrchr(p_filename, '.');
    if (p_dot && 0 == strcmp(p_dot + 1, p_extension))
    {
        return 0;
    }

    return 1;
}

// 按路径 加载 目录下的 所有动态库
static void load_by_path_klbappex_plugins(klbappex_plugins_t* p_appex, sds path)
{
    DIR* dir = opendir(path);
    if (NULL == dir)
    {
        return; // 未打开
    }

    sds filepath = sdsempty();

    while (true)
    {
        struct dirent* p_entry = readdir(dir);

        if (NULL == p_entry)
        {
            break;
        }

        if (0 == strcmp(p_entry->d_name, ".") || 0 == strcmp(p_entry->d_name, ".."))
        {
            continue; // 当前目录
        }

        filepath = sdscpy(filepath, path);
        filepath = sdscat(filepath, "/");
        filepath = sdscat(filepath, p_entry->d_name);

        struct stat buf;
        int result = stat(filepath, &buf);

        if (0 == result)
        {
            if (S_ISDIR(buf.st_mode))
            {
                // 目录
            }
            else if (S_ISREG(buf.st_mode))
            {
                // 文件
                if (0 == check_extension_klbappex_plugins(p_entry->d_name, "so"))
                {
                    load_file_klbappex_plugins(p_appex, filepath);
                }
            }
        }
    }

    KLB_FREE_BY(filepath, sdsfree);
    closedir(dir);
}

#endif

// 预加载(动态库)插件
void klbappex_plugins_preload(klbappex_plugins_t* p_appex)
{
    if (!p_appex->enable)
    {
        return; // 未启用
    }

    // 从路径列表中依次加载 动态库
    while (true)
    {
        klb_mutex_lock(p_appex->p_path_mutex);
        sds dir = klb_nlist_pop_head(p_appex->p_path_nlist);
        klb_mutex_unlock(p_appex->p_path_mutex);

        if (NULL != dir)
        {
            // 有目录, 则加载目录所有 动态库
            load_by_path_klbappex_plugins(p_appex, dir);

            KLB_FREE_BY(dir, sdsfree);
        }
        else
        {
            break; // 结束了
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// 导出函数




//////////////////////////////////////////////////////////////////////////
// 注册 / 获取

/// @brief 获取 klua
/// @return klbappex_plugins_t*
klbappex_plugins_t* klbappex_get_plugins(klb_app_t* p_app)
{
    return (klbappex_plugins_t*)klb_app_get_extension(p_app, KLBAPPEX_PLUGINS);
}

klbappex_plugins_t* klbappex_get_plugins2()
{
    klb_app_t* p_app = klb_app_instance();
    return klbappex_get_plugins(p_app);
}

/// @brief 注册 plugins 扩展
int klbappex_register_plugins(klb_app_t* p_app)
{
    klb_app_extension_t ex = { 0 };

    ex.cb_create = klbappex_plugins_create;
    ex.cb_destroy = klbappex_plugins_destroy;
    ex.cb_control = klbappex_plugins_control;
    ex.cb_loop_once = NULL;

    klb_app_register_extension(p_app, KLBAPPEX_PLUGINS, &ex);

    return 0;
}

//end
