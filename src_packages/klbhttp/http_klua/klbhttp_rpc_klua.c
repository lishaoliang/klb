// Doc Encode : UTF-8 BOM, Unix(LF)
// khttp RPC: require("khttp").connect_rpc / listen_rpc
#include "klbhttp/http_klua/klbhttp_rpc_klua.h"
#include "klbhttp/core/klb_httprpcer.h"
#include "klbhttp/core/klb_httpclient_conn.h"
#include "klbhttp/core/klb_httpserve_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klblisten/klb_netprobe_conn.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 帧: HTTP 正文 = [klb_mnp_rpc_t] + 载荷

/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_khttprpc(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


static const char* ctype_klb_httprpc(int rpctype)
{
    if (KLB_MNP_RPC_JSON == rpctype)
    {
        return "application/json";
    }

    return "application/octet-stream";
}

static int send_http_rpc(klb_netconn_t* p_conn, bool is_client, const char* p_host, int port,
    int rpctype, int method, int sequence, const uint8_t* p_payload, int payload_len)
{
    if (NULL == p_conn)
    {
        return 1;
    }

    if (0 > payload_len)
    {
        payload_len = 0;
    }

    int body_len = (int)sizeof(klb_mnp_rpc_t) + payload_len;
    char host_hdr[320];
    const char* p_name = (NULL != p_host && '\0' != p_host[0]) ? p_host : "localhost";
    bool v6 = (NULL != strchr(p_name, ':'));
    bool bracket = (v6 && '[' != p_name[0]);
    int host_n = 0;

    if (bracket)
    {
        host_n = snprintf(host_hdr, sizeof(host_hdr), "[%s]:%d", p_name, port);
    }
    else
    {
        host_n = snprintf(host_hdr, sizeof(host_hdr), "%s:%d", p_name, port);
    }

    if (0 >= host_n || host_n >= (int)sizeof(host_hdr))
    {
        return 1;
    }

    char head[640];
    int head_len = 0;
    if (is_client)
    {
        head_len = snprintf(head, sizeof(head),
            "POST /rpc HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            host_hdr,
            ctype_klb_httprpc(rpctype),
            body_len);
    }
    else
    {
        head_len = snprintf(head, sizeof(head),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            ctype_klb_httprpc(rpctype),
            body_len);
    }

    if (0 >= head_len || head_len >= (int)sizeof(head))
    {
        return 1;
    }

    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = (uint32_t)sequence;
    rpc.rpctype = (uint32_t)rpctype;
    rpc.method = (uint32_t)method;
    rpc.encode = KLB_MNP_ENCODE_NULL;
    rpc.size = (uint32_t)body_len;

    klb_buf_t* p_msg = klb_buf_malloc(head_len + body_len, false);
    klb_buf_write(p_msg, head, head_len);
    klb_buf_write(p_msg, (const char*)&rpc, (int)sizeof(rpc));
    if (0 < payload_len && NULL != p_payload)
    {
        klb_buf_write(p_msg, (const char*)p_payload, payload_len);
    }

    if (is_client)
    {
        klb_httpclient_conn_send_buf(p_conn, p_msg);
    }
    else
    {
        klb_httpserve_conn_send_buf(p_conn, p_msg);
    }

    return 0;
}

static klb_buf_t* take_rpc_from_http(klb_buf_t* p_data)
{
    int data_len = p_data->end - p_data->start;
    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        return NULL;
    }

    klb_mnp_text_t* p_txt = (klb_mnp_text_t*)(p_data->p_buf + p_data->start);
    int head_len = (int)p_txt->head_size;
    int total = (int)p_txt->size;
    int hdr = (int)sizeof(klb_mnp_text_t);

    if (0 > head_len || total < hdr + head_len || total > data_len)
    {
        return NULL;
    }

    int body_len = total - hdr - head_len;
    if (body_len < (int)sizeof(klb_mnp_rpc_t))
    {
        return NULL;
    }

    const char* p_body = (const char*)(p_txt + 1) + head_len;
    klb_mnp_rpc_t rpc = { 0 };
    memcpy(&rpc, p_body, sizeof(rpc));

    if (KLB_MNP_RPC_LUA != (int)rpc.rpctype && KLB_MNP_RPC_JSON != (int)rpc.rpctype)
    {
        return NULL;
    }

    if ((int)rpc.size != body_len)
    {
        return NULL;
    }

    klb_buf_t* p_out = klb_buf_malloc(body_len, false);
    klb_buf_write(p_out, p_body, body_len);
    return p_out;
}

