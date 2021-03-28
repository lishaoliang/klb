#include "klb_type.h"
#include "klua/klua.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_obj.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_help.h"
#include "klbutil/klb_hlist.h"
#include <assert.h>


#define KLUA_KGUI_module        "kgui"
#define KLUA_KGUI_kwnd          "kgui.kwnd"


typedef struct klua_kgui_t 
{
    klb_gui_t*      p_gui;

    klua_env_t*     p_env;
    lua_State*      L;

    klb_hlist_t*    p_bind_hlist;   ///< klua_kgui_bind_t*
}klua_kgui_t;

typedef struct klua_kgui_bind_t_
{
    klua_kgui_t*    p_kgui;
    int             on_command;     ///< 绑定的 lua函数
}klua_kgui_bind_t;

typedef struct klua_kwnd_t_
{
    klb_wnd_t*      p_wnd;
}klua_kwnd_t;

//////////////////////////////////////////////////////////////////////////

static klua_kgui_t* g_klua_kgui = NULL;
static uint32_t     g_klua_module = 0;
static uint32_t     g_klua_kwnd = 0;

//////////////////////////////////////////////////////////////////////////


static klua_kgui_t* klua_kgui_create(klua_env_t* p_env)
{
    klua_kgui_t* p_kgui = KLB_MALLOC(klua_kgui_t, 1, 0);
    KLB_MEMSET(p_kgui, 0, sizeof(klua_kgui_t));

    p_kgui->p_gui = klb_gui_create(NULL);
    p_kgui->p_bind_hlist = klb_hlist_create(10000);

    return p_kgui;
}

static void klua_kgui_destroy(klua_kgui_t* p_kgui)
{
    assert(NULL != p_kgui);

    while (0 < klb_hlist_size(p_kgui->p_bind_hlist))
    {
        klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)klb_hlist_pop_head(p_kgui->p_bind_hlist);
        if (0 < p_bind->on_command)
        {
            luaL_unref(p_kgui->L, LUA_REGISTRYINDEX, p_bind->on_command);
            p_bind->on_command = 0;
        }

        KLB_FREE(p_bind);
    }

    KLB_FREE_BY(p_kgui->p_bind_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_kgui->p_env, klua_env_destroy);
    KLB_FREE_BY(p_kgui->p_gui, klb_gui_destroy);
    KLB_FREE(p_kgui);
}


static int cb_klua_kgui_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    assert(NULL != p_wnd);
    klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)p_wnd->p_udata;
    assert(NULL != p_bind);

    if (p_bind->on_command <= 0)
    {
        return 0;
    }

    lua_State* L = p_bind->p_kgui->L;
    KLUA_HELP_TOP_B(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, p_bind->on_command);      // @0. 压入命令
    lua_pushlightuserdata(L, p_wnd);                            // @1. 窗口对象
    lua_pushinteger(L, msg);                                    // @2. 消息
    lua_pushinteger(L, p_pt1->x);                               // @3. 点1的坐标x
    lua_pushinteger(L, p_pt1->y);                               // @4. 点1的坐标y
    lua_pushinteger(L, p_pt2->x);                               // @5. 点2的坐标x
    lua_pushinteger(L, p_pt2->y);                               // @6. 点2的坐标y
    lua_pushinteger(L, lparam);                                 // @7. 参数1
    lua_pushinteger(L, wparam);                                 // @8. 参数2

    if (LUA_OK != lua_pcall(L, 8, 1, 0))
    {
        klua_help_report(L, LUA_ERRRUN);
        KLUA_HELP_TOP_E(L);
        return -1;
    }

    int ret = (int)luaL_checkinteger(L, -1);                    // #1. 0. 消息终止,不再"冒泡"; msg. 任然以msg"冒泡"; 非0. 转换为其他消息"冒泡"
    lua_pop(L, 1);

    KLUA_HELP_TOP_E(L);
    return ret;
}

