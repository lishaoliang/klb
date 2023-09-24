// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"
#include "klbgui/shwnd/klbshw_messagebox.h"
#include "klbutil/klb_obj.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_help.h"
#include "klbutil/klb_hlist.h"
#include "klua/extension/klua_ex_gui.h"
#include "klbutil/klb_map.h"
#include "klua/klua_seri.h"
#include "klua/klua_gui.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// kwnd lua 接口
//  1. gui的所有组件归纳为 klb_wnd_t*
//  2. 则 kwnd 提供公共的操作接口

static klb_wnd_t* to_klua_klb_wnd(lua_State* L, int idx)
{
    luaL_checktype(L, idx, LUA_TLIGHTUSERDATA);
    klb_wnd_t* p_wnd = (klb_wnd_t*)lua_topointer(L, idx);
    return p_wnd;
}

///////////////////////////////////
// set/get

static int klua_kwnd_set(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    int ret = 1;
    if (NULL != p_wnd->vtable.on_set)
    {
        ret = p_wnd->vtable.on_set(p_wnd, p_in);
    }

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)

    KLB_FREE_BY(p_in, klb_map_destroy);
    return 1;
}

static int klua_kwnd_get(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    klb_map_t* p_in = klua_seri_map_pack(L, 1);         ///< @2 ~ @N 参数

    klb_map_t* p_out = NULL;
    
    if (NULL != p_wnd->vtable.on_get)
    {
        p_out = p_wnd->vtable.on_get(p_wnd, p_in);
    }

    int n = 0;
    if (NULL != p_out)
    {
        n = klua_seri_map_unpack(L, 1, p_out);
    }

    KLB_FREE_BY(p_in, klb_map_destroy);
    KLB_FREE_BY(p_out, klb_map_destroy);

    return n;
}

///////////////////////////////////
// wnd

static int klua_kwnd_show(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    bool show = klua_check_option_boolean(L, 2, true);  ///< @2 

    klb_wnd_show(p_wnd, show);

    return 0;
}

static int klua_kwnd_move(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    int x = (int)luaL_checkinteger(L, 2);               ///< @2 x
    int y = (int)luaL_checkinteger(L, 3);               ///< @3 y

    klb_wnd_move(p_wnd, x, y);

    return 0;
}

static int klua_kwnd_resize(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    int w = (int)luaL_checkinteger(L, 2);               ///< @2 w
    int h = (int)luaL_checkinteger(L, 3);               ///< @3 h

    klb_wnd_resize(p_wnd, w, h);

    return 0;
}

///////////////////////////////////
// 绘图

