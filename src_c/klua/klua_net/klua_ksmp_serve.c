// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_net/klua_ksmp_serve.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klbsmp/klb_smpserve_conn.h"
#include "klbnet/klbsmp/klb_smpserverpc_conn.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// klua smp serve


#define KLUA_KSMPSERVE_HANDLE       "KSMPSERVE_HANDLE*"


/// @struct klua_ksmpserve_t
/// @brief  SMP服务连接
typedef struct klua_ksmpserve_t_
{
    // Lua相关
    struct
    {
        lua_State*                  L;              ///< L
        lua_State*                  co_recv;        ///< co_recv协程

        klua_env_t*                 p_env;          ///< lua环境
        klua_ex_coroutine_t*        p_coex;         ///< Lua协程扩展
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*             p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*              p_smp_conn;     ///< SMP 服务连接
    };

    // 数据
    struct
    {
        klb_nlist_t*                p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_ksmpserve_t;


////////////////////////////////////////
static klua_ksmpserve_t* new_klua_ksmpserve(lua_State* L)
{
    klua_ksmpserve_t* p_serve = (klua_ksmpserve_t*)lua_newuserdata(L, sizeof(klua_ksmpserve_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_ksmpserve_t));
    luaL_setmetatable(L, KLUA_KSMPSERVE_HANDLE);
    return p_serve;
}

static klua_ksmpserve_t* to_klua_ksmpserve(lua_State* L, int index)
{
    klua_ksmpserve_t* p_serve = (klua_ksmpserve_t*)luaL_checkudata(L, index, KLUA_KSMPSERVE_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'ksmpserve' expected");
    return p_serve;
}

static int klua_ksmpserve_tostring(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);

    lua_pushfstring(L, "ksmpserve:%p", p_serve);
    return 1;
}

static int klua_ksmpserve_close(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);

    if (NULL != p_serve->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_text_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_text_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_serve->p_smp_conn, klb_netconn_destroy);
    KLB_FREE_BY(p_serve->p_text_nlist, klb_nlist_destroy);

    return 0;
}

//////////////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_ksmpserve(klua_ksmpserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv && 0 < klb_nlist_size(p_serve->p_text_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行

        klb_buf_t* p_txt = klb_nlist_pop_head(p_serve->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // #1 body
            lua_pushstring(L, "text"); // #2 "text"
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程 结束
static int call_co_recv_end_text_klua_ksmpserve(klua_ksmpserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv)
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        lua_pushstring(L, ""); // #1 body
        lua_pushstring(L, "exit"); // #2 "text"

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包时触发
static int on_recv_data_klua_ksmpserve(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpserve_t* p_serve = (klua_ksmpserve_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_serve->p_text_nlist, p_data);

            call_co_recv_text_klua_ksmpserve(p_serve);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }

    return 0;
}

//////////////////////////////////////////////////
// 

static int klua_ksmpserve_send_text(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);                  // @1. self

    size_t head_len = 0;
    const char* p_head = (const char*)luaL_checklstring(L, 2, &head_len);   // @2. head

    const char* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3)) { p_body = (const char*)luaL_checklstring(L, 3, &body_len); } // @3. body

    int ret = 1;

    if (NULL != p_serve->p_smp_conn)
    {
        ret = klb_netconn_send_text(p_serve->p_smp_conn, 0, 0, (const uint8_t*)p_head, (int)head_len, (const uint8_t*)p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);            // #1.  错误码
    return 1;
}

static int klua_ksmpserve_send_media(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);      // @1. self
    klb_buf_t* p_buf = lua_touserdata(L, 2);                    // @2. 媒体数据

    int ret = 1;
    if (NULL != p_buf)
    {
        ret = klb_netconn_send_media(p_serve->p_smp_conn, p_buf);
    }

    lua_pushinteger(L, ret);            // #1.  错误码
    return 1;
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_klua_ksmpserve(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserve_t* p_serve = (klua_ksmpserve_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_text_klua_ksmpserve(p_serve);
    }

    return 0;
}

