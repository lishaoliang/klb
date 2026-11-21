// Doc Encode : UTF-8 BOM, Unix(LF)
// klbhttp Lua 绑定: require("khttp")
#include "klbhttp/klbhttp.h"
#include "klbhttp/core/klb_httpclient_conn.h"
#include "klbhttp/core/klb_httpserve_conn.h"
#include "klbhttp/http_klua/klbhttp_rpc_klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klblisten/klb_netprobe_conn.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// khttp 连接 (client / serve 共用 userdata)

#define KLUA_KHTTP_HANDLE               "KHTTP_HANDLE*"             ///< Lua meta标示


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_khttp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @struct klua_khttp_t
/// @brief  khttp 连接
typedef struct klua_khttp_t_
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
        klb_netconn_t*          p_conn;         ///< HTTP 连接
        bool                    is_client;      ///< true.客户端; false.服务端
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 HTTP 文本; 存储 klb_buf_t*
        int                     recv_code;      ///< 最近一次非0网络码; 0.无
        char                    peer[128];      ///< 对端 "ip:port"
    };
}klua_khttp_t;


////////////////////////////////////////
static klua_khttp_t* new_klua_khttp(lua_State* L)
{
    klua_khttp_t* p_http = (klua_khttp_t*)lua_newuserdata(L, sizeof(klua_khttp_t));
    KLB_MEMSET(p_http, 0, sizeof(klua_khttp_t));
    luaL_setmetatable(L, KLUA_KHTTP_HANDLE);
    return p_http;
}

static klua_khttp_t* to_klua_khttp(lua_State* L, int index)
{
    klua_khttp_t* p_http = (klua_khttp_t*)luaL_checkudata(L, index, KLUA_KHTTP_HANDLE);
    luaL_argcheck(L, NULL != p_http, index, "'khttp' expected");
    return p_http;
}

static int klua_khttp_tostring(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    lua_pushfstring(L, "khttp:%p", p_http);
    return 1;
}

static int klua_khttp_close(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    if (NULL != p_http->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_http->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_http->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_http->p_text_nlist, klb_nlist_destroy);

    if (NULL != p_http->p_conn)
    {
        klb_netconn_bind_recv_data(p_http->p_conn, NULL);
        klb_netconn_set_udata(p_http->p_conn, NULL);

        if (p_http->is_client)
        {
            klb_httpclient_conn_free(p_http->p_conn);
        }
        else
        {
            klb_httpserve_conn_free(p_http->p_conn);
        }

        p_http->p_conn = NULL;
    }

    return 0;
}

////////////////////////////////////////

// 压栈 HTTP TEXT 包: "text", head, body
static int push_http_text_klua_khttp(lua_State* L, klb_buf_t* p_data)
{
    assert(NULL != p_data);

    int data_len = p_data->end - p_data->start;
    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        lua_pushstring(L, "text");
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    klb_mnp_text_t* p_txt = (klb_mnp_text_t*)(p_data->p_buf + p_data->start);
    const char* p_head = (const char*)(p_txt + 1);
    int head_len = (int)p_txt->head_size;
    // size 为 27 位位域, 超过 128MiB 的请求体会回绕; 本进程内缓冲长度为真源
    int body_len = data_len - head_len - (int)sizeof(klb_mnp_text_t);
    if (body_len < 0)
    {
        body_len = 0;
    }

    lua_pushstring(L, "text");
    lua_pushlstring(L, p_head, (size_t)head_len);
    lua_pushlstring(L, p_head + head_len, (size_t)body_len);
    return 3;
}

