// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_obj.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_help.h"
#include "klbutil/klb_hlist.h"
#include "klua/extension/klua_ex_gui.h"
#include <assert.h>


#define KLUA_KGUI_module        "kgui"
#define KLUA_KGUI_kwnd          "kgui.kwnd"

typedef struct klua_kwnd_t_
{
    klb_wnd_t*      p_wnd;
}klua_kwnd_t;

//////////////////////////////////////////////////////////////////////////
static uint32_t     g_klua_module = 0;
static uint32_t     g_klua_kwnd = 0;

//////////////////////////////////////////////////////////////////////////


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


static int klua_kgui_attach_canvas(lua_State* L)
{
    klb_canvas_t* ptr = (klb_canvas_t*)luaL_checklightuserdata(L, 1);      ///< @1. 显存画布

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    klb_gui_attach_canvas(p_gui, ptr);

    return 0;
}

static void klua_kgui_msg_callback(void* p_obj, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
{
    klua_ex_gui_t* p_ex = p_obj;

    if (p_ex)
    {
        klb_gui_t* p_gui = klua_ex_gui_get(p_ex);
        klb_gui_push(p_gui, msg, x1, y1, x2, y2, lparam, wparam);
    }
}

static int klua_kgui_get_msg_callback(lua_State* L)
{
    klua_ex_gui_t* p_ex = klua_ex_get_gui_by_L(L);

    lua_pushlightuserdata(L, (void*)klua_kgui_msg_callback);
    lua_pushlightuserdata(L, (void*)p_ex);
    return 2;
}

static int klua_kgui_push(lua_State* L)
{
    int msg = (int)luaL_checkinteger(L, 1);                     ///< @1. 消息
    int x1 = (int)luaL_checkinteger(L, 2);                      ///< @2. 相对父窗口x1坐标
    int y1 = (int)luaL_checkinteger(L, 3);                      ///< @3. 相对父窗口y1坐标
    int x2 = (int)luaL_checkinteger(L, 4);                      ///< @4. 相对父窗口x2坐标
    int y2 = (int)luaL_checkinteger(L, 5);                      ///< @5. 相对父窗口y2坐标
    int lparam = (int)luaL_checkinteger(L, 6);                  ///< @6. 参数1
    int wparam = (int)luaL_checkinteger(L, 7);                  ///< @7. 参数2

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    if (NULL != p_gui)
    {
        klb_gui_push(p_gui, msg, x1, y1, x2, y2, lparam, wparam);
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
    const char* p_type = luaL_checkstring(L, 1);            ///< @1. 窗口/控件类型
    const char* p_path_name = luaL_checkstring(L, 2);       ///< @2. 路径名: eg. "/home/btn1"
    int x = (int)luaL_checkinteger(L, 3);                   ///< @3. 相对父窗口x坐标
    int y = (int)luaL_checkinteger(L, 4);                   ///< @4. 相对父窗口y坐标
    int w = (int)luaL_checkinteger(L, 5);                   ///< @5. 相对父窗口宽
    int h = (int)luaL_checkinteger(L, 6);                   ///< @6. 相对父窗口高

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    klb_wnd_t* p_wnd = NULL;
    int ret = klb_gui_append(p_gui, p_type, p_path_name, x, y, w, h, &p_wnd);

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
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    return 0;
}

static int klua_kgui_bind_command(lua_State* L)
{
    klua_ex_gui_t* p_ex = klua_ex_get_gui_by_L(L);

    const char* p_path_name = luaL_checkstring(L, 1);                   ///< @1. 路径名: eg. "/home/btn1"
    int ret = klua_ex_gui_bind_command(p_ex, p_path_name, 2);           ///< @2. Lua函数

    lua_pushinteger(L, ret);                                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_set(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    const char* p_json = luaL_checkstring(L, 2);        ///< @2. JSON参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_set(p_gui, p_path_name, p_json);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_get(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    const char* p_json = luaL_checkstring(L, 2);        ///< @2. JSON参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    char* p_str = klb_gui_get(p_gui, p_path_name, p_json);
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

static int klua_kgui_do_model(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_do_model(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_end_model(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_end_model(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_end_model_all(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_end_model_all(p_gui);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

int klua_open_kgui(lua_State* L)
{
    static luaL_Reg kgui_lib[] =
    {
        { "get_msg_callback",   klua_kgui_get_msg_callback },
        { "attach_canvas",      klua_kgui_attach_canvas },

        { "push",               klua_kgui_push },

        { "append",             klua_kgui_append },
        { "remove",             klua_kgui_remove },
        { "bind_command",       klua_kgui_bind_command },
        { "set",                klua_kgui_set },
        { "get",                klua_kgui_get },

        { "do_model",           klua_kgui_do_model },
        { "end_model",          klua_kgui_end_model },
        { "end_model_all",      klua_kgui_end_model_all },

        { NULL,                 NULL }
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
