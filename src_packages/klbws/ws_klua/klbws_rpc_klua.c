// Doc Encode : UTF-8 BOM, Unix(LF)
// kws RPC: require("kws").connect_rpc / listen_rpc
#include "klbws/ws_klua/klbws_rpc_klua.h"
#include "klbws/core/klb_wsrpcer.h"
#include "klbws/core/klb_wsclient_conn.h"
#include "klbws/core/klb_wsserve_conn.h"
#include "klbws/core/klb_websocket.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbthird/sds.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kwsrpc(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


//////////////////////////////////////////////////////////////////////////
// 帧: WS BINARY 载荷 = [klb_mnp_rpc_t] + 正文

static int send_ws_rpc(klb_netconn_t* p_conn, bool is_client, int rpctype, int method, int sequence,
    const uint8_t* p_payload, int payload_len)
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
    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = (uint32_t)sequence;
    rpc.rpctype = (uint32_t)rpctype;
    rpc.method = (uint32_t)method;
    rpc.encode = KLB_MNP_ENCODE_NULL;
    rpc.size = (uint32_t)body_len;

    klb_buf_t* p_body = klb_buf_malloc(body_len, false);
    klb_buf_write(p_body, (const char*)&rpc, (int)sizeof(rpc));
    if (0 < payload_len && NULL != p_payload)
    {
        klb_buf_write(p_body, (const char*)p_payload, payload_len);
    }

    uint8_t mask_key[4] = { 0 };
    const uint8_t* p_mask = NULL;
    if (is_client)
    {
        klb_websocket_mask_key(mask_key);
        p_mask = mask_key;
    }

    klb_buf_t* p_frame = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_BINARY, p_mask,
        p_body->p_buf + p_body->start, klb_buf_data_len(p_body));
    KLB_FREE_BY(p_body, klb_buf_unref);

    if (NULL == p_frame)
    {
        return 1;
    }

    // send_buf 接管 p_frame, 失败时由其释放
    if (is_client)
    {
        return klb_wsclient_conn_send_buf(p_conn, p_frame);
    }

    return klb_wsserve_conn_send_buf(p_conn, p_frame);
}

static klb_buf_t* take_rpc_from_ws(klb_buf_t* p_data)
{
    int data_len = p_data->end - p_data->start;
    int hdr = (int)sizeof(klb_mnp_binary_t);

    if (data_len < hdr)
    {
        return NULL;
    }

    klb_mnp_binary_t* p_bin = (klb_mnp_binary_t*)(p_data->p_buf + p_data->start);
    int head_len = (int)p_bin->head_size;
    int total = (int)p_bin->size;

    if (0 > head_len || total < hdr + head_len || total > data_len)
    {
        return NULL;
    }

    int body_len = total - hdr - head_len;
    if (body_len < (int)sizeof(klb_mnp_rpc_t))
    {
        return NULL;
    }

    const char* p_body = (const char*)(p_bin + 1) + head_len;
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

static void clear_buf_nlist_klb_wsrpc(klb_nlist_t* p_nlist)
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

static int push_rpc_body_klb_wsrpc(lua_State* L, klb_buf_t* p_buf)
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

static int pack_payload_klb_wsrpc(lua_State* L, int rpctype, int base_idx, const uint8_t** p_out, int* p_len, klb_buf_t** p_hold)
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

static bool text_has_ctl_klb_wsrpc(const char* p, size_t n)
{
    for (size_t i = 0; i < n; ++i)
    {
        unsigned char c = (unsigned char)p[i];
        if ('\r' == c || '\n' == c)
        {
            return true;
        }
    }

    return false;
}

static void append_host_klb_wsrpc(sds* p_str, const char* p_host, int port, bool tls)
{
    bool v6 = (NULL != strchr(p_host, ':'));
    bool bracket = (v6 && '[' != p_host[0]);
    bool omit = ((!tls && 80 == port) || (tls && 443 == port));
    sds str = *p_str;

    str = sdscat(str, "Host: ");
    if (bracket)
    {
        str = sdscat(str, "[");
        str = sdscat(str, p_host);
        str = sdscat(str, "]");
    }
    else
    {
        str = sdscat(str, p_host);
    }

    if (!omit)
    {
        char port_buf[16];
        snprintf(port_buf, sizeof(port_buf), ":%d", port);
        str = sdscat(str, port_buf);
    }

    str = sdscat(str, "\r\n");
    *p_str = str;
}


//////////////////////////////////////////////////////////////////////////
// kws client rpc

#define KLUA_KWSCLIENTRPC_HANDLE        "KWSCLIENTRPC_HANDLE*"


/// @struct klua_kwsclientrpc_t
/// @brief  WS-RPC 客户端
typedef struct klua_kwsclientrpc_t_
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
        klb_netconn_t*          p_client_conn;  ///< WS 客户端连接
    };
}klua_kwsclientrpc_t;