// 唤醒 co_recv
static int call_co_recv_klua_khttp(klua_khttp_t* p_http)
{
    assert(NULL != p_http);

    if (NULL == p_http->co_recv)
    {
        return -1;
    }

    if (0 >= klb_nlist_size(p_http->p_text_nlist) && 0 == p_http->recv_code)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_http->p_coex, p_http->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_http->p_env), p_http->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_http->co_recv = NULL;

    int nargs = 0;
    klb_buf_t* p_txt = klb_nlist_pop_head(p_http->p_text_nlist);
    if (NULL != p_txt)
    {
        nargs = push_http_text_klua_khttp(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
    }
    else
    {
        int code = p_http->recv_code;
        p_http->recv_code = 0;
        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        nargs = 2;
    }

    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 退出时唤醒 co_recv
static int call_co_recv_end_klua_khttp(klua_khttp_t* p_http)
{
    assert(NULL != p_http);

    if (NULL == p_http->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_http->p_coex, p_http->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_http->p_env), p_http->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_http->co_recv = NULL;

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_khttp(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_khttp_t* p_http = (klua_khttp_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_http->p_text_nlist, p_data);
            call_co_recv_klua_khttp(p_http);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else
    {
        p_http->recv_code = code;
        call_co_recv_klua_khttp(p_http);
    }

    return 0;
}

static void setup_klua_khttp(klua_khttp_t* p_http, lua_State* L, klb_netconn_t* p_conn, bool is_client)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_http->L = L;
    p_http->co_recv = NULL;
    p_http->p_env = p_env;
    p_http->p_coex = klua_coroutine_get(p_env);

    p_http->p_netmulti = klua_netmulti_get(p_env);
    p_http->p_conn = p_conn;
    p_http->is_client = is_client;

    p_http->p_text_nlist = klb_nlist_create();
    p_http->recv_code = 0;

    klb_netconn_set_udata(p_conn, p_http);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_khttp);
}

////////////////////////////////////////

