// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_wnd.h"
#include "klbutil/klb_obj.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_help.h"
#include "klbutil/klb_hlist.h"
#include "klua/extension/klua_extension.h"
#include "klua/extension/klua_ex_gui.h"
#include "klbutil/klb_map.h"
#include "klua/klua_seri.h"
#include "klua/klua_gui.h"
#include "klbgui/extensions/klbuiex_time.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////


#define KLUA_KWND_HANDLE           "KLUA_KWND_HANDLE*"    ///< Lua meta标示


/// @struct klua_kwnd_t
/// @brief  窗口
///    [2025/7+]重新设计 kwnd 接口
///    1. 由kgui.get_kwnd()接口生成
///    2. 与一个 窗口(klb_wnd_t*) 对应
typedef struct klua_kwnd_t_
{
    klb_wnd_t*      p_wnd;      ///< 窗口
}klua_kwnd_t;

//////////////////////////////////////////////////////////////////////////

static klua_kwnd_t* new_klua_kwnd(lua_State* L)
{
    klua_kwnd_t* p_kwnd = (klua_kwnd_t*)lua_newuserdata(L, sizeof(klua_kwnd_t));
    KLB_MEMSET(p_kwnd, 0, sizeof(klua_kwnd_t));
    luaL_setmetatable(L, KLUA_KWND_HANDLE);

    return p_kwnd;
}

static klua_kwnd_t* to_klua_kwnd(lua_State* L, int index)
{
    klua_kwnd_t* p_kwnd = (klua_kwnd_t*)luaL_checkudata(L, index, KLUA_KWND_HANDLE);
    luaL_argcheck(L, NULL != p_kwnd, index, "'kwnd' expected");
    return p_kwnd;
}

static int klua_kwnd_gc(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);

    p_kwnd->p_wnd = NULL;

    return 0;
}

static int klua_kwnd_tostring(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);

    lua_pushfstring(L, "kwnd:%p", p_kwnd);
    return 1;
}

///////////////////////////////////

// 设置/获取 样式
static int klua_kwnd_style(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    uint32_t style = 0;
    int idx = 2;

    if (klua_is_integer(L, idx))
    {
        // 设置
        style = (uint32_t)lua_tointeger(L, idx);

        klb_wnd_set_style(p_kwnd->p_wnd, style);
    }
    else
    {
        // 获取
        style = klb_wnd_get_style(p_kwnd->p_wnd);
    }

    lua_pushinteger(L, style);
    return 1;
}

// 设置/获取 显示状态
static int klua_kwnd_show(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    bool show = false;
    int idx = 2;

    if (klua_is_boolean(L, idx))
    {
        // 设置
        show = luaL_checkboolean(L, idx);

        klb_wnd_show(p_kwnd->p_wnd, show);
    }
    else
    {
        // 获取
        show = klb_wnd_is_show(p_kwnd->p_wnd);
    }

    lua_pushboolean(L, show);
    return 1;
}

// 设置/获取 隐藏状态
static int klua_kwnd_hide(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    bool hide = false;
    int idx = 2;

    if (klua_is_boolean(L, idx))
    {
        // 设置
        hide = luaL_checkboolean(L, idx);

        klb_wnd_hide(p_kwnd->p_wnd, hide);
    }
    else
    {
        // 获取
        hide = klb_wnd_is_hide(p_kwnd->p_wnd);
    }

    lua_pushboolean(L, hide);
    return 1;
}

// 设置/获取 静态TIP
static int klua_kwnd_tip(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    int idx = 2;
    if (klua_is_string(L, idx))
    {
        // 设置静态TIP
        const char* p_tip = lua_tostring(L, idx);
        klb_wnd_set_tip(p_kwnd->p_wnd, p_tip);

        lua_pushstring(L, p_tip);
    }
    else
    {
        // 获取静态TIP
        const sds tip = klb_wnd_get_tip(p_kwnd->p_wnd);

        lua_pushstring(L, ((NULL != tip) ? tip : ""));
    }

    return 1;
}

// 设置/获取 动态TIP
static int klua_kwnd_tip_dynamic(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    int idx = 2;
    if (klua_is_string(L, idx))
    {
        // 设置动态TIP
        const char* p_tip = lua_tostring(L, idx);
        klb_wnd_set_tip_dynamic(p_kwnd->p_wnd, p_tip);

        lua_pushstring(L, p_tip);
    }
    else
    {
        // 获取动态TIP
        const sds tip = klb_wnd_get_tip_dynamic(p_kwnd->p_wnd);

        lua_pushstring(L, ((NULL != tip) ? tip : ""));
    }

    return 1;
}