static klua_kwsclientrpc_t* new_klua_kwsclientrpc(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = (klua_kwsclientrpc_t*)lua_newuserdata(L, sizeof(klua_kwsclientrpc_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_kwsclientrpc_t));
    luaL_setmetatable(L, KLUA_KWSCLIENTRPC_HANDLE);

    p_client->L = L;
    p_client->p_env = klua_env_get_by_L(L);
    p_client->p_coex = klua_coroutine_get(p_client->p_env);
    p_client->p_netmulti = klua_netmulti_get(p_client->p_env);
    p_client->p_rpc_nlist = klb_nlist_create();
    p_client->p_res_nlist = klb_nlist_create();
    return p_client;
}

static klua_kwsclientrpc_t* to_klua_kwsclientrpc(lua_State* L, int index)
{
    klua_kwsclientrpc_t* p_client = (klua_kwsclientrpc_t*)luaL_checkudata(L, index, KLUA_KWSCLIENTRPC_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'kwsclientrpc' expected");
    return p_client;
}

static int klua_kwsclientrpc_tostring(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);
    lua_pushfstring(L, "kwsclientrpc:%p", p_client);
    return 1;
}

static int klua_kwsclientrpc_close(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);

    if (NULL != p_client->p_client_conn)
    {
        klb_netconn_bind_recv_data(p_client->p_client_conn, NULL);
        klb_netconn_set_udata(p_client->p_client_conn, NULL);
        klb_wsclient_conn_free(p_client->p_client_conn);
        p_client->p_client_conn = NULL;
    }

    clear_buf_nlist_klb_wsrpc(p_client->p_rpc_nlist);
    clear_buf_nlist_klb_wsrpc(p_client->p_res_nlist);
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_rpc_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_res_nlist, klb_nlist_destroy);
    return 0;
}

static void update_rpc_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_rpc_buf = p_buf;
    }
}

static void update_res_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->res, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_res_buf = p_buf;
    }
}

static int call_co_recv_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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
    int num = push_rpc_body_klb_wsrpc(L, p_buf);
    update_rpc_klua_kwsclientrpc(p_client, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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

static int call_co_res_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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
    int num = push_rpc_body_klb_wsrpc(L, p_buf);
    update_res_klua_kwsclientrpc(p_client, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_res_end_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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

static int call_co_wait_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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

static void dispatch_client_klb_wsrpc(klua_kwsclientrpc_t* p_client, klb_buf_t* p_rpc)
{
    klb_mnp_rpc_t* p_hdr = (klb_mnp_rpc_t*)(p_rpc->p_buf + p_rpc->start);

    if (KLB_MNP_RPC_POST == (int)p_hdr->method || KLB_MNP_RPC_NOTIFY == (int)p_hdr->method)
    {
        klb_nlist_push_tail(p_client->p_rpc_nlist, p_rpc);
        call_co_recv_klua_kwsclientrpc(p_client);
    }
    else if (KLB_MNP_RPC_RESPONSE == (int)p_hdr->method && p_client->req_sequence == (int)p_hdr->sequence)
    {
        klb_nlist_push_tail(p_client->p_res_nlist, p_rpc);
        call_co_res_klua_kwsclientrpc(p_client);
    }
    else
    {
        KLB_FREE_BY(p_rpc, klb_buf_unref);
    }
}

static void end_client_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
{
    call_co_recv_end_klua_kwsclientrpc(p_client);
    call_co_res_end_klua_kwsclientrpc(p_client);
    call_co_wait_klua_kwsclientrpc(p_client);
}

static int on_recv_data_klua_kwsclientrpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kwsclientrpc_t* p_client = (klua_kwsclientrpc_t*)p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        if (KLB_MNP_BINARY == packtype && NULL != p_data)
        {
            klb_buf_t* p_rpc = take_rpc_from_ws(p_data);
            KLB_FREE_BY(p_data, klb_buf_unref);
            if (NULL != p_rpc)
            {
                dispatch_client_klb_wsrpc(p_client, p_rpc);
            }
        }
        else if (KLB_WEBSOCKET_PACK_CLOSE == packtype)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            end_client_klua_kwsclientrpc(p_client);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_NETCODE_WBUF_EMPTY == code)
    {
        call_co_wait_klua_kwsclientrpc(p_client);
    }
    else if (KLB_SOCKET_CONNECT != code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        end_client_klua_kwsclientrpc(p_client);
    }

    return 0;
}

