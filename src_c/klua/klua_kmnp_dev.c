#include "klua/klua.h"
#include "mnp/klb_mnp_stream.h"
#include "mnp/klb_mnp_dev.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include "platform/klb_dynamic_link.h"
#include <string.h>


static klb_dl_t*                g_mnp_dev_dl = NULL;
static klb_mnp_dev_interface_t  g_mnp_dev_interface = { 0 };


static int klua_kmnp_dev_dlopen(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);    ///< @1. dll/so 路径
    int open = 1;

#if defined(__KLB_USE_KMNP_DEV_STATIC__)
    open = klb_mnp_dev_open(KLB_MNP_DEV_V1, &g_mnp_dev_interface, sizeof(g_mnp_dev_interface));
#else
    if (NULL == g_mnp_dev_dl)
    {
        g_mnp_dev_dl = klb_dlopen(p_path);
        KLB_LOG("klb_dlopen path:[%s],ptr:[0x%p]\n", p_path, g_mnp_dev_dl);
    }

    if (NULL != g_mnp_dev_dl)
    {
        klb_mnp_dev_open_interface open_interface = (klb_mnp_dev_open_interface)klb_dlsym(g_mnp_dev_dl, "klb_mnp_dev_open");
        KLB_LOG("klb_dlsym sym:[klb_mnp_dev_open],ptr:[0x%p]\n", open_interface);

        if (NULL != open_interface)
        {
            open = open_interface(KLB_MNP_DEV_V1, &g_mnp_dev_interface, sizeof(g_mnp_dev_interface));
            KLB_LOG("klb_mnp_dev_open ret:[%d]\n", open);
        }
    }
#endif

    lua_pushinteger(L, open);                      ///< #1. 0.成功; 非0.失败
    return 1;
}

static int klua_kmnp_dev_dlclose(lua_State* L)
{
#if defined(__KLB_USE_KMNP_DEV_STATIC__)

#else
    memset(&g_mnp_dev_interface, 0, sizeof(g_mnp_dev_interface));

    KLB_LOG("klb_dlclose ptr:[0x%p]\n", g_mnp_dev_dl);
    KLB_FREE_BY(g_mnp_dev_dl, klb_dlclose);
#endif

    return 0;
}

static int klua_kmnp_dev_version(lua_State* L)
{
    uint32_t version = 0;
    if (g_mnp_dev_interface.version)
    {
        version = g_mnp_dev_interface.version();
    }

    uint32_t v1 = 0, v2 = 0, v3 = 0;
    v1 = version / 10000;
    
    uint32_t v23 = version % 10000;
    v2 = v23 / 100;
    v3 = v23 % 100;

    lua_pushfstring(L, "v%d.%d.%d", v1, v2, v3);   ///< #1. 字符串版本
    lua_pushinteger(L, version);                   ///< #2. 数值版本
    return 2;
}

static int klua_kmnp_dev_project(lua_State* L)
{
    static const char* p_emulator = "emulator";

    char* p_project = (char*)(p_emulator);
    if (g_mnp_dev_interface.project)
    {
        p_project = (char*)g_mnp_dev_interface.project();
    }

    lua_pushstring(L, p_project);                   ///< #1. 项目名
    return 1;
}

static int klua_kmnp_dev_init(lua_State* L)
{
    const char* p_json = luaL_checkstring(L, 1);    ///< @1. 初始化json参数
    
    int ret = 1;
    if (g_mnp_dev_interface.init)
    {
        ret = g_mnp_dev_interface.init(p_json);
    }

    lua_pushinteger(L, ret);                         ///< #1. 0.成功; 非0.失败
    return 1;
}

static int klua_kmnp_dev_start(lua_State* L)
{
    const char* p_json = luaL_checkstring(L, 1);    ///< @1. 启动工作线程json参数

    int ret = 1;
    if (g_mnp_dev_interface.start)
    {
        ret = g_mnp_dev_interface.start(p_json);
    }

    lua_pushinteger(L, ret);                         ///< #1. 0.成功; 非0.失败
    return 1;
}

static int klua_kmnp_dev_stop(lua_State* L)
{
    if (g_mnp_dev_interface.stop)
    {
        g_mnp_dev_interface.stop();
    }

    return 0;
}

static int klua_kmnp_dev_quit(lua_State* L)
{
    if (g_mnp_dev_interface.quit)
    {
        g_mnp_dev_interface.quit();
    }

    return 0;
}

static int klua_kmnp_dev_control(lua_State* L)
{
    const char* p_cmd = luaL_checkstring(L, 1);     ///< @1. 命令
    const char* p_lparam = luaL_checkstring(L, 2);  ///< @2. json参数1
    const char* p_wparam = luaL_checkstring(L, 3);  ///< @3. json参数2

    char* p_ret = NULL;
    if (g_mnp_dev_interface.control)
    {
        p_ret = g_mnp_dev_interface.control(p_cmd, p_lparam, p_wparam);
    }

    if (NULL != p_ret)
    {
        lua_pushstring(L, p_ret);                   ///< #1. json回复
        g_mnp_dev_interface.free(p_ret);
    }
    else
    {
        lua_pushstring(L, "");
    }

    return 1;
}

static int klua_kmnp_dev_set_stream_receiver(lua_State* L)
{
    int index = (int)luaL_checkinteger(L, 1);       ///< @1. 序号
    void* ptr = luaL_checklightuserdata(L, 2);      ///< @2. 流回调函数

    if (g_mnp_dev_interface.set_stream_receiver)
    {
        g_mnp_dev_interface.set_stream_receiver(index, (klb_mnp_stream_cb)ptr);
    }

    return 0;
}

static int klua_kmnp_dev_get_canvas(lua_State* L)
{
    int index = (int)luaL_checkinteger(L, 1);       ///< @1. 序号[0, 16)

    klb_canvas_t* p_canvas = NULL;
    if (g_mnp_dev_interface.get_canvas)
    {
        p_canvas = g_mnp_dev_interface.get_canvas(index);
    }

    if (NULL != p_canvas)
    {
        lua_pushlightuserdata(L, p_canvas);         ///< #1. canvas指针
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int klua_kmnp_dev_set_msg_receiver(lua_State* L)
{
    int index = (int)luaL_checkinteger(L, 1);       ///< @1. 序号
    void* ptr = luaL_checklightuserdata(L, 2);      ///< @2. 消息回调函数

    if (g_mnp_dev_interface.set_msg_receiver)
    {
        g_mnp_dev_interface.set_msg_receiver(index, (klb_mnp_msg_cb)ptr);
    }

    return 0;
}

int klua_open_kmnp_dev(lua_State* L)
{
    static luaL_Reg kmnp_dev_lib[] =
    {
        { "dlopen",     klua_kmnp_dev_dlopen },
        { "dlclose",    klua_kmnp_dev_dlclose },

        { "version",    klua_kmnp_dev_version },
        { "project",    klua_kmnp_dev_project },

        { "init",       klua_kmnp_dev_init },
        { "start",      klua_kmnp_dev_start },
        { "stop",       klua_kmnp_dev_stop },
        { "quit",       klua_kmnp_dev_quit },

        { "control",                klua_kmnp_dev_control },
        { "set_stream_receiver",    klua_kmnp_dev_set_stream_receiver },
        { "get_canvas",             klua_kmnp_dev_get_canvas },
        { "set_msg_receiver",       klua_kmnp_dev_set_msg_receiver },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kmnp_dev_lib);

    return 1;
}
