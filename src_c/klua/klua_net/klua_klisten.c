// Doc Encode : UTF-8 BOM, Unix(LF)
// klisten Lua 绑定: 共享 listen + probe 分发
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_netlisten_opts.h"
#include "klbnet/klblisten/klb_netmux.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netcode.h"
#include "klbutil/klb_obj.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#define KLUA_KLISTEN_HANDLE             "KLISTEN_HANDLE*"

/// @brief 默认 listen backlog (max_connect); 可由 cfg.max_connect 覆盖
#define KLUA_KLISTEN_MAX_CONNECT_DEFAULT    20


/// @struct klua_klisten_t
/// @brief  klisten userdata
typedef struct klua_klisten_t_
{
    lua_State*                      L;              ///< L
    lua_State*                      co_accept;      ///< co_accept 协程
    klua_env_t*                     p_env;          ///< env
    klua_ex_coroutine_t*            p_coex;         ///< 协程扩展

    klb_netmux_t*                   p_mux;          ///< 分发器
    bool                            closed;         ///< 已 close
}klua_klisten_t;


/// @struct klua_klisten_sock_payload_t
/// @brief  klisten sock 的 klb_obj extra 载荷 (跨线程经 klist 传递)
typedef struct klua_klisten_sock_payload_t_
{
    klb_socket_fd               fd;         ///< socket fd
    struct sockaddr_in          addr;       ///< 对端 IPv4 (兼容)
    struct sockaddr_storage     addr_sa;    ///< 完整对端地址
    bool                        tls;        ///< 是否 TLS ClientHello
    int                         protocol;   ///< klb_protocol_e
    int                         listen_port;///< 本 mux 监听端口 (主机序); 对齐 klb_netmux_socket_t
}klua_klisten_sock_payload_t;


static const char* g_klisten_obj_module = "klisten";
static const char* g_klisten_obj_name = "sock";


//////////////////////////////////////////////////////////////////////////
// 内部


static void destroy_sock_obj_klua_klisten(klb_obj_t* p_obj)
{
    klua_klisten_sock_payload_t* p_pay = NULL;

    assert(NULL != p_obj);

    p_pay = (klua_klisten_sock_payload_t*)p_obj->extra;
    KLB_SOCKET_CLOSE(p_pay->fd);
    KLB_FREE(p_obj);
}


static bool is_sock_obj_klua_klisten(klb_obj_t* p_obj)
{
    if (NULL == p_obj)
    {
        return false;
    }

    if (destroy_sock_obj_klua_klisten != p_obj->cb_destroy)
    {
        return false;
    }

    if (NULL == p_obj->p_module || 0 != strcmp(p_obj->p_module, g_klisten_obj_module))
    {
        return false;
    }

    if (NULL == p_obj->p_name || 0 != strcmp(p_obj->p_name, g_klisten_obj_name))
    {
        return false;
    }

    return true;
}


static klua_klisten_t* new_klua_klisten(lua_State* L)
{
    klua_klisten_t* p_listen = (klua_klisten_t*)lua_newuserdata(L, sizeof(klua_klisten_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_klisten_t));
    luaL_setmetatable(L, KLUA_KLISTEN_HANDLE);
    return p_listen;
}


