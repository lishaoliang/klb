// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klbutil/klb_nlist.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klbrtsp/klb_rtspclient_conn.h"
#include "klua/klua_net/klua_krtsp_serve.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// rtsp client

#define KLUA_KRTSPCLIENT_HANDLE                 "KRTSPCLIENT_HANDLE*"       ///< Lua meta标示


typedef struct klua_krtsp_client_t_
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
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 客户端连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_krtsp_client_t;


////////////////////////////////////////
static klua_krtsp_client_t* new_klua_krtsp_client(lua_State* L)
{
    klua_krtsp_client_t* p_client = (klua_krtsp_client_t*)lua_newuserdata(L, sizeof(klua_krtsp_client_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_krtsp_client_t));
    luaL_setmetatable(L, KLUA_KRTSPCLIENT_HANDLE);
    return p_client;
}

static klua_krtsp_client_t* to_klua_krtsp_client(lua_State* L, int index)
{
    klua_krtsp_client_t* p_client = (klua_krtsp_client_t*)luaL_checkudata(L, index, KLUA_KRTSPCLIENT_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'krtsp-client' expected");
    return p_client;
}

static int klua_krtsp_client_tostring(lua_State* L)
{
    klua_krtsp_client_t* p_client = to_klua_krtsp_client(L, 1);

    lua_pushfstring(L, "krtsp-client:%p", p_client);
    return 1;
}

static int klua_krtsp_client_close(lua_State* L)
{
    klua_krtsp_client_t* p_client = to_klua_krtsp_client(L, 1);

    // 清空
    if (NULL != p_client->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_text_nlist))
        {
            klb_nlist_pop_head(p_client->p_text_nlist);
        }

    }

    KLB_FREE_BY(p_client->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_rtsp_conn, klb_netconn_destroy);

    return 0;
}

////////////////////////////////////////

static int call_co_recv_text_klua_krtsp_client(klua_krtsp_client_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_recv && 0 < klb_nlist_size(p_client->p_text_nlist))
    {
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv(klua_krtsp_client_co_recv) 函数; 这里会存在执行函数的交替执行
        klb_buf_t* p_buf = klb_nlist_pop_head(p_client->p_text_nlist);

        lua_pushstring(L, "text"); // 
        lua_pushlstring(L, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start); // body

        KLB_FREE_BY(p_buf, klb_buf_unref);

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int on_recv_text_klua_krtsp_client(klua_krtsp_client_t* p_client)
{
    call_co_recv_text_klua_krtsp_client(p_client);

    return 0;
}

// 数据接收
static int on_recv_data_klua_krtsp_client(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_krtsp_client_t* p_client = p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_client->p_text_nlist, p_data);

            on_recv_text_klua_krtsp_client(p_client);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }

    return 0;
}

////////////////////////////////////////

static int klua_krtsp_client_send_text(lua_State* L)
{
    klua_krtsp_client_t* p_client = to_klua_krtsp_client(L, 1);  ///< @1. self

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);    ///< @2. text head

    int ret = 1;

    if (NULL != p_client->p_rtsp_conn)
    {
        ret = klb_netconn_send_text(p_client->p_rtsp_conn, 0, 0, (const uint8_t*)p_head, head_len, NULL, 0);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_krtsp_client_co_recv(lua_State* L)
{
    klua_krtsp_client_t* p_client = to_klua_krtsp_client(L, 1);  ///< @1. self
    klua_check_coroutine(L, "co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    klb_buf_t* p_buf = klb_nlist_head(p_client->p_text_nlist);
    if (NULL != p_buf)
    {
        lua_pushstring(L, "text"); // "text"
        lua_pushlstring(L, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start); // body

        klb_nlist_pop_head(p_client->p_text_nlist);
        KLB_FREE_BY(p_buf, klb_buf_unref);

        return 2;
    }
    else
    {
        p_client->co_recv = L;
        return lua_yield(L, lua_gettop(L));
    }
}

////////////////////////////////////////

static void klua_krtsp_client_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtsp_client_close },

        { "send_text",      klua_krtsp_client_send_text },

        { "co_recv",        klua_krtsp_client_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krtsp_client_close },
        { "__close",         klua_krtsp_client_close },
        { "__tostring",      klua_krtsp_client_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPCLIENT_HANDLE);/* metatable for KLUA_KRTSP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}


////////////////////////////////////////
// rtsp client

static int klua_krtsp_client_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);

    klb_netconn_t* p_rtsp_conn = klb_rtspclient_connect(p_netmulti, p_host, port);

    if (NULL == p_rtsp_conn)
    {
        lua_pushnil(L);                                             ///< #1. nil
    }
    else
    {
        // 
        klua_krtsp_client_t* p_client = new_klua_krtsp_client(L);   ///< #1. meta: klua_krtsp_client_createmeta

        p_client->L = L;
        p_client->co_recv = NULL;
        p_client->p_env = p_env;

        p_client->p_netmulti = p_netmulti;
        p_client->p_rtsp_conn = p_rtsp_conn;

        p_client->p_text_nlist = klb_nlist_create();


        // 绑定数据接收
        p_rtsp_conn->p_udata = p_client;
        klb_netconn_bind_recv(p_rtsp_conn, on_recv_data_klua_krtsp_client);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// rtsp
int klua_open_krtsp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",            klua_krtsp_client_connect },        ///< 客户端 发起连接

        { "listen",             klua_krtsp_serve_listen },          ///< 服务端 监听

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // k rtsp client
    klua_krtsp_client_createmeta(L);

    // k rtsp serve
    klua_krtspserve_createmeta(L);
    
    // k rtsp serve listen
    klua_krtspserve_listen_createmeta(L);

    return 1;
}

//end
