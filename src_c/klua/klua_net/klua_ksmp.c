// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klbsmp/klb_smpclient_conn.h"
#include "klbnet/klbsmp/klb_smpclientrpc_conn.h"
#include "klua/klua_net/klua_ksmp_serve.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// ksmp client


#define KLUA_KSMPCLIENT_HANDLE          "KSMPCLIENT_HANDLE*"        ///< Lua meta标示


/// @struct klua_ksmpclient_t
/// @brief  KLua-SMP客户端
typedef struct klua_ksmpclient_t_
{
    // Lua 相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_smp_conn;     ///< smp 客户端连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
        klb_nlist_t*            p_media_nlist;  ///< 待读取的 媒体列表; 存储 klb_buf_t*
    };
}klua_ksmpclient_t;


////////////////////////////////////////
static klua_ksmpclient_t* new_klua_ksmpclient(lua_State* L)
{
    klua_ksmpclient_t* p_client = (klua_ksmpclient_t*)lua_newuserdata(L, sizeof(klua_ksmpclient_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_ksmpclient_t));
    luaL_setmetatable(L, KLUA_KSMPCLIENT_HANDLE);
    return p_client;
}

static klua_ksmpclient_t* to_klua_ksmpclient(lua_State* L, int index)
{
    klua_ksmpclient_t* p_client = (klua_ksmpclient_t*)luaL_checkudata(L, index, KLUA_KSMPCLIENT_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'ksmpclient' expected");
    return p_client;
}

static int klua_ksmpclient_tostring(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);

    lua_pushfstring(L, "ksmpclient:%p", p_client);
    return 1;
}

static int klua_ksmpclient_close(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);

    // 清空 文本
    if (NULL != p_client->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    // 清空视频
    if (NULL != p_client->p_media_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_media_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_media_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_client->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_media_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_smp_conn, klb_netconn_destroy);

    return 0;
}

////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_ksmpclient(klua_ksmpclient_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_recv &&
        (0 < klb_nlist_size(p_client->p_text_nlist) || 0 < klb_nlist_size(p_client->p_media_nlist)))
    {
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行

        klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushstring(L, "text"); // 
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // body
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }

        klb_buf_t* p_media = klb_nlist_pop_head(p_client->p_media_nlist);
        if (NULL != p_media)
        {
            lua_pushstring(L, "media"); // 
            lua_pushlightuserdata(L, p_media);
        }

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_ksmpclient(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpclient_t* p_client = p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_client->p_text_nlist, p_data);

            call_co_recv_text_klua_ksmpclient(p_client);
        }
        else if (KLB_MNP_MEDIA == packtype)
        {
            klb_nlist_push_tail(p_client->p_media_nlist, p_data);

            call_co_recv_text_klua_ksmpclient(p_client);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }

    return 0;
}

////////////////////////////////////////