// 更新TIP
static int klua_kwnd_tip_update(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    klb_wnd_tip_update(p_kwnd->p_wnd);

    return 0;
}

// 基于父窗口移动到指定的相对坐标
static int klua_kwnd_move(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self
    int x = (int)klua_check_option_integer(L, 2, 0);
    int y = (int)klua_check_option_integer(L, 3, 0);

    klb_wnd_move(p_kwnd->p_wnd, x, y);

    return 0;
}

// 重新设置控件大小
static int klua_kwnd_resize(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self
    int w = (int)luaL_checkinteger(L, 2);
    int h = (int)luaL_checkinteger(L, 3);

    klb_wnd_resize(p_kwnd->p_wnd, w, h);

    return 0;
}

// 刷新
static int klua_kwnd_refresh(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    klb_wnd_update(p_kwnd->p_wnd);

    return 0;
}

// 设置
static int klua_kwnd_set(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    int ret = klb_wnd_set(p_kwnd->p_wnd, p_in);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

// 获取
static int klua_kwnd_get(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_map_t* p_out = klb_wnd_get(p_kwnd->p_wnd, p_in);

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);
    return n;
}

// 获取GUI的当前 系统滴答数(单位毫秒ms)
static int klua_kwnd_tick_count(lua_State* L)
{
    klua_kwnd_t* p_kwnd = to_klua_kwnd(L, 1);           ///< @1 自身self

    int64_t tc = klb_wnd_get_tick_count(p_kwnd->p_wnd);

    lua_pushinteger(L, tc);
    return 1;
}

///////////////////////////////////
// klua kwnd 接口