static klua_klisten_t* to_klua_klisten(lua_State* L, int index)
{
    klua_klisten_t* p_listen = (klua_klisten_t*)luaL_checkudata(L, index, KLUA_KLISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'klisten' expected");
    return p_listen;
}


static const char* proto_name_klua_klisten(int protocol)
{
    if (KLB_PROTOCOL_HTTP == protocol)
    {
        return "http";
    }
    else if (KLB_PROTOCOL_RTSP == protocol)
    {
        return "rtsp";
    }
    else if (KLB_PROTOCOL_SMP == protocol)
    {
        return "smp";
    }
    else if (KLB_PROTOCOL_RPC_SMP_LUA == protocol
        || KLB_PROTOCOL_RPC_SMP_JSON == protocol)
    {
        return "smprpc";
    }
    else if (KLB_PROTOCOL_MNP == protocol)
    {
        return "mnp";
    }
    else if (KLB_PROTOCOL_RPC_MNP_LUA == protocol
        || KLB_PROTOCOL_RPC_MNP_JSON == protocol)
    {
        return "mnprpc";
    }

    return "unknown";
}


static int call_co_accept_end_klua_klisten(klua_klisten_t* p_listen);
static int klua_klisten_set_mux_opts(lua_State* L);
static int push_err_klua_klisten(lua_State* L, int code);
static void push_addr_klua_klisten(lua_State* L, const struct sockaddr_storage* p_addr);
static int push_co_accept_result_klua_klisten(lua_State* L, klb_netmux_socket_t* p_sock);


/// @brief 将 pop 结果压入协程返回
/// @return int 压入值个数 (ok=4; error=5 含 code)
static int push_co_accept_result_klua_klisten(lua_State* L, klb_netmux_socket_t* p_sock)
{
    assert(NULL != L);

    if (NULL == p_sock)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "error");
        lua_pushboolean(L, 0);
        lua_pushinteger(L, KLB_NETCODE_ACCEPT_FAIL);
        return 5;
    }

    if (0 != p_sock->err_code)
    {
        int code = p_sock->err_code;

        KLB_FREE(p_sock);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "error");
        lua_pushboolean(L, 0);
        lua_pushinteger(L, code);
        return 5;
    }

    lua_pushstring(L, proto_name_klua_klisten(p_sock->protocol));
    lua_pushlightuserdata(L, p_sock);
    lua_pushstring(L, "ok");
    lua_pushboolean(L, p_sock->tls ? 1 : 0);
    return 4;
}


static int call_co_accept_klua_klisten(klua_klisten_t* p_listen)
{
    lua_State* L = NULL;
    klb_netmux_socket_t* p_sock = NULL;
    int narg = 0;
    int status = 0;

    assert(NULL != p_listen);

    if (NULL == p_listen->co_accept || NULL == p_listen->p_mux)
    {
        return -1;
    }

    if (0 >= klb_netmux_queue_size(p_listen->p_mux))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
    if (NULL == L)
    {
        return -1;
    }

    p_listen->co_accept = NULL;

    p_sock = klb_netmux_pop(p_listen->p_mux);
    narg = push_co_accept_result_klua_klisten(L, p_sock);

    status = lua_pcall(L, narg, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}


static int call_co_accept_end_klua_klisten(klua_klisten_t* p_listen)
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
    lua_pushnil(L);
    lua_pushstring(L, "exit");
    lua_pushboolean(L, 0);

    int status = lua_pcall(L, 4, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}


static void on_ready_klua_klisten(klb_netmux_t* p_mux, void* ptr)
{
    klua_klisten_t* p_listen = (klua_klisten_t*)ptr;

    (void)p_mux;

    if (NULL == p_listen || p_listen->closed)
    {
        return;
    }

    call_co_accept_klua_klisten(p_listen);
}


static int on_yield_accept_klua_klisten(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_klisten_t* p_listen = (klua_klisten_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_klisten(p_listen);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// methods


static int klua_klisten_tostring(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);

    lua_pushfstring(L, "klisten:%p", p_listen);
    return 1;
}


static int klua_klisten_close(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);

    if (p_listen->closed)
    {
        return 0;
    }

    p_listen->closed = true;
    call_co_accept_end_klua_klisten(p_listen);

    if (NULL != p_listen->p_mux)
    {
        klb_netmux_set_ready(p_listen->p_mux, NULL, NULL);
        klb_netmux_destroy(p_listen->p_mux);
        p_listen->p_mux = NULL;
    }

    return 0;
}


static int klua_klisten_port(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    int port = 0;

    if (NULL != p_listen->p_mux)
    {
        port = klb_netmux_get_port(p_listen->p_mux);
    }

    lua_pushinteger(L, port);
    return 1;
}


/// @brief 配置的绑定地址 (INADDR_ANY 为空串)
/// @return host[string]
static int klua_klisten_host(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    const char* p_host = "";

    if (NULL != p_listen->p_mux)
    {
        const char* p = klb_netmux_get_host(p_listen->p_mux);
        if (NULL != p)
        {
            p_host = p;
        }
    }

    lua_pushstring(L, p_host);
    return 1;
}


/// @brief 回读监听本地地址 (getsockname)
/// @return ip[string|nil]   本地地址文本; 未 open 为 nil
/// @return port[number|nil] 本地端口 (主机序); 未 open 为 nil
static int klua_klisten_local_addr(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    struct sockaddr_storage addr = { 0 };

    if (NULL == p_listen->p_mux
        || 0 != klb_netmux_get_local_sa(p_listen->p_mux, &addr, NULL))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    push_addr_klua_klisten(L, &addr);
    return 2;
}


/// @brief 监听 native fd
/// @return fd[number|nil]
static int klua_klisten_native_fd(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    klb_socket_fd fd = INVALID_SOCKET;

    if (NULL != p_listen->p_mux)
    {
        fd = klb_netmux_native_fd(p_listen->p_mux);
    }

    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)fd);
    return 1;
}


