#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_kthread.h"
#include "klua/klua_data.h"
#include "klbmem/klb_mem.h"
#include "klua/extension/klua_ex_lpc.h"
#include <assert.h>


#define KLUA_KLPC_HANDLE     "KLUA_KNCM_HANDLE*"


typedef struct klua_klpc_t_
{
    klua_env_t*             p_env;          ///< lua环境
    lua_State*              L;              ///< L

    klua_ex_lpc_t*          p_ex;           ///< LPC扩展

    int                     reg_on_callback;///< 注册的应答函数
    sds                     sequence;       ///< 已经开始了等待序列号
}klua_klpc_t;

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

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

static void free_klua_klpc(klua_klpc_t* p_klpc)
{
    if (0 < p_klpc->reg_on_callback)
    {
        klua_unref_registryindex(p_klpc->L, p_klpc->reg_on_callback);
        p_klpc->reg_on_callback = 0;
    }

    if (NULL != p_klpc->sequence)
    {
        klua_ex_lpc_remove_response(p_klpc->p_ex, p_klpc->sequence);
        KLB_FREE_BY(p_klpc->sequence, sdsfree);
    }
}

//////////////////////////////////////////////////////////////////////////
static int call_klua_klpc(klua_klpc_t* p_klpc, klua_lpc_msg_t* p_msg)
{
    assert(NULL != p_klpc);
    if (p_klpc->reg_on_callback <= 0) return EXIT_FAILURE;

    lua_State* L = p_klpc->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_klpc->reg_on_callback); /* to call reg_on_callback in protected mode */

    // 数据
    for (int i = 0; i < p_msg->data_num; i++)
    {
        klua_pushdata(L, &p_msg->data[i]);
    }

    int status = lua_pcall(L, p_msg->data_num, 0, 0);           /* do the call */
    klua_env_report(p_klpc->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static cb_lib_klua_klpc_call_lpc_response(void* ptr, klua_ex_lpc_t* p_ex, klua_lpc_msg_t* p_msg)
{
    klua_klpc_t* p_klpc = (klua_klpc_t*)ptr;
    
    // 调用Lua脚本
    call_klua_klpc(p_klpc, p_msg);

    // 释放
    free_klua_klpc(p_klpc);
}

/// @brief 调用(其他线程)函数: 无返回
static int lib_klua_klpc_post(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    const char* p_name = luaL_checkstring(L, 2);        ///< @2. 线程名称
    const char* p_func_name = luaL_checkstring(L, 3);   ///< @3. 被调用的远程函数名称

    int argc = lua_gettop(L);
    int data_num = argc - 3;
    assert(0 <= data_num);

    //
    klua_lpc_msg_t* p_msg = KLB_MALLOC(klua_lpc_msg_t, 1, data_num * sizeof(klua_data_t));
    KLB_MEMSET(p_msg, 0, sizeof(klua_lpc_msg_t) + data_num * sizeof(klua_data_t));

    p_msg->ex_name = sdsnew(KLUA_EX_LPC_NAME);
    p_msg->cb_destroy = klua_lpc_msg_destroy;
    p_msg->msg = KLUA_LPC_MSG_POST;
    p_msg->src_name = sdsnew("");
    p_msg->func_name = sdsnew(p_func_name);
    p_msg->sequence = sdsnew("");

    p_msg->data_num = data_num;
    for (int i = 0; i < data_num; i++)
    {
        klua_checkdata(L, &p_msg->data[i], i + 4);
    }

    klua_kthread_push_msg(p_name, p_msg);

    ///< #1. true.成功; false.失败
    return 0;
}

/// @brief 调用(其他线程)函数: 有返回
static int lib_klua_klpc_call(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);           ///< @1. KLUA_KLPC_HANDLE
    const char* p_name = luaL_checkstring(L, 2);        ///< @2. 线程名称
    const char* p_func_name = luaL_checkstring(L, 3);   ///< @3. 被调用的远程函数名称
    luaL_checktype(L, 4, LUA_TFUNCTION);                ///< @4. 函数

    assert(NULL == p_klpc->sequence);
    assert(p_klpc->reg_on_callback <= 0);
    p_klpc->reg_on_callback = klua_ref_registryindex(L, 4);

    int argc = lua_gettop(L);
    int data_num = argc - 4;
    assert(0 <= data_num);

    //
    klua_lpc_msg_t* p_msg = KLB_MALLOC(klua_lpc_msg_t, 1, data_num * sizeof(klua_data_t));
    KLB_MEMSET(p_msg, 0, sizeof(klua_lpc_msg_t) + data_num * sizeof(klua_data_t));

    p_msg->ex_name = sdsnew(KLUA_EX_LPC_NAME);
    p_msg->cb_destroy = klua_lpc_msg_destroy;
    p_msg->msg = KLUA_LPC_MSG_REQUEST;
    p_msg->src_name = sdsdup(klua_env_get_name(p_klpc->p_env));
    p_msg->func_name = sdsnew(p_func_name);
    p_msg->sequence = klua_ex_lpc_new_wait_response(p_klpc->p_ex, cb_lib_klua_klpc_call_lpc_response, p_klpc);

    p_msg->data_num = data_num;
    for (int i = 0; i < data_num; i++)
    {
        klua_checkdata(L, &p_msg->data[i], i + 5);
    }

    klua_kthread_push_msg(p_name, p_msg);

    // 保存序列号
    p_klpc->sequence = sdsdup(p_msg->sequence);

    ///< #1. true.成功; false.失败
    return 0;
}