static int klua_khttp_send(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    const uint8_t* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3))
    {
        p_body = (const uint8_t*)luaL_checklstring(L, 3, &body_len);
    }

    int ret = 1;
    if (NULL != p_http->p_conn)
    {
        ret = klb_netconn_send_text(p_http->p_conn, 0, (const uint8_t*)p_head, (int)head_len, p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_khttp_send_file(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);
    const char* p_path = luaL_checkstring(L, 3);

    int64_t offset = 0;
    int64_t length = 0;
    if (klua_is_table(L, 4))
    {
        lua_getfield(L, 4, "offset");
        if (lua_isnumber(L, -1))
        {
            offset = (int64_t)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 4, "length");
        if (lua_isnumber(L, -1))
        {
            length = (int64_t)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }

    int ret = 1;
    if (NULL != p_http->p_conn)
    {
        if (p_http->is_client)
        {
            ret = klb_httpclient_conn_send_file(p_http->p_conn, (const uint8_t*)p_head, (int)head_len,
                p_path, offset, length);
        }
        else
        {
            ret = klb_httpserve_conn_send_file(p_http->p_conn, (const uint8_t*)p_head, (int)head_len,
                p_path, offset, length);
        }
    }

    lua_pushinteger(L, ret);
    return 1;
}

/// @brief khttp:set_connect(bool); 客户端 CONNECT 应答模式
static int klua_khttp_set_connect(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    bool connect = klua_check_option_boolean(L, 2, false);

    int ret = KLB_NETCODE_INVAL;
    if (p_http->is_client && NULL != p_http->p_conn)
    {
        ret = klb_httpclient_conn_set_connect(p_http->p_conn, connect);
    }

    lua_pushinteger(L, ret);
    return 1;
}

/// @brief khttp:upgrade_tls(); 客户端明文连接就地升级 TLS
static int klua_khttp_upgrade_tls(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    int ret = KLB_NETCODE_INVAL;
    if (p_http->is_client && NULL != p_http->p_conn)
    {
        ret = klb_httpclient_conn_tls_upgrade(p_http->p_conn);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int on_yield_recv_klua_khttp(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_khttp_t* p_http = (klua_khttp_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_khttp(p_http);
    }

    return 0;
}

static int klua_khttp_tls(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    bool tls = false;

    if (NULL != p_http->p_conn && NULL != p_http->p_conn->p_socket)
    {
        tls = klb_socket_is_tls(p_http->p_conn->p_socket);
    }

    lua_pushboolean(L, tls);
    return 1;
}

static int klua_khttp_peer(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    lua_pushstring(L, p_http->peer);
    return 1;
}

/// @brief khttp:set_timeout(read_ms[, write_ms[, idle_ms]]) 或 set_timeout({read=,write=,idle=})
static int klua_khttp_set_timeout(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    int read_ms = 0;
    int write_ms = 0;
    int idle_ms = 0;

    if (NULL == p_http->p_conn)
    {
        lua_pushinteger(L, KLB_NETCODE_INVAL);
        return 1;
    }

    if (klua_is_table(L, 2))
    {
        lua_getfield(L, 2, "read");
        read_ms = (int)klua_check_option_integer(L, -1, 0);
        lua_pop(L, 1);

        lua_getfield(L, 2, "write");
        write_ms = (int)klua_check_option_integer(L, -1, 0);
        lua_pop(L, 1);

        lua_getfield(L, 2, "idle");
        idle_ms = (int)klua_check_option_integer(L, -1, 0);
        lua_pop(L, 1);
    }
    else
    {
        read_ms = (int)luaL_optinteger(L, 2, 0);
        write_ms = (int)luaL_optinteger(L, 3, 0);
        idle_ms = (int)luaL_optinteger(L, 4, 0);
    }

    if (p_http->is_client)
    {
        klb_httpclient_conn_set_timeout(p_http->p_conn, read_ms, write_ms, idle_ms);
    }
    else
    {
        klb_httpserve_conn_set_timeout(p_http->p_conn, read_ms, write_ms, idle_ms);
    }

    lua_pushinteger(L, 0);
    return 1;
}

/// @brief khttp:set_max_body(max_body); 0=不限制
static int klua_khttp_set_max_body(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    int64_t max_body = (int64_t)luaL_optinteger(L, 2, 0);

    if (NULL == p_http->p_conn)
    {
        lua_pushinteger(L, KLB_NETCODE_INVAL);
        return 1;
    }

    if (p_http->is_client)
    {
        klb_httpclient_conn_set_max_body(p_http->p_conn, max_body);
    }
    else
    {
        klb_httpserve_conn_set_max_body(p_http->p_conn, max_body);
    }

    lua_pushinteger(L, 0);
    return 1;
}

static int klua_khttp_co_recv(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);
    klua_check_coroutine(L, "khttp:co_recv must in coroutine!");
    assert(NULL == p_http->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_http->p_text_nlist);
    if (NULL != p_txt)
    {
        klb_nlist_pop_head(p_http->p_text_nlist);
        int n = push_http_text_klua_khttp(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    if (0 != p_http->recv_code)
    {
        int code = p_http->recv_code;
        p_http->recv_code = 0;

        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    p_http->co_recv = L;
    return klua_coroutine_yield(p_http->p_coex, L, on_yield_recv_klua_khttp, p_http);
}

static int klua_khttp_upgrade(lua_State* L)
{
    klua_khttp_t* p_http = to_klua_khttp(L, 1);

    if (p_http->is_client || NULL == p_http->p_conn)
    {
        return push_err_klua_khttp(L, KLB_NETCODE_INVAL);
    }

    if (!klb_httpserve_wbuf_is_empty(p_http->p_conn))
    {
        return push_err_klua_khttp(L, KLB_NETCODE_BUSY);
    }

    klb_netmulti_ws_adopt_fn adopt = klb_netmulti_get_ws_adopt();
    if (NULL == adopt)
    {
        return push_err_klua_khttp(L, KLB_NETCODE_NOTSUP);
    }

    klb_netconn_t* p_conn = p_http->p_conn;
    char peer[128];
    klb_socket_t* p_socket = NULL;
    klb_buf_t* p_tail = NULL;
    int ret = 0;

    memcpy(peer, p_http->peer, sizeof(peer));
    klb_netconn_bind_recv_data(p_conn, NULL);
    klb_netconn_set_udata(p_conn, NULL);

    ret = klb_httpserve_conn_take(p_conn, &p_socket, &p_tail);
    if (0 != ret)
    {
        klb_netconn_set_udata(p_conn, p_http);
        klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_khttp);
        return push_err_klua_khttp(L, ret);
    }

    p_http->p_conn = NULL;
    return adopt(L, p_http->p_netmulti, p_socket, p_tail, peer);
}

static void klua_khttp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_khttp_close },

        { "send",           klua_khttp_send },
        { "send_file",      klua_khttp_send_file },

        { "co_recv",        klua_khttp_co_recv },

        { "tls",            klua_khttp_tls },
        { "peer",           klua_khttp_peer },

        { "set_timeout",    klua_khttp_set_timeout },
        { "set_max_body",   klua_khttp_set_max_body },

        { "set_connect",    klua_khttp_set_connect },
        { "upgrade_tls",    klua_khttp_upgrade_tls },

        { "upgrade",        klua_khttp_upgrade },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_khttp_close },
        { "__close",        klua_khttp_close },
        { "__tostring",     klua_khttp_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTP_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

////////////////////////////////////////
// khttp client

static int klua_khttp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    int code = 0;
    bool tls = false;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_khttp(L, KLB_NETCODE_INVAL);
    }

    p_conn = klb_httpclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_conn)
    {
        return push_err_klua_khttp(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    klua_khttp_t* p_http = new_klua_khttp(L);
    setup_klua_khttp(p_http, L, p_conn, true);
    return 1;
}

//////////////////////////////////////////////////////////////////////////
// khttp listen

#define KLUA_KHTTP_LISTEN_HANDLE        "KHTTP_LISTEN_HANDLE*"      ///< Lua meta标示


/// @struct klua_khttp_listen_t
/// @brief  khttp 服务监听
typedef struct klua_khttp_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< co_accept协程

        klua_env_t*             p_env;          ///< Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    // 监听
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
        bool                    tls;            ///< 是否 TLS
        bool                    plain;          ///< tls 时是否同时收明文 HTTP; 默认 false
        klb_socket_tls_server_ctx_t* p_tls_ctx; ///< TLS 服务端上下文
    };

    // 数据
    struct
    {
        bool                    closed;         ///< 已 close; 探测回调勿再入队
        klb_nlist_t*            p_probe_nlist;  ///< 进行中的探测; 存储 klb_netconn_t*
        klb_nlist_t*            p_socket_nlist; ///< 待 accept 的 socket; 存储 klb_netlisten_socket_t*
    };
}klua_khttp_listen_t;


/// @struct klua_khttp_probe_t
/// @brief  探测上下文: listen 指针 + accept 地址
typedef struct klua_khttp_probe_t_
{
    klua_khttp_listen_t*        p_listen;       ///< 所属 listen
    struct sockaddr_in          addr;           ///< accept 地址
}klua_khttp_probe_t;


////////////////////////////////////////
static klua_khttp_listen_t* new_klua_khttp_listen(lua_State* L)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)lua_newuserdata(L, sizeof(klua_khttp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_khttp_listen_t));
    luaL_setmetatable(L, KLUA_KHTTP_LISTEN_HANDLE);
    return p_listen;
}