/// @brief 以新绑定地址重开监听 (同端口; 既有连接不受影响)
/// @param [in]  host[string|nil]  新绑定地址; nil/"" → INADDR_ANY
/// @return ok[boolean]            成功为 true
/// @return code[number|nil]       失败时 klb_netcode_e
static int klua_klisten_rebind(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    const char* p_host = NULL;
    int rc = 0;

    if (p_listen->closed || NULL == p_listen->p_mux)
    {
        return push_err_klua_klisten(L, KLB_NETCODE_EINVAL);
    }

    if (!lua_isnoneornil(L, 2))
    {
        p_host = luaL_checkstring(L, 2);
        if ('\0' == p_host[0])
        {
            p_host = NULL;
        }
    }

    rc = klb_netmux_rebind(p_listen->p_mux, p_host);
    if (0 != rc)
    {
        return push_err_klua_klisten(L, rc);
    }

    lua_pushboolean(L, 1);
    return 1;
}


static int klua_klisten_co_accept(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);
    klua_check_coroutine(L, "klisten:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (p_listen->closed || NULL == p_listen->p_mux)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "exit");
        lua_pushboolean(L, 0);
        return 4;
    }

    klb_netmux_socket_t* p_sock = klb_netmux_pop(p_listen->p_mux);
    if (NULL != p_sock)
    {
        return push_co_accept_result_klua_klisten(L, p_sock);
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_klisten, p_listen);
}


static void klua_klisten_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_klisten_close },
        { "co_accept",      klua_klisten_co_accept },
        { "port",           klua_klisten_port },
        { "host",           klua_klisten_host },
        { "local_addr",     klua_klisten_local_addr },
        { "native_fd",      klua_klisten_native_fd },
        { "rebind",         klua_klisten_rebind },
        { "set_mux_opts",   klua_klisten_set_mux_opts },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_klisten_close },
        { "__close",        klua_klisten_close },
        { "__tostring",     klua_klisten_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KLISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


static int klua_klisten_drop(lua_State* L)
{
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)lua_touserdata(L, 1);
    if (NULL == p_sock)
    {
        return 0;
    }

    KLB_SOCKET_CLOSE(p_sock->fd);
    KLB_FREE(p_sock);
    return 0;
}


/// @brief 将地址压入 Lua 栈: ip[string], port[number]
static void push_addr_klua_klisten(lua_State* L, const struct sockaddr_storage* p_addr)
{
    char ip[64] = { 0 };
    int port = 0;

    assert(NULL != L);
    assert(NULL != p_addr);

    if (0 != klb_socket_format_addr(p_addr, ip, (int)sizeof(ip), &port))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return;
    }

    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
}


/// @brief 取 netmux sock 对端地址; 供 Lua 侧 co_accept 回传 addr
/// @return ip[string|nil], port[number|nil]
static int klua_klisten_sock_addr(lua_State* L)
{
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)lua_touserdata(L, 1);

    if (NULL == p_sock)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    push_addr_klua_klisten(L, &p_sock->addr_sa);
    return 2;
}