static void clear_buf_nlist_klb_httprpc(klb_nlist_t* p_nlist)
{
    if (NULL == p_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_nlist))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }
}

static int push_rpc_body_klb_httprpc(lua_State* L, klb_buf_t* p_buf)
{
    klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
    char* p_data = p_buf->p_buf + p_buf->start + sizeof(klb_mnp_rpc_t);
    int data_len = p_buf->end - p_buf->start - (int)sizeof(klb_mnp_rpc_t);

    if (0 > data_len)
    {
        data_len = 0;
    }

    if (KLB_MNP_RPC_JSON == (int)p_rpc->rpctype)
    {
        if (0 < data_len)
        {
            lua_pushlstring(L, p_data, (size_t)data_len);
        }
        else
        {
            lua_pushstring(L, "");
        }

        return 1;
    }

    return klua_seri_map_binary_unpack(L, 1, p_data, data_len);
}

static int pack_payload_klb_httprpc(lua_State* L, int rpctype, int base_idx, const uint8_t** p_out, int* p_len, klb_buf_t** p_hold)
{
    *p_out = NULL;
    *p_len = 0;
    *p_hold = NULL;

    if (KLB_MNP_RPC_LUA == rpctype)
    {
        klb_buf_t* p_data = klua_seri_map_binary_pack(L, base_idx);
        if (NULL == p_data)
        {
            return 1;
        }

        *p_hold = p_data;
        *p_out = (const uint8_t*)(p_data->p_buf + p_data->start);
        *p_len = klb_buf_data_len(p_data);
        return 0;
    }

    if (KLB_MNP_RPC_JSON == rpctype)
    {
        size_t json_len = 0;
        const char* p_json = luaL_checklstring(L, base_idx + 1, &json_len);
        *p_out = (const uint8_t*)p_json;
        *p_len = (int)json_len;
        return 0;
    }

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// khttp client rpc

#define KLUA_KHTTPCLIENTRPC_HANDLE      "KHTTPCLIENTRPC_HANDLE*"


/// @struct klua_khttpclientrpc_t
/// @brief  HTTP-RPC 客户端
typedef struct klua_khttpclientrpc_t_
{
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua 环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua 协程扩展
        klb_netmulti_t*         p_netmulti;     ///< 复用
    };

    struct
    {
        lua_State*              co_recv;        ///< co_recv 协程
        lua_State*              co_call;        ///< co_call 协程
        lua_State*              co_wait;        ///< co_wait 协程
    };

    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< POST/NOTIFY, 存储 klb_buf_t*
        klb_mnp_rpc_t           rpc;            ///< 上一个收到的 RPC 头
        klb_buf_t*              p_rpc_buf;      ///< 上一个 POST/NOTIFY 帧
    };

    struct
    {
        int                     req_sequence;   ///< 最近一次 REQUEST 序号
        klb_nlist_t*            p_res_nlist;    ///< RESPONSE, 存储 klb_buf_t*
        klb_mnp_rpc_t           res;            ///< 上一个 RESPONSE 头
        klb_buf_t*              p_res_buf;      ///< 上一个 RESPONSE 帧
    };

    struct
    {
        klb_netconn_t*          p_client_conn;  ///< HTTP 客户端连接
        char                    host[128];      ///< Host
        int                     port;           ///< 端口
    };
}klua_khttpclientrpc_t;


static klua_khttpclientrpc_t* new_klua_khttpclientrpc(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = (klua_khttpclientrpc_t*)lua_newuserdata(L, sizeof(klua_khttpclientrpc_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_khttpclientrpc_t));
    luaL_setmetatable(L, KLUA_KHTTPCLIENTRPC_HANDLE);

    p_client->L = L;
    p_client->p_env = klua_env_get_by_L(L);
    p_client->p_coex = klua_coroutine_get(p_client->p_env);
    p_client->p_netmulti = klua_netmulti_get(p_client->p_env);
    p_client->p_rpc_nlist = klb_nlist_create();
    p_client->p_res_nlist = klb_nlist_create();
    return p_client;
}