static int klua_ksmpclient_send_text(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);  ///< @1. self

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);    ///< @2. text head

    int ret = 1;

    if (NULL != p_client->p_smp_conn)
    {
        ret = klb_netconn_send_text(p_client->p_smp_conn, 0, 0, (const uint8_t*)p_head, head_len, NULL, 0);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_ksmpclient_co_recv(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);  ///< @1. self
    klua_check_coroutine(L, "ksmpclient:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_client->p_text_nlist);
    if (NULL != p_txt)
    {
        lua_pushstring(L, "text"); // "text"
        lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // body

        klb_nlist_pop_head(p_client->p_text_nlist);
        KLB_FREE_BY(p_txt, klb_buf_unref_next);

        return 2;
    }

    klb_buf_t* p_media = klb_nlist_head(p_client->p_media_nlist);
    if (NULL != p_media)
    {
        lua_pushstring(L, "media"); // "media"
        lua_pushlightuserdata(L, p_media); // C 指针

        klb_nlist_pop_head(p_client->p_media_nlist);

        return 2;
    }

    p_client->co_recv = L;
    return lua_yield(L, lua_gettop(L));
}

////////////////////////////////////////

static void klua_ksmpclient_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",             klua_ksmpclient_close },

        { "send_text",              klua_ksmpclient_send_text },

        { "co_recv",                klua_ksmpclient_co_recv },

        { NULL,                     NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",                NULL },  /* place holder */
        { "__gc",                   klua_ksmpclient_close },
        { "__close",                klua_ksmpclient_close },
        { "__tostring",             klua_ksmpclient_tostring },
        { NULL,                     NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPCLIENT_HANDLE);/* metatable for KLUA_KSMPCLIENT_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}


////////////////////////////////////////
// ksmp client

static int klua_ksmpclient_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);

    klb_netconn_t* p_smp_conn = klb_smpclient_connect(p_netmulti, p_host, port);


    if (NULL == p_smp_conn)
    {
        lua_pushnil(L);                                             ///< #1. nil
    }
    else
    {
        // 
        klua_ksmpclient_t* p_client = new_klua_ksmpclient(L);       ///< #1. meta: klua_ksmpclient_createmeta

        p_client->L = L;
        p_client->co_recv = NULL;
        p_client->p_env = p_env;

        p_client->p_netmulti = p_netmulti;
        p_client->p_smp_conn = p_smp_conn;

        p_client->p_text_nlist = klb_nlist_create();
        p_client->p_media_nlist = klb_nlist_create();

        // 绑定数据接收
        p_smp_conn->p_udata = p_client;
        klb_netconn_bind_recv_data(p_smp_conn, on_recv_data_klua_ksmpclient);
    }

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// ksmp client rpc

#define KLUA_KSMPCLIENTRPC_HANDLE               "KSMPCLIENTRPC_HANDLE*"       ///< Lua meta标示


typedef struct klua_ksmpclientrpc_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_rpc_conn;     ///< RPC 客户端连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_ksmpclientrpc_t;


////////////////////////////////////////
static klua_ksmpclientrpc_t* new_klua_ksmpclientrpc(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = (klua_ksmpclientrpc_t*)lua_newuserdata(L, sizeof(klua_ksmpclientrpc_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_ksmpclientrpc_t));
    luaL_setmetatable(L, KLUA_KSMPCLIENTRPC_HANDLE);
    return p_client;
}

static klua_ksmpclientrpc_t* to_klua_ksmpclientrpc(lua_State* L, int index)
{
    klua_ksmpclientrpc_t* p_client = (klua_ksmpclientrpc_t*)luaL_checkudata(L, index, KLUA_KSMPCLIENTRPC_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'ksmpclientrpc' expected");
    return p_client;
}

static int klua_ksmpclientrpc_tostring(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);

    lua_pushfstring(L, "ksmpclientrpc:%p", p_client);
    return 1;
}

static int klua_ksmpclientrpc_close(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);

    // 清空 文本
    if (NULL != p_client->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_client->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_rpc_conn, klb_netconn_destroy);

    return 0;
}

////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_recv && 0 < klb_nlist_size(p_client->p_text_nlist))
    {
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行

        klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushstring(L, "text"); // 
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // body
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_ksmpclientrpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpclientrpc_t* p_client = p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_client->p_text_nlist, p_data);

            call_co_recv_text_klua_ksmpclientrpc(p_client);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }

    return 0;
}

////////////////////////////////////////

// POST RPC, 无需等待回复
static int klua_ksmpclientrpc_post(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self
    klb_buf_t* p_data = klua_seri_map_binary_pack(L, 1);            ///< @2 ~ @N 参数

    int ret = klb_netconn_send_rpc_lua(p_client->p_rpc_conn, 0, 0, NULL, 0, p_data->p_buf + p_data->start, p_data->end - p_data->start);

    KLB_FREE_BY(p_data, klb_buf_unref);
    lua_pushinteger(L, ret);
    return 1;
}

// CO CALL RPC, 需要等待回复
static int klua_ksmpclientrpc_co_call(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self
    //klb_buf_t* p_data = klua_seri_map_binary_pack(L, 1);            ///< @2 ~ @N 参数


    return 0;
}

////////////////////////////////////////
// createmeta

static void klua_ksmpclientrpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpclientrpc_close },

        { "post",           klua_ksmpclientrpc_post },
        { "co_call",        klua_ksmpclientrpc_co_call },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_ksmpclientrpc_close },
        { "__close",         klua_ksmpclientrpc_close },
        { "__tostring",      klua_ksmpclientrpc_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPCLIENTRPC_HANDLE);/* metatable for KLUA_KSMPCLIENTRPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

////////////////////////////////////////
// ksmp client rpc

static int klua_ksmpclient_connect_rpc(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号

    // 
    klua_ksmpclientrpc_t* p_client = new_klua_ksmpclientrpc(L);     ///< #1. meta: klua_ksmpclientrpc_createmeta

    p_client->L = L;
    p_client->co_recv = NULL;
    p_client->p_env = klua_env_get_by_L(L);

    p_client->p_netmulti = klua_netmulti_get(p_client->p_env);
    p_client->p_rpc_conn = klb_smpclientrpc_connect(p_client->p_netmulti, p_host, port);

    p_client->p_text_nlist = klb_nlist_create();

    // 绑定数据接收
    klb_netconn_t* p_rpc_conn = p_client->p_rpc_conn;
    if (NULL != p_rpc_conn)
    {
        p_rpc_conn->p_udata = p_client;
        klb_netconn_bind_recv_data(p_rpc_conn, on_recv_data_klua_ksmpclientrpc);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// "ksmp"
int klua_open_ksmp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",                klua_ksmpclient_connect },          ///< 客户端 发起连接
        { "connect_rpc",            klua_ksmpclient_connect_rpc },      ///< 客户端 发起 RPC 连接

        { "listen",                 klua_ksmpserve_listen },            ///< 服务端 监听
        { "listen_rpc",             klua_ksmpserverpc_listen },         ///< 服务端 RPC 监听

        { NULL,                     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // k smp client
    klua_ksmpclient_createmeta(L);

    // k smp serve
    klua_ksmpserve_createmeta(L);

    // k smp serve listen
    klua_ksmpserve_listen_createmeta(L);

    // k smp client rpc
    klua_ksmpclientrpc_createmeta(L);

    // k smp serve rpc
    klua_ksmpserverpc_createmeta(L);

    // k smp serve rpc listen
    klua_ksmpserverpc_listen_createmeta(L);

    return 1;
}

//end