/// @brief 取 netmux sock native fd
static int klua_klisten_sock_fd(lua_State* L)
{
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)lua_touserdata(L, 1);

    if (NULL == p_sock || INVALID_SOCKET == p_sock->fd)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)p_sock->fd);
    return 1;
}


/// @brief 取 netmux sock 探测协议名 (与 co_accept 首返回值同形)
/// @return proto[string|nil]       http/smp/mnp/smprpc/mnprpc/...; 无 sock 为 nil
static int klua_klisten_sock_protocol(lua_State* L)
{
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)lua_touserdata(L, 1);

    if (NULL == p_sock)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, proto_name_klua_klisten(p_sock->protocol));
    return 1;
}


/// @brief 将 netmux sock 封为 klb_obj (供 klist 跨线程传递); 取得所有权
static int klua_klisten_wrap(lua_State* L)
{
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)lua_touserdata(L, 1);
    klb_obj_t* p_obj = NULL;
    klua_klisten_sock_payload_t* p_pay = NULL;

    if (NULL == p_sock)
    {
        lua_pushnil(L);
        return 1;
    }

    p_obj = KLB_MALLOCZ(klb_obj_t, 1, sizeof(klua_klisten_sock_payload_t));
    p_obj->p_module = g_klisten_obj_module;
    p_obj->p_name = g_klisten_obj_name;
    p_obj->cb_destroy = destroy_sock_obj_klua_klisten;

    p_pay = (klua_klisten_sock_payload_t*)p_obj->extra;
    p_pay->fd = p_sock->fd;
    p_pay->addr = p_sock->addr;
    p_pay->addr_sa = p_sock->addr_sa;
    p_pay->tls = p_sock->tls;
    p_pay->protocol = p_sock->protocol;
    p_pay->listen_port = p_sock->listen_port;

    // 节点本身释放; fd 已迁入 obj
    p_sock->fd = INVALID_SOCKET;
    KLB_FREE(p_sock);

    lua_pushlightuserdata(L, p_obj);
    return 1;
}


/// @brief 从 klb_obj 还原 netmux sock (供 khttp/krtsp.new_serve); 取得 fd 所有权
static int klua_klisten_unwrap(lua_State* L)
{
    klb_obj_t* p_obj = (klb_obj_t*)lua_touserdata(L, 1);
    klua_klisten_sock_payload_t* p_pay = NULL;
    klb_netmux_socket_t* p_sock = NULL;

    if (!is_sock_obj_klua_klisten(p_obj))
    {
        lua_pushnil(L);
        return 1;
    }

    p_pay = (klua_klisten_sock_payload_t*)p_obj->extra;
    if (INVALID_SOCKET == p_pay->fd)
    {
        KLB_OBJ_DESTROY(p_obj);
        lua_pushnil(L);
        return 1;
    }

    p_sock = KLB_MALLOCZ(klb_netmux_socket_t, 1, 0);
    p_sock->fd = p_pay->fd;
    p_sock->addr = p_pay->addr;
    p_sock->addr_sa = p_pay->addr_sa;
    p_sock->tls = p_pay->tls;
    p_sock->protocol = p_pay->protocol;
    p_sock->err_code = 0;
    p_sock->listen_port = p_pay->listen_port;

    // 释放信封且不再关 fd
    p_pay->fd = INVALID_SOCKET;
    KLB_OBJ_DESTROY(p_obj);

    lua_pushlightuserdata(L, p_sock);
    return 1;
}


/// @brief 销毁未 unwrap 的 sock obj (关 fd); 非本模块 obj 忽略
static int klua_klisten_destroy_obj(lua_State* L)
{
    klb_obj_t* p_obj = (klb_obj_t*)lua_touserdata(L, 1);

    if (!is_sock_obj_klua_klisten(p_obj))
    {
        return 0;
    }

    KLB_OBJ_DESTROY(p_obj);
    return 0;
}


