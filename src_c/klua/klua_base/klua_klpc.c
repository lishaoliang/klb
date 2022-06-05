// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_kthread.h"
#include "klua/klua_data.h"
#include "klua/extension/klua_ex_lpc.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/klua_kthread.h"
#include "klua/lua-skynet/lua-seri.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_list.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_KLPC_MODULE_HANDLE     "KLUA_KLPC_MODULE_HANDLE*"

typedef struct klua_klpc_module_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L
    klua_ex_lpc_t*          p_lpc;

    sds                     name;           ///< 模块名称

    int                     reg_on_recv;    ///< recv接收数据
    lua_State*              co_recv;        ///< "co_recv"函数对应的协程

    klb_list_t*             p_msg_list;     ///< 收到的消息列表: 当未通过携程收消息时暂存于此
}klua_klpc_module_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_reg_on_recv_klua_klpc_module(klua_klpc_module_t* p_mo, klua_msg_t* p_msg)
{
    assert(NULL != p_mo);

    if (NULL != p_mo->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_mo->p_env), p_mo->co_recv);
        if (NULL == L) return -1; // 未处理

        p_mo->co_recv = NULL; // 清空

        lua_pushstring(L, p_msg->src_name);
        lua_pushlstring(L, p_msg->p_msg, p_msg->msg_size);
        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }
    else
    {
        if (p_mo->reg_on_recv <= 0) return -1; // 未处理

        lua_State* L = p_mo->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_mo->reg_on_recv);   /* to call reg in protected mode */

        lua_pushstring(L, p_msg->src_name);
        int num = luaseri_unpack_by_buffer(L, 2, p_msg->p_msg, p_msg->msg_size);

        int status = lua_pcall(L, num + 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int cb_msg_klua_klpc_module(void* ptr, klua_msg_t* p_msg)
{
    klua_klpc_module_t* p_mo = (klua_klpc_module_t*)ptr;
    bool free_msg = true;

    if (KLUA_LPC_POST == p_msg->type ||
        KLUA_LPC_REQUEST == p_msg->type)
    {
        if (call_lua_reg_on_recv_klua_klpc_module(p_mo, p_msg) < 0)
        {
            klb_list_push_tail(p_mo->p_msg_list, p_msg);

            free_msg = false;
        }
    }

    if (free_msg)
    {
        klua_msg_free(p_msg);
    }

    return 0;
}

////////////////////////////////////////

static klua_klpc_module_t* new_klua_klpc_module(lua_State* L)
{
    klua_klpc_module_t* p_mo = (klua_klpc_module_t*)lua_newuserdata(L, sizeof(klua_klpc_module_t));
    KLB_MEMSET(p_mo, 0, sizeof(klua_klpc_module_t));
    luaL_setmetatable(L, KLUA_KLPC_MODULE_HANDLE);
    return p_mo;
}

static klua_klpc_module_t* to_klua_klpc_module(lua_State* L, int index)
{
    klua_klpc_module_t* p_mo = (klua_klpc_module_t*)luaL_checkudata(L, index, KLUA_KLPC_MODULE_HANDLE);
    luaL_argcheck(L, NULL != p_mo, index, "'klpc_module' expected");
    return p_mo;
}

static int klua_klpc_module_close(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    if (NULL != p_mo->name)
    {
        klua_ex_lpc_delete(p_mo->p_lpc, p_mo->name);
    }

    klua_unref_registryindex(L, p_mo->reg_on_recv);

    if (NULL != p_mo->p_msg_list)
    {
        while (0 < klb_list_size(p_mo->p_msg_list))
        {
            klua_msg_t* p_msg = (klua_msg_t*)klb_list_pop_head(p_mo->p_msg_list);
            klua_msg_free(p_msg);
        }
    }

    KLB_FREE_BY(p_mo->p_msg_list, klb_list_destroy);
    KLB_FREE_BY(p_mo->name, sdsfree);

    return 0;
}

static int klua_klpc_module_tostring(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    lua_pushfstring(L, "klpc_module:%p", p_mo);
    return 1;
}

static int klua_klpc_module_on_recv(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    klua_unref_registryindex(L, p_mo->reg_on_recv);
    p_mo->reg_on_recv = klua_ref_registryindex(L, 2);

    return 0;
}

static int klua_klpc_module_co_recv(lua_State* L)
{
    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "klpc.new():co_recv must in coroutine!");
        return 0;
    }

    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    if (0 < klb_list_size(p_mo->p_msg_list))
    {
        klua_msg_t* p_msg = (klua_msg_t*)klb_list_pop_head(p_mo->p_msg_list);

        lua_pushstring(L, p_msg->src_name);
        lua_pushlstring(L, p_msg->p_msg, p_msg->msg_size);

        klua_msg_free(p_msg);
        return 2;
    }
    else
    {
        assert(NULL == p_mo->co_recv);
        p_mo->co_recv = L;

        return lua_yield(L, lua_gettop(L));
    }
}

