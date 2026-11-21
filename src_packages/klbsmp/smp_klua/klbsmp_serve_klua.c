// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsmp/smp_klua/klbsmp_serve_klua.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbsmp/core/klb_smpserve_conn.h"
#include "klbsmp/core/klb_smpserverpc_conn.h"
#include "klbsmp/core/klb_smprpcer.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// klua smp serve


#define KLUA_KSMPSERVE_HANDLE       "KSMPSERVE_HANDLE*"


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_ksmpserve(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


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
        klb_nlist_t*                p_binary_nlist; ///< 待读取的 二进制列表; 存储 klb_buf_t*
        klb_nlist_t*                p_media_nlist;  ///< 待读取的 媒体列表; 存储 klb_buf_t*

        int                         recv_code;      ///< 非0时, 下一次 co_recv 返回 error
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

    if (NULL != p_serve->p_binary_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_binary_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_binary_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
        }
    }

    if (NULL != p_serve->p_media_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_media_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_media_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_serve->p_smp_conn, klb_netconn_destroy);
    KLB_FREE_BY(p_serve->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_serve->p_binary_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_serve->p_media_nlist, klb_nlist_destroy);

    return 0;
}

//////////////////////////////////////////////////

static int push_text_klua_ksmpserve(lua_State* L, const char* p_kind, klb_buf_t* p_data)
{
    int data_len = p_data->end - p_data->start;

    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        lua_pushstring(L, p_kind);
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    klb_mnp_text_t* p_txt = (klb_mnp_text_t*)(p_data->p_buf + p_data->start);
    const char* p_head = (const char*)(p_txt + 1);
    int head_len = (int)p_txt->head_size;
    int body_len = (int)p_txt->size - head_len - (int)sizeof(klb_mnp_text_t);

    if (0 > body_len)
    {
        body_len = 0;
    }

    lua_pushstring(L, p_kind);
    lua_pushlstring(L, p_head, (size_t)head_len);
    lua_pushlstring(L, p_head + head_len, (size_t)body_len);
    return 3;
}

static int push_media_klua_ksmpserve(lua_State* L, klb_buf_t* p_data)
{
    lua_pushstring(L, "media");
    lua_pushlightuserdata(L, p_data);
    return 2;
}