static klua_khttp_listen_t* to_klua_khttp_listen(lua_State* L, int index)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)luaL_checkudata(L, index, KLUA_KHTTP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'khttp listen' expected");
    return p_listen;
}

static int klua_khttp_listen_tostring(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    lua_pushfstring(L, "khttp listen:%p", p_listen);
    return 1;
}

static void remove_probe_klua_khttp_listen(klua_khttp_listen_t* p_listen, klb_netconn_t* p_probe)
{
    assert(NULL != p_listen);
    assert(NULL != p_probe);

    if (NULL == p_listen->p_probe_nlist)
    {
        return;
    }

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_listen->p_probe_nlist);
    while (NULL != p_iter)
    {
        klb_nlist_iter_t* p_next = klb_nlist_next(p_iter);
        if (p_probe == (klb_netconn_t*)klb_nlist_data(p_iter))
        {
            klb_nlist_remove(p_listen->p_probe_nlist, p_iter);
            break;
        }

        p_iter = p_next;
    }
}

static void abort_probes_klua_khttp_listen(klua_khttp_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL == p_listen->p_probe_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_listen->p_probe_nlist))
    {
        klb_netconn_t* p_probe = (klb_netconn_t*)klb_nlist_pop_head(p_listen->p_probe_nlist);
        klua_khttp_probe_t* p_ctx = (klua_khttp_probe_t*)p_probe->p_udata;

        klb_netconn_set_udata(p_probe, NULL);
        KLB_FREE(p_ctx);
        klb_netprobe_conn_free(p_probe);
    }
}

static int klua_khttp_listen_close(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    p_listen->closed = true;

    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);
    abort_probes_klua_khttp_listen(p_listen);
    KLB_FREE_BY(p_listen->p_probe_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_listen->p_tls_ctx, klb_socket_tls_server_ctx_destroy);

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

static void format_peer_klua_khttp(char* p_dst, int cap, const struct sockaddr_in* p_addr)
{
    klua_netlisten_format_peer_in(p_dst, cap, p_addr);
}