static klua_khttpclientrpc_t* to_klua_khttpclientrpc(lua_State* L, int index)
{
    klua_khttpclientrpc_t* p_client = (klua_khttpclientrpc_t*)luaL_checkudata(L, index, KLUA_KHTTPCLIENTRPC_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'khttpclientrpc' expected");
    return p_client;
}

static int klua_khttpclientrpc_tostring(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);
    lua_pushfstring(L, "khttpclientrpc:%p", p_client);
    return 1;
}

static int klua_khttpclientrpc_close(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);

    if (NULL != p_client->p_client_conn)
    {
        klb_netconn_bind_recv_data(p_client->p_client_conn, NULL);
        klb_netconn_set_udata(p_client->p_client_conn, NULL);
        klb_httpclient_conn_free(p_client->p_client_conn);
        p_client->p_client_conn = NULL;
    }

    clear_buf_nlist_klb_httprpc(p_client->p_rpc_nlist);
    clear_buf_nlist_klb_httprpc(p_client->p_res_nlist);
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_rpc_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_res_nlist, klb_nlist_destroy);
    return 0;
}

static void update_rpc_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_rpc_buf = p_buf;
    }
}

static void update_res_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->res, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_res_buf = p_buf;
    }
}

static int call_co_recv_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    if (NULL == p_client->co_recv || 0 >= klb_nlist_size(p_client->p_rpc_nlist))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_recv = NULL;

    klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_client->p_rpc_nlist);
    int num = push_rpc_body_klb_httprpc(L, p_buf);
    update_rpc_klua_khttpclientrpc(p_client, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    if (NULL == p_client->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_recv = NULL;
    lua_pushnil(L);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_res_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    if (NULL == p_client->co_call)
    {
        return -1;
    }

    while (0 < klb_nlist_size(p_client->p_res_nlist))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_head(p_client->p_res_nlist);
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_tmp->p_buf + p_tmp->start);

        if (KLB_MNP_RPC_RESPONSE == (int)p_rpc->method && p_client->req_sequence == (int)p_rpc->sequence)
        {
            break;
        }

        klb_nlist_pop_head(p_client->p_res_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    if (0 >= klb_nlist_size(p_client->p_res_nlist))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_call));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_call);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_call = NULL;

    klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_client->p_res_nlist);
    int num = push_rpc_body_klb_httprpc(L, p_buf);
    update_res_klua_khttpclientrpc(p_client, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_res_end_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    if (NULL == p_client->co_call)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_call));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_call);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_call = NULL;
    lua_pushnil(L);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_wait_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    if (NULL == p_client->co_wait)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_wait));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_wait);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_wait = NULL;
    lua_pushnil(L);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static void dispatch_client_klb_httprpc(klua_khttpclientrpc_t* p_client, klb_buf_t* p_rpc)
{
    klb_mnp_rpc_t* p_hdr = (klb_mnp_rpc_t*)(p_rpc->p_buf + p_rpc->start);

    if (KLB_MNP_RPC_POST == (int)p_hdr->method || KLB_MNP_RPC_NOTIFY == (int)p_hdr->method)
    {
        klb_nlist_push_tail(p_client->p_rpc_nlist, p_rpc);
        call_co_recv_klua_khttpclientrpc(p_client);
    }
    else if (KLB_MNP_RPC_RESPONSE == (int)p_hdr->method && p_client->req_sequence == (int)p_hdr->sequence)
    {
        klb_nlist_push_tail(p_client->p_res_nlist, p_rpc);
        call_co_res_klua_khttpclientrpc(p_client);
    }
    else
    {
        KLB_FREE_BY(p_rpc, klb_buf_unref);
    }
}