static int pop_recv_klua_ksmpserve(lua_State* L, klua_ksmpserve_t* p_serve)
{
    klb_buf_t* p_txt = klb_nlist_pop_head(p_serve->p_text_nlist);
    if (NULL != p_txt)
    {
        int n = push_text_klua_ksmpserve(L, "text", p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_bin = klb_nlist_pop_head(p_serve->p_binary_nlist);
    if (NULL != p_bin)
    {
        int n = push_text_klua_ksmpserve(L, "binary", p_bin);
        KLB_FREE_BY(p_bin, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_media = klb_nlist_pop_head(p_serve->p_media_nlist);
    if (NULL != p_media)
    {
        return push_media_klua_ksmpserve(L, p_media);
    }

    if (0 != p_serve->recv_code)
    {
        int code = p_serve->recv_code;
        p_serve->recv_code = 0;

        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    return 0;
}

static bool has_recv_klua_ksmpserve(klua_ksmpserve_t* p_serve)
{
    if (0 < klb_nlist_size(p_serve->p_text_nlist) ||
        0 < klb_nlist_size(p_serve->p_binary_nlist) ||
        0 < klb_nlist_size(p_serve->p_media_nlist) ||
        0 != p_serve->recv_code)
    {
        return true;
    }

    return false;
}

// 调用Lua协程
static int call_co_recv_klua_ksmpserve(klua_ksmpserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL == p_serve->co_recv || !has_recv_klua_ksmpserve(p_serve))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_serve->co_recv = NULL;

    int nargs = pop_recv_klua_ksmpserve(L, p_serve);
    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 调用Lua协程 结束
static int call_co_recv_end_klua_ksmpserve(klua_ksmpserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL == p_serve->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_serve->co_recv = NULL;

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
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
            call_co_recv_klua_ksmpserve(p_serve);
        }
        else if (KLB_MNP_BINARY == packtype)
        {
            klb_nlist_push_tail(p_serve->p_binary_nlist, p_data);
            call_co_recv_klua_ksmpserve(p_serve);
        }
        else if (KLB_MNP_MEDIA == packtype)
        {
            klb_nlist_push_tail(p_serve->p_media_nlist, p_data);
            call_co_recv_klua_ksmpserve(p_serve);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }
    else if (KLB_SOCKET_CONNECT == code || KLB_NETCODE_WBUF_EMPTY == code)
    {
        return 0;
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref_next);
        p_serve->recv_code = code;
        call_co_recv_klua_ksmpserve(p_serve);
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
        ret = klb_netconn_send_text(p_serve->p_smp_conn, 0, (const uint8_t*)p_head, (int)head_len, (const uint8_t*)p_body, (int)body_len);
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

/// @brief 发送背压: 写缓存未发送字节数
/// @note Lua: n = ksmpserve:buffered_amount()
static int klua_ksmpserve_buffered_amount(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);
    int64_t n = 0;

    if (NULL != p_serve->p_smp_conn)
    {
        n = klb_smpserve_wbuf_size(p_serve->p_smp_conn);
    }

    lua_pushinteger(L, (lua_Integer)n);
    return 1;
}

static int klua_ksmpserve_send_binary(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);

    size_t head_len = 0;
    const char* p_head = (const char*)luaL_checklstring(L, 2, &head_len);

    const char* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3))
    {
        p_body = (const char*)luaL_checklstring(L, 3, &body_len);
    }

    int ret = 1;
    if (NULL != p_serve->p_smp_conn)
    {
        ret = klb_netconn_send_binary(p_serve->p_smp_conn, 0,
            (const uint8_t*)p_head, (int)head_len, (const uint8_t*)p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_ksmpserve_free_media(lua_State* L)
{
    to_klua_ksmpserve(L, 1);
    klb_buf_t* p_media = (klb_buf_t*)lua_touserdata(L, 2);

    KLB_FREE_BY(p_media, klb_buf_unref);
    return 0;
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_klua_ksmpserve(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserve_t* p_serve = (klua_ksmpserve_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_ksmpserve(p_serve);
    }

    return 0;
}

static int klua_ksmpserve_co_recv(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);
    klua_check_coroutine(L, "ksmpserve:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (has_recv_klua_ksmpserve(p_serve))
    {
        return pop_recv_klua_ksmpserve(L, p_serve);
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_klua_ksmpserve, p_serve);
}


/// @brief 最近收包时刻 (含 PING/PONG); 供代理泵送刷新空闲计时
static int klua_ksmpserve_last_recv_ms(lua_State* L)
{
    klua_ksmpserve_t* p_serve = to_klua_ksmpserve(L, 1);

    if (NULL == p_serve->p_smp_conn || NULL == p_serve->p_smp_conn->p_socket)
    {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)p_serve->p_smp_conn->p_socket->last_recv_tc);
    return 1;
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserve_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpserve_close },            ///< 关闭连接

        { "send_text",      klua_ksmpserve_send_text },        ///< 发送文本数据
        { "send_binary",    klua_ksmpserve_send_binary },      ///< 发送二进制数据
        { "send_media",     klua_ksmpserve_send_media },       ///< 发送媒体数据
        { "buffered_amount", klua_ksmpserve_buffered_amount }, ///< 写缓存未发送字节数

        { "co_recv",        klua_ksmpserve_co_recv },          ///< 接收数据
        { "free_media",     klua_ksmpserve_free_media },       ///< 释放媒体包
        { "last_recv_ms",   klua_ksmpserve_last_recv_ms },     ///< 最近收包时刻

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
    p_serve->p_binary_nlist = klb_nlist_create();
    p_serve->p_media_nlist = klb_nlist_create();
    p_serve->recv_code = 0;

    p_serve->p_smp_conn = klb_smpserve_conn_create(p_serve->p_netmulti, p_socket);

    klb_netconn_set_udata(p_serve->p_smp_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve->p_smp_conn, on_recv_data_klua_ksmpserve);

    return p_serve;
}


/// @brief 由已探测 sock 创建 SMP 服务连接; 可选 `{cert,key}` 供 TLS(smps)
int klua_ksmpserve_new_serve(lua_State* L)
{
    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)lua_touserdata(L, 1);

    if (NULL == p_listen_socket)
    {
        return push_err_klua_ksmpserve(L, KLB_NETCODE_EINVAL);
    }

    bool tls = p_listen_socket->tls;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    bool has_ping = false;
    int ping_interval_ms = -1;
    int ping_timeout_ms = -1;

    if (klua_is_table(L, 2))
    {
        lua_getfield(L, 2, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, tls);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "cert");
        if (klua_is_string(L, -1))
        {
            p_cert = lua_tolstring(L, -1, &cert_len);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "key");
        if (klua_is_string(L, -1))
        {
            p_key = lua_tolstring(L, -1, &key_len);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "ping_interval_ms");
        if (lua_isnumber(L, -1))
        {
            ping_interval_ms = (int)lua_tointeger(L, -1);
            has_ping = true;
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "ping_timeout_ms");
        if (lua_isnumber(L, -1))
        {
            ping_timeout_ms = (int)lua_tointeger(L, -1);
            has_ping = true;
        }
        lua_pop(L, 1);
    }

    klb_socket_fd fd = p_listen_socket->fd;
    KLB_FREE(p_listen_socket);

    klb_socket_t* p_socket = NULL;
    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(
            p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_ENOMEM);
        }
    }

    klua_ksmpserve_t* p_serve = new_klua_ksmpserve(L);

    p_serve->L = L;
    p_serve->co_recv = NULL;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);

    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->p_text_nlist = klb_nlist_create();
    p_serve->p_binary_nlist = klb_nlist_create();
    p_serve->p_media_nlist = klb_nlist_create();
    p_serve->recv_code = 0;

    p_serve->p_smp_conn = klb_smpserve_conn_create(p_serve->p_netmulti, p_socket);

    if (has_ping)
    {
        klb_smpserve_conn_set_ping(p_serve->p_smp_conn, ping_interval_ms, ping_timeout_ms);
    }

    klb_netconn_set_udata(p_serve->p_smp_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve->p_smp_conn, on_recv_data_klua_ksmpserve);

    return 1;
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

    // 释放 监听连接
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);

    // 释放 已经监听得到的连接数据
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

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
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
static int on_accept_klua_ksmpserve_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
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

