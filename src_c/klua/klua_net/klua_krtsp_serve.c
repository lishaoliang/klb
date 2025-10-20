// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_net/klua_krtsp_serve.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klbrtsp/klb_rtspserve_conn.h"
#include "klbutil/klb_nlist.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// klua rtsp serve

#define KLUA_KRTSPSERVE_HANDLE          "KRTSPSERVE_HANDLE*"

typedef struct klua_krtspserve_t_
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
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 服务连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_krtspserve_t;

////////////////////////////////////////
static klua_krtspserve_t* new_klua_krtspserve(lua_State* L)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)lua_newuserdata(L, sizeof(klua_krtspserve_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_krtspserve_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_HANDLE);
    return p_serve;
}

static klua_krtspserve_t* to_klua_krtspserve(lua_State* L, int index)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'krtspserve' expected");
    return p_serve;
}

static int klua_krtspserve_tostring(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    lua_pushfstring(L, "krtspserve:%p", p_serve);
    return 1;
}

static int klua_krtspserve_close(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

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

static int klua_krtspserve_send_text(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    return 0;
}

static int klua_krtspserve_co_recv(lua_State* L)
{
    klua_krtspserve_t* p_listen = to_klua_krtspserve(L, 1);
    klua_check_coroutine(L, "krtspserve:co_recv must in coroutine!");

    assert(NULL == p_listen->co_recv);
    p_listen->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtspserve_close },            ///< 

        { "send_text",      klua_krtspserve_send_text },        ///< 

        { "co_recv",        klua_krtspserve_co_recv },          ///< 

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_close },
        { "__close",        klua_krtspserve_close },
        { "__tostring",     klua_krtspserve_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_HANDLE);   /* metatable for KLUA_KRTSPSERVE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}



//////////////////////////////////////////////////////////////////////////
// klua rtsp serve listen

#define KLUA_KRTSPSERVE_LISTEN_HANDLE    "KRTSPSERVE_LISTEN_HANDLE*"


/// @struct klua_krtspserve_listen_socket_t
/// @brief  监听得到 socket
typedef struct klua_krtspserve_listen_socket_t_
{
    klb_socket_fd               fd;             ///< socket fd
    struct sockaddr_in          addr;           ///< 地址
    bool                        tls;            ///< tls
}klua_krtspserve_listen_socket_t;


/// @struct klua_krtspserve_listen_t
/// @brief  rtsp 服务监听
typedef struct klua_krtspserve_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< sync的"co_accept"函数对应的协程

        klua_env_t*             p_env;          ///< lua环境
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
        klb_nlist_t*            p_socket_nlist; ///< 待读取的 socket 列表; 存储 klua_krtspserve_listen_socket_t*
    };
}klua_krtspserve_listen_t;

////////////////////////////////////////
static klua_krtspserve_listen_t* new_klua_krtspserve_listen(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)lua_newuserdata(L, sizeof(klua_krtspserve_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_krtspserve_listen_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    return p_listen;
}

static klua_krtspserve_listen_t* to_klua_krtspserve_listen(lua_State* L, int index)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'krtspserve listen' expected");
    return p_listen;
}

static int klua_krtspserve_listen_tostring(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    lua_pushfstring(L, "krtspserve listen:%p", p_listen);
    return 1;
}

static int klua_krtspserve_listen_close(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    if (NULL != p_listen->p_listen_conn)
    {
        klb_netlisten_conn_close(p_listen->p_listen_conn);
    }

    if (NULL != p_listen->p_socket_nlist)
    {
        while (0 < klb_nlist_size(p_listen->p_socket_nlist))
        {
            klua_krtspserve_listen_socket_t* p_tmp = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

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

// 创建
static klua_krtspserve_t* new_klua_krtspserve_by_socket(klua_krtspserve_listen_t* p_listen, klua_krtspserve_listen_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_krtspserve_t* p_serve = new_klua_krtspserve(p_listen->L);

    p_serve->L = p_listen->L;
    p_serve->co_recv = NULL;
    p_serve->p_env = p_listen->p_env;

    p_serve->p_netmulti = p_listen->p_netmulti;

    p_serve->p_rtsp_conn = klb_rtspserve_conn_create(p_serve->p_netmulti, p_socket);

    p_serve->p_text_nlist = klb_nlist_create();

    return p_serve;
}

// call lua 协程
static int call_co_accept_klua_krtspserve_listen(klua_krtspserve_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        // Bug. 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_accept 函数; 这里会存在执行函数的交替执行

        klua_krtspserve_listen_socket_t* p_listen_socket = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        lua_pushstring(L, "ok"); // 
        new_klua_krtspserve_by_socket(p_listen, p_listen_socket); // 

        KLB_FREE(p_listen_socket);

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

/// @brief 当监听到socket连接建立时回调函数
static int on_accept_klua_krtspserve_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)p_conn->p_udata;

    klua_krtspserve_listen_socket_t* p_tmp = KLB_MALLOCZ(klua_krtspserve_listen_socket_t, 1, 0);

    {
        p_tmp->fd = fd;
        memcpy(&p_tmp->addr, p_addr, sizeof(struct sockaddr_in));
        p_tmp->tls = tls;
    }

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);

    // call lua
    call_co_accept_klua_krtspserve_listen(p_listen);

    return 0;
}

//////////////////////////////////////////////////

static int klua_krtspserve_listen_co_accept(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    klua_check_coroutine(L, "krtspserve:co_accept must in coroutine!");

    // 若列表中有数据, 则直接返回
    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klua_krtspserve_listen_socket_t* p_listen_socket = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        lua_pushstring(L, "ok"); // 
        new_klua_krtspserve_by_socket(p_listen, p_listen_socket); // 

        KLB_FREE(p_listen_socket);

        return 2;
    }

    // 若列表中无数据, 则执行协程等待
    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krtspserve_listen_close },     ///< 

        { "co_accept",      klua_krtspserve_listen_co_accept }, ///< 

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_listen_close },
        { "__close",        klua_krtspserve_listen_close },
        { "__tostring",     klua_krtspserve_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);/* metatable for KLUA_KRTSPSERVE_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////


/// @brief 监听端口
int klua_krtsp_serve_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);                        ///< @1. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = klb_netlisten_conn_create(p_netmulti);

    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_krtspserve_listen, NULL);
    klb_netlisten_conn_open(p_listen_conn, port, 20);

    klua_krtspserve_listen_t* p_listen = new_klua_krtspserve_listen(L);

    // 初始化
    {
        p_listen->L = L;
        p_listen->co_accept = NULL;
        p_listen->p_env = p_env;

        p_listen->p_netmulti = p_netmulti;
        p_listen->p_listen_conn = p_listen_conn;

        p_listen->p_socket_nlist = klb_nlist_create();

        klb_netconn_set_udata(p_listen_conn, p_listen);
    }

    return 1;
}

//end
