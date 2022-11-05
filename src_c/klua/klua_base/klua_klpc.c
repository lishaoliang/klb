// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_thread.h"
#include "klua/extension/klua_ex_lpc.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_KLPC_MODULE_HANDLE     "KLUA_KLPC_MODULE_HANDLE*"

typedef struct klua_klpc_module_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L
    klua_ex_lpc_t*          p_lpc;

    sds                     name;           ///< 模块名称

    lua_State*              co_recv;        ///< "co_recv"函数对应的协程
    klb_nlist_t*             p_msg_list;     ///< 收到的消息列表: 当未通过携程收消息时暂存于此

    bool                    run;            ///< 是否正常运行
    int                     err_co_recv;    ///< 错误"co_recv"计数
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

        int count = 0;
        if (NULL != p_msg)
        {
            const char* p_data = p_msg->p_data->p_buf + p_msg->p_data->start;
            int data_len = p_msg->p_data->end - p_msg->p_data->start;           

            lua_pushstring(L, p_msg->src_name);
            int num = luaseri_map_binary_unpack(L, 2, p_data, data_len);
            count = num + 1;
        }

        int status = lua_pcall(L, count, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

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
            klb_nlist_push_tail(p_mo->p_msg_list, p_msg);

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

static int klua_klpc_module_tostring(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    lua_pushfstring(L, "klpc_module:%p", p_mo);
    return 1;
}

static int klua_klpc_module_close(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    // 释放lpc
    if (p_mo->run)
    {
        klua_ex_lpc_delete(p_mo->p_lpc, p_mo->name);
    }

    p_mo->run = false;

    // 让"co_recv"返回
    call_lua_reg_on_recv_klua_klpc_module(p_mo, NULL);

    // 清空消息
    if (NULL != p_mo->p_msg_list)
    {
        while (0 < klb_nlist_size(p_mo->p_msg_list))
        {
            klua_msg_t* p_msg = (klua_msg_t*)klb_nlist_pop_head(p_mo->p_msg_list);
            klua_msg_free(p_msg);
        }
    }

    KLB_FREE_BY(p_mo->p_msg_list, klb_nlist_destroy);
    KLB_FREE_BY(p_mo->name, sdsfree);

    return 0;
}

static int klua_klpc_module_status(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    lua_pushboolean(L, p_mo->run);
    return 1;
}

static int klua_klpc_module_co_recv(lua_State* L)
{
    klua_check_coroutine(L, "klpc.new_module():co_recv() must in coroutine!");
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    if (!p_mo->run)
    {
        p_mo->err_co_recv++; // 错误计数

        if (10000 < p_mo->err_co_recv)
        {
            luaL_error(L, "klpc.new_module() status error,not run!");
        }

        return 0; // 已经关闭
    }

    if (0 < klb_nlist_size(p_mo->p_msg_list))
    {
        klua_msg_t* p_msg = (klua_msg_t*)klb_nlist_pop_head(p_mo->p_msg_list);

        const char* p_data = p_msg->p_data->p_buf + p_msg->p_data->start;
        int data_len = p_msg->p_data->end - p_msg->p_data->start;

        lua_pushstring(L, p_msg->src_name);
        int num = luaseri_map_binary_unpack(L, 2, p_data, data_len);

        klua_msg_free(p_msg);
        return num + 1;
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

    if (!p_mo->run)
    {
        lua_pushboolean(L, false);
        return 1; // 已经关闭
    }

    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 2, &name_len);

    if (name_len <= 0)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_RESPONSE;
    ptr->p_data = luaseri_map_binary_pack(L, 2); // pack buffer

    strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

    if (0 != klua_thread_push_lpc_msg(p_name, ptr))
    {
        // 目标不存在
        klua_msg_free(ptr);

        lua_pushboolean(L, false);
    }
    else
    {
        lua_pushboolean(L, true);
    }

    return 1;
}

static int klua_klpc_module_notify(lua_State* L)
{
    klua_klpc_module_t* p_mo = to_klua_klpc_module(L, 1);

    if (!p_mo->run)
    {
        lua_pushboolean(L, false);
        return 1; // 已经关闭
    }

    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 2, &name_len);

    if (name_len <= 0)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_NOTIFY;
    ptr->p_data = luaseri_map_binary_pack(L, 2); // pack buffer

    strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

    if (0 != klua_thread_push_lpc_msg(p_name, ptr))
    {
        // 目标不存在
        klua_msg_free(ptr);

        lua_pushboolean(L, false);
    }
    else
    {
        lua_pushboolean(L, true);
    }

    return 1;
}

////////////////////////////////////////

static void klua_klpc_module_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_klpc_module_close },

        { "status",         klua_klpc_module_status },

        { "co_recv",        klua_klpc_module_co_recv },
        { "response",       klua_klpc_module_response },

        { "notify",         klua_klpc_module_notify },

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

/////////////////////////////////////////

static int lib_klua_klpc_new_module(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);

    klua_klpc_module_t* p_mo = new_klua_klpc_module(L);

    p_mo->p_env = klua_env_get_by_L(L);
    p_mo->L = L;
    p_mo->p_lpc = klua_ex_get_lpc(p_mo->p_env);
    p_mo->name = sdsnew(p_name);

    p_mo->p_msg_list = klb_nlist_create();

    p_mo->run = true;

    klua_ex_lpc_new_module(p_mo->p_lpc, p_mo->name, cb_msg_klua_klpc_module, p_mo);

    return 1;
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

    lua_State*              co_recv;        ///< "co_recv"(call)函数对应的协程
    lua_State*              co_recv_notify; ///< "co_recv_notify"函数对应的协程

    bool                    run;            ///< 是否正常运行
}klua_klpc_t;