static int klua_kwnd_draw_clear(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_clear(p_wnd, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_point(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    int x = (int)luaL_checkinteger(L, 2);               ///< @2 x
    int y = (int)luaL_checkinteger(L, 3);               ///< @3 y

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_point(p_wnd, x, y, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_points(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    klb_point_t* p_points = NULL;
    int count = 0;

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_points(p_wnd, p_points, count, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_line(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    int x1 = (int)luaL_checkinteger(L, 2);              ///< @2 x1
    int y1 = (int)luaL_checkinteger(L, 3);              ///< @3 y1
    int x2 = (int)luaL_checkinteger(L, 4);              ///< @4 x2
    int y2 = (int)luaL_checkinteger(L, 5);              ///< @5 y2

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_line(p_wnd, x1, y1, x2, y2, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_lines(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    klb_point_t* p_points = NULL;
    int count = 0;
    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_lines(p_wnd, p_points, count, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_rect(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    klb_rect_t rect = { 0, 0, 0, 0 };
    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_rect(p_wnd, &rect, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_rects(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_rects(p_wnd, NULL, 0, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_fill_rect(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    klb_rect_t rect = { 0, 0, 0, 0 };
    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_fill_rect(p_wnd, &rect, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_fill_rects(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    uint32_t color = 0xFF101010;

    int ret = klb_wnd_draw_fill_rects(p_wnd, NULL, 0, &color);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_draw_text(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*

    klb_rect_t rect = { 0, 0, 0, 0 };
    uint32_t color = 0xFF101010;
    int font_h = 20;

    int ret = klb_wnd_draw_text(p_wnd, &rect, NULL, 0, &color, &font_h);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwnd_draw_draw_image(lua_State* L)
{
    klb_wnd_t* p_wnd = to_klua_klb_wnd(L, 1);           ///< @1 klb_wnd_t*
    const char* p_path = luaL_checkstring(L, 2);        ///< @2

    klb_rect_t dst = { 0, 0, 0, 0 };
    klb_rect_t src = { 0, 0, 0, 0 };

    int ret = klb_wnd_draw_image(p_wnd, NULL, p_path, NULL);

    lua_pushinteger(L, ret);
    return 1;
}

///////////////////////////////////
// kwnd

int klua_open_kwnd(lua_State* L)
{
    static luaL_Reg kwnd_lib[] =
    {
        // set/get
        { "set",            klua_kwnd_set },
        { "get",            klua_kwnd_get },

        // wnd
        { "show",           klua_kwnd_show },
        { "move",           klua_kwnd_move },
        { "resize",         klua_kwnd_resize },

        // 绘图
        { "draw_clear",     klua_kwnd_draw_clear },
        { "draw_point",     klua_kwnd_draw_point },
        { "draw_points",    klua_kwnd_draw_points },
        { "draw_line",      klua_kwnd_draw_line },
        { "draw_lines",     klua_kwnd_draw_lines },
        { "draw_rect",      klua_kwnd_draw_rect },
        { "draw_rects",     klua_kwnd_draw_rects },
        { "draw_fill_rect", klua_kwnd_draw_fill_rect },
        { "draw_fill_rects",klua_kwnd_draw_fill_rects },
        { "draw_text",      klua_kwnd_draw_draw_text },
        { "draw_image",     klua_kwnd_draw_draw_image },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kwnd_lib);

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// kgui lua 接口

//static int klua_kgui_attach_canvas(lua_State* L)
//{
//    klb_canvas_t* ptr = (klb_canvas_t*)luaL_checklightuserdata(L, 1);      ///< @1. 显存画布
//
//    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
//
//    klb_gui_attach_canvas(p_gui, ptr);
//
//    return 0;
//}

//static void klua_kgui_msg_callback(void* p_obj, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
//{
//    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)p_obj;
//
//    if (p_ex)
//    {
//        klb_gui_t* p_gui = klua_ex_gui_get(p_ex);
//        klb_gui_push_msg(p_gui, msg, x1, y1, x2, y2, lparam, wparam);
//    }
//}

//static int klua_kgui_get_msg_callback(lua_State* L)
//{
//    klua_ex_gui_t* p_ex = klua_ex_get_gui_by_L(L);
//
//    lua_pushlightuserdata(L, (void*)klua_kgui_msg_callback);
//    lua_pushlightuserdata(L, (void*)p_ex);
//    return 2;
//}

//static int klua_kgui_push_msg(lua_State* L)
//{
//    int msg = (int)luaL_checkinteger(L, 1);                     ///< @1. 消息
//    int x1 = (int)luaL_checkinteger(L, 2);                      ///< @2. 相对父窗口x1坐标
//    int y1 = (int)luaL_checkinteger(L, 3);                      ///< @3. 相对父窗口y1坐标
//    int x2 = (int)luaL_checkinteger(L, 4);                      ///< @4. 相对父窗口x2坐标
//    int y2 = (int)luaL_checkinteger(L, 5);                      ///< @5. 相对父窗口y2坐标
//    int lparam = (int)luaL_checkinteger(L, 6);                  ///< @6. 参数1
//    int wparam = (int)luaL_checkinteger(L, 7);                  ///< @7. 参数2
//
//    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
//
//    if (NULL != p_gui)
//    {
//        klb_gui_push_msg(p_gui, msg, x1, y1, x2, y2, lparam, wparam);
//        lua_pushinteger(L, 0);
//    }
//    else
//    {
//        lua_pushinteger(L, 1);
//    }
//
//    return 1;
//}

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

static int klua_kgui_load_image(lua_State* L)
{
    const char* p_key = luaL_checkstring(L, 1);
    const char* p_img_path = luaL_checkstring(L, 2);

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    int ret = klb_gui_load_image(p_gui, p_key, p_img_path);

    lua_pushinteger(L, ret);
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

static int klua_kgui_model(lua_State* L)
{
    const char* p_path_name = luaL_checkstring(L, 1);   ///< @1. 路径名: eg. "/home/btn1"

    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));

    int ret = klb_gui_model(p_gui, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
    return 1;
}

static int klua_kgui_model_end(lua_State* L)
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
    int ret = klb_gui_model_end(p_gui, all, p_path_name);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
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

static int klua_kgui_messagebox_std(lua_State* L)
{
    const char* p_title = luaL_checkstring(L, 1);           ///< @1. 标题
    const char* p_body_text = luaL_checkstring(L, 2);       ///< @2. 提示内容

    // 获取共享的消息框
    klb_gui_t* p_gui = klua_ex_gui_get(klua_ex_get_gui_by_L(L));
    klb_wnd_t* p_wnd = klbui_shwnd_get_messagebox(p_gui);

    // 设置参数
    {
        klbshw_messagebox_set_title(p_wnd, p_title);
        klbshw_messagebox_set_body_text(p_wnd, p_body_text);
    }

    // 移动到屏幕中心
    {
        int w = 0, h = 0;
        klbshw_messagebox_wh(p_gui, &w, &h);

        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_gui, &screen_w, &screen_h);

        int x = (screen_w - w) / 2;
        int y = (screen_h - h) / 2;
        klb_wnd_move(p_wnd, x, y);
    }

    // 弹出消息框
    int ret = klb_gui_messagebox_wnd(p_gui, p_wnd);

    lua_pushinteger(L, ret);                            ///< #1. 0.成功; 非0.失败(错误码)
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

int klua_open_kgui(lua_State* L)
{
    static luaL_Reg kgui_lib[] =
    {
        // css
        { "set_default_css",    klua_kgui_set_default_css },
        { "get_default_css",    klua_kgui_get_default_css },

        { "set_global_css",     klua_kgui_set_global_css },
        { "get_global_css",     klua_kgui_get_global_css },
        { "has_global_css",     klua_kgui_has_global_css },

        { "set_shwnd_css",      klua_kgui_set_shwnd_css },
        { "get_shwnd_css",      klua_kgui_get_shwnd_css },

        // image
        { "load_image",         klua_kgui_load_image },

        // wnd
        { "append",             klua_kgui_append },
        { "remove",             klua_kgui_remove },
        { "clear",              klua_kgui_clear },
        { "bind_command",       klua_kgui_bind_command },

        // get/set param
        { "set",                klua_kgui_set },
        { "get",                klua_kgui_get },

        // dialog
        { "model",              klua_kgui_model },
        { "model_end",          klua_kgui_model_end },

        { "popup",              klua_kgui_popup },
        { "popup_end",          klua_kgui_popup_end },

        { "messagebox",         klua_kgui_messagebox },
        { "messagebox_end",     klua_kgui_messagebox_end },

        { "messagebox_std",     klua_kgui_messagebox_std },     // 弹出内置的共享消息框

        // wnd
        { "show",               klua_kgui_show },
        { "move",               klua_kgui_move },
        { "resize",             klua_kgui_resize },
        { "wndpos",             klua_kgui_wndpos },             // 获取窗口位置

        { "suggestw",           klua_kgui_suggestw },           // 窗口建议宽度
        { "suggesth",           klua_kgui_suggesth },           // 窗口建议高度

        // gui get (w,h)
        { "wh",                 klua_kgui_get_wh },             // 获取主画布宽高(即屏幕宽高)

        // 事件辅助函数
        { "to_event",           klua_kgui_to_event },
        { "b1_event",           klua_kgui_b1_event },
        { "b2_event",           klua_kgui_b2_event },
        { "b3_event",           klua_kgui_b3_event },

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kgui_lib);

    return 1;
}