/// @brief listen 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_klisten(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @brief 热更新 TLS 投递面 (已 open 的 mux); opts 同 listen cfg 三项
static int klua_klisten_set_mux_opts(lua_State* L)
{
    klua_klisten_t* p_listen = to_klua_klisten(L, 1);

    if (p_listen->closed || NULL == p_listen->p_mux)
    {
        return push_err_klua_klisten(L, KLB_NETCODE_EINVAL);
    }

    if (!klua_is_table(L, 2))
    {
        return push_err_klua_klisten(L, KLB_NETCODE_EINVAL);
    }

    lua_getfield(L, 2, "tls_as_http");
    if (!lua_isnil(L, -1))
    {
        bool enable = klua_check_option_boolean(L, -1, true);
        klb_netmux_set_tls_as_http(p_listen->p_mux, enable);
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "tls_as_mnp");
    if (!lua_isnil(L, -1))
    {
        bool enable = klua_check_option_boolean(L, -1, false);
        klb_netmux_set_tls_as_mnp(p_listen->p_mux, enable);
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "tls_as_rtsp");
    if (!lua_isnil(L, -1))
    {
        bool enable = klua_check_option_boolean(L, -1, false);
        klb_netmux_set_tls_as_rtsp(p_listen->p_mux, enable);
    }
    lua_pop(L, 1);

    lua_pushboolean(L, 1);
    return 1;
}


static int klua_klisten_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);
    bool tls_as_http = true;
    bool tls_as_mnp = false;
    bool tls_as_rtsp = false;
    int max_connect = KLUA_KLISTEN_MAX_CONNECT_DEFAULT;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    int rc = 0;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        // step1. klisten 专有: TLS 嗅探映射
        lua_getfield(L, 2, "tls_as_http");
        if (!lua_isnil(L, -1))
        {
            tls_as_http = klua_check_option_boolean(L, -1, true);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "tls_as_mnp");
        if (!lua_isnil(L, -1))
        {
            tls_as_mnp = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "tls_as_rtsp");
        if (!lua_isnil(L, -1))
        {
            tls_as_rtsp = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);

        // step2. 与各协议 listen 同源: host / max_connect / family / socket opts
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_klisten(L, rc);
        }
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_klisten(L, KLB_NETCODE_EINVAL);
    }

    klb_netmux_t* p_mux = klb_netmux_create(p_netmulti);
    if (NULL == p_mux)
    {
        return push_err_klua_klisten(L, KLB_NETCODE_ENOMEM);
    }

    klb_netmux_set_tls_as_http(p_mux, tls_as_http);
    klb_netmux_set_tls_as_mnp(p_mux, tls_as_mnp);
    klb_netmux_set_tls_as_rtsp(p_mux, tls_as_rtsp);

    rc = klb_netmux_open_addr_ex(p_mux, ('\0' == host[0]) ? NULL : host, port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netmux_destroy(p_mux);
        return push_err_klua_klisten(L, rc);
    }

    klua_klisten_t* p_listen = new_klua_klisten(L);

    p_listen->L = L;
    p_listen->co_accept = NULL;
    p_listen->p_env = p_env;
    p_listen->p_coex = klua_coroutine_get(p_env);
    p_listen->p_mux = p_mux;
    p_listen->closed = false;

    klb_netmux_set_ready(p_mux, on_ready_klua_klisten, p_listen);

    return 1;
}


//////////////////////////////////////////////////////////////////////////


int klua_open_klisten(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "listen",         klua_klisten_listen },
        { "drop",           klua_klisten_drop },
        { "sock_addr",      klua_klisten_sock_addr },
        { "sock_fd",        klua_klisten_sock_fd },
        { "sock_protocol",  klua_klisten_sock_protocol },
        { "wrap",           klua_klisten_wrap },
        { "unwrap",         klua_klisten_unwrap },
        { "destroy_obj",    klua_klisten_destroy_obj },

        { NULL,             NULL }
    };

    luaL_newlib(L, lib);
    klua_klisten_createmeta(L);

    return 1;
}

// end