static int on_recv_data_klua_khttpclientrpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_khttpclientrpc_t* p_client = (klua_khttpclientrpc_t*)p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        if (KLB_MNP_TEXT == packtype && NULL != p_data)
        {
            klb_buf_t* p_rpc = take_rpc_from_http(p_data);
            KLB_FREE_BY(p_data, klb_buf_unref);
            if (NULL != p_rpc)
            {
                dispatch_client_klb_httprpc(p_client, p_rpc);
            }
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_NETCODE_WBUF_EMPTY == code)
    {
        call_co_wait_klua_khttpclientrpc(p_client);
    }
    else if (KLB_SOCKET_CONNECT != code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        call_co_recv_end_klua_khttpclientrpc(p_client);
        call_co_res_end_klua_khttpclientrpc(p_client);
        call_co_wait_klua_khttpclientrpc(p_client);
    }

    return 0;
}

static int next_sequence_klua_khttpclientrpc(klua_khttpclientrpc_t* p_client)
{
    int sequence = p_client->req_sequence + 1;

    if (1000 > sequence)
    {
        sequence = 1000;
    }

    if (1000000 < sequence)
    {
        sequence = 1000;
    }

    return sequence;
}

static int send_klua_khttpclientrpc(lua_State* L, int method, klua_khttpclientrpc_t** p_out, int* p_sequence)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);
    int rpctype = (int)luaL_checkinteger(L, 2);
    int sequence = (KLB_MNP_RPC_REQUEST == method) ? next_sequence_klua_khttpclientrpc(p_client) : 0;

    if (NULL != p_out)
    {
        *p_out = p_client;
    }

    if (NULL != p_sequence)
    {
        *p_sequence = sequence;
    }

    if (NULL == p_client->p_client_conn)
    {
        return 1;
    }

    const uint8_t* p_payload = NULL;
    int payload_len = 0;
    klb_buf_t* p_hold = NULL;
    int ret = pack_payload_klb_httprpc(L, rpctype, 2, &p_payload, &payload_len, &p_hold);

    if (0 == ret)
    {
        ret = send_http_rpc(p_client->p_client_conn, true, p_client->host, p_client->port,
            rpctype, method, sequence, p_payload, payload_len);
    }

    KLB_FREE_BY(p_hold, klb_buf_unref);
    return ret;
}

static int klua_khttpclientrpc_post(lua_State* L)
{
    int ret = send_klua_khttpclientrpc(L, KLB_MNP_RPC_POST, NULL, NULL);
    lua_pushinteger(L, ret);
    return 1;
}

static int klua_khttpclientrpc_notify(lua_State* L)
{
    int ret = send_klua_khttpclientrpc(L, KLB_MNP_RPC_NOTIFY, NULL, NULL);
    lua_pushinteger(L, ret);
    return 1;
}

static int on_yield_klua_khttpclientrpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_khttpclientrpc_t* p_client = (klua_khttpclientrpc_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_khttpclientrpc(p_client);
        call_co_res_end_klua_khttpclientrpc(p_client);
        call_co_wait_klua_khttpclientrpc(p_client);
    }

    return 0;
}

static int klua_khttpclientrpc_co_recv(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);
    klua_check_coroutine(L, "khttpclientrpc:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    if (0 < klb_nlist_size(p_client->p_rpc_nlist))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_client->p_rpc_nlist);
        int num = push_rpc_body_klb_httprpc(L, p_buf);
        update_rpc_klua_khttpclientrpc(p_client, p_buf);
        return num;
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_khttpclientrpc, p_client);
}

static int klua_khttpclientrpc_co_call(lua_State* L)
{
    klua_check_coroutine(L, "khttpclientrpc:co_call must in coroutine!");

    klua_khttpclientrpc_t* p_client = NULL;
    int sequence = 0;
    int ret = send_klua_khttpclientrpc(L, KLB_MNP_RPC_REQUEST, &p_client, &sequence);

    if (0 == ret)
    {
        assert(NULL == p_client->co_call);
        p_client->co_call = L;
        p_client->req_sequence = sequence;
        return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_khttpclientrpc, p_client);
    }

    lua_pushnil(L);
    return 1;
}

static int klua_khttpclientrpc_co_wait(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);
    klua_check_coroutine(L, "khttpclientrpc:co_wait must in coroutine!");
    assert(NULL == p_client->co_wait);

    if (NULL == p_client->p_client_conn || klb_httpclient_wbuf_is_empty(p_client->p_client_conn))
    {
        lua_pushnil(L);
        return 1;
    }

    p_client->co_wait = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_khttpclientrpc, p_client);
}