static int klua_kgui_bind_command_in(klua_kgui_t* p_kgui, const char* p_path_name, int idx)
{
    assert(NULL != g_klua_kgui);
    luaL_checktype(p_kgui->L, idx, LUA_TFUNCTION);
    lua_pushvalue(p_kgui->L, idx);

    int func = luaL_ref(p_kgui->L, LUA_REGISTRYINDEX);
    assert(0 < func);

    bool insert = false;
    int name_len = strlen(p_path_name);

    klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)klb_hlist_find(p_kgui->p_bind_hlist, p_path_name, name_len);
    if (NULL == p_bind)
    {
        p_bind = KLB_MALLOC(klua_kgui_bind_t, 1, 0);
        KLB_MEMSET(p_bind, 0, sizeof(klua_kgui_bind_t));

        p_bind->p_kgui = p_kgui;
        insert = true;
    }
    else
    {
        // 已经存在响应函数, 则删除原来的响应函数
        if (0 < p_bind->on_command)
        {
            luaL_unref(p_kgui->L, LUA_REGISTRYINDEX, p_bind->on_command);
            p_bind->on_command = 0;
        }
    }

    p_bind->on_command = func;

    int ret = klb_gui_bind_command(g_klua_kgui->p_gui, p_path_name, cb_klua_kgui_on_command, p_bind);

    if (0 == ret)
    {
        if (insert)
        {
            klb_hlist_push_tail(p_kgui->p_bind_hlist, p_path_name, name_len, p_bind);
        }
    }
    else
    {
        if (0 < p_bind->on_command)
        {
            luaL_unref(p_kgui->L, LUA_REGISTRYINDEX, p_bind->on_command);
            p_bind->on_command = 0;
        }
        KLB_FREE(p_bind);
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////
static klb_obj_t* klua_newuserdata_kwnd(lua_State* L, klb_wnd_t* p_wnd)
{
    int obj_len = sizeof(klb_obj_t) + sizeof(klua_kwnd_t);
    klb_obj_t* p_obj = (klb_obj_t*)lua_newuserdata(L, obj_len);
    memset(p_obj, 0, obj_len);

    p_obj->module = g_klua_module;
    p_obj->name = g_klua_kwnd;

    klua_kwnd_t* p_kwnd = (klua_kwnd_t*)p_obj->obj;
    p_kwnd->p_wnd;

    return p_obj;
}

static klua_kwnd_t* klua_check_kwnd(lua_State* L, int idx)
{
    luaL_checktype(L, idx, LUA_TUSERDATA);
    klb_obj_t* p_obj = (klb_obj_t*)lua_touserdata(L, idx);

    if (p_obj->module == g_klua_module &&
        p_obj->name == g_klua_kwnd)
    {
        return (klua_kwnd_t*)p_obj->obj;
    }

    luaL_argerror(L, idx, KLUA_KGUI_kwnd);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// kgui lua 接口

static int klua_kgui_init(lua_State* L)
{
    assert(NULL == g_klua_kgui);
    const char* p_json = luaL_checkstring(L, 1);            ///< @1. 参数

    g_klua_kgui = klua_kgui_create(NULL);

    return 0;
}

static int klua_kgui_start(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    const char* p_name = luaL_checkstring(L, 1);            ///< @1. 线程名称
    const char* p_entry = luaL_checkstring(L, 2);           ///< @2. 加载入口脚本; eg.'xxx.xxx'

    lua_CFunction preload = NULL;
    if (LUA_TLIGHTUSERDATA == lua_type(L, 3))
    {
        preload = (lua_CFunction)lua_touserdata(L, 3);      ///< @3. 预加库载函数
    }

    lua_CFunction loader = NULL;
    if (LUA_TLIGHTUSERDATA == lua_type(L, 4))
    {
        loader = (lua_CFunction)lua_touserdata(L, 4);       ///< @4. 自定义加载脚本文件函数
    }

    g_klua_kgui->p_env = klua_env_create(preload);
    g_klua_kgui->L = klua_env_get_L(g_klua_kgui->p_env);
    klua_env_set_udata(g_klua_kgui->p_env, g_klua_kgui);
    klua_env_dolibrary(g_klua_kgui->p_env, p_entry);

    klb_gui_start(g_klua_kgui->p_gui);

    lua_pushboolean(L, true);                               ///< #1. 是否成功
    lua_pushstring(L, "");                                  ///< #2. 错误字符串
    return 2;
}

static int klua_kgui_stop(lua_State* L)
{
    if (NULL != g_klua_kgui && NULL != g_klua_kgui->p_gui)
    {
        klb_gui_stop(g_klua_kgui->p_gui);
    }

    return 0;
}

static int klua_kgui_quit(lua_State* L)
{
    KLB_FREE_BY(g_klua_kgui, klua_kgui_destroy);

    return 0;
}

static int klua_kgui_attach_canvas(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    klb_canvas_t* ptr = (klb_canvas_t*)luaL_checklightuserdata(L, 1);      ///< @1. 显存画布

    klb_gui_attach_canvas(g_klua_kgui->p_gui, ptr);

    return 0;
}

static void klua_kgui_msg_callback(int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
{
    if (NULL != g_klua_kgui && NULL != g_klua_kgui->p_gui)
    {
        klb_gui_push(g_klua_kgui->p_gui, msg, x1, y1, x2, y2, lparam, wparam);
    }
}

static int klua_kgui_get_msg_callback(lua_State* L)
{
    assert(NULL != g_klua_kgui);

    lua_pushlightuserdata(L, (void*)klua_kgui_msg_callback);
    return 1;
}

static int klua_kgui_push(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    int msg = (int)luaL_checkinteger(L, 1);                     ///< @1. 消息
    int x1 = (int)luaL_checkinteger(L, 2);                      ///< @2. 相对父窗口x1坐标
    int y1 = (int)luaL_checkinteger(L, 3);                      ///< @3. 相对父窗口y1坐标
    int x2 = (int)luaL_checkinteger(L, 4);                      ///< @4. 相对父窗口x2坐标
    int y2 = (int)luaL_checkinteger(L, 5);                      ///< @5. 相对父窗口y2坐标
    int lparam = (int)luaL_checkinteger(L, 6);                  ///< @6. 参数1
    int wparam = (int)luaL_checkinteger(L, 7);                  ///< @7. 参数2

    if (NULL != g_klua_kgui && NULL != g_klua_kgui->p_gui)
    {
        klb_gui_push(g_klua_kgui->p_gui, msg, x1, y1, x2, y2, lparam, wparam);
        lua_pushinteger(L, 0);
    }
    else
    {
        lua_pushinteger(L, 1);
    }

    return 1;
}

static int klua_kgui_append(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    const char* p_type = luaL_checkstring(L, 1);            ///< @1. 窗口/控件类型
    const char* p_path_name = luaL_checkstring(L, 2);       ///< @2. 路径名: eg. "/home/btn1"
    int x = (int)luaL_checkinteger(L, 3);                   ///< @3. 相对父窗口x坐标
    int y = (int)luaL_checkinteger(L, 4);                   ///< @4. 相对父窗口y坐标
    int w = (int)luaL_checkinteger(L, 5);                   ///< @5. 相对父窗口宽
    int h = (int)luaL_checkinteger(L, 6);                   ///< @6. 相对父窗口高

    klb_wnd_t* p_wnd = NULL;
    int ret = klb_gui_append(g_klua_kgui->p_gui, p_type, p_path_name, x, y, w, h, &p_wnd);

    lua_pushinteger(L, ret);                                ///< #1. 0.成功; 非0.失败(错误码)

    if (0 == ret)
    {
        klua_newuserdata_kwnd(L, p_wnd);                    ///< #2.
    }
    else
    {
        lua_pushnil(L);
    }

    return 2;
}

static int klua_kgui_remove(lua_State* L)
{
    assert(NULL != g_klua_kgui);

    return 0;
}

static int klua_kgui_bind_command(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    const char* p_path_name = luaL_checkstring(L, 1);                   ///< @1. 路径名: eg. "/home/btn1"
    int ret = klua_kgui_bind_command_in(g_klua_kgui, p_path_name, 2);   ///< @2. Lua函数

    lua_pushinteger(L, ret);                                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_set(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    const char* p_json = luaL_checkstring(L, 2);        ///< @2. JSON参数

    int ret = klb_gui_set(g_klua_kgui->p_gui, p_path_name, p_json);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_get(lua_State* L)
{
    assert(NULL != g_klua_kgui);
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    const char* p_json = luaL_checkstring(L, 2);        ///< @2. JSON参数

    char* p_str = klb_gui_get(g_klua_kgui->p_gui, p_path_name, p_json);
    if (NULL != p_str)
    {
        lua_pushstring(L, p_str);                       ///< #1. JSON回复
        KLB_FREE(p_str);
    }
    else
    {
        lua_pushstring(L, "");
    }

    return 1;
}

int klua_open_kgui(lua_State* L)
{
    static luaL_Reg kgui_lib[] =
    {
        { "init",       klua_kgui_init },
        { "start",      klua_kgui_start },
        { "stop",       klua_kgui_stop },
        { "quit",       klua_kgui_quit },

        { "attach_canvas",      klua_kgui_attach_canvas },
        { "get_msg_callback",   klua_kgui_get_msg_callback },

        { "push",           klua_kgui_push },

        { "append",         klua_kgui_append },
        { "remove",         klua_kgui_remove },
        { "bind_command",   klua_kgui_bind_command },
        { "set",            klua_kgui_set },
        { "get",            klua_kgui_get },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kgui_lib);

    // 初始化hash
    g_klua_module = klb_obj_hash(KLUA_KGUI_module, strlen(KLUA_KGUI_module));
    g_klua_kwnd   = klb_obj_hash(KLUA_KGUI_kwnd, strlen(KLUA_KGUI_kwnd));

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// kwnd

static int klua_kwnd_set(lua_State* L)
{
    klua_kwnd_t* p_kwnd = klua_check_kwnd(L, 1);

    return 0;
}

static int klua_kwnd_get(lua_State* L)
{
    return 0;
}

int klua_open_kwnd(lua_State* L)
{
    static luaL_Reg kwnd_lib[] =
    {
        { "set",            klua_kwnd_set },
        { "get",            klua_kwnd_get },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kwnd_lib);

    return 1;
}