static int klua_ksmpserve_listen_port(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_ksmpserve_listen_local_addr(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_ksmpserve_listen_native_fd(lua_State* L)
{
    klua_ksmpserve_listen_t* p_listen = to_klua_ksmpserve_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

void klua_ksmpserve_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",              klua_ksmpserve_listen_close },          ///< 关闭监听

        { "co_accept",          klua_ksmpserve_listen_co_accept },      ///< 接收 新连接
        { "port",               klua_ksmpserve_listen_port },
        { "local_addr",         klua_ksmpserve_listen_local_addr },
        { "native_fd",          klua_ksmpserve_listen_native_fd },

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
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_ksmpserve_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_ksmpserve(L, rc);
        }
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_ksmpserve(L, KLB_NETCODE_EINVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_ksmpserve_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return push_err_klua_ksmpserve(L, rc);
    }

    p_listen = new_klua_ksmpserve_listen(L);   ///< #1. 监听对象

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
    // klua 相关
    struct
    {
        lua_State*              L;              ///< (只读)L
        klua_env_t*             p_env;          ///< (只读)lua环境
        klua_ex_coroutine_t*    p_coex;         ///< (只读)Lua协程扩展
        klb_netmulti_t*         p_netmulti;     ///< (只读)复用; net multiplex
    };

    // lua 协程
    struct
    {
        lua_State*              co_recv;        ///< co_recv协程
    };

    // RPC数据
    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< 待读取的 RPC 数据列表; 存储 klb_buf_t*

        klb_mnp_rpc_t           rpc;            ///< 上一个RPC信息
        klb_buf_t*              p_rpc_buf;      ///< RPC数据
    };

    // 网络相关
    struct
    {

        klb_netconn_t*          p_serve_conn;    ///< SMP-RPC 服务连接
    };
}klua_ksmpserverpc_t;


////////////////////////////////////////
static klua_ksmpserverpc_t* new_klua_ksmpserverpc(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)lua_newuserdata(L, sizeof(klua_ksmpserverpc_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_ksmpserverpc_t));
    luaL_setmetatable(L, KLUA_KSMPSERVERPC_HANDLE);

    // 初始化 只读 参数
    {
        p_serve->L = L;
        p_serve->p_env = klua_env_get_by_L(L);
        p_serve->p_coex = klua_coroutine_get(p_serve->p_env);
        p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    }

    // 初始化 其他参数
    {
        p_serve->co_recv = NULL;
        p_serve->p_rpc_nlist = klb_nlist_create();
    }

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

    // 释放 服务连接
    KLB_FREE_BY(p_serve->p_serve_conn, klb_smpserverpc_conn_free);

    // 释放 已经获取的 RPC 数据
    if (NULL != p_serve->p_rpc_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_rpc_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_rpc_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_serve->p_rpc_nlist, klb_nlist_destroy);

    // 释放 临时缓存
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    return 0;
}