static int klua_khttpclientrpc_status(lua_State* L)
{
    klua_khttpclientrpc_t* p_client = to_klua_khttpclientrpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_httprpcer_to_rpctype_string((int)p_client->rpc.rpctype));
    klua_setfield_string(L, "method", klb_httprpcer_to_method_string((int)p_client->rpc.method));
    klua_setfield_integer(L, "sequence", (lua_Integer)p_client->rpc.sequence);
    klua_setfield_string(L, "res_rpctype", klb_httprpcer_to_rpctype_string((int)p_client->res.rpctype));
    klua_setfield_string(L, "res_method", klb_httprpcer_to_method_string((int)p_client->res.method));
    klua_setfield_integer(L, "res_sequence", (lua_Integer)p_client->res.sequence);
    return 1;
}

void klua_khttpclientrpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_khttpclientrpc_close },
        { "post",           klua_khttpclientrpc_post },
        { "notify",         klua_khttpclientrpc_notify },
        { "co_recv",        klua_khttpclientrpc_co_recv },
        { "co_call",        klua_khttpclientrpc_co_call },
        { "co_wait",        klua_khttpclientrpc_co_wait },
        { "status",         klua_khttpclientrpc_status },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_khttpclientrpc_close },
        { "__close",        klua_khttpclientrpc_close },
        { "__tostring",     klua_khttpclientrpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTPCLIENTRPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int klua_khttp_connect_rpc(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    bool tls = false;
    size_t host_len = 0;
    int code = 0;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_khttpclientrpc_t* p_client = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);
    }

    host_len = strlen(p_host);
    if (0 == host_len || host_len >= 128)
    {
        return push_err_klua_khttprpc(L, KLB_NETCODE_EINVAL);
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_khttprpc(L, KLB_NETCODE_EINVAL);
    }

    p_conn = klb_httpclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_conn)
    {
        return push_err_klua_khttprpc(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    p_client = new_klua_khttpclientrpc(L);
    memcpy(p_client->host, p_host, host_len + 1);
    p_client->port = port;
    klb_netconn_set_udata(p_conn, p_client);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_khttpclientrpc);
    p_client->p_client_conn = p_conn;
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// khttp serve rpc

#define KLUA_KHTTPSERVERPC_HANDLE       "KHTTPSERVERPC_HANDLE*"


/// @struct klua_khttpserverpc_t
/// @brief  HTTP-RPC 服务连接
typedef struct klua_khttpserverpc_t_
{
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua 环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua 协程扩展
        klb_netmulti_t*         p_netmulti;     ///< 复用
    };

    struct
    {
        lua_State*              co_recv;        ///< co_recv 协程
    };

    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< POST/NOTIFY/REQUEST, 存储 klb_buf_t*
        klb_mnp_rpc_t           rpc;            ///< 上一个收到的 RPC 头
        klb_buf_t*              p_rpc_buf;      ///< 上一个 RPC 帧
    };

    struct
    {
        klb_netconn_t*          p_serve_conn;   ///< HTTP 服务连接
    };
}klua_khttpserverpc_t;


static klua_khttpserverpc_t* new_klua_khttpserverpc(lua_State* L)
{
    klua_khttpserverpc_t* p_serve = (klua_khttpserverpc_t*)lua_newuserdata(L, sizeof(klua_khttpserverpc_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_khttpserverpc_t));
    luaL_setmetatable(L, KLUA_KHTTPSERVERPC_HANDLE);

    p_serve->L = L;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);
    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->p_rpc_nlist = klb_nlist_create();
    return p_serve;
}

static klua_khttpserverpc_t* to_klua_khttpserverpc(lua_State* L, int index)
{
    klua_khttpserverpc_t* p_serve = (klua_khttpserverpc_t*)luaL_checkudata(L, index, KLUA_KHTTPSERVERPC_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'khttpserverpc' expected");
    return p_serve;
}

static int klua_khttpserverpc_tostring(lua_State* L)
{
    klua_khttpserverpc_t* p_serve = to_klua_khttpserverpc(L, 1);
    lua_pushfstring(L, "khttpserverpc:%p", p_serve);
    return 1;
}

static int klua_khttpserverpc_close(lua_State* L)
{
    klua_khttpserverpc_t* p_serve = to_klua_khttpserverpc(L, 1);

    if (NULL != p_serve->p_serve_conn)
    {
        klb_netconn_bind_recv_data(p_serve->p_serve_conn, NULL);
        klb_netconn_set_udata(p_serve->p_serve_conn, NULL);
        klb_httpserve_conn_free(p_serve->p_serve_conn);
        p_serve->p_serve_conn = NULL;
    }

    clear_buf_nlist_klb_httprpc(p_serve->p_rpc_nlist);
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_serve->p_rpc_nlist, klb_nlist_destroy);
    return 0;
}