/// @brief 主动关闭
static int lib_klua_klpc_close(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);
    free_klua_klpc(p_klpc);

    return 0;
}

/// @brief gc
static int lib_klua_klpc_gc(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);
    free_klua_klpc(p_klpc);

    return 0;
}

static int lib_klua_klpc_tostring(lua_State* L)
{
    klua_klpc_t* p_klpc = to_klua_klpc(L, 1);

    lua_pushfstring(L, "klpc:%p", p_klpc);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

static void klua_klpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "post",           lib_klua_klpc_post },
        { "call",           lib_klua_klpc_call },

        { "close",          lib_klua_klpc_close },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            lib_klua_klpc_gc},
        {"__close",         lib_klua_klpc_gc},
        {"__tostring",      lib_klua_klpc_tostring},
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


/// @brief 注册被(其他线程)调用函数
static int lib_klua_klpc_register(lua_State* L)
{
    size_t name_len = 0;
    const char* p_func_name = luaL_checklstring(L, 1, &name_len);   ///< @1. 函数名称
    luaL_checktype(L, 2, LUA_TFUNCTION);                            ///< @2. 函数

    klua_env_t* p_env = klua_env_get_by_L(L);
    klua_ex_lpc_t* p_ex = klua_ex_get_lpc(p_env);

    klua_ex_lpc_register_function(p_ex, p_func_name, name_len, 2);

    return 0;
}

/// @brief 响应被调用者
static int lib_klua_klpc_response(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);

    size_t sequence_len = 0;
    const char* p_sequence = luaL_checklstring(L, 2, &sequence_len);

    if (sequence_len <= 0)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    int argc = lua_gettop(L);
    int data_num = argc - 2;
    assert(0 <= data_num);

    klua_lpc_msg_t* p_msg = KLB_MALLOC(klua_lpc_msg_t, 1, data_num * sizeof(klua_data_t));
    KLB_MEMSET(p_msg, 0, sizeof(klua_lpc_msg_t) + data_num * sizeof(klua_data_t));

    p_msg->ex_name = sdsnew(KLUA_EX_LPC_NAME);
    p_msg->cb_destroy = klua_lpc_msg_destroy;
    p_msg->msg = KLUA_LPC_MSG_RESPONSE;
    p_msg->sequence = sdsnew(p_sequence);

    p_msg->data_num = data_num;
    for (int i = 0; i < data_num; i++)
    {
        klua_checkdata(L, &p_msg->data[i], i + 3);
    }

    klua_kthread_push_msg(p_name, p_msg);

    lua_pushboolean(L, true);
    return 1;
}

/// @brief 新建一个LPC
static int lib_klua_klpc_new(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    klua_ex_lpc_t* p_ex = klua_ex_get_lpc(p_env);

    klua_klpc_t* p_klpc = new_klua_klpc(L);
    p_klpc->p_env = p_env;
    p_klpc->L = L;
    p_klpc->p_ex = p_ex;

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_klpc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "register",           lib_klua_klpc_register },
        { "response",           lib_klua_klpc_response },

        { "new",                lib_klua_klpc_new },

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KLPC_HANDLE
    klua_klpc_createmeta(L);

    return 1;
}