//////////////////////////////////////////////////

// 放入/更新 RPC 数据
static void update_rpc_klua_ksmpserverpc(klua_ksmpserverpc_t* p_serve, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_serve->rpc, p_rpc, sizeof(klb_mnp_rpc_t));

        p_serve->p_rpc_buf = p_buf;
    }
}

static int push_rpc_body_klua_ksmpserverpc(lua_State* L, klb_buf_t* p_buf)
{
    klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
    char* p_data = p_buf->p_buf + p_buf->start + sizeof(klb_mnp_rpc_t);
    int data_len = p_buf->end - p_buf->start - (int)sizeof(klb_mnp_rpc_t);

    if (0 > data_len)
    {
        data_len = 0;
    }

    if (KLB_MNP_RPC_JSON == p_rpc->rpctype)
    {
        lua_pushlstring(L, p_data, (size_t)data_len);
        return 1;
    }

    return klua_seri_map_binary_unpack(L, 1, p_data, data_len);
}

// 调用Lua协程
static int call_co_recv_klua_ksmpserverpc(klua_ksmpserverpc_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv && 0 < klb_nlist_size(p_serve->p_rpc_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行
        klb_buf_t* p_buf = klb_nlist_pop_head(p_serve->p_rpc_nlist);
        int num = push_rpc_body_klua_ksmpserverpc(L, p_buf);
        update_rpc_klua_ksmpserverpc(p_serve, p_buf);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程 结束
static int call_co_recv_end_klua_ksmpserverpc(klua_ksmpserverpc_t* p_serve)
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
        bool need_free = true;

        if (KLB_MNP_RPC_LUA == packtype ||
            KLB_MNP_RPC_JSON == packtype)
        {
            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_data->p_buf + p_data->start);

            if (KLB_MNP_RPC_POST == p_rpc->method ||
                KLB_MNP_RPC_NOTIFY == p_rpc->method ||
                KLB_MNP_RPC_REQUEST == p_rpc->method)
            {
                need_free = false;

                klb_nlist_push_tail(p_serve->p_rpc_nlist, p_data);
                call_co_recv_klua_ksmpserverpc(p_serve);
            }
        }

        if (need_free) { KLB_FREE_BY(p_data, klb_buf_unref); }
    }
    else if (KLB_SOCKET_CONNECT != code && KLB_NETCODE_WBUF_EMPTY != code)
    {
        call_co_recv_end_klua_ksmpserverpc(p_serve);
    }

    return 0;
}

//////////////////////////////////////////////////
// 

/// @brief 发送 RPC 数据
/// @param [in]     method              RPC方法: eg. KLB_MNP_RPC_POST, KLB_MNP_RPC_NOTIFY, KLB_MNP_RPC_RESPONSE
/// @return int Lua返回参数个数
static int klua_ksmpserverpc_send(lua_State* L, klb_mnp_rpc_method_e method)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);      ///< @1 self
    int rpctype = (int)luaL_checkinteger(L, 2);                     ///< @2 RPC类型: KLB_MNP_RPC_LUA / KLB_MNP_RPC_JSON
    
    int sequence = 0, base_idx = 2;
    if (KLB_MNP_RPC_RESPONSE == method)
    {
        sequence = (int)luaL_checkinteger(L, base_idx + 1);         ///< @3 sequence : 序号
        base_idx += 1; // 偏移
    }

    int ret = 1;

    if (KLB_MNP_RPC_LUA == rpctype)
    {
        ///< POST/NOTIFY : @3 ~ @N 参数
        ///< RESPONSE : @4 ~ @N 参数
        klb_buf_t* p_data = klua_seri_map_binary_pack(L, base_idx);         ///< base_idx = 2时: @3 ~ @N 参数

        ret = klb_smpserverpc_conn_send_buf(p_serve->p_serve_conn, KLB_MNP_RPC_LUA, method, sequence, p_data);

        if (0 != ret) { KLB_FREE_BY(p_data, klb_buf_unref); }
    }
    else if (KLB_MNP_RPC_JSON == rpctype)
    {
        ///< POST/NOTIFY : @3 JSON数据
        ///< RESPONSE : @4 JSON数据
        size_t json_len = 0;
        const char* p_json = luaL_checklstring(L, base_idx + 1, &json_len);    ///< base_idx = 2时: @3 JSON数据

        ret = klb_smpserverpc_conn_send(p_serve->p_serve_conn, KLB_MNP_RPC_JSON, method, sequence, (const uint8_t*)p_json, (int)json_len);
    }

    lua_pushinteger(L, ret);            ///< #1 错误码
    return 1;
}