static klua_khttp_t* new_klua_khttp_by_socket(lua_State* L, klua_khttp_listen_t* p_listen, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = NULL;
    if (p_listen_socket->tls)
    {
        p_socket = klb_socket_async_create_tls_server(p_listen_socket->fd, p_listen->p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(p_listen_socket->fd);
            return NULL;
        }
    }
    else
    {
        p_socket = klb_socket_async_create(p_listen_socket->fd);
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = klb_httpserve_conn_create(p_netmulti, p_socket);

    klua_khttp_t* p_http = new_klua_khttp(L);
    setup_klua_khttp(p_http, L, p_conn, false);
    format_peer_klua_khttp(p_http->peer, (int)sizeof(p_http->peer), &p_listen_socket->addr);
    return p_http;
}

static int call_co_accept_klua_khttp_listen(klua_khttp_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL == p_listen->co_accept || 0 >= klb_nlist_size(p_listen->p_socket_nlist))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
    if (NULL == L)
    {
        return -1;
    }

    p_listen->co_accept = NULL;

    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
    if (NULL == new_klua_khttp_by_socket(L, p_listen, p_listen_socket))
    {
        lua_pushnil(L);
        lua_pushstring(L, "error");
    }
    else
    {
        lua_pushstring(L, "ok");
    }
    KLB_FREE(p_listen_socket);

    int status = lua_pcall(L, 2, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_accept_end_klua_khttp_listen(klua_khttp_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL == p_listen->co_accept)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
    if (NULL == L)
    {
        return -1;
    }

    p_listen->co_accept = NULL;

    lua_pushnil(L);
    lua_pushstring(L, "exit");

    int status = lua_pcall(L, 2, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int on_probe_done_klua_khttp_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, int protocol, bool tls, const uint8_t* p_peek, int peek_len)
{
    klua_khttp_probe_t* p_ctx = (klua_khttp_probe_t*)ptr;
    klua_khttp_listen_t* p_listen = p_ctx->p_listen;

    (void)p_peek;
    (void)peek_len;

    remove_probe_klua_khttp_listen(p_listen, p_conn);
    klb_netconn_set_udata(p_conn, NULL);

    bool take = false;
    if (!p_listen->closed)
    {
        if (tls)
        {
            take = (NULL != p_listen->p_tls_ctx);
        }
        else if (KLB_PROTOCOL_HTTP == protocol)
        {
            take = (!p_listen->tls) || p_listen->plain;
        }
    }

    if (!take)
    {
        KLB_SOCKET_CLOSE(fd);
        KLB_FREE(p_ctx);
        return 0;
    }

    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);
    p_tmp->fd = fd;
    p_tmp->addr = p_ctx->addr;
    p_tmp->tls = tls;
    KLB_FREE(p_ctx);

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);
    call_co_accept_klua_khttp_listen(p_listen);

    return 0;
}

static int on_accept_klua_khttp_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)p_conn->p_udata;

    (void)ptr;
    (void)tls;

    if (p_listen->closed)
    {
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klua_khttp_probe_t* p_ctx = KLB_MALLOCZ(klua_khttp_probe_t, 1, 0);
    p_ctx->p_listen = p_listen;
    p_ctx->addr = *p_addr;

    klb_netconn_t* p_probe = klb_netprobe_conn_create(p_listen->p_netmulti);
    klb_netconn_set_udata(p_probe, p_ctx);
    klb_netprobe_conn_set_done(p_probe, on_probe_done_klua_khttp_listen, p_ctx);

    if (0 != klb_netprobe_conn_open(p_probe, fd, 0))
    {
        klb_netconn_set_udata(p_probe, NULL);
        KLB_FREE(p_ctx);
        klb_netprobe_conn_free(p_probe);
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klb_nlist_push_tail(p_listen->p_probe_nlist, p_probe);
    return 0;
}

static int on_yield_accept_klua_khttp_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_khttp_listen(p_listen);
    }

    return 0;
}

/// @brief 热换 TLS 服务端证书 (新 accept 用新 ctx; 旧连接仍持旧 ctx 引用)
static int klua_khttp_listen_reload_tls(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    klb_socket_tls_server_ctx_t* p_new = NULL;

    if (p_listen->closed)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    if (!klua_is_table(L, 2))
    {
        return luaL_error(L, "reload_tls need table {cert,key}");
    }

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

    if ((NULL == p_cert) || (NULL == p_key) || (cert_len <= 0) || (key_len <= 0))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    p_new = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
    if (NULL == p_new)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    // 旧 ctx 引用计数: 活跃连接仍可用, listen 侧释放持有
    if (NULL != p_listen->p_tls_ctx)
    {
        klb_socket_tls_server_ctx_destroy(p_listen->p_tls_ctx);
        p_listen->p_tls_ctx = NULL;
    }

    p_listen->p_tls_ctx = p_new;
    p_listen->tls = true;
    lua_pushboolean(L, 1);
    return 1;
}


