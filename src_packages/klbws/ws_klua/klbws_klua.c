// Doc Encode : UTF-8 BOM, Unix(LF)
// klbws Lua 绑定: require("kws")
#include "klbws/klbws.h"
#include "klbws/core/klb_wsclient_conn.h"
#include "klbws/core/klb_wsserve_conn.h"
#include "klbws/core/klb_websocket.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbthird/sds.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// kws 连接 (client / serve 共用 userdata)

#define KLUA_KWS_HANDLE                 "KWS_HANDLE*"               ///< Lua meta标示


/// @struct klua_kws_t
/// @brief  kws 连接
typedef struct klua_kws_t_
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
        klb_netconn_t*          p_conn;         ///< WS 连接
        bool                    is_client;      ///< true.客户端; false.服务端
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 TEXT/HTTP; 存储 klb_buf_t*
        klb_nlist_t*            p_binary_nlist; ///< 待读取的 BINARY; 存储 klb_buf_t*
        int                     recv_code;      ///< 最近一次非0网络码; 0.无
    };
}klua_kws_t;


////////////////////////////////////////
static klua_kws_t* new_klua_kws(lua_State* L)
{
    klua_kws_t* p_kws = (klua_kws_t*)lua_newuserdata(L, sizeof(klua_kws_t));
    KLB_MEMSET(p_kws, 0, sizeof(klua_kws_t));
    luaL_setmetatable(L, KLUA_KWS_HANDLE);
    return p_kws;
}

static klua_kws_t* to_klua_kws(lua_State* L, int index)
{
    klua_kws_t* p_kws = (klua_kws_t*)luaL_checkudata(L, index, KLUA_KWS_HANDLE);
    luaL_argcheck(L, NULL != p_kws, index, "'kws' expected");
    return p_kws;
}

static int klua_kws_tostring(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    lua_pushfstring(L, "kws:%p", p_kws);
    return 1;
}