// POST RPC 数据
static int klua_ksmpserverpc_post(lua_State* L)
{
    return klua_ksmpserverpc_send(L, KLB_MNP_RPC_POST);
}

// NOTIFY RPC 数据
static int klua_ksmpserverpc_notify(lua_State* L)
{
    return klua_ksmpserverpc_send(L, KLB_MNP_RPC_NOTIFY);
}

// RESPONSE RPC 数据
static int klua_ksmpserverpc_response(lua_State* L)
{
    return klua_ksmpserverpc_send(L, KLB_MNP_RPC_RESPONSE);
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_ksmpserverpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpserverpc_t* p_serve = (klua_ksmpserverpc_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_ksmpserverpc(p_serve);
    }

    return 0;
}

// co recv 数据
static int klua_ksmpserverpc_co_recv(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);
    klua_check_coroutine(L, "ksmpserverpc:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (0 < klb_nlist_size(p_serve->p_rpc_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_serve->p_rpc_nlist);
        int num = push_rpc_body_klua_ksmpserverpc(L, p_buf);

        update_rpc_klua_ksmpserverpc(p_serve, p_buf);

        return num;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_ksmpserverpc, p_serve);
}

// 获取当前 状态
static int klua_ksmpserverpc_status(lua_State* L)
{
    klua_ksmpserverpc_t* p_serve = to_klua_ksmpserverpc(L, 1);

    lua_newtable(L);

    {
        klua_setfield_string(L, "rpctype", klb_smprpcer_to_rpctype_string(p_serve->rpc.rpctype)); // RPC数据类型
        klua_setfield_string(L, "method", klb_smprpcer_to_method_string(p_serve->rpc.method)); // 方法
        klua_setfield_integer(L, "sequence", p_serve->rpc.sequence); // 序号
    }

    return 1; ///< #1 table
}

//////////////////////////////////////////////////
// createmeta

void klua_ksmpserverpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpserverpc_close },              ///< 关闭连接

        { "post",           klua_ksmpserverpc_post },               ///< 提交RPC数据
        { "notify",         klua_ksmpserverpc_notify },             ///< 通知RPC数据
        { "response",       klua_ksmpserverpc_response },           ///< 回复RPC数据

        { "co_recv",        klua_ksmpserverpc_co_recv },            ///< 接收数据

        { "status",         klua_ksmpserverpc_status },             ///< 获取当前 状态

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

static klua_ksmpserverpc_t* new_klua_ksmpserverpc_by_klb_socket(lua_State* L, klb_socket_t* p_socket)
{
    klua_ksmpserverpc_t* p_serve = new_klua_ksmpserverpc(L);
    klb_netconn_t* p_serve_conn = NULL;

    assert(NULL != p_socket);

    p_serve_conn = klb_smpserverpc_conn_create(p_serve->p_netmulti, p_socket);
    klb_netconn_set_udata(p_serve_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve_conn, on_recv_data_klua_ksmpserverpc);
    p_serve->p_serve_conn = p_serve_conn;

    return p_serve;
}


static klua_ksmpserverpc_t* new_klua_ksmpserverpc_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);

    return new_klua_ksmpserverpc_by_klb_socket(L, p_socket);
}


//////////////////////////////////////////////////////////////////////////
// klua smp serve rpc listen

#define KLUA_KSMPSERVERPC_LISTEN_HANDLE         "KSMPSERVERPC_LISTEN_HANDLE*"


/// @struct klua_ksmpserverpc_listen_t
/// @brief  SMP服务 RPC 监听
typedef struct klua_ksmpserverpc_listen_t_
{
    // klua 相关
    struct
    {
        lua_State*              L;              ///< (只读)L
        klua_env_t*             p_env;          ///< (只读)Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< (只读)Lua协程扩展
        klb_netmulti_t*         p_netmulti;     ///< (只读)复用; net multiplex
    };

    // 数据
    struct
    {
        lua_State*              co_accept;      ///< "co_accept"函数对应的协程

        klb_nlist_t*            p_socket_nlist; ///< 待读取的 socket 列表; 存储 klb_netlisten_socket_t*
    };

    // 网络连接
    struct
    {
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };
}klua_ksmpserverpc_listen_t;


