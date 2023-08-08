// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_gui.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_gui.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include <assert.h>


typedef struct klua_ex_gui_t_ klua_ex_gui_t;


typedef struct klua_kgui_bind_t_
{
    klua_ex_gui_t*  p_ex;
    int             on_command;         ///< 绑定的 lua函数
}klua_kgui_bind_t;


typedef struct klua_ex_gui_t_
{
    klua_env_t*     p_env;              ///< Lua环境
    lua_State*      L;                  ///< lua

    klb_gui_t*      p_gui;              ///< gui实例

    klb_hlist_t*    p_bind_hlist;       ///< klua_kgui_bind_t*
}klua_ex_gui_t;

//////////////////////////////////////////////////////////////////////////
static int klua_ex_gui_quit(klua_ex_gui_t* p_ex);


static void klua_ex_gui_clear_bind(klua_ex_gui_t* p_ex)
{
    if (NULL != p_ex->p_bind_hlist)
    {
        // 清空绑定
        while (0 < klb_hlist_size(p_ex->p_bind_hlist))
        {
            klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)klb_hlist_pop_head(p_ex->p_bind_hlist);
            if (0 < p_bind->on_command)
            {
                luaL_unref(p_ex->L, LUA_REGISTRYINDEX, p_bind->on_command);
                p_bind->on_command = 0;
            }

            KLB_FREE(p_bind);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

static void* klua_ex_gui_create(klua_env_t* p_env)
{
    klua_ex_gui_t* p_ex = KLB_MALLOCZ(klua_ex_gui_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    p_ex->p_gui = klb_gui_create(NULL);
    klb_gui_attach_klua_env(p_ex->p_gui, p_env);

    p_ex->p_bind_hlist = klb_hlist_create(0);

    return p_ex;
}

static void klua_ex_gui_destroy(void* ptr)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    // 退出清理
    klua_ex_gui_quit(p_ex);

    KLB_FREE(p_ex);
}

// 要退出了, 清理资源等
static int klua_ex_gui_quit(klua_ex_gui_t* p_ex)
{
    // KLUA_ENV_EX_quit / klua_ex_gui_destroy 时都会调用

    // 清空gui使用的数据
    if (NULL != p_ex->p_gui)
    {
        klb_gui_clear(p_ex->p_gui);
    }

    klua_ex_gui_clear_bind(p_ex);

    // 直接销毁
    KLB_FREE_BY(p_ex->p_bind_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_ex->p_gui, klb_gui_destroy);

    return 0;
}

static int klua_ex_gui_ctrl(void* ptr, klua_env_t* p_env, int opt, uint8_t* p_param_in_out, int param_size)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)ptr;

    switch (opt)
    {
    case KLUA_ENV_EX_quit:
        return klua_ex_gui_quit(p_ex);
        break;

    default:
        break;
    }

    return 0;
}

static int klua_ex_gui_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)ptr;

    klb_gui_loop_once(p_ex->p_gui, now);

    return 0;
}

//////////////////////////////////////////////////////////////////////////


klb_gui_t* klua_ex_gui_get(klua_ex_gui_t* p_ex)
{
    assert(NULL != p_ex);
    return p_ex->p_gui;
}

static int on_command_klua_ex_gui(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    assert(NULL != p_wnd);
    klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)p_wnd->p_udata;
    assert(NULL != p_bind);

    if (p_bind->on_command <= 0)
    {
        return 0;
    }

    lua_State* L = p_bind->p_ex->L;
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
        klua_env_report_by_L(L, LUA_ERRRUN);
        KLUA_HELP_TOP_E(L);
        return -1;
    }

    int ret = (int)luaL_checkinteger(L, -1);                    // #1. 0. 消息终止,不再"冒泡"; msg. 任然以msg"冒泡"; 非0. 转换为其他消息"冒泡"
    lua_pop(L, 1);

    KLUA_HELP_TOP_E(L);
    return ret;
}

int klua_ex_gui_bind_command(klua_ex_gui_t* p_ex, const char* p_path_name, int idx)
{
    luaL_checktype(p_ex->L, idx, LUA_TFUNCTION);
    lua_pushvalue(p_ex->L, idx);

    int func = luaL_ref(p_ex->L, LUA_REGISTRYINDEX);
    assert(0 < func);

    bool insert = false;
    int name_len = strlen(p_path_name);

    klua_kgui_bind_t* p_bind = (klua_kgui_bind_t*)klb_hlist_find(p_ex->p_bind_hlist, p_path_name, name_len);
    if (NULL == p_bind)
    {
        p_bind = KLB_MALLOCZ(klua_kgui_bind_t, 1, 0);

        p_bind->p_ex = p_ex;
        insert = true;
    }
    else
    {
        // 已经存在响应函数, 则删除原来的响应函数
        if (0 < p_bind->on_command)
        {
            luaL_unref(p_ex->L, LUA_REGISTRYINDEX, p_bind->on_command);
            p_bind->on_command = 0;
        }
    }

    p_bind->on_command = func;

    int ret = klb_gui_bind_command(p_ex->p_gui, p_path_name, on_command_klua_ex_gui, p_bind);

    if (0 == ret)
    {
        if (insert)
        {
            klb_hlist_push_tail(p_ex->p_bind_hlist, p_path_name, name_len, p_bind);
        }
    }
    else
    {
        if (0 < p_bind->on_command)
        {
            luaL_unref(p_ex->L, LUA_REGISTRYINDEX, p_bind->on_command);
            p_bind->on_command = 0;
        }

        klb_hlist_remove_bykey(p_ex->p_bind_hlist, p_path_name, name_len);
        KLB_FREE(p_bind);
    }

    return ret;
}

int klua_ex_gui_clear(klua_ex_gui_t* p_ex)
{
    int ret = klb_gui_clear(p_ex->p_gui);
    klua_ex_gui_clear_bind(p_ex);

    return ret;
}


//////////////////////////////////////////////////////////////////////////

int klua_ex_register_gui(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_gui_create;
    ex.cb_destroy = klua_ex_gui_destroy;
    ex.cb_ctrl = klua_ex_gui_ctrl;
    ex.cb_loop_once = klua_ex_gui_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_GUI_NAME, &ex);

    return 0;
}

klua_ex_gui_t* klua_ex_get_gui(klua_env_t* p_env)
{
    klua_ex_gui_t* p_ex = (klua_ex_gui_t*)klua_env_get_extension(p_env, KLUA_EX_GUI_NAME);
    return p_ex;
}

klua_ex_gui_t* klua_ex_get_gui_by_L(lua_State* L)
{
    return klua_ex_get_gui(klua_env_get_by_L(L));
}

//////////////////////////////////////////////////////////////////////////

klb_gui_t* klua_gui_get(klua_env_t* p_env)
{
    return klua_ex_gui_get(klua_ex_get_gui(p_env));
}

klb_gui_t* klua_gui_get_by_L(lua_State* L)
{
    return klua_gui_get(klua_env_get_by_L(L));
}