static int next_sequence_klua_kwsclientrpc(klua_kwsclientrpc_t* p_client)
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

static int send_klua_kwsclientrpc(lua_State* L, int method, klua_kwsclientrpc_t** p_out, int* p_sequence)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);
    int rpctype = (int)luaL_checkinteger(L, 2);
    int sequence = (KLB_MNP_RPC_REQUEST == method) ? next_sequence_klua_kwsclientrpc(p_client) : 0;

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
    int ret = pack_payload_klb_wsrpc(L, rpctype, 2, &p_payload, &payload_len, &p_hold);

    if (0 == ret)
    {
        ret = send_ws_rpc(p_client->p_client_conn, true, rpctype, method, sequence, p_payload, payload_len);
    }

    KLB_FREE_BY(p_hold, klb_buf_unref);
    return ret;
}

static int klua_kwsclientrpc_post(lua_State* L)
{
    int ret = send_klua_kwsclientrpc(L, KLB_MNP_RPC_POST, NULL, NULL);
    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwsclientrpc_notify(lua_State* L)
{
    int ret = send_klua_kwsclientrpc(L, KLB_MNP_RPC_NOTIFY, NULL, NULL);
    lua_pushinteger(L, ret);
    return 1;
}

static int on_yield_klua_kwsclientrpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kwsclientrpc_t* p_client = (klua_kwsclientrpc_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        end_client_klua_kwsclientrpc(p_client);
    }

    return 0;
}

static int klua_kwsclientrpc_co_recv(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);
    klua_check_coroutine(L, "kwsclientrpc:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    if (0 < klb_nlist_size(p_client->p_rpc_nlist))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_client->p_rpc_nlist);
        int num = push_rpc_body_klb_wsrpc(L, p_buf);
        update_rpc_klua_kwsclientrpc(p_client, p_buf);
        return num;
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_kwsclientrpc, p_client);
}

static int klua_kwsclientrpc_co_call(lua_State* L)
{
    klua_check_coroutine(L, "kwsclientrpc:co_call must in coroutine!");

    klua_kwsclientrpc_t* p_client = NULL;
    int sequence = 0;
    int ret = send_klua_kwsclientrpc(L, KLB_MNP_RPC_REQUEST, &p_client, &sequence);

    if (0 == ret)
    {
        assert(NULL == p_client->co_call);
        p_client->co_call = L;
        p_client->req_sequence = sequence;
        return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_kwsclientrpc, p_client);
    }

    lua_pushnil(L);
    return 1;
}

static int klua_kwsclientrpc_co_wait(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);
    klua_check_coroutine(L, "kwsclientrpc:co_wait must in coroutine!");
    assert(NULL == p_client->co_wait);

    if (NULL == p_client->p_client_conn || klb_wsclient_wbuf_is_empty(p_client->p_client_conn))
    {
        lua_pushnil(L);
        return 1;
    }

    p_client->co_wait = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_klua_kwsclientrpc, p_client);
}

static int klua_kwsclientrpc_status(lua_State* L)
{
    klua_kwsclientrpc_t* p_client = to_klua_kwsclientrpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_wsrpcer_to_rpctype_string((int)p_client->rpc.rpctype));
    klua_setfield_string(L, "method", klb_wsrpcer_to_method_string((int)p_client->rpc.method));
    klua_setfield_integer(L, "sequence", (lua_Integer)p_client->rpc.sequence);
    klua_setfield_string(L, "res_rpctype", klb_wsrpcer_to_rpctype_string((int)p_client->res.rpctype));
    klua_setfield_string(L, "res_method", klb_wsrpcer_to_method_string((int)p_client->res.method));
    klua_setfield_integer(L, "res_sequence", (lua_Integer)p_client->res.sequence);
    return 1;
}