static void update_rpc_klua_khttpserverpc(klua_khttpserverpc_t* p_serve, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_serve->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_serve->p_rpc_buf = p_buf;
    }
}

static int call_co_recv_klua_khttpserverpc(klua_khttpserverpc_t* p_serve)
{
    if (NULL == p_serve->co_recv || 0 >= klb_nlist_size(p_serve->p_rpc_nlist))
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

    klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_serve->p_rpc_nlist);
    int num = push_rpc_body_klb_httprpc(L, p_buf);
    update_rpc_klua_khttpserverpc(p_serve, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_khttpserverpc(klua_khttpserverpc_t* p_serve)
{
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
    lua_pushnil(L);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int on_recv_data_klua_khttpserverpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_khttpserverpc_t* p_serve = (klua_khttpserverpc_t*)p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        if (KLB_MNP_TEXT == packtype && NULL != p_data)
        {
            klb_buf_t* p_rpc = take_rpc_from_http(p_data);
            KLB_FREE_BY(p_data, klb_buf_unref);

            if (NULL != p_rpc)
            {
                klb_mnp_rpc_t* p_hdr = (klb_mnp_rpc_t*)(p_rpc->p_buf + p_rpc->start);
                int method = (int)p_hdr->method;

                if (KLB_MNP_RPC_POST == method || KLB_MNP_RPC_NOTIFY == method || KLB_MNP_RPC_REQUEST == method)
                {
                    klb_nlist_push_tail(p_serve->p_rpc_nlist, p_rpc);
                    call_co_recv_klua_khttpserverpc(p_serve);
                }
                else
                {
                    KLB_FREE_BY(p_rpc, klb_buf_unref);
                }
            }
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_NETCODE_WBUF_EMPTY == code)
    {
        // 服务端无 co_wait
    }
    else if (KLB_SOCKET_CONNECT != code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        call_co_recv_end_klua_khttpserverpc(p_serve);
    }

    return 0;
}

static int send_klua_khttpserverpc(lua_State* L, int method)
{
    klua_khttpserverpc_t* p_serve = to_klua_khttpserverpc(L, 1);
    int rpctype = (int)luaL_checkinteger(L, 2);
    int sequence = 0;
    int base_idx = 2;

    if (KLB_MNP_RPC_RESPONSE == method)
    {
        sequence = (int)luaL_checkinteger(L, base_idx + 1);
        base_idx += 1;
    }

    int ret = 1;
    if (NULL != p_serve->p_serve_conn)
    {
        const uint8_t* p_payload = NULL;
        int payload_len = 0;
        klb_buf_t* p_hold = NULL;
        ret = pack_payload_klb_httprpc(L, rpctype, base_idx, &p_payload, &payload_len, &p_hold);

        if (0 == ret)
        {
            ret = send_http_rpc(p_serve->p_serve_conn, false, NULL, 0,
                rpctype, method, sequence, p_payload, payload_len);
        }

        KLB_FREE_BY(p_hold, klb_buf_unref);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_khttpserverpc_post(lua_State* L)
{
    return send_klua_khttpserverpc(L, KLB_MNP_RPC_POST);
}

static int klua_khttpserverpc_notify(lua_State* L)
{
    return send_klua_khttpserverpc(L, KLB_MNP_RPC_NOTIFY);
}

static int klua_khttpserverpc_response(lua_State* L)
{
    return send_klua_khttpserverpc(L, KLB_MNP_RPC_RESPONSE);
}

static int on_yield_klua_khttpserverpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_khttpserverpc_t* p_serve = (klua_khttpserverpc_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_khttpserverpc(p_serve);
    }

    return 0;
}

static int klua_khttpserverpc_co_recv(lua_State* L)
{
    klua_khttpserverpc_t* p_serve = to_klua_khttpserverpc(L, 1);
    klua_check_coroutine(L, "khttpserverpc:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (0 < klb_nlist_size(p_serve->p_rpc_nlist))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_serve->p_rpc_nlist);
        int num = push_rpc_body_klb_httprpc(L, p_buf);
        update_rpc_klua_khttpserverpc(p_serve, p_buf);
        return num;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_klua_khttpserverpc, p_serve);
}

static int klua_khttpserverpc_status(lua_State* L)
{
    klua_khttpserverpc_t* p_serve = to_klua_khttpserverpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_httprpcer_to_rpctype_string((int)p_serve->rpc.rpctype));
    klua_setfield_string(L, "method", klb_httprpcer_to_method_string((int)p_serve->rpc.method));
    klua_setfield_integer(L, "sequence", (lua_Integer)p_serve->rpc.sequence);
    return 1;
}

void klua_khttpserverpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_khttpserverpc_close },
        { "post",           klua_khttpserverpc_post },
        { "notify",         klua_khttpserverpc_notify },
        { "response",       klua_khttpserverpc_response },
        { "co_recv",        klua_khttpserverpc_co_recv },
        { "status",         klua_khttpserverpc_status },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_khttpserverpc_close },
        { "__close",        klua_khttpserverpc_close },
        { "__tostring",     klua_khttpserverpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTPSERVERPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////
// khttp rpc listen

#define KLUA_KHTTPRPC_LISTEN_HANDLE     "KHTTPRPC_LISTEN_HANDLE*"


/// @struct klua_khttprpc_listen_t
/// @brief  HTTP-RPC 监听
typedef struct klua_khttprpc_listen_t_
{
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< co_accept 协程
        klua_env_t*             p_env;          ///< lua 环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua 协程扩展
    };

    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
        bool                    tls;            ///< 是否 TLS
        bool                    plain;          ///< tls 时是否同时收明文
        klb_socket_tls_server_ctx_t* p_tls_ctx; ///< TLS 服务端上下文
    };

    struct
    {
        bool                    closed;         ///< 已 close
        klb_nlist_t*            p_probe_nlist;  ///< 探测中的连接
        klb_nlist_t*            p_socket_nlist; ///< 待 accept 的 socket
    };
}klua_khttprpc_listen_t;