static int klua_ksmpserve_co_recv(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);
    klua_check_coroutine(L, "ksmpserve:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_serve->p_text_nlist);
    if (NULL != p_txt)
    {
        lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // #1. body
        lua_pushstring(L, "text"); // #2. "text"

        klb_nlist_pop_head(p_serve->p_text_nlist);
        KLB_FREE_BY(p_txt, klb_buf_unref_next);

        return 2;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_klua_ksmpserve, p_serve);
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserve_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpserve_close },            ///< 关闭连接

        { "send_text",      klua_ksmpserve_send_text },        ///< 发送文本数据
        { "send_media",     klua_ksmpserve_send_media },       ///< 发送媒体数据

        { "co_recv",        klua_ksmpserve_co_recv },          ///< 接收数据

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_ksmpserve_close },
        { "__close",        klua_ksmpserve_close },
        { "__tostring",     klua_ksmpserve_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPSERVE_HANDLE);    /* metatable for KLUA_KSMPSERVE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}

//////////////////////////////////////////////////

// 创建
static klua_ksmpserve_t* new_klua_ksmpserve_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_ksmpserve_t* p_serve = new_klua_ksmpserve(L);

    p_serve->L = L;
    p_serve->co_recv = NULL;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);

    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->p_text_nlist = klb_nlist_create();

    p_serve->p_smp_conn = klb_smpserve_conn_create(p_serve->p_netmulti, p_socket);

    klb_netconn_set_udata(p_serve->p_smp_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve->p_smp_conn, on_recv_data_klua_ksmpserve);

    return p_serve;
}

//////////////////////////////////////////////////////////////////////////
// klua smp serve listen

#define KLUA_KSMPSERVE_LISTEN_HANDLE    "KSMPSERVE_LISTEN_HANDLE*"


/// @struct klua_ksmpserve_listen_t
/// @brief  rtsp 服务监听
typedef struct klua_ksmpserve_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< sync的"co_accept"函数对应的协程

        klua_env_t*             p_env;          ///< Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    // 监听
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待读取的 socket 列表; 存储 klb_netlisten_socket_t*
    };
}klua_ksmpserve_listen_t;

////////////////////////////////////////
static klua_ksmpserve_listen_t* new_klua_ksmpserve_listen(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = (klua_ksmpserve_listen_t*)lua_newuserdata(L, sizeof(klua_ksmpserve_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_ksmpserve_listen_t));
    luaL_setmetatable(L, KLUA_KSMPSERVE_LISTEN_HANDLE);
    return p_listen;
}

static klua_ksmpserve_listen_t* to_klua_ksmpserve_listen(lua_State* L, int index)
{
    klua_ksmpserve_listen_t* p_listen = (klua_ksmpserve_listen_t*)luaL_checkudata(L, index, KLUA_KSMPSERVE_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ksmpserve listen' expected");
    return p_listen;
}

static int klua_ksmpserve_listen_tostring(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);

    lua_pushfstring(L, "ksmpserve listen:%p", p_listen);
    return 1;
}

static int klua_ksmpserve_listen_close(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);

    if (NULL != p_listen->p_listen_conn)
    {
        klb_netlisten_conn_close(p_listen->p_listen_conn);
    }

    if (NULL != p_listen->p_socket_nlist)
    {
        while (0 < klb_nlist_size(p_listen->p_socket_nlist))
        {
            klb_netlisten_socket_t* p_tmp = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

            KLB_SOCKET_CLOSE(p_tmp->fd);
            KLB_FREE(p_tmp);
        }
    }

    KLB_FREE_BY(p_listen->p_socket_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netconn_destroy);

    return 0;
}

//////////////////////////////////////////////////
// 

// call lua 协程
static int call_co_accept_klua_ksmpserve_listen(klua_ksmpserve_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_accept 函数; 这里会存在执行函数的交替执行

        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_ksmpserve_by_socket(L, p_listen_socket); // #1. lua - userdata
        KLB_FREE(p_listen_socket);

        int status = lua_pcall(L, 1, 0, 0);     /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// call lua 协程 结束
static int call_co_accept_end_klua_ksmpserve_listen(klua_ksmpserve_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        lua_pushlightuserdata(L, NULL); // #1. lua - userdata

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

/// @brief 当监听到socket连接建立时回调函数
static int on_accept_klua_ksmpserve_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_ksmpserve_listen_t* p_listen = (klua_ksmpserve_listen_t*)p_conn->p_udata;

    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);

    {
        p_tmp->fd = fd;
        memcpy(&p_tmp->addr, p_addr, sizeof(struct sockaddr_in));
        p_tmp->tls = tls;
    }

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);

    // call lua
    call_co_accept_klua_ksmpserve_listen(p_listen);

    return 0;
}

//////////////////////////////////////////////////

// 当需要退出等的 退出函数
static int on_yield_accept_klua_ksmpserve_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserve_listen_t* p_listen = (klua_ksmpserve_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_ksmpserve_listen(p_listen);
    }

    return 0;
}

static int klua_ksmpserve_listen_co_accept(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);
    klua_check_coroutine(L, "ksmpserve:co_accept must in coroutine!");

    // 只有一个协程
    assert(NULL == p_listen->co_accept);

    // 若列表中有数据, 则直接返回
    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_ksmpserve_by_socket(L, p_listen_socket); // #1. lua - userdata
        KLB_FREE(p_listen_socket);

        return 1;
    }

    // 若列表中无数据, 则执行协程等待
    p_listen->co_accept = L;

    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_ksmpserve_listen, p_listen);
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserve_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",              klua_ksmpserve_listen_close },          ///< 关闭监听

        { "co_accept",          klua_ksmpserve_listen_co_accept },      ///< 接收 新连接

        { NULL,                 NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",            NULL },  /* place holder */
        { "__gc",               klua_ksmpserve_listen_close },
        { "__close",            klua_ksmpserve_listen_close },
        { "__tostring",         klua_ksmpserve_listen_tostring },
        { NULL,                 NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPSERVE_LISTEN_HANDLE);/* metatable for KLUA_KSMPSERVE_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////
// k smp serve listen

/// @brief 监听端口
int klua_ksmpserve_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);                            ///< @1. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = klb_netlisten_conn_create(p_netmulti);

    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_ksmpserve_listen, NULL);
    klb_netlisten_conn_open(p_listen_conn, port, 20);

    klua_ksmpserve_listen_t* p_listen = new_klua_ksmpserve_listen(L);   ///< #1. 监听对象

    // 初始化
    {
        p_listen->L = L;
        p_listen->co_accept = NULL;
        p_listen->p_env = p_env;
        p_listen->p_coex = klua_coroutine_get(p_env);

        p_listen->p_netmulti = p_netmulti;
        p_listen->p_listen_conn = p_listen_conn;

        p_listen->p_socket_nlist = klb_nlist_create();

        klb_netconn_set_udata(p_listen_conn, p_listen);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// k lua smp serve RPC


#define KLUA_KSMPSERVERPC_HANDLE          "KSMPSERVERPC_HANDLE*"


/// @struct klua_ksmpserverpc_t
/// @brief  SMP服务RPC连接
typedef struct klua_ksmpserverpc_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 服务连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_ksmpserverpc_t;


////////////////////////////////////////
static klua_ksmpserverpc_t* new_klua_ksmpserverpc(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)lua_newuserdata(L, sizeof(klua_ksmpserverpc_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_ksmpserverpc_t));
    luaL_setmetatable(L, KLUA_KSMPSERVERPC_HANDLE);
    return p_serve;
}

static klua_ksmpserverpc_t* to_klua_ksmpserverpc(lua_State* L, int index)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)luaL_checkudata(L, index, KLUA_KSMPSERVERPC_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'ksmpserverpc' expected");
    return p_serve;
}

static int klua_ksmpserverpc_tostring(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);

    lua_pushfstring(L, "ksmpserverpc:%p", p_serve);
    return 1;
}

static int klua_ksmpserverpc_close(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);

    if (NULL != p_serve->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_text_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_text_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_serve->p_rtsp_conn, klb_netconn_destroy);
    KLB_FREE_BY(p_serve->p_text_nlist, klb_nlist_destroy);

    return 0;
}

//////////////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_ksmpserverpc(klua_ksmpserverpc_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv && 0 < klb_nlist_size(p_serve->p_text_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行
        klb_buf_t* p_txt = klb_nlist_pop_head(p_serve->p_text_nlist);

        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_txt->p_buf + p_txt->start);
        char* p_data = p_txt->p_buf + p_txt->start + sizeof(klb_mnp_rpc_t);
        int data_len = p_txt->end - p_txt->start - sizeof(klb_mnp_rpc_t);

        int num = klua_seri_map_binary_unpack(L, 1, p_data, data_len);  // #1 ~ #N. 展开lua数据

        KLB_FREE_BY(p_txt, klb_buf_unref);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);


        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程 结束
static int call_co_recv_end_text_klua_ksmpserverpc(klua_ksmpserverpc_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv)
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        lua_pushnil(L); // #1 空

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包时触发
static int on_recv_data_klua_ksmpserverpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_RPC_LUA == packtype ||
            KLB_MNP_RPC_JSON == packtype)
        {
            klb_nlist_push_tail(p_serve->p_text_nlist, p_data);

            call_co_recv_text_klua_ksmpserverpc(p_serve);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }

    return 0;
}

//////////////////////////////////////////////////
// 

// POST RPC 数据
static int klua_ksmpserverpc_post(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);      ///< @1 self
    klb_buf_t* p_data = klua_seri_map_binary_pack(L, 1);            ///< @2 ~ @N 参数

    int ret = klb_netconn_send_rpc_lua(p_serve->p_rtsp_conn, 0, 0, NULL, 0, p_data->p_buf + p_data->start, p_data->end - p_data->start);

    KLB_FREE_BY(p_data, klb_buf_unref);
    lua_pushinteger(L, ret);
    return 1;
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_klua_ksmpserverpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_text_klua_ksmpserverpc(p_serve);
    }

    return 0;
}

static int klua_ksmpserverpc_co_recv(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);
    klua_check_coroutine(L, "ksmpserverpc:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (0 < klb_nlist_size(p_serve->p_text_nlist))
    {
        klb_buf_t* p_txt = klb_nlist_pop_head(p_serve->p_text_nlist);

        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_txt->p_buf + p_txt->start);
        char* p_data = p_txt->p_buf + p_txt->start + sizeof(klb_mnp_rpc_t);
        int data_len = p_txt->end - p_txt->start - sizeof(klb_mnp_rpc_t);

        int num = klua_seri_map_binary_unpack(L, 1, p_data, data_len);  // #1 ~ #N. 展开lua数据

        klb_nlist_pop_head(p_serve->p_text_nlist);
        KLB_FREE_BY(p_txt, klb_buf_unref);

        return num;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_klua_ksmpserverpc, p_serve);
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserverpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpserverpc_close },              ///< 关闭连接

        { "post",           klua_ksmpserverpc_post },               ///< 发送RPC数据

        { "co_recv",        klua_ksmpserverpc_co_recv },            ///< 接收数据

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_ksmpserverpc_close },
        { "__close",        klua_ksmpserverpc_close },
        { "__tostring",     klua_ksmpserverpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPSERVERPC_HANDLE); /* metatable for KLUA_KSMPSERVERPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}

//////////////////////////////////////////////////
// 创建

static klua_ksmpserverpc_t* new_klua_ksmpserverpc_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_ksmpserverpc_t* p_serve = new_klua_ksmpserverpc(L);

    p_serve->L = L;
    p_serve->co_recv = NULL;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);

    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->p_text_nlist = klb_nlist_create();

    p_serve->p_rtsp_conn = klb_smpserverpc_conn_create(p_serve->p_netmulti, p_socket);

    klb_netconn_set_udata(p_serve->p_rtsp_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve->p_rtsp_conn, on_recv_data_klua_ksmpserverpc);

    return p_serve;
}


//////////////////////////////////////////////////////////////////////////
// klua smp serve rpc listen

#define KLUA_KSMPSERVERPC_LISTEN_HANDLE         "KSMPSERVERPC_LISTEN_HANDLE*"


/// @struct klua_ksmpserverpc_listen_t
/// @brief  SMP服务 RPC 监听
typedef struct klua_ksmpserverpc_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< sync的"co_accept"函数对应的协程

        klua_env_t*             p_env;          ///< Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    // 监听
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待读取的 socket 列表; 存储 klb_netlisten_socket_t*
    };
}klua_ksmpserverpc_listen_t;

////////////////////////////////////////
static klua_ksmpserverpc_listen_t* new_klua_ksmpserverpc_listen(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)lua_newuserdata(L, sizeof(klua_ksmpserverpc_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_ksmpserverpc_listen_t));
    luaL_setmetatable(L, KLUA_KSMPSERVERPC_LISTEN_HANDLE);
    return p_listen;
}

static klua_ksmpserverpc_listen_t* to_klua_ksmpserverpc_listen(lua_State* L, int index)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)luaL_checkudata(L, index, KLUA_KSMPSERVERPC_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ksmpserverpc listen' expected");
    return p_listen;
}

static int klua_ksmpserverpc_listen_tostring(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);

    lua_pushfstring(L, "ksmpserverpc listen:%p", p_listen);
    return 1;
}

static int klua_ksmpserverpc_listen_close(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);

    if (NULL != p_listen->p_listen_conn)
    {
        klb_netlisten_conn_close(p_listen->p_listen_conn);
    }

    if (NULL != p_listen->p_socket_nlist)
    {
        while (0 < klb_nlist_size(p_listen->p_socket_nlist))
        {
            klb_netlisten_socket_t* p_tmp = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

            KLB_SOCKET_CLOSE(p_tmp->fd);
            KLB_FREE(p_tmp);
        }
    }

    KLB_FREE_BY(p_listen->p_socket_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netconn_destroy);

    return 0;
}

//////////////////////////////////////////////////
// 

// call lua 协程
static int call_co_accept_klua_ksmpserverpc_listen(klua_ksmpserverpc_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_accept 函数; 这里会存在执行函数的交替执行

        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_ksmpserverpc_by_socket(L, p_listen_socket);    // #1. lua - userdata
        KLB_FREE(p_listen_socket);

        int status = lua_pcall(L, 1, 0, 0);                     /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// call lua 协程 结束
static int call_co_accept_end_klua_ksmpserverpc_listen(klua_ksmpserverpc_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        lua_pushlightuserdata(L, NULL); // #1. lua - userdata

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

/// @brief 当监听到socket连接建立时回调函数
static int on_accept_klua_ksmpserverpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)p_conn->p_udata;

    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);

    {
        p_tmp->fd = fd;
        memcpy(&p_tmp->addr, p_addr, sizeof(struct sockaddr_in));
        p_tmp->tls = tls;
    }

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);

    // call lua
    call_co_accept_klua_ksmpserverpc_listen(p_listen);

    return 0;
}

//////////////////////////////////////////////////

// 当需要退出等的 退出函数
static int on_yield_accept_klua_ksmpserverpc_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_ksmpserverpc_listen(p_listen);
    }

    return 0;
}

static int klua_ksmpserverpc_listen_co_accept(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    klua_check_coroutine(L, "ksmpserverpc:co_accept must in coroutine!");

    // 若列表中有数据, 则直接返回
    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_ksmpserverpc_by_socket(L, p_listen_socket);    // #1. lua - userdata
        KLB_FREE(p_listen_socket);

        return 2;
    }

    // 若列表中无数据, 则执行协程等待
    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_ksmpserverpc_listen, p_listen);
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserverpc_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_ksmpserverpc_listen_close },     ///< 关闭监听

        { "co_accept",      klua_ksmpserverpc_listen_co_accept }, ///< 接收 新连接

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_ksmpserverpc_listen_close },
        { "__close",        klua_ksmpserverpc_listen_close },
        { "__tostring",     klua_ksmpserverpc_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPSERVERPC_LISTEN_HANDLE);/* metatable for KLUA_KSMPSERVERPC_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////
// k smp serve RPC listen

/// @brief 监听 RPC 端口
int klua_ksmpserverpc_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);                        ///< @1. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = klb_netlisten_conn_create(p_netmulti);

    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_ksmpserverpc_listen, NULL);
    klb_netlisten_conn_open(p_listen_conn, port, 20);

    klua_ksmpserverpc_listen_t* p_listen = new_klua_ksmpserverpc_listen(L); ///< #1. 监听对象

    // 初始化
    {
        p_listen->L = L;
        p_listen->co_accept = NULL;
        p_listen->p_env = p_env;
        p_listen->p_coex = klua_coroutine_get(p_env);

        p_listen->p_netmulti = p_netmulti;
        p_listen->p_listen_conn = p_listen_conn;

        p_listen->p_socket_nlist = klb_nlist_create();

        klb_netconn_set_udata(p_listen_conn, p_listen);
    }

    return 1;
}

//end