void klua_kwsclientrpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kwsclientrpc_close },
        { "post",           klua_kwsclientrpc_post },
        { "notify",         klua_kwsclientrpc_notify },
        { "co_recv",        klua_kwsclientrpc_co_recv },
        { "co_call",        klua_kwsclientrpc_co_call },
        { "co_wait",        klua_kwsclientrpc_co_wait },
        { "status",         klua_kwsclientrpc_status },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kwsclientrpc_close },
        { "__close",        klua_kwsclientrpc_close },
        { "__tostring",     klua_kwsclientrpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KWSCLIENTRPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static sds build_upgrade_klb_wsrpc(const char* p_host, int port, const char* p_path, bool tls,
    const char* p_protocol, char* p_key, int key_cap)
{
    uint8_t nonce[16];
    klb_websocket_random(nonce, 16);

    if (0 != klb_websocket_b64(nonce, 16, p_key, key_cap))
    {
        return NULL;
    }

    sds str = sdsnew("");
    str = sdscatfmt(str, "GET %s HTTP/1.1\r\n", p_path);
    append_host_klb_wsrpc(&str, p_host, port, tls);
    str = sdscat(str, "Connection: Upgrade\r\n");
    str = sdscat(str, "Upgrade: websocket\r\n");
    str = sdscat(str, "Sec-WebSocket-Version: 13\r\n");
    str = sdscatfmt(str, "Sec-WebSocket-Key: %s\r\n", p_key);

    if (NULL != p_protocol && '\0' != p_protocol[0])
    {
        str = sdscatfmt(str, "Sec-WebSocket-Protocol: %s\r\n", p_protocol);
    }

    str = sdscat(str, "\r\n");
    return str;
}

int klua_kws_connect_rpc(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    bool tls = false;
    char path[256];
    char protocol[128];

    memcpy(path, "/rpc", sizeof("/rpc"));
    protocol[0] = '\0';

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);

        lua_getfield(L, 3, "path");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_path = lua_tolstring(L, -1, &n);
            if (0 == n || n >= sizeof(path) || text_has_ctl_klb_wsrpc(p_path, n))
            {
                lua_pop(L, 1);
                return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
            }

            memcpy(path, p_path, n + 1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 3, "protocol");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_proto = lua_tolstring(L, -1, &n);
            if (n >= sizeof(protocol) || (0 < n && text_has_ctl_klb_wsrpc(p_proto, n)))
            {
                lua_pop(L, 1);
                return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
            }

            if (0 < n)
            {
                memcpy(protocol, p_proto, n + 1);
            }
        }
        lua_pop(L, 1);
    }

    if (text_has_ctl_klb_wsrpc(p_host, strlen(p_host)))
    {
        return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
    }

    char key[32] = { 0 };
    int code = 0;
    sds req = build_upgrade_klb_wsrpc(p_host, port, path, tls, protocol, key, (int)sizeof(key));
    if (NULL == req)
    {
        return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = NULL;

    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(req, sdsfree);
        return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
    }

    p_conn = klb_wsclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_conn)
    {
        KLB_FREE_BY(req, sdsfree);
        return push_err_klua_kwsrpc(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    klua_kwsclientrpc_t* p_client = new_klua_kwsclientrpc(L);
    klb_netconn_set_udata(p_conn, p_client);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_kwsclientrpc);
    klb_wsclient_conn_set_key(p_conn, key);
    klb_wsclient_conn_set_protocol(p_conn, protocol);
    p_client->p_client_conn = p_conn;

    klb_wsclient_conn_send(p_conn, (const uint8_t*)req, (int)sdslen(req), NULL, 0);
    KLB_FREE_BY(req, sdsfree);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// kws serve rpc

#define KLUA_KWSSERVERPC_HANDLE         "KWSSERVERPC_HANDLE*"


/// @struct klua_kwsserverpc_t
/// @brief  WS-RPC 服务连接
typedef struct klua_kwsserverpc_t_
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
        klb_netconn_t*          p_serve_conn;   ///< WS 服务连接
    };
}klua_kwsserverpc_t;