/// @struct klua_khttprpc_probe_t
/// @brief  探测上下文
typedef struct klua_khttprpc_probe_t_
{
    klua_khttprpc_listen_t*     p_listen;       ///< 所属 listen
    struct sockaddr_in          addr;           ///< accept 地址
}klua_khttprpc_probe_t;


static klua_khttprpc_listen_t* new_klua_khttprpc_listen(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = (klua_khttprpc_listen_t*)lua_newuserdata(L, sizeof(klua_khttprpc_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_khttprpc_listen_t));
    luaL_setmetatable(L, KLUA_KHTTPRPC_LISTEN_HANDLE);
    return p_listen;
}

static klua_khttprpc_listen_t* to_klua_khttprpc_listen(lua_State* L, int index)
{
    klua_khttprpc_listen_t* p_listen = (klua_khttprpc_listen_t*)luaL_checkudata(L, index, KLUA_KHTTPRPC_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'khttprpc listen' expected");
    return p_listen;
}

static int klua_khttprpc_listen_tostring(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);
    lua_pushfstring(L, "khttprpc listen:%p", p_listen);
    return 1;
}

static void remove_probe_klua_khttprpc_listen(klua_khttprpc_listen_t* p_listen, klb_netconn_t* p_probe)
{
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

static void abort_probes_klua_khttprpc_listen(klua_khttprpc_listen_t* p_listen)
{
    if (NULL == p_listen->p_probe_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_listen->p_probe_nlist))
    {
        klb_netconn_t* p_probe = (klb_netconn_t*)klb_nlist_pop_head(p_listen->p_probe_nlist);
        klua_khttprpc_probe_t* p_ctx = (klua_khttprpc_probe_t*)p_probe->p_udata;

        klb_netconn_set_udata(p_probe, NULL);
        KLB_FREE(p_ctx);
        klb_netprobe_conn_free(p_probe);
    }
}