////////////////////////////////////////
static klua_ksmpserverpc_listen_t* new_klua_ksmpserverpc_listen(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)lua_newuserdata(L, sizeof(klua_ksmpserverpc_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_ksmpserverpc_listen_t));
    luaL_setmetatable(L, KLUA_KSMPSERVERPC_LISTEN_HANDLE);

    // 初始化 只读 参数
    {
        p_listen->L = L;
        p_listen->p_env = klua_env_get_by_L(L);
        p_listen->p_coex = klua_coroutine_get(p_listen->p_env);
        p_listen->p_netmulti = klua_netmulti_get(p_listen->p_env);
    }

    // 初始化 其他参数
    {
        p_listen->co_accept = NULL;
        p_listen->p_socket_nlist = klb_nlist_create();
    }

    return p_listen;
}

static klua_ksmpserverpc_listen_t* to_klua_ksmpserverpc_listen(lua_State* L, int index)
{
    klua_ksmpserverpc_listen_t* p_listen = (klua_ksmpserverpc_listen_t*)luaL_checkudata(L, index, KLUA_KSMPSERVERPC_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ksmpserverpc.listen' expected");
    return p_listen;
}

static int klua_ksmpserverpc_listen_tostring(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    int port = (NULL != p_listen->p_listen_conn) ? klb_netlisten_conn_get_port(p_listen->p_listen_conn) : 0;

    lua_pushfstring(L, "ksmpserverpc.listen:[%p],port:[%d]", p_listen, port);
    return 1;
}

static int klua_ksmpserverpc_listen_close(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);

    // 释放 监听连接
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);

    // 释放 已经 监听得到的 socket
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

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
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

    if (NULL != p_listen->co_accept)
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
static int on_accept_klua_ksmpserverpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
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

static int klua_ksmpserverpc_listen_port(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_ksmpserverpc_listen_local_addr(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_ksmpserverpc_listen_native_fd(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static int klua_ksmpserverpc_listen_co_accept(lua_State* L)
{
    klua_ksmpserverpc_listen_t* p_listen = to_klua_ksmpserverpc_listen(L, 1);
    klua_check_coroutine(L, "ksmpserverpc.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    // 若列表中有数据, 则直接返回
    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_ksmpserverpc_by_socket(L, p_listen_socket);    // #1. lua - userdata
        KLB_FREE(p_listen_socket);

        return 1;
    }

    // 若列表中无数据, 则执行协程等待 
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
        { "port",           klua_ksmpserverpc_listen_port },
        { "local_addr",     klua_ksmpserverpc_listen_local_addr },
        { "native_fd",      klua_ksmpserverpc_listen_native_fd },

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
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_ksmpserverpc_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_ksmpserve(L, rc);
        }
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_ksmpserve(L, KLB_NETCODE_EINVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_ksmpserverpc_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return push_err_klua_ksmpserve(L, rc);
    }

    p_listen = new_klua_ksmpserverpc_listen(L); ///< #1. 监听对象
    klb_netconn_set_udata(p_listen_conn, p_listen);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_ksmpserverpc_listen, p_listen);
    p_listen->p_listen_conn = p_listen_conn;

    return 1;
}


/// @brief 由已探测 sock 创建 SMP-RPC 服务连接; 可选 `{cert,key}` 供 TLS(smprpcs)
int klua_ksmpserverpc_new_serve(lua_State* L)
{
    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)lua_touserdata(L, 1);
    bool tls = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;

    if (NULL == p_listen_socket)
    {
        return push_err_klua_ksmpserve(L, KLB_NETCODE_EINVAL);
    }

    tls = p_listen_socket->tls;

    if (klua_is_table(L, 2))
    {
        lua_getfield(L, 2, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, tls);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "cert");
        if (klua_is_string(L, -1))
        {
            p_cert = lua_tolstring(L, -1, &cert_len);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "key");
        if (klua_is_string(L, -1))
        {
            p_key = lua_tolstring(L, -1, &key_len);
        }
        lua_pop(L, 1);
    }

    fd = p_listen_socket->fd;
    KLB_FREE(p_listen_socket);

    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(
            p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_ksmpserve(L, KLB_NETCODE_ENOMEM);
        }
    }

    new_klua_ksmpserverpc_by_klb_socket(L, p_socket);
    return 1;
}

// end