static klua_kwsserverpc_t* new_klua_kwsserverpc(lua_State* L)
{
    klua_kwsserverpc_t* p_serve = (klua_kwsserverpc_t*)lua_newuserdata(L, sizeof(klua_kwsserverpc_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_kwsserverpc_t));
    luaL_setmetatable(L, KLUA_KWSSERVERPC_HANDLE);

    p_serve->L = L;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);
    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->p_rpc_nlist = klb_nlist_create();
    return p_serve;
}

static klua_kwsserverpc_t* to_klua_kwsserverpc(lua_State* L, int index)
{
    klua_kwsserverpc_t* p_serve = (klua_kwsserverpc_t*)luaL_checkudata(L, index, KLUA_KWSSERVERPC_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'kwsserverpc' expected");
    return p_serve;
}

static int klua_kwsserverpc_tostring(lua_State* L)
{
    klua_kwsserverpc_t* p_serve = to_klua_kwsserverpc(L, 1);
    lua_pushfstring(L, "kwsserverpc:%p", p_serve);
    return 1;
}

static int klua_kwsserverpc_close(lua_State* L)
{
    klua_kwsserverpc_t* p_serve = to_klua_kwsserverpc(L, 1);

    if (NULL != p_serve->p_serve_conn)
    {
        klb_netconn_bind_recv_data(p_serve->p_serve_conn, NULL);
        klb_netconn_set_udata(p_serve->p_serve_conn, NULL);
        klb_wsserve_conn_free(p_serve->p_serve_conn);
        p_serve->p_serve_conn = NULL;
    }

    clear_buf_nlist_klb_wsrpc(p_serve->p_rpc_nlist);
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_serve->p_rpc_nlist, klb_nlist_destroy);
    return 0;
}

static void update_rpc_klua_kwsserverpc(klua_kwsserverpc_t* p_serve, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_serve->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_serve->p_rpc_buf = p_buf;
    }
}

static int call_co_recv_klua_kwsserverpc(klua_kwsserverpc_t* p_serve)
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
    int num = push_rpc_body_klb_wsrpc(L, p_buf);
    update_rpc_klua_kwsserverpc(p_serve, p_buf);

    int status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_kwsserverpc(klua_kwsserverpc_t* p_serve)
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

static int on_recv_data_klua_kwsserverpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kwsserverpc_t* p_serve = (klua_kwsserverpc_t*)p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        if (KLB_MNP_BINARY == packtype && NULL != p_data)
        {
            klb_buf_t* p_rpc = take_rpc_from_ws(p_data);
            KLB_FREE_BY(p_data, klb_buf_unref);

            if (NULL != p_rpc)
            {
                klb_mnp_rpc_t* p_hdr = (klb_mnp_rpc_t*)(p_rpc->p_buf + p_rpc->start);
                int rpc_method = (int)p_hdr->method;

                if (KLB_MNP_RPC_POST == rpc_method || KLB_MNP_RPC_NOTIFY == rpc_method || KLB_MNP_RPC_REQUEST == rpc_method)
                {
                    klb_nlist_push_tail(p_serve->p_rpc_nlist, p_rpc);
                    call_co_recv_klua_kwsserverpc(p_serve);
                }
                else
                {
                    KLB_FREE_BY(p_rpc, klb_buf_unref);
                }
            }
        }
        else if (KLB_WEBSOCKET_PACK_CLOSE == packtype)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            call_co_recv_end_klua_kwsserverpc(p_serve);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_NETCODE_WBUF_EMPTY != code && KLB_SOCKET_CONNECT != code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        call_co_recv_end_klua_kwsserverpc(p_serve);
    }

    return 0;
}

static int send_klua_kwsserverpc(lua_State* L, int method)
{
    klua_kwsserverpc_t* p_serve = to_klua_kwsserverpc(L, 1);
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
        ret = pack_payload_klb_wsrpc(L, rpctype, base_idx, &p_payload, &payload_len, &p_hold);

        if (0 == ret)
        {
            ret = send_ws_rpc(p_serve->p_serve_conn, false, rpctype, method, sequence, p_payload, payload_len);
        }

        KLB_FREE_BY(p_hold, klb_buf_unref);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kwsserverpc_post(lua_State* L)
{
    return send_klua_kwsserverpc(L, KLB_MNP_RPC_POST);
}

static int klua_kwsserverpc_notify(lua_State* L)
{
    return send_klua_kwsserverpc(L, KLB_MNP_RPC_NOTIFY);
}

static int klua_kwsserverpc_response(lua_State* L)
{
    return send_klua_kwsserverpc(L, KLB_MNP_RPC_RESPONSE);
}

static int on_yield_klua_kwsserverpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kwsserverpc_t* p_serve = (klua_kwsserverpc_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_kwsserverpc(p_serve);
    }

    return 0;
}