static int klua_khttprpc_listen_close(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);

    p_listen->closed = true;
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);
    abort_probes_klua_khttprpc_listen(p_listen);
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

static klua_khttpserverpc_t* new_klua_khttpserverpc_by_socket(lua_State* L, klua_khttprpc_listen_t* p_listen, klb_netlisten_socket_t* p_listen_socket)
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
    klua_khttpserverpc_t* p_serve = new_klua_khttpserverpc(L);

    klb_netconn_set_udata(p_conn, p_serve);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_khttpserverpc);
    p_serve->p_serve_conn = p_conn;
    return p_serve;
}

static int call_co_accept_klua_khttprpc_listen(klua_khttprpc_listen_t* p_listen)
{
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
    if (NULL == new_klua_khttpserverpc_by_socket(L, p_listen, p_listen_socket))
    {
        lua_pushnil(L);
    }
    KLB_FREE(p_listen_socket);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_accept_end_klua_khttprpc_listen(klua_khttprpc_listen_t* p_listen)
{
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

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int on_probe_done_klua_khttprpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, int protocol, bool tls, const uint8_t* p_peek, int peek_len)
{
    klua_khttprpc_probe_t* p_ctx = (klua_khttprpc_probe_t*)ptr;
    klua_khttprpc_listen_t* p_listen = p_ctx->p_listen;

    (void)p_peek;
    (void)peek_len;

    remove_probe_klua_khttprpc_listen(p_listen, p_conn);
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
    call_co_accept_klua_khttprpc_listen(p_listen);
    return 0;
}

static int on_accept_klua_khttprpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_khttprpc_listen_t* p_listen = (klua_khttprpc_listen_t*)p_conn->p_udata;

    (void)ptr;
    (void)tls;

    if (p_listen->closed)
    {
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klua_khttprpc_probe_t* p_ctx = KLB_MALLOCZ(klua_khttprpc_probe_t, 1, 0);
    p_ctx->p_listen = p_listen;
    p_ctx->addr = *p_addr;

    klb_netconn_t* p_probe = klb_netprobe_conn_create(p_listen->p_netmulti);
    klb_netconn_set_udata(p_probe, p_ctx);
    klb_netprobe_conn_set_done(p_probe, on_probe_done_klua_khttprpc_listen, p_ctx);

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

static int on_yield_accept_klua_khttprpc_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_khttprpc_listen_t* p_listen = (klua_khttprpc_listen_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_khttprpc_listen(p_listen);
    }

    return 0;
}

static int klua_khttprpc_listen_port(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_khttprpc_listen_local_addr(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_khttprpc_listen_native_fd(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static int klua_khttprpc_listen_co_accept(lua_State* L)
{
    klua_khttprpc_listen_t* p_listen = to_klua_khttprpc_listen(L, 1);
    klua_check_coroutine(L, "khttprpc.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        if (NULL == new_klua_khttpserverpc_by_socket(L, p_listen, p_listen_socket))
        {
            lua_pushnil(L);
        }
        KLB_FREE(p_listen_socket);
        return 1;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_khttprpc_listen, p_listen);
}

void klua_khttpserverpc_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_khttprpc_listen_close },
        { "co_accept",      klua_khttprpc_listen_co_accept },
        { "port",           klua_khttprpc_listen_port },
        { "local_addr",     klua_khttprpc_listen_local_addr },
        { "native_fd",      klua_khttprpc_listen_native_fd },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_khttprpc_listen_close },
        { "__close",        klua_khttprpc_listen_close },
        { "__tostring",     klua_khttprpc_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTPRPC_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int klua_khttp_listen_rpc(lua_State* L)
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
    klua_khttprpc_listen_t* p_listen = NULL;

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
            return push_err_klua_khttprpc(L, rc);
        }
    }

    if (tls)
    {
        p_tls_ctx = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            return push_err_klua_khttprpc(L, KLB_NETCODE_TLS_CERT);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_khttprpc(L, KLB_NETCODE_EINVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_khttprpc_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_khttprpc(L, rc);
    }

    p_listen = new_klua_khttprpc_listen(L);
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

// end