static int klua_klpc_module_response(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 2, &name_len);

    if (0 < name_len)
    {
        // pack buffer
        luaseri_pack_from(L, 2);

        //
        klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
        ptr->type = KLUA_LPC_RESPONSE;
        ptr->p_msg = (char*)lua_topointer(L, -2);
        ptr->msg_size = lua_tointeger(L, -1);

        strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

        if (0 != klua_kthread_push_msg(p_name, ptr))
        {
            // 目标不存在
            klua_msg_free(ptr);
        }
    }

    return 0;
}

////////////////////////////////////////

static void klua_klpc_module_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_klpc_module_close },

        //{ "on_recv",        klua_klpc_module_on_recv },
        { "co_recv",        klua_klpc_module_co_recv },

        { "response",       klua_klpc_module_response },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_klpc_module_close },
        { "__close",         klua_klpc_module_close },
        { "__tostring",      klua_klpc_module_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KLPC_MODULE_HANDLE); /* metatable for KLUA_KLPC_MODULE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}


//////////////////////////////////////////////////////////////////////////

#define KLUA_KLPC_HANDLE     "KLUA_KLPC_HANDLE*"

// 
typedef struct klua_klpc_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L
    klua_ex_lpc_t*          p_lpc;

    sds                     name;

    int                     reg_on_call;    ///< call
    lua_State*              co_call;        ///< "call"函数对应的协程
}klua_klpc_t;

/////////////////////////////////////


static int call_lua_reg_on_call_klua_klpc(klua_klpc_t* p_klpc, klua_msg_t* p_msg)
{
    assert(NULL != p_klpc);

    if (NULL != p_klpc->co_call)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_klpc->p_env), p_klpc->co_call);
        if (NULL == L) return -1; // 未处理

        p_klpc->co_call = NULL; // 清空

        int num = luaseri_unpack_by_buffer(L, 1, p_msg->p_msg, p_msg->msg_size);
        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }
    else
    {
        if (p_klpc->co_call <= 0) return -1; // 未处理

        lua_State* L = p_klpc->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_klpc->co_call);     /* to call reg in protected mode */

        int num = luaseri_unpack_by_buffer(L, 1, p_msg->p_msg, p_msg->msg_size);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int cb_msg_klua_klpc(void* ptr, klua_msg_t* p_msg)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)ptr;

    if (KLUA_LPC_RESPONSE == p_msg->type)
    {
        call_lua_reg_on_call_klua_klpc(p_klpc, p_msg);
    }

    klua_msg_free(p_msg);
    return 0;
}

///////////////////////////////////////

static klua_klpc_t* new_klua_klpc(lua_State* L)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)lua_newuserdata(L, sizeof(klua_klpc_t));
    KLB_MEMSET(p_klpc, 0, sizeof(klua_klpc_t));
    luaL_setmetatable(L, KLUA_KLPC_HANDLE);
    return p_klpc;
}