static int klua_kwsserverpc_co_recv(lua_State* L)
{
    klua_kwsserverpc_t* p_serve = to_klua_kwsserverpc(L, 1);
    klua_check_coroutine(L, "kwsserverpc:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (0 < klb_nlist_size(p_serve->p_rpc_nlist))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_serve->p_rpc_nlist);
        int num = push_rpc_body_klb_wsrpc(L, p_buf);
        update_rpc_klua_kwsserverpc(p_serve, p_buf);
        return num;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_klua_kwsserverpc, p_serve);
}

static int klua_kwsserverpc_status(lua_State* L)
{
    klua_kwsserverpc_t* p_serve = to_klua_kwsserverpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_wsrpcer_to_rpctype_string((int)p_serve->rpc.rpctype));
    klua_setfield_string(L, "method", klb_wsrpcer_to_method_string((int)p_serve->rpc.method));
    klua_setfield_integer(L, "sequence", (lua_Integer)p_serve->rpc.sequence);
    return 1;
}

void klua_kwsserverpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kwsserverpc_close },
        { "post",           klua_kwsserverpc_post },
        { "notify",         klua_kwsserverpc_notify },
        { "response",       klua_kwsserverpc_response },
        { "co_recv",        klua_kwsserverpc_co_recv },
        { "status",         klua_kwsserverpc_status },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kwsserverpc_close },
        { "__close",        klua_kwsserverpc_close },
        { "__tostring",     klua_kwsserverpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KWSSERVERPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////
// kws rpc listen

#define KLUA_KWSRPC_LISTEN_HANDLE       "KWSRPC_LISTEN_HANDLE*"


/// @struct klua_kwsrpc_listen_t
/// @brief  WS-RPC 监听
typedef struct klua_kwsrpc_listen_t_
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
        klb_socket_tls_server_ctx_t* p_tls_ctx; ///< TLS 服务端上下文
        char                    path[256];      ///< 路径过滤, 默认 /rpc
        char                    protocol[128];  ///< 子协议, 空为不协商
    };

    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待 accept 的 socket
    };
}klua_kwsrpc_listen_t;


static klua_kwsrpc_listen_t* new_klua_kwsrpc_listen(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = (klua_kwsrpc_listen_t*)lua_newuserdata(L, sizeof(klua_kwsrpc_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_kwsrpc_listen_t));
    luaL_setmetatable(L, KLUA_KWSRPC_LISTEN_HANDLE);
    return p_listen;
}

static klua_kwsrpc_listen_t* to_klua_kwsrpc_listen(lua_State* L, int index)
{
    klua_kwsrpc_listen_t* p_listen = (klua_kwsrpc_listen_t*)luaL_checkudata(L, index, KLUA_KWSRPC_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'kwsrpc listen' expected");
    return p_listen;
}

static int klua_kwsrpc_listen_tostring(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);
    lua_pushfstring(L, "kwsrpc listen:%p", p_listen);
    return 1;
}

static int klua_kwsrpc_listen_close(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);

    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);
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

static klua_kwsserverpc_t* new_klua_kwsserverpc_by_socket(lua_State* L, klua_kwsrpc_listen_t* p_listen, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = NULL;

    if (p_listen->tls)
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
    klb_netconn_t* p_conn = klb_wsserve_conn_create(p_netmulti, p_socket);
    klb_wsserve_conn_set_filter(p_conn, p_listen->path, p_listen->protocol);

    klua_kwsserverpc_t* p_serve = new_klua_kwsserverpc(L);
    klb_netconn_set_udata(p_conn, p_serve);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_kwsserverpc);
    p_serve->p_serve_conn = p_conn;
    return p_serve;
}

static int call_co_accept_klua_kwsrpc_listen(klua_kwsrpc_listen_t* p_listen)
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
    if (NULL == new_klua_kwsserverpc_by_socket(L, p_listen, p_listen_socket))
    {
        lua_pushnil(L);
    }
    KLB_FREE(p_listen_socket);

    int status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);
    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_accept_end_klua_kwsrpc_listen(klua_kwsrpc_listen_t* p_listen)
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