static int klua_khttp_listen_port(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_khttp_listen_local_addr(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_khttp_listen_native_fd(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static int klua_khttp_listen_co_accept(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    klua_check_coroutine(L, "khttp.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        if (NULL == new_klua_khttp_by_socket(L, p_listen, p_listen_socket))
        {
            lua_pushnil(L);
            lua_pushstring(L, "error");
        }
        else
        {
            lua_pushstring(L, "ok");
        }
        KLB_FREE(p_listen_socket);

        return 2;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_khttp_listen, p_listen);
}

static void klua_khttp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_khttp_listen_close },
        { "reload_tls",     klua_khttp_listen_reload_tls },

        { "co_accept",      klua_khttp_listen_co_accept },
        { "port",           klua_khttp_listen_port },
        { "local_addr",     klua_khttp_listen_local_addr },
        { "native_fd",      klua_khttp_listen_native_fd },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_khttp_listen_close },
        { "__close",        klua_khttp_listen_close },
        { "__tostring",     klua_khttp_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTP_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

/// @brief 由已探测 socket (klisten / netmux lightuserdata) 创建 HTTP 服务连接
static int klua_khttp_new_serve(lua_State* L)
{
    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)lua_touserdata(L, 1);
    bool tls = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    klb_socket_fd fd = INVALID_SOCKET;
    struct sockaddr_in addr;
    klb_socket_t* p_socket = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_khttp_t* p_http = NULL;

    if (NULL == p_listen_socket)
    {
        return push_err_klua_khttp(L, KLB_NETCODE_INVAL);
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
    addr = p_listen_socket->addr;
    KLB_FREE(p_listen_socket);

    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_khttp(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        // 连接持有一份 ref; 释放 create 时的初始 ref
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);

        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_khttp(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_khttp(L, KLB_NETCODE_NOMEM);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_socket, klb_socket_destroy);
        return push_err_klua_khttp(L, KLB_NETCODE_INVAL);
    }

    p_conn = klb_httpserve_conn_create(p_netmulti, p_socket);
    p_http = new_klua_khttp(L);
    setup_klua_khttp(p_http, L, p_conn, false);
    format_peer_klua_khttp(p_http->peer, (int)sizeof(p_http->peer), &addr);

    return 1;
}


static int klua_khttp_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);
    bool tls = false;
    bool plain = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    klb_socket_tls_server_ctx_t* p_tls_ctx = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_listen_conn = NULL;
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_khttp_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        lua_getfield(L, 2, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);

        lua_getfield(L, 2, "plain");
        plain = klua_check_option_boolean(L, -1, false);
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

        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_khttp(L, rc);
        }
    }

    if (tls)
    {
        p_tls_ctx = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            return push_err_klua_khttp(L, KLB_NETCODE_TLS_CERT);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_khttp(L, KLB_NETCODE_INVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_khttp_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_khttp(L, rc);
    }

    p_listen = new_klua_khttp_listen(L);

    p_listen->L = L;
    p_listen->co_accept = NULL;
    p_listen->p_env = p_env;
    p_listen->p_coex = klua_coroutine_get(p_env);

    p_listen->p_netmulti = p_netmulti;
    p_listen->p_listen_conn = p_listen_conn;
    p_listen->tls = tls;
    p_listen->plain = plain;
    p_listen->p_tls_ctx = p_tls_ctx;

    p_listen->closed = false;
    p_listen->p_probe_nlist = klb_nlist_create();
    p_listen->p_socket_nlist = klb_nlist_create();

    klb_netconn_set_udata(p_listen_conn, p_listen);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_khttp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",        klua_khttp_connect },
        { "connect_rpc",    klua_khttp_connect_rpc },

        { "listen",         klua_khttp_listen },
        { "listen_rpc",     klua_khttp_listen_rpc },
        { "new_serve",      klua_khttp_new_serve },

        { NULL,         NULL }
    };

    luaL_newlib(L, lib);

    klua_khttp_createmeta(L);
    klua_khttp_listen_createmeta(L);

    klua_khttpclientrpc_createmeta(L);
    klua_khttpserverpc_createmeta(L);
    klua_khttpserverpc_listen_createmeta(L);

    return 1;
}

// end