static klua_klpc_t* to_klua_klpc(lua_State* L, int index)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)luaL_checkudata(L, index, KLUA_KLPC_HANDLE);
    luaL_argcheck(L, NULL != p_klpc, index, "'klpc' expected");
    return p_klpc;
}

/// @brief 主动关闭
static int klua_klpc_close(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    if (NULL != p_klpc->name)
    {
        klua_ex_lpc_delete(p_klpc->p_lpc, p_klpc->name);
    }

    klua_unref_registryindex(L, p_klpc->reg_on_call);
    KLB_FREE_BY(p_klpc->name, sdsfree);

    return 0;
}

static int klua_klpc_tostring(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    lua_pushfstring(L, "klpc:%p", p_klpc);
    return 1;
}

///////////////////////////////////////

static int klua_klpc_co_call(lua_State* L)
{
    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "klpc.new():co_call must in coroutine!");
        return 0;
    }

    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    const char* p_dst_name = luaL_checkstring(L, 2);

    // pack buffer
    luaseri_pack_from(L, 2);

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_REQUEST;
    ptr->p_msg = (char*)lua_topointer(L, -2);
    ptr->msg_size = lua_tointeger(L, -1);

    strncpy(ptr->dst_name, p_dst_name, KLUA_LPC_NAME_LEN);
    strncpy(ptr->src_name, p_klpc->name, KLUA_LPC_NAME_LEN);

    if (0 == klua_kthread_push_msg(p_dst_name, ptr))
    {
        assert(NULL == p_klpc->co_call);
        p_klpc->co_call = L;

        return lua_yield(L, lua_gettop(L));
    }
    else
    {
        // 目标不存在
        klua_msg_free(ptr);
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////

static void klua_klpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_klpc_close },

        { "co_call",        klua_klpc_co_call },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            klua_klpc_close },
        {"__close",         klua_klpc_close },
        {"__tostring",      klua_klpc_tostring},
        {NULL,              NULL}
    };

    luaL_newmetatable(L, KLUA_KLPC_HANDLE); /* metatable for KLUA_KLPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

static int klua_klpc_new_module(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);

    klua_klpc_module_t* p_mo = new_klua_klpc_module(L);

    p_mo->p_env = klua_env_get_by_L(L);
    p_mo->L = L;
    p_mo->p_lpc = klua_ex_get_lpc(p_mo->p_env);
    p_mo->name = sdsnew(p_name);

    p_mo->p_msg_list = klb_list_create();

    klua_ex_lpc_new_module(p_mo->p_lpc, p_mo->name, cb_msg_klua_klpc_module, p_mo);

    return 1;
}

/// @brief 新建一个LPC
static int klua_klpc_new(lua_State* L)
{
    klua_klpc_t* p_klpc = new_klua_klpc(L);
    p_klpc->p_env = klua_env_get_by_L(L);
    p_klpc->L = L;
    p_klpc->p_lpc = klua_ex_get_lpc(p_klpc->p_env);

    p_klpc->name = klua_ex_lpc_new_lpc(p_klpc->p_lpc, cb_msg_klua_klpc, p_klpc);

    return 1;
}

static int klua_klpc_post(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);

    // pack buffer
    luaseri_pack_from(L, 1);

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_POST;
    ptr->p_msg = (char*)lua_topointer(L, -2);
    ptr->msg_size = lua_tointeger(L, -1);

    strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

    int ret = klua_kthread_push_msg(p_name, ptr);
    if (0 != ret)
    {
        klua_msg_free(ptr);
    }

    lua_pushboolean(L, (0 == ret) ? true : false);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_klpc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new_module",         klua_klpc_new_module },
        { "new",                klua_klpc_new },

        { "post",               klua_klpc_post },

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KLPC_MODULE_HANDLE
    klua_klpc_module_createmeta(L);

    // KLUA_KLPC_HANDLE
    klua_klpc_createmeta(L);

    return 1;
}