static int klua_kws_close(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    if (NULL != p_kws->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_kws->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_kws->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    if (NULL != p_kws->p_binary_nlist)
    {
        while (0 < klb_nlist_size(p_kws->p_binary_nlist))
        {
            klb_buf_t* p_bin = klb_nlist_pop_head(p_kws->p_binary_nlist);
            KLB_FREE_BY(p_bin, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_kws->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_kws->p_binary_nlist, klb_nlist_destroy);

    if (NULL != p_kws->p_conn)
    {
        klb_netconn_bind_recv_data(p_kws->p_conn, NULL);
        klb_netconn_set_udata(p_kws->p_conn, NULL);

        if (p_kws->is_client)
        {
            klb_wsclient_conn_free(p_kws->p_conn);
        }
        else
        {
            klb_wsserve_conn_free(p_kws->p_conn);
        }

        p_kws->p_conn = NULL;
    }

    return 0;
}

////////////////////////////////////////

// 压栈 TEXT/HTTP 包: "text"|"http", head, body
static int push_ws_text_klua_kws(lua_State* L, klb_buf_t* p_data)
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
    int body_len = (int)p_txt->size - head_len - (int)sizeof(klb_mnp_text_t);
    if (body_len < 0)
    {
        body_len = 0;
    }

    if (0 < head_len)
    {
        lua_pushstring(L, "http");
    }
    else
    {
        lua_pushstring(L, "text");
    }

    lua_pushlstring(L, p_head, (size_t)head_len);
    lua_pushlstring(L, p_head + head_len, (size_t)body_len);
    return 3;
}

// 压栈 BINARY 包: "binary", "", body
static int push_ws_binary_klua_kws(lua_State* L, klb_buf_t* p_data)
{
    assert(NULL != p_data);

    int data_len = p_data->end - p_data->start;
    int off = (int)sizeof(klb_mnp_binary_t);
    if (data_len < off)
    {
        lua_pushstring(L, "binary");
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    lua_pushstring(L, "binary");
    lua_pushstring(L, "");
    lua_pushlstring(L, p_data->p_buf + p_data->start + off, (size_t)(data_len - off));
    return 3;
}

// 唤醒 co_recv
static int call_co_recv_klua_kws(klua_kws_t* p_kws)
{
    assert(NULL != p_kws);

    if (NULL == p_kws->co_recv)
    {
        return -1;
    }

    if (0 >= klb_nlist_size(p_kws->p_text_nlist) &&
        0 >= klb_nlist_size(p_kws->p_binary_nlist) &&
        0 == p_kws->recv_code)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_kws->p_coex, p_kws->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_kws->p_env), p_kws->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_kws->co_recv = NULL;

    int nargs = 0;
    klb_buf_t* p_txt = klb_nlist_pop_head(p_kws->p_text_nlist);
    if (NULL != p_txt)
    {
        nargs = push_ws_text_klua_kws(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
    }
    else
    {
        klb_buf_t* p_bin = klb_nlist_pop_head(p_kws->p_binary_nlist);
        if (NULL != p_bin)
        {
            nargs = push_ws_binary_klua_kws(L, p_bin);
            KLB_FREE_BY(p_bin, klb_buf_unref);
        }
        else
        {
            lua_pushstring(L, "error");
            lua_pushinteger(L, p_kws->recv_code);
            p_kws->recv_code = 0;
            nargs = 2;
        }
    }

    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 退出时唤醒 co_recv
static int call_co_recv_end_klua_kws(klua_kws_t* p_kws)
{
    assert(NULL != p_kws);

    if (NULL == p_kws->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_kws->p_coex, p_kws->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_kws->p_env), p_kws->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_kws->co_recv = NULL;

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_kws(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kws_t* p_kws = (klua_kws_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_kws->p_text_nlist, p_data);
            call_co_recv_klua_kws(p_kws);
        }
        else if (KLB_MNP_BINARY == packtype)
        {
            klb_nlist_push_tail(p_kws->p_binary_nlist, p_data);
            call_co_recv_klua_kws(p_kws);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        p_kws->recv_code = code;
        call_co_recv_klua_kws(p_kws);
    }

    return 0;
}

static void setup_klua_kws(klua_kws_t* p_kws, lua_State* L, klb_netconn_t* p_conn, bool is_client)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_kws->L = L;
    p_kws->co_recv = NULL;
    p_kws->p_env = p_env;
    p_kws->p_coex = klua_coroutine_get(p_env);

    p_kws->p_netmulti = klua_netmulti_get(p_env);
    p_kws->p_conn = p_conn;
    p_kws->is_client = is_client;

    p_kws->p_text_nlist = klb_nlist_create();
    p_kws->p_binary_nlist = klb_nlist_create();
    p_kws->recv_code = 0;

    klb_netconn_set_udata(p_conn, p_kws);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_kws);
}

////////////////////////////////////////

static int klua_kws_send_text(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    size_t text_len = 0;
    const char* p_text = luaL_checklstring(L, 2, &text_len);

    int ret = 1;
    if (NULL != p_kws->p_conn)
    {
        ret = klb_netconn_send_text(p_kws->p_conn, 0, (const uint8_t*)p_text, (int)text_len, NULL, 0);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kws_send_binary(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    size_t bin_len = 0;
    const char* p_bin = luaL_checklstring(L, 2, &bin_len);

    int ret = 1;
    if (NULL != p_kws->p_conn)
    {
        uint8_t mask_key[4] = { 0x12, 0x34, 0x56, 0x78 };
        const uint8_t* p_mask = p_kws->is_client ? mask_key : NULL;
        klb_buf_t* p_frame = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_BINARY, p_mask, p_bin, (int)bin_len);

        if (p_kws->is_client)
        {
            ret = klb_wsclient_conn_send_buf(p_kws->p_conn, p_frame);
        }
        else
        {
            ret = klb_wsserve_conn_send_buf(p_kws->p_conn, p_frame);
        }
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int on_yield_recv_klua_kws(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kws_t* p_kws = (klua_kws_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_kws(p_kws);
    }

    return 0;
}

static int klua_kws_co_recv(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);
    klua_check_coroutine(L, "kws:co_recv must in coroutine!");
    assert(NULL == p_kws->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_kws->p_text_nlist);
    if (NULL != p_txt)
    {
        klb_nlist_pop_head(p_kws->p_text_nlist);
        int n = push_ws_text_klua_kws(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_bin = klb_nlist_head(p_kws->p_binary_nlist);
    if (NULL != p_bin)
    {
        klb_nlist_pop_head(p_kws->p_binary_nlist);
        int n = push_ws_binary_klua_kws(L, p_bin);
        KLB_FREE_BY(p_bin, klb_buf_unref);
        return n;
    }

    if (0 != p_kws->recv_code)
    {
        int code = p_kws->recv_code;
        p_kws->recv_code = 0;

        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    p_kws->co_recv = L;
    return klua_coroutine_yield(p_kws->p_coex, L, on_yield_recv_klua_kws, p_kws);
}

static void klua_kws_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kws_close },

        { "send_text",      klua_kws_send_text },
        { "send_binary",    klua_kws_send_binary },

        { "co_recv",        klua_kws_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kws_close },
        { "__close",        klua_kws_close },
        { "__tostring",     klua_kws_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KWS_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

////////////////////////////////////////
// kws client

static void send_upgrade_klua_kws(klb_netconn_t* p_conn, const char* p_host, int port, const char* p_path)
{
    sds str = sdsnew("");

    str = sdscatfmt(str, "GET %s HTTP/1.1\r\n", p_path);
    str = sdscatfmt(str, "Host: %s:%i\r\n", p_host, port);
    str = sdscat(str, "Connection: Upgrade\r\n");
    str = sdscat(str, "Upgrade: websocket\r\n");
    str = sdscat(str, "Sec-WebSocket-Version: 13\r\n");
    str = sdscat(str, "Sec-WebSocket-Key: fcoOCnZknPpAcsTt5Ptf0w==\r\n");
    str = sdscat(str, "Content-Length: 0\r\n");
    str = sdscat(str, "\r\n");

    klb_wsclient_conn_send(p_conn, (const uint8_t*)str, (int)sdslen(str), NULL, 0);
    KLB_FREE_BY(str, sdsfree);
}

static int klua_kws_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);

    const char* p_path = "/";
    if (klua_is_string(L, 3))
    {
        p_path = luaL_checkstring(L, 3);
    }

    bool tls = false;
    int opts_idx = klua_is_table(L, 3) ? 3 : 4;
    if (klua_is_table(L, opts_idx))
    {
        lua_getfield(L, opts_idx, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = klb_wsclient_connect(p_netmulti, p_host, port, tls);

    if (NULL == p_conn)
    {
        lua_pushnil(L);
    }
    else
    {
        // step1. 绑定 Lua userdata
        klua_kws_t* p_kws = new_klua_kws(L);
        setup_klua_kws(p_kws, L, p_conn, true);

        // step2. 发送 WS 握手
        send_upgrade_klua_kws(p_conn, p_host, port, p_path);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// kws listen

#define KLUA_KWS_LISTEN_HANDLE          "KWS_LISTEN_HANDLE*"        ///< Lua meta标示


/// @struct klua_kws_listen_t
/// @brief  kws 服务监听
typedef struct klua_kws_listen_t_
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
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待 accept 的 socket; 存储 klb_netlisten_socket_t*
    };
}klua_kws_listen_t;


////////////////////////////////////////
static klua_kws_listen_t* new_klua_kws_listen(lua_State* L)
{
    klua_kws_listen_t* p_listen = (klua_kws_listen_t*)lua_newuserdata(L, sizeof(klua_kws_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_kws_listen_t));
    luaL_setmetatable(L, KLUA_KWS_LISTEN_HANDLE);
    return p_listen;
}

static klua_kws_listen_t* to_klua_kws_listen(lua_State* L, int index)
{
    klua_kws_listen_t* p_listen = (klua_kws_listen_t*)luaL_checkudata(L, index, KLUA_KWS_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'kws listen' expected");
    return p_listen;
}

static int klua_kws_listen_tostring(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);

    lua_pushfstring(L, "kws listen:%p", p_listen);
    return 1;
}

static int klua_kws_listen_close(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);

    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);

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

static klua_kws_t* new_klua_kws_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = klb_wsserve_conn_create(p_netmulti, p_socket);

    klua_kws_t* p_kws = new_klua_kws(L);
    setup_klua_kws(p_kws, L, p_conn, false);
    return p_kws;
}

static int call_co_accept_klua_kws_listen(klua_kws_listen_t* p_listen)
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
    new_klua_kws_by_socket(L, p_listen_socket);
    KLB_FREE(p_listen_socket);

    lua_pushstring(L, "ok");

    int status = lua_pcall(L, 2, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_accept_end_klua_kws_listen(klua_kws_listen_t* p_listen)
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

static int on_accept_klua_kws_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_kws_listen_t* p_listen = (klua_kws_listen_t*)p_conn->p_udata;

    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);
    p_tmp->fd = fd;
    p_tmp->addr = *p_addr;
    p_tmp->tls = tls;

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);
    call_co_accept_klua_kws_listen(p_listen);

    return 0;
}

static int on_yield_accept_klua_kws_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kws_listen_t* p_listen = (klua_kws_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_kws_listen(p_listen);
    }

    return 0;
}

static int klua_kws_listen_co_accept(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);
    klua_check_coroutine(L, "kws.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        new_klua_kws_by_socket(L, p_listen_socket);
        KLB_FREE(p_listen_socket);

        lua_pushstring(L, "ok");
        return 2;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_kws_listen, p_listen);
}

static void klua_kws_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kws_listen_close },

        { "co_accept",      klua_kws_listen_co_accept },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kws_listen_close },
        { "__close",        klua_kws_listen_close },
        { "__tostring",     klua_kws_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KWS_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static int klua_kws_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = klb_netlisten_conn_create(p_netmulti);

    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_kws_listen, NULL);
    klb_netlisten_conn_open(p_listen_conn, port, 20);

    klua_kws_listen_t* p_listen = new_klua_kws_listen(L);

    p_listen->L = L;
    p_listen->co_accept = NULL;
    p_listen->p_env = p_env;
    p_listen->p_coex = klua_coroutine_get(p_env);

    p_listen->p_netmulti = p_netmulti;
    p_listen->p_listen_conn = p_listen_conn;

    p_listen->p_socket_nlist = klb_nlist_create();

    klb_netconn_set_udata(p_listen_conn, p_listen);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_kws(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    klua_kws_connect },

        { "listen",     klua_kws_listen },

        { NULL,         NULL }
    };

    luaL_newlib(L, lib);

    klua_kws_createmeta(L);
    klua_kws_listen_createmeta(L);

    return 1;
}

// end