static int on_accept_klua_kwsrpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_kwsrpc_listen_t* p_listen = (klua_kwsrpc_listen_t*)p_conn->p_udata;
    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);

    (void)ptr;
    (void)tls;

    p_tmp->fd = fd;
    p_tmp->addr = *p_addr;
    p_tmp->tls = p_listen->tls;
    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);
    call_co_accept_klua_kwsrpc_listen(p_listen);
    return 0;
}

static int on_yield_accept_klua_kwsrpc_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kwsrpc_listen_t* p_listen = (klua_kwsrpc_listen_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_kwsrpc_listen(p_listen);
    }

    return 0;
}

static int klua_kwsrpc_listen_port(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_kwsrpc_listen_local_addr(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_kwsrpc_listen_native_fd(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static int klua_kwsrpc_listen_co_accept(lua_State* L)
{
    klua_kwsrpc_listen_t* p_listen = to_klua_kwsrpc_listen(L, 1);
    klua_check_coroutine(L, "kwsrpc.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        if (NULL == new_klua_kwsserverpc_by_socket(L, p_listen, p_listen_socket))
        {
            lua_pushnil(L);
        }
        KLB_FREE(p_listen_socket);
        return 1;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_kwsrpc_listen, p_listen);
}

void klua_kwsserverpc_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kwsrpc_listen_close },
        { "co_accept",      klua_kwsrpc_listen_co_accept },
        { "port",           klua_kwsrpc_listen_port },
        { "local_addr",     klua_kwsrpc_listen_local_addr },
        { "native_fd",      klua_kwsrpc_listen_native_fd },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kwsrpc_listen_close },
        { "__close",        klua_kwsrpc_listen_close },
        { "__tostring",     klua_kwsrpc_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KWSRPC_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int klua_kws_listen_rpc(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);
    bool tls = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    char path[256];
    char protocol[128];
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    int rc = 0;

    open_opts.family = AF_INET;
    KLB_MEMSET(path, 0, sizeof(path));
    KLB_MEMSET(protocol, 0, sizeof(protocol));
    memcpy(path, "/rpc", sizeof("/rpc"));

    if (klua_is_table(L, 2))
    {
        lua_getfield(L, 2, "tls");
        tls = klua_check_option_boolean(L, -1, false);
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

        lua_getfield(L, 2, "path");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_path = lua_tolstring(L, -1, &n);
            if (n >= sizeof(path) || text_has_ctl_klb_wsrpc(p_path, n))
            {
                lua_pop(L, 1);
                lua_pushnil(L);
                return 1;
            }

            if (0 < n)
            {
                memcpy(path, p_path, n + 1);
            }
            else
            {
                path[0] = '\0';
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "protocol");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_proto = lua_tolstring(L, -1, &n);
            if (n >= sizeof(protocol) || (0 < n && text_has_ctl_klb_wsrpc(p_proto, n)))
            {
                lua_pop(L, 1);
                lua_pushnil(L);
                return 1;
            }

            if (0 < n)
            {
                memcpy(protocol, p_proto, n + 1);
            }
        }
        lua_pop(L, 1);

        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_kwsrpc(L, rc);
        }
    }

    klb_socket_tls_server_ctx_t* p_tls_ctx = NULL;
    if (tls)
    {
        p_tls_ctx = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            return push_err_klua_kwsrpc(L, KLB_NETCODE_TLS_CERT);
        }
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_kwsrpc_listen_t* p_listen = NULL;

    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_kwsrpc(L, KLB_NETCODE_INVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_kwsrpc_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_kwsrpc(L, rc);
    }

    p_listen = new_klua_kwsrpc_listen(L);
    p_listen->L = L;
    p_listen->co_accept = NULL;
    p_listen->p_env = p_env;
    p_listen->p_coex = klua_coroutine_get(p_env);
    p_listen->p_netmulti = p_netmulti;
    p_listen->p_listen_conn = p_listen_conn;
    p_listen->tls = tls;
    p_listen->p_tls_ctx = p_tls_ctx;
    memcpy(p_listen->path, path, sizeof(p_listen->path));
    memcpy(p_listen->protocol, protocol, sizeof(p_listen->protocol));
    p_listen->p_socket_nlist = klb_nlist_create();
    klb_netconn_set_udata(p_listen_conn, p_listen);
    return 1;
}

// end