/////////////////////////////////////


static int call_lua_reg_co_recv_klua_klpc(klua_klpc_t* p_klpc, klua_msg_t* p_msg)
{
    assert(NULL != p_klpc);

    if (NULL != p_klpc->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_klpc->p_env), p_klpc->co_recv);
        if (NULL == L) return -1; // 未处理

        p_klpc->co_recv = NULL; // 清空

        const char* p_data = p_msg->p_data->p_buf + p_msg->p_data->start;
        int data_len = p_msg->p_data->end - p_msg->p_data->start;

        int num = luaseri_map_binary_unpack(L, 1, p_data, data_len);
        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_reg_co_recv_notify_klua_klpc(klua_klpc_t* p_klpc, klua_msg_t* p_msg)
{
    assert(NULL != p_klpc);

    if (NULL != p_klpc->co_recv_notify)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_klpc->p_env), p_klpc->co_recv_notify);
        if (NULL == L) return -1; // 未处理

        p_klpc->co_recv_notify = NULL; // 清空

        const char* p_data = p_msg->p_data->p_buf + p_msg->p_data->start;
        int data_len = p_msg->p_data->end - p_msg->p_data->start;

        int num = luaseri_map_binary_unpack(L, 1, p_data, data_len);
        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int cb_msg_klua_klpc(void* ptr, klua_msg_t* p_msg)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)ptr;

    if (KLUA_LPC_RESPONSE == p_msg->type)
    {
        call_lua_reg_co_recv_klua_klpc(p_klpc, p_msg);
    }
    else if(KLUA_LPC_NOTIFY == p_msg->type)
    {
        call_lua_reg_co_recv_notify_klua_klpc(p_klpc, p_msg);
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

static int klua_klpc_tostring(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    lua_pushfstring(L, "klpc:%p", p_klpc);
    return 1;
}

///////////////////////////////////////

/// @brief 主动关闭
static int klua_klpc_close(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    if (NULL != p_klpc->name)
    {
        klua_ex_lpc_delete(p_klpc->p_lpc, p_klpc->name);
    }

    KLB_FREE_BY(p_klpc->name, sdsfree);

    return 0;
}

static int klua_klpc_status(lua_State* L)
{
    lua_pushboolean(L, true);
    lua_pushinteger(L, 0);
    return 2;
}

static int klua_klpc_post(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    const char* p_name = luaL_checkstring(L, 2);

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_POST;
    ptr->p_data = luaseri_map_binary_pack(L, 2); // pack buffer

    strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

    int ret = klua_thread_push_lpc_msg(p_name, ptr);
    if (0 != ret)
    {
        klua_msg_free(ptr);
    }

    lua_pushboolean(L, (0 == ret) ? true : false);
    return 1;
}

static int klua_klpc_co_call(lua_State* L)
{
    klua_check_coroutine(L, "klpc.new():co_call() must in coroutine!");

    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    const char* p_dst_name = luaL_checkstring(L, 2);

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_REQUEST;
    ptr->p_data = luaseri_map_binary_pack(L, 2);  // pack buffer

    strncpy(ptr->dst_name, p_dst_name, KLUA_LPC_NAME_LEN);
    strncpy(ptr->src_name, p_klpc->name, KLUA_LPC_NAME_LEN);

    if (0 == klua_thread_push_lpc_msg(p_dst_name, ptr))
    {
        assert(NULL == p_klpc->co_recv);
        p_klpc->co_recv = L;

        return lua_yield(L, lua_gettop(L));
    }
    else
    {
        // 目标不存在
        klua_msg_free(ptr);
        return 0;
    }
}

static int klua_klpc_co_recv_notify(lua_State* L)
{
    klua_check_coroutine(L, "klpc.new():co_recv_notify() must in coroutine!"); 
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE

    assert(NULL == p_klpc->co_recv_notify);
    p_klpc->co_recv_notify = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////////////////////////////

static void klua_klpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_klpc_close },

        { "status",         klua_klpc_status },

        { "post",           klua_klpc_post },

        { "co_call",        klua_klpc_co_call },
        { "co_recv_notify", klua_klpc_co_recv_notify },

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

/// @brief 新建一个LPC
static int lib_klua_klpc_new(lua_State* L)
{
    klua_klpc_t* p_klpc = new_klua_klpc(L);
    p_klpc->p_env = klua_env_get_by_L(L);
    p_klpc->L = L;
    p_klpc->p_lpc = klua_ex_get_lpc(p_klpc->p_env);

    p_klpc->name = klua_ex_lpc_new_lpc(p_klpc->p_lpc, cb_msg_klua_klpc, p_klpc);

    return 1;
}

static int lib_klua_klpc_post(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);

    //
    klua_msg_t* ptr = KLB_MALLOCZ(klua_msg_t, 1, 0);
    ptr->type = KLUA_LPC_POST;
    ptr->p_data = luaseri_map_binary_pack(L, 1); // pack buffer

    strncpy(ptr->dst_name, p_name, KLUA_LPC_NAME_LEN);

    int ret = klua_thread_push_lpc_msg(p_name, ptr);
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
        { "new_module",         lib_klua_klpc_new_module },
        { "new",                lib_klua_klpc_new },

        { "post",               lib_klua_klpc_post },

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