static void klua_kwnd_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {

        // 样式
        { "style",           klua_kwnd_style },         // 设置/获取 样式

        // 状态
        { "show",            klua_kwnd_show },          // 设置/获取 显示状态
        { "hide",            klua_kwnd_hide },          // 设置/获取 隐藏状态

        // tip
        { "tip",             klua_kwnd_tip },           // 设置/获取 静态TIP
        { "tip_dynamic",     klua_kwnd_tip_dynamic },   // 设置/获取 动态TIP
        { "tip_update",      klua_kwnd_tip_update },    // 更新TIP

        // 窗口位置/大小
        { "move",            klua_kwnd_move },           // 基于父窗口移动到指定的相对坐标
        { "resize",          klua_kwnd_resize },         // 重新设置控件大小

        // 刷新
        { "refresh",         klua_kwnd_refresh },        // 刷新

        // 设置 / 获取
        { "set",             klua_kwnd_set },            // 设置
        { "get",             klua_kwnd_get },            // 获取

        // 系统滴答
        { "tick_count",      klua_kwnd_tick_count },     // 获取GUI的当前 系统滴答数(单位毫秒ms)

        { NULL,              NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kwnd_gc },
        { "__close",         klua_kwnd_gc },
        { "__tostring",      klua_kwnd_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KWND_HANDLE); /* metatable for KLUA_KWND_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////
// kgui lua 接口

static int klua_kgui_using_cpp(lua_State* L)
{
    klua_using_cpp_gui(klua_env_get_by_L(L));

    return 0;
}

static int klua_kgui_set_default_css(lua_State* L)
{
    klb_map_t* p_in = klua_seri_map_pack(L, 0);         ///< @1 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_default_css_set(p_gui, p_in);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

static int klua_kgui_get_default_css(lua_State* L)
{
    klb_map_t* p_in = klua_seri_map_pack(L, 0);         ///< @1 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_map_t* p_out = klb_gui_default_css_get(p_gui, p_in);

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);
    return n;
}

static int klua_kgui_set_global_css(lua_State* L)
{
    const char* p_type = luaL_checkstring(L, 1);        ///< 类型
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_globalcss_set(p_gui, p_type, p_in);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

static int klua_kgui_get_global_css(lua_State* L)
{
    const char* p_type = luaL_checkstring(L, 1);        ///< 类型
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_map_t* p_out = klb_gui_globalcss_get(p_gui, p_type, p_in);

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);
    return n;
}

// 是否支持某个类型的全局CSS
static int klua_kgui_has_global_css(lua_State* L)
{
    const char* p_type = luaL_checkstring(L, 1);        ///< 类型

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    if (NULL != klb_gui_globalcss_get_ptr(p_gui, p_type))
    {
        lua_pushboolean(L, true); // 有全局CSS属性数据, 支持
    }
    else
    {
        lua_pushboolean(L, false); // 不支持
    }

    return 1;
}

static int klua_kgui_set_shwnd_css(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_shwnd_css_set(p_gui, p_path, p_in);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

static int klua_kgui_get_shwnd_css(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_map_t* p_out = klb_gui_shwnd_css_get(p_gui, p_path, p_in);

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);
    return n;
}

static int klua_kgui_is_multi_canvas_layer(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    bool is_multi = klb_gui_is_multi_canvas_layer(p_gui);

    lua_pushboolean(L, is_multi);
    return 1;
}

static int klua_kgui_load_image(lua_State* L)
{
    const char* p_key = luaL_checkstring(L, 1);
    const char* p_img_path = luaL_checkstring(L, 2);

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_load_image(p_gui, p_key, p_img_path);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kgui_clear_msg(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    klb_gui_clear_msg(p_gui);

    return 0;
}

static int klua_kgui_get_kwnd(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);       ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_wnd_t* p_wnd = klb_gui_find_wnd(p_gui, p_path_name);

    if (NULL != p_wnd)
    {
        klua_kwnd_t* p_kwnd = new_klua_kwnd(L);             ///< #1. 成功; kwnd 接口

        p_kwnd->p_wnd = p_wnd;
    }
    else
    {
        lua_pushnil(L);                                     ///< #1. 失败; nil
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

    uint32_t style = (uint32_t)klua_check_option_integer(L, 7, 0);  ///< @7. [可选]样式

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_append(p_gui, p_type, p_path_name, x, y, w, h, style);

    lua_pushinteger(L, ret);                                ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_remove(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);       ///< @1. 路径名: eg. "/home"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_remove(p_gui, p_path_name);

    lua_pushinteger(L, ret);                                ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_clear(lua_State* L)
{
    klua_ex_gui_t* p_ex = klua_ex_get_gui_by_L(L);

    int ret = klua_ex_gui_clear_async(p_ex, 1);             ///< @1. Lua函数

    lua_pushinteger(L, ret);                                ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_bind_command(lua_State* L)
{
    klua_ex_gui_t* p_ex = klua_ex_get_gui_by_L(L);

    const char* p_path_name = luaL_checkstring(L, 1);                   ///< @1. 路径名: eg. "/home/btn1"
    int ret = klua_ex_gui_bind_command(p_ex, p_path_name, 2);           ///< @2. Lua函数

    lua_pushinteger(L, ret);                                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_call_control_and_command(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);                   ///< @1. 路径名: eg. "/home/btn1"
    int msg = (int)luaL_checkinteger(L, 2);                             ///< @2. msg,事件: eg. KLBUI_onparsewindow/KLBUI_onparsedialog

    klb_point_t pt1 = { 0 }, pt2 = { 0 };
    pt1.x = (int)klua_check_option_integer(L, 3, 0);                    ///< @3. x1
    pt1.y = (int)klua_check_option_integer(L, 4, 0);                    ///< @4. y1
    pt2.x = (int)klua_check_option_integer(L, 5, 0);                    ///< @5. x2
    pt2.y = (int)klua_check_option_integer(L, 6, 0);                    ///< @6. y2

    int lparam = (int)klua_check_option_integer(L, 7, 0);               ///< @7. lparam
    int wparam = (int)klua_check_option_integer(L, 8, 0);               ///< @8. wparam

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_call_control_and_command(p_gui, p_path_name, msg, &pt1, &pt2, lparam, wparam);

    lua_pushinteger(L, ret);                                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_set(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_set(p_gui, p_path_name, p_in);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

static int klua_kgui_get(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_map_t* p_out = klb_gui_get(p_gui, p_path_name, p_in);

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);
    return n;
}

static int klua_kgui_modal(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_modal(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_modal_end(lua_State* L)
{
    bool all = true;
    if (LUA_TBOOLEAN == lua_type(L, 1))
    {
        all = (0 == lua_toboolean(L, 1)) ? false : true;    ///< @1. 是否全部
    }

    const char* p_path_name = NULL;    
    if (LUA_TSTRING == lua_type(L, 2))
    {
        p_path_name = luaL_checkstring(L, 2);               ///< @2. 路径名: eg. "/home/btn1"
    }

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_modal_end(p_gui, all, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_modal_num(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int num = klb_gui_modal_num(p_gui);

    lua_pushinteger(L, num);                            ///< #1. 获取 modal 窗口数
    return 1;
}

static int klua_kgui_popup(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/menu1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_popup(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_popup_end(lua_State* L)
{
    bool all = true;
    if (LUA_TBOOLEAN == lua_type(L, 1))
    {
        all = (0 == lua_toboolean(L, 1)) ? false : true;
    }

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_popup_end(p_gui, all);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_popup_num(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int num = klb_gui_popup_num(p_gui);

    lua_pushinteger(L, num);                            ///< #1. 获取 popup 窗口数
    return 1;
}

static int klua_kgui_messagebox(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/messagebox1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_messagebox(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_messagebox_end(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_messagebox_end(p_gui);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_messagebox_num(lua_State* L)
{
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int num = klb_gui_messagebox_num(p_gui);

    lua_pushinteger(L, num);                                ///< #1. 获取 messagebox 窗口数
    return 1;
}

static int klua_kgui_show(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    bool show = luaL_checkboolean(L, 2);                ///< @2. 是否显示

    int ret = klb_gui_show(klua_gui_get_by_L(L), p_path_name, show);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_move(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    int x = (int)luaL_checkinteger(L, 2);                    ///< @2. 基于父窗口X
    int y = (int)luaL_checkinteger(L, 3);                    ///< @3. 基于父窗口Y

    int ret = klb_gui_move(klua_gui_get_by_L(L), p_path_name, x, y);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_resize(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"
    int w = (int)luaL_checkinteger(L, 2);                    ///< @2. 宽
    int h = (int)luaL_checkinteger(L, 3);                    ///< @3. 高

    int ret = klb_gui_resize(klua_gui_get_by_L(L), p_path_name, w, h);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_wndpos(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);           ///< @1. 路径名: eg. "/home/btn1"
    bool is_in_canvas = klua_check_option_boolean(L, 2, true);  ///< @2. 默认(true); true:相对于画布;  false:相对父窗口 

    klb_rect_t rect = { 0 };
    int ret = 0;

    if (is_in_canvas)
    {
        ret = klb_gui_wndpos_in_canvas(klua_gui_get_by_L(L), p_path_name, &rect);
    }
    else
    {
        ret = klb_gui_wndpos_in_parent(klua_gui_get_by_L(L), p_path_name, &rect);
    }

    lua_newtable(L);                                            ///< #1. table
    if (0 == ret)
    {
        klua_setfield_integer(L, "x", rect.x);
        klua_setfield_integer(L, "y", rect.y);
        klua_setfield_integer(L, "w", rect.w);
        klua_setfield_integer(L, "h", rect.h);
    }

    return 1;
}

static int klua_kgui_suggestw(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    int w = 0;
    klb_gui_suggestw(klua_gui_get_by_L(L), p_path_name, &w);

    lua_pushinteger(L, w);                              ///< #1. 建议宽度
    return 1;
}

static int klua_kgui_suggesth(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    int h = 0;
    klb_gui_suggesth(klua_gui_get_by_L(L), p_path_name, &h);

    lua_pushinteger(L, h);                              ///< #1. 建议高度
    return 1;
}

static int klua_kgui_focusdelay(lua_State* L)
{
    lua_Integer timeout = luaL_checkinteger(L, 1);

    klb_gui_set_focusdelay(klua_gui_get_by_L(L), timeout);

    return 0;
}

static int klua_kgui_refresh(lua_State* L)
{
    klb_gui_update(klua_gui_get_by_L(L));
    return 0;
}

static int klua_kgui_get_wh(lua_State* L)
{
    int w = 0, h = 0;
    klb_gui_get_wh(klua_gui_get_by_L(L), &w, &h);

    lua_pushinteger(L, w);                            ///< #1. 宽
    lua_pushinteger(L, h);                            ///< #2. 高
    return 2;
}

static int klua_kgui_to_event(lua_State* L)
{
    int32_t e = (int32_t)luaL_checkinteger(L, 1);

    e &= 0x00FFFFFF;

    lua_pushinteger(L, e);
    return 1;
}

static int klua_kgui_b1_event(lua_State* L)
{
    int32_t e = (int32_t)luaL_checkinteger(L, 1);

    lua_pushboolean(L, (e & KLBUI_event_bit1) ? true : false);
    return 1;
}

static int klua_kgui_b2_event(lua_State* L)
{
    int32_t e = (int32_t)luaL_checkinteger(L, 1);

    lua_pushboolean(L, (e & KLBUI_event_bit2) ? true : false);
    return 1;
}

static int klua_kgui_b3_event(lua_State* L)
{
    int32_t e = (int32_t)luaL_checkinteger(L, 1);

    lua_pushboolean(L, (e & KLBUI_event_bit3) ? true : false);
    return 1;
}

static int klua_kgui_tick_count(lua_State* L)
{
    // 注意: GUI内部的 系统滴答值, 可能和真实 系统滴答值 存在 不完全一致的情况
    // GUI 会在内部 主循环 开始时 和系统滴答值 保持同步
    // 上层使用时, 一定要注意: 要么使用 GUI的伪系统滴答值, 要么使用真实的 系统滴答值, 不要混用

    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    int64_t tc = klb_gui_get_tick_count(p_gui);

    lua_pushinteger(L, tc);
    return 1;
}

static int klua_kgui_ticker_interval(lua_State* L)
{
    int64_t interval = luaL_checkinteger(L, 1);

    klb_gui_t* p_gui = klua_gui_get_by_L(L);
    klb_gui_set_ticker_interval(p_gui, interval);

    return 0;
}

// 绑定/解绑 用户图层对应的 窗口
static int klua_kgui_bind_udatalayer(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    int ret = 0;

    int idx = 1;
    if (LUA_TSTRING == lua_type(L, idx))
    {
        // 绑定窗口
        const char* p_path = lua_tostring(L, idx);
        ret = klb_gui_udatalayer_bind(p_gui, p_path);
    }
    else
    {
        ret = klb_gui_udatalayer_bind_wnd(p_gui, NULL);
    }

    lua_pushinteger(L, ret);
    return 1;
}

// 移动用户图层
static int klua_kgui_move_udatalayer(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    int x = (int)klua_check_option_integer(L, 1, 0);
    int y = (int)klua_check_option_integer(L, 2, 0);

    klb_gui_udatalayer_move(p_gui, x, y);

    return 0;
}

// 显示/隐藏 用户图层
static int klua_kgui_show_udatalayer(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    bool show = klua_check_option_boolean(L, 1, false);

    klb_gui_udatalayer_show(p_gui, show);

    return 0;
}

// 绑定/解绑 等待图层对应的 窗口
static int klua_kgui_bind_waitlayer(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    int ret = 0;

    int idx = 1;
    if (LUA_TSTRING == lua_type(L, idx))
    {
        // 绑定窗口
        const char* p_path = lua_tostring(L, idx);
        ret = klb_gui_waitlayer_bind(p_gui, p_path);
    }
    else
    {
        ret = klb_gui_waitlayer_bind_wnd(p_gui, NULL);
    }

    lua_pushinteger(L, ret);
    return 1;
}

// 移动 等待图层
static int klua_kgui_move_waitlayer(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    int x = (int)klua_check_option_integer(L, 1, 0);
    int y = (int)klua_check_option_integer(L, 2, 0);

    klb_gui_waitlayer_move(p_gui, x, y);

    return 0;
}

// 开启/关闭 等待
static int klua_kgui_wait(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    bool wait = klua_check_option_boolean(L, 1, false);

    klb_gui_wait(p_gui, wait);

    return 0;
}

// 设置/获取 是否完整绘制
static int klua_kgui_redraw_full_event(lua_State* L)
{
    klb_gui_t* p_gui = klua_gui_get_by_L(L);

    bool is_full = true;
    int idx = 1;

    if (klua_is_boolean(L, idx))
    {
        // 设置
        is_full = luaL_checkboolean(L, idx);

        klb_gui_set_redraw_full_event(p_gui, is_full);
    }
    else
    {
        // 获取
        is_full = klb_gui_get_redraw_full_event(p_gui);
    }

    lua_pushboolean(L, is_full);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_kgui(lua_State* L)
{
    static luaL_Reg kgui_lib[] =
    {
        // cpp 启用 内部CPP扩展 : 即 支持使用 CPP 相关的GUI接口
        { "using_cpp",          klua_kgui_using_cpp },

        // css
        { "set_default_css",    klua_kgui_set_default_css },
        { "get_default_css",    klua_kgui_get_default_css },

        { "set_global_css",     klua_kgui_set_global_css },
        { "get_global_css",     klua_kgui_get_global_css },
        { "has_global_css",     klua_kgui_has_global_css },

        { "set_shwnd_css",      klua_kgui_set_shwnd_css },
        { "get_shwnd_css",      klua_kgui_get_shwnd_css },

        // 画布
        { "is_multi_canvas_layer",      klua_kgui_is_multi_canvas_layer }, // 获取时候支持多画布图层模式

        // image
        { "load_image",         klua_kgui_load_image },     // 加载图片资源

        // 消息事件 队列
        { "clear_msg",          klua_kgui_clear_msg },      // 清空 消息事件队列

        // wnd
        { "get_kwnd",           klua_kgui_get_kwnd },       // 获取 kwnd 操作接口 

        // wnd
        { "append",             klua_kgui_append },
        { "remove",             klua_kgui_remove },
        { "clear",              klua_kgui_clear },
        { "bind_command",       klua_kgui_bind_command },

        { "call_control_and_command",     klua_kgui_call_control_and_command },

        // get/set param
        { "set",                klua_kgui_set },
        { "get",                klua_kgui_get },

        // dialog
        { "modal",              klua_kgui_modal },
        { "modal_end",          klua_kgui_modal_end },
        { "modal_num",          klua_kgui_modal_num },

        { "popup",              klua_kgui_popup },
        { "popup_end",          klua_kgui_popup_end },
        { "popup_num",          klua_kgui_popup_num },

        { "messagebox",         klua_kgui_messagebox },
        { "messagebox_end",     klua_kgui_messagebox_end },
        { "messagebox_num",     klua_kgui_messagebox_num },     // 

        // wnd
        { "show",               klua_kgui_show },               // 显隐窗口
        { "move",               klua_kgui_move },               // 移动窗口位置(相对父窗口)
        { "resize",             klua_kgui_resize },             // 重置窗口大小
        { "wndpos",             klua_kgui_wndpos },             // 获取窗口位置

        { "suggestw",           klua_kgui_suggestw },           // 窗口建议宽度
        { "suggesth",           klua_kgui_suggesth },           // 窗口建议高度

        // 聚焦延时
        { "focusdelay",         klua_kgui_focusdelay },         // 设置聚焦延时消息时间(单位毫秒)

        // 刷新
        { "refresh",            klua_kgui_refresh },            // 刷新所有窗口

        // gui get (w,h)
        { "wh",                 klua_kgui_get_wh },             // 获取主画布宽高(即屏幕宽高)

        // 事件辅助函数
        { "to_event",           klua_kgui_to_event },           // 去除event值 的特殊标记
        { "b1_event",           klua_kgui_b1_event },           // event值 是否有 b1 特殊标记
        { "b2_event",           klua_kgui_b2_event },           // event值 是否有 b2 特殊标记
        { "b3_event",           klua_kgui_b3_event },           // event值 是否有 b3 特殊标记

        // GUI系统滴答数(伪)
        { "tick_count",         klua_kgui_tick_count },         // 获取GUI系统滴答数(伪); (单位毫秒)

        // 内部控件定时器运行间隔
        { "ticker_interval",    klua_kgui_ticker_interval },    // 设置/获取 内部控件定时器运行间隔; (单位毫秒)


        // 用户图层(udata layer)操作
        { "bind_udatalayer",    klua_kgui_bind_udatalayer },    // 绑定/解绑 用户图层对应的 窗口
        { "move_udatalayer",    klua_kgui_move_udatalayer },    // 移动 用户图层
        { "show_udatalayer",    klua_kgui_show_udatalayer },    // 显示/隐藏 用户图层


        // 等待图层(wait layer)操作
        { "bind_waitlayer",     klua_kgui_bind_waitlayer },     // 绑定/解绑 等待图层对应的 窗口
        { "move_waitlayer",     klua_kgui_move_waitlayer },     // 移动 等待图层
        { "wait",               klua_kgui_wait },               // 开启/关闭 等待


        { "redraw_full_event",  klua_kgui_redraw_full_event },  // 设置/获取 是否完整绘制

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kgui_lib);

    // KLUA_KWND_HANDLE
    klua_kwnd_createmeta(L);

    return 1;
}

// end
