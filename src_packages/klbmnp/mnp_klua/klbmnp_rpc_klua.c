// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmnp/mnp_klua/klbmnp_rpc_klua.h"
#include "klbmnp/core/klb_mnpclientrpc_conn.h"
#include "klbmnp/core/klb_mnpserverpc_conn.h"
#include "klbmnp/core/klb_mnprpcer.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include "klbplatform/klb_time.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// kmnp client rpc

#define KLUA_KMNPCLIENTRPC_HANDLE           "KMNPCLIENTRPC_HANDLE*"


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kmnprpc(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @struct klua_kmnprpc_waiter_t
/// @brief  单个 in-flight 请求的等待协程
typedef struct klua_kmnprpc_waiter_t_
{
    int                     sequence;       ///< 请求序号
    lua_State*              co;             ///< 等待协程
}klua_kmnprpc_waiter_t;


/// @struct klua_kmnpclientrpc_t
/// @brief  MNP-RPC 客户端
typedef struct klua_kmnpclientrpc_t_
{
    struct
    {
        lua_State*              L;              ///< (只读)L
        klua_env_t*             p_env;          ///< (只读)lua环境
        klua_ex_coroutine_t*    p_coex;         ///< (只读)Lua协程扩展
        klb_netmulti_t*         p_netmulti;     ///< (只读)复用
    };

    struct
    {
        lua_State*              co_recv;        ///< co_recv协程
        klb_nlist_t*            p_call_nlist;   ///< in-flight co_call; 存储 klua_kmnprpc_waiter_t*
        lua_State*              co_wait;        ///< co_wait协程
    };

    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< POST/NOTIFY; 存储 klb_buf_t*
        klb_mnp_rpc_t           rpc;            ///< 上一个 RPC
        klb_buf_t*              p_rpc_buf;      ///< RPC数据
    };

    struct
    {
        int                     req_sequence;   ///< 请求序号
        klb_nlist_t*            p_res_nlist;    ///< RESPONSE; 存储 klb_buf_t*
        klb_mnp_rpc_t           res;            ///< 上一个回应
        klb_buf_t*              p_res_buf;      ///< 回应数据
    };

    struct
    {
        klb_netconn_t*          p_client_conn;  ///< RPC 客户端连接
    };
}klua_kmnpclientrpc_t;


static klua_kmnpclientrpc_t* new_klua_kmnpclientrpc(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = (klua_kmnpclientrpc_t*)lua_newuserdata(L, sizeof(klua_kmnpclientrpc_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_kmnpclientrpc_t));
    luaL_setmetatable(L, KLUA_KMNPCLIENTRPC_HANDLE);

    p_client->L = L;
    p_client->p_env = klua_env_get_by_L(L);
    p_client->p_coex = klua_coroutine_get(p_client->p_env);
    p_client->p_netmulti = klua_netmulti_get(p_client->p_env);

    p_client->co_recv = NULL;
    p_client->p_call_nlist = klb_nlist_create();
    p_client->co_wait = NULL;
    p_client->p_rpc_nlist = klb_nlist_create();
    p_client->p_res_nlist = klb_nlist_create();
    p_client->req_sequence = 99;

    return p_client;
}

static klua_kmnpclientrpc_t* to_klua_kmnpclientrpc(lua_State* L, int index)
{
    klua_kmnpclientrpc_t* p_client = (klua_kmnpclientrpc_t*)luaL_checkudata(L, index, KLUA_KMNPCLIENTRPC_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'kmnpclientrpc' expected");
    return p_client;
}

static int klua_kmnpclientrpc_tostring(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);
    lua_pushfstring(L, "kmnpclientrpc:%p", p_client);
    return 1;
}

static void clear_buf_nlist_klua_kmnpclientrpc(klb_nlist_t* p_buf_nlist)
{
    if (NULL == p_buf_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_buf_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_buf_nlist);
        KLB_FREE_BY(p_buf, klb_buf_unref);
    }
}

static void clear_waiter_nlist_klua_kmnpclientrpc(klb_nlist_t* p_call_nlist)
{
    if (NULL == p_call_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_call_nlist))
    {
        klua_kmnprpc_waiter_t* p_wait = (klua_kmnprpc_waiter_t*)klb_nlist_pop_head(p_call_nlist);
        KLB_FREE(p_wait);
    }
}

static int klua_kmnpclientrpc_close(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);

    if (NULL != p_client->p_client_conn)
    {
        // 连接为延迟关闭: 先解绑回调与 udata, 避免关闭后回调访问已释放的 nlist
        klb_netconn_bind_recv_data(p_client->p_client_conn, NULL);
        klb_netconn_set_udata(p_client->p_client_conn, NULL);
    }

    KLB_FREE_BY(p_client->p_client_conn, klb_mnpclientrpc_conn_free);

    // 清协程挂起点与列表: 关闭后不再唤醒等待者, 且防回调重入访问已释放列表
    p_client->co_recv = NULL;
    p_client->co_wait = NULL;

    clear_buf_nlist_klua_kmnpclientrpc(p_client->p_rpc_nlist);
    clear_buf_nlist_klua_kmnpclientrpc(p_client->p_res_nlist);
    clear_waiter_nlist_klua_kmnpclientrpc(p_client->p_call_nlist);

    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_rpc_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_call_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_res_nlist, klb_nlist_destroy);

    return 0;
}

static void update_rpc_klua_kmnpclientrpc(klua_kmnpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_rpc_buf = p_buf;
    }
}

static int push_rpc_body_klua_kmnpclientrpc(lua_State* L, klb_buf_t* p_buf)
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

static int call_co_recv_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    lua_State* L = NULL;
    klb_buf_t* p_buf = NULL;
    int num = 0;
    int status = 0;

    assert(NULL != p_client);

    if (NULL == p_client->co_recv || NULL == p_client->p_rpc_nlist || 0 >= klb_nlist_size(p_client->p_rpc_nlist))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_recv = NULL;
    p_buf = klb_nlist_pop_head(p_client->p_rpc_nlist);
    num = push_rpc_body_klua_kmnpclientrpc(L, p_buf);
    update_rpc_klua_kmnpclientrpc(p_client, p_buf);

    status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int call_co_recv_end_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    lua_State* L = NULL;
    int status = 0;

    assert(NULL != p_client);

    if (NULL == p_client->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_recv = NULL;
    lua_pushnil(L);

    status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static void update_res_klua_kmnpclientrpc(klua_kmnpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->res, p_rpc, sizeof(klb_mnp_rpc_t));
        p_client->p_res_buf = p_buf;
    }
}

/// @brief 按序号取出等待者 (移除并返回)
static klua_kmnprpc_waiter_t* take_waiter_klua_kmnpclientrpc(klua_kmnpclientrpc_t* p_client, int sequence)
{
    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_client->p_call_nlist);
    while (NULL != p_iter)
    {
        klua_kmnprpc_waiter_t* p_wait = (klua_kmnprpc_waiter_t*)klb_nlist_data(p_iter);
        if (NULL != p_wait && sequence == p_wait->sequence)
        {
            return (klua_kmnprpc_waiter_t*)klb_nlist_remove(p_client->p_call_nlist, p_iter);
        }

        p_iter = klb_nlist_next(p_iter);
    }

    return NULL;
}


static bool has_waiter_klua_kmnpclientrpc(klua_kmnpclientrpc_t* p_client, int sequence)
{
    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_client->p_call_nlist);
    while (NULL != p_iter)
    {
        klua_kmnprpc_waiter_t* p_wait = (klua_kmnprpc_waiter_t*)klb_nlist_data(p_iter);
        if (NULL != p_wait && sequence == p_wait->sequence)
        {
            return true;
        }

        p_iter = klb_nlist_next(p_iter);
    }

    return false;
}


static int call_co_res_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    lua_State* L = NULL;
    klb_buf_t* p_buf = NULL;
    klua_kmnprpc_waiter_t* p_wait = NULL;
    int num = 0;
    int status = 0;

    assert(NULL != p_client);

    while (NULL != p_client->p_res_nlist && 0 < klb_nlist_size(p_client->p_res_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_head(p_client->p_res_nlist);
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_tmp->p_buf + p_tmp->start);
        p_wait = NULL;

        if (KLB_MNP_RPC_RESPONSE == p_rpc->method)
        {
            p_wait = take_waiter_klua_kmnpclientrpc(p_client, (int)p_rpc->sequence);
        }

        if (NULL == p_wait)
        {
            // 无等待者: 丢弃过期/未知回应
            klb_nlist_pop_head(p_client->p_res_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
            continue;
        }

        lua_State* p_co = p_wait->co;
        KLB_FREE(p_wait);
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_co));

        L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_co);
        if (NULL == L)
        {
            klb_nlist_pop_head(p_client->p_res_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
            continue;
        }

        p_buf = klb_nlist_pop_head(p_client->p_res_nlist);
        num = push_rpc_body_klua_kmnpclientrpc(L, p_buf);
        update_res_klua_kmnpclientrpc(p_client, p_buf);

        status = lua_pcall(L, num, 0, 0);
        klua_env_report_by_L(L, status);

        if (LUA_OK == status)
        {
            return 0;
        }

        return 1;
    }

    return -1;
}


static int call_co_res_end_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    lua_State* L = NULL;
    int status = 0;
    int ret = -1;

    assert(NULL != p_client);

    while (NULL != p_client->p_call_nlist && 0 < klb_nlist_size(p_client->p_call_nlist))
    {
        klua_kmnprpc_waiter_t* p_wait = (klua_kmnprpc_waiter_t*)klb_nlist_pop_head(p_client->p_call_nlist);

        lua_State* p_co = p_wait->co;
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_co));

        L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_co);
        if (NULL != L)
        {
            lua_pushnil(L);

            status = lua_pcall(L, 1, 0, 0);
            klua_env_report_by_L(L, status);

            ret = (LUA_OK == status) ? 0 : 1;
        }

        KLB_FREE(p_wait);
    }

    return ret;
}

static int call_co_wait_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    lua_State* L = NULL;
    int status = 0;

    assert(NULL != p_client);

    if (NULL == p_client->co_wait)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_wait));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_wait);
    if (NULL == L)
    {
        return -1;
    }

    p_client->co_wait = NULL;
    lua_pushnil(L);

    status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int on_recv_data_klua_kmnpclientrpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kmnpclientrpc_t* p_client = (klua_kmnpclientrpc_t*)p_conn->p_udata;

    if (NULL == p_client)
    {
        // 客户端已关闭 (close 已解绑 udata): 丢弃残留应收数据
        if (NULL != p_data)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }

        return 0;
    }

    if (KLB_NETCODE_OK == code)
    {
        bool need_free = true;

        if (KLB_MNP_RPC_LUA == packtype || KLB_MNP_RPC_JSON == packtype)
        {
            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_data->p_buf + p_data->start);

            if (KLB_MNP_RPC_POST == p_rpc->method || KLB_MNP_RPC_NOTIFY == p_rpc->method)
            {
                need_free = false;
                klb_nlist_push_tail(p_client->p_rpc_nlist, p_data);
                call_co_recv_kmnpclientrpc(p_client);
            }
            else if (KLB_MNP_RPC_RESPONSE == p_rpc->method && has_waiter_klua_kmnpclientrpc(p_client, (int)p_rpc->sequence))
            {
                need_free = false;
                klb_nlist_push_tail(p_client->p_res_nlist, p_data);
                call_co_res_kmnpclientrpc(p_client);
            }
        }

        if (need_free)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_NETCODE_WBUF_EMPTY == code)
    {
        call_co_wait_kmnpclientrpc(p_client);
    }
    else if (KLB_SOCKET_CONNECT != code)
    {
        call_co_recv_end_kmnpclientrpc(p_client);
        call_co_res_end_kmnpclientrpc(p_client);
        call_co_wait_kmnpclientrpc(p_client);
    }

    return 0;
}

static int get_next_sequence_klua_kmnpclientrpc(klua_kmnpclientrpc_t* p_client)
{
    int sequence = p_client->req_sequence + 1;
    int guard = 0;

    if (1000 > sequence || 1000000 < sequence)
    {
        sequence = 1000;
    }

    // 序号回绕时跳过仍在 in-flight 的请求, 避免复用
    while (guard < 1000000 && has_waiter_klua_kmnpclientrpc(p_client, sequence))
    {
        ++sequence;
        if (1000000 < sequence)
        {
            sequence = 1000;
        }

        ++guard;
    }

    return sequence;
}

static int klua_kmnpclientrpc_send(lua_State* L, klb_mnp_rpc_method_e method, klua_kmnpclientrpc_t** p_out, int* p_sequence)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);
    int rpctype = (int)luaL_checkinteger(L, 2);
    int sequence = 0;
    int ret = 1;

    if (KLB_MNP_RPC_REQUEST == method)
    {
        sequence = get_next_sequence_klua_kmnpclientrpc(p_client);
    }

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
        return ret;
    }

    if (KLB_MNP_RPC_LUA == rpctype)
    {
        klb_buf_t* p_data = klua_seri_map_binary_pack(L, 2);

        ret = klb_mnpclientrpc_conn_send_buf(p_client->p_client_conn, KLB_MNP_RPC_LUA, method, sequence, p_data);
        if (0 != ret)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_MNP_RPC_JSON == rpctype)
    {
        size_t json_len = 0;
        const char* p_json = luaL_checklstring(L, 3, &json_len);

        ret = klb_mnpclientrpc_conn_send(p_client->p_client_conn, KLB_MNP_RPC_JSON, method, sequence, (const uint8_t*)p_json, (int)json_len);
    }

    return ret;
}

static int klua_kmnpclientrpc_post(lua_State* L)
{
    int ret = klua_kmnpclientrpc_send(L, KLB_MNP_RPC_POST, NULL, NULL);

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kmnpclientrpc_notify(lua_State* L)
{
    int ret = klua_kmnpclientrpc_send(L, KLB_MNP_RPC_NOTIFY, NULL, NULL);

    lua_pushinteger(L, ret);
    return 1;
}

static int on_yield_recv_klua_kmnpclientrpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kmnpclientrpc_t* p_client = (klua_kmnpclientrpc_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_kmnpclientrpc(p_client);
        call_co_res_end_kmnpclientrpc(p_client);
        call_co_wait_kmnpclientrpc(p_client);
    }

    return 0;
}

static int klua_kmnpclientrpc_co_recv(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);

    klua_check_coroutine(L, "kmnpclientrpc:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    if (0 < klb_nlist_size(p_client->p_rpc_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_client->p_rpc_nlist);
        int num = push_rpc_body_klua_kmnpclientrpc(L, p_buf);

        update_rpc_klua_kmnpclientrpc(p_client, p_buf);
        return num;
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_kmnpclientrpc, p_client);
}

static int klua_kmnpclientrpc_co_call(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = NULL;
    int sequence = 0;
    int ret = 0;
    int timeout_ms = 0;
    int top = 0;

    klua_check_coroutine(L, "kmnpclientrpc:co_call must in coroutine!");

    top = lua_gettop(L);
    if (2 <= top && LUA_TNUMBER == lua_type(L, top))
    {
        timeout_ms = (int)lua_tointeger(L, top);
        lua_settop(L, top - 1);
    }

    ret = klua_kmnpclientrpc_send(L, KLB_MNP_RPC_REQUEST, &p_client, &sequence);
    if (0 == ret)
    {
        klua_kmnprpc_waiter_t* p_wait = KLB_MALLOCZ(klua_kmnprpc_waiter_t, 1, 0);

        p_wait->sequence = sequence;
        p_wait->co = L;
        p_client->req_sequence = sequence;
        klb_nlist_push_tail(p_client->p_call_nlist, p_wait);
        if (0 < timeout_ms && NULL != p_client->p_client_conn)
        {
            klb_mnpclientrpc_conn_set_op_deadline(p_client->p_client_conn,
                klb_tick_counti64() + (int64_t)timeout_ms);
        }
        else if (NULL != p_client->p_client_conn)
        {
            klb_mnpclientrpc_conn_set_op_deadline(p_client->p_client_conn, 0);
        }
        return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_kmnpclientrpc, p_client);
    }

    lua_pushnil(L);
    return 1;
}

static int klua_kmnpclientrpc_co_wait(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);

    klua_check_coroutine(L, "kmnpclientrpc:co_wait must in coroutine!");
    assert(NULL == p_client->co_wait);

    if (NULL == p_client->p_client_conn || klb_mnpclientrpc_wbuf_is_empty(p_client->p_client_conn))
    {
        lua_pushnil(L);
        return 1;
    }

    p_client->co_wait = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_kmnpclientrpc, p_client);
}

static int klua_kmnpclientrpc_status(lua_State* L)
{
    klua_kmnpclientrpc_t* p_client = to_klua_kmnpclientrpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_mnprpcer_to_rpctype_string(p_client->rpc.rpctype));
    klua_setfield_string(L, "method", klb_mnprpcer_to_method_string((klb_mnp_rpc_method_e)p_client->rpc.method));
    klua_setfield_integer(L, "sequence", p_client->rpc.sequence);
    klua_setfield_string(L, "res_rpctype", klb_mnprpcer_to_rpctype_string(p_client->res.rpctype));
    klua_setfield_string(L, "res_method", klb_mnprpcer_to_method_string((klb_mnp_rpc_method_e)p_client->res.method));
    klua_setfield_integer(L, "res_sequence", p_client->res.sequence);

    return 1;
}

void klua_kmnpclientrpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kmnpclientrpc_close },
        { "post",           klua_kmnpclientrpc_post },
        { "notify",         klua_kmnpclientrpc_notify },
        { "co_recv",        klua_kmnpclientrpc_co_recv },
        { "co_call",        klua_kmnpclientrpc_co_call },
        { "co_wait",        klua_kmnpclientrpc_co_wait },
        { "status",         klua_kmnpclientrpc_status },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmnpclientrpc_close },
        { "__close",        klua_kmnpclientrpc_close },
        { "__tostring",     klua_kmnpclientrpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMNPCLIENTRPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int klua_kmnpclient_connect_rpc(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    int code = 0;
    bool tls = false;
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_client_conn = NULL;
    klua_kmnpclientrpc_t* p_client = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_kmnprpc(L, KLB_NETCODE_EINVAL);
    }

    p_client_conn = klb_mnpclientrpc_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_client_conn)
    {
        return push_err_klua_kmnprpc(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    p_client = new_klua_kmnpclientrpc(L);
    klb_netconn_set_udata(p_client_conn, p_client);
    klb_netconn_bind_recv_data(p_client_conn, on_recv_data_klua_kmnpclientrpc);
    p_client->p_client_conn = p_client_conn;

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// kmnp serve rpc

#define KLUA_KMNPSERVERPC_HANDLE            "KMNPSERVERPC_HANDLE*"


/// @struct klua_kmnpserverpc_t
/// @brief  MNP 服务 RPC 连接
typedef struct klua_kmnpserverpc_t_
{
    struct
    {
        lua_State*              L;              ///< (只读)L
        klua_env_t*             p_env;          ///< (只读)lua环境
        klua_ex_coroutine_t*    p_coex;         ///< (只读)Lua协程扩展
        klb_netmulti_t*         p_netmulti;     ///< (只读)复用
    };

    struct
    {
        lua_State*              co_recv;        ///< co_recv协程
    };

    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< 待读取 RPC; 存储 klb_buf_t*
        klb_mnp_rpc_t           rpc;            ///< 上一个 RPC
        klb_buf_t*              p_rpc_buf;      ///< RPC数据
    };

    struct
    {
        klb_netconn_t*          p_serve_conn;   ///< RPC 服务连接
        char                    peer[128];      ///< 对端 "ip:port"; 未知为空串
    };
}klua_kmnpserverpc_t;


static klua_kmnpserverpc_t* new_klua_kmnpserverpc(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = (klua_kmnpserverpc_t*)lua_newuserdata(L, sizeof(klua_kmnpserverpc_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_kmnpserverpc_t));
    luaL_setmetatable(L, KLUA_KMNPSERVERPC_HANDLE);

    p_serve->L = L;
    p_serve->p_env = klua_env_get_by_L(L);
    p_serve->p_coex = klua_coroutine_get(p_serve->p_env);
    p_serve->p_netmulti = klua_netmulti_get(p_serve->p_env);
    p_serve->co_recv = NULL;
    p_serve->p_rpc_nlist = klb_nlist_create();

    return p_serve;
}

static klua_kmnpserverpc_t* to_klua_kmnpserverpc(lua_State* L, int index)
{
    klua_kmnpserverpc_t* p_serve = (klua_kmnpserverpc_t*)luaL_checkudata(L, index, KLUA_KMNPSERVERPC_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'kmnpserverpc' expected");
    return p_serve;
}

static int klua_kmnpserverpc_tostring(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);

    lua_pushfstring(L, "kmnpserverpc:%p", p_serve);
    return 1;
}

static int klua_kmnpserverpc_close(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);

    KLB_FREE_BY(p_serve->p_serve_conn, klb_mnpserverpc_conn_free);

    if (NULL != p_serve->p_rpc_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_rpc_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_rpc_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_serve->p_rpc_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    return 0;
}

static void update_rpc_klua_kmnpserverpc(klua_kmnpserverpc_t* p_serve, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_serve->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_serve->rpc, p_rpc, sizeof(klb_mnp_rpc_t));
        p_serve->p_rpc_buf = p_buf;
    }
}

static int push_rpc_body_klua_kmnpserverpc(lua_State* L, klb_buf_t* p_buf)
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

static int call_co_recv_klua_kmnpserverpc(klua_kmnpserverpc_t* p_serve)
{
    lua_State* L = NULL;
    klb_buf_t* p_buf = NULL;
    int num = 0;
    int status = 0;

    assert(NULL != p_serve);

    if (NULL == p_serve->co_recv || 0 >= klb_nlist_size(p_serve->p_rpc_nlist))
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_serve->co_recv = NULL;
    p_buf = klb_nlist_pop_head(p_serve->p_rpc_nlist);
    num = push_rpc_body_klua_kmnpserverpc(L, p_buf);
    update_rpc_klua_kmnpserverpc(p_serve, p_buf);

    status = lua_pcall(L, num, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int call_co_recv_end_klua_kmnpserverpc(klua_kmnpserverpc_t* p_serve)
{
    lua_State* L = NULL;
    int status = 0;

    assert(NULL != p_serve);

    if (NULL == p_serve->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

    L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_serve->co_recv = NULL;
    lua_pushnil(L);

    status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int on_recv_data_klua_kmnpserverpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kmnpserverpc_t* p_serve = (klua_kmnpserverpc_t*)p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        bool need_free = true;

        if (KLB_MNP_RPC_LUA == packtype || KLB_MNP_RPC_JSON == packtype)
        {
            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_data->p_buf + p_data->start);

            if (KLB_MNP_RPC_POST == p_rpc->method ||
                KLB_MNP_RPC_NOTIFY == p_rpc->method ||
                KLB_MNP_RPC_REQUEST == p_rpc->method)
            {
                need_free = false;
                klb_nlist_push_tail(p_serve->p_rpc_nlist, p_data);
                call_co_recv_klua_kmnpserverpc(p_serve);
            }
        }

        if (need_free)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_SOCKET_CONNECT != code && KLB_NETCODE_WBUF_EMPTY != code)
    {
        call_co_recv_end_klua_kmnpserverpc(p_serve);
    }

    return 0;
}

static int klua_kmnpserverpc_send(lua_State* L, klb_mnp_rpc_method_e method)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);
    int rpctype = (int)luaL_checkinteger(L, 2);
    int sequence = 0;
    int base_idx = 2;
    int ret = 1;

    if (KLB_MNP_RPC_RESPONSE == method)
    {
        sequence = (int)luaL_checkinteger(L, base_idx + 1);
        base_idx += 1;
    }

    if (NULL == p_serve->p_serve_conn)
    {
        lua_pushinteger(L, ret);
        return 1;
    }

    if (KLB_MNP_RPC_LUA == rpctype)
    {
        klb_buf_t* p_data = klua_seri_map_binary_pack(L, base_idx);

        ret = klb_mnpserverpc_conn_send_buf(p_serve->p_serve_conn, KLB_MNP_RPC_LUA, method, sequence, p_data);
        if (0 != ret)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else if (KLB_MNP_RPC_JSON == rpctype)
    {
        size_t json_len = 0;
        const char* p_json = luaL_checklstring(L, base_idx + 1, &json_len);

        ret = klb_mnpserverpc_conn_send(p_serve->p_serve_conn, KLB_MNP_RPC_JSON, method, sequence, (const uint8_t*)p_json, (int)json_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kmnpserverpc_post(lua_State* L)
{
    return klua_kmnpserverpc_send(L, KLB_MNP_RPC_POST);
}

static int klua_kmnpserverpc_notify(lua_State* L)
{
    return klua_kmnpserverpc_send(L, KLB_MNP_RPC_NOTIFY);
}

static int klua_kmnpserverpc_response(lua_State* L)
{
    return klua_kmnpserverpc_send(L, KLB_MNP_RPC_RESPONSE);
}

static int on_yield_recv_klua_kmnpserverpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kmnpserverpc_t* p_serve = (klua_kmnpserverpc_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_kmnpserverpc(p_serve);
    }

    return 0;
}

static int klua_kmnpserverpc_co_recv(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);

    klua_check_coroutine(L, "kmnpserverpc:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (0 < klb_nlist_size(p_serve->p_rpc_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_serve->p_rpc_nlist);
        int num = push_rpc_body_klua_kmnpserverpc(L, p_buf);

        update_rpc_klua_kmnpserverpc(p_serve, p_buf);
        return num;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_kmnpserverpc, p_serve);
}

static int klua_kmnpserverpc_status(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);

    lua_newtable(L);
    klua_setfield_string(L, "rpctype", klb_mnprpcer_to_rpctype_string(p_serve->rpc.rpctype));
    klua_setfield_string(L, "method", klb_mnprpcer_to_method_string((klb_mnp_rpc_method_e)p_serve->rpc.method));
    klua_setfield_integer(L, "sequence", p_serve->rpc.sequence);

    return 1;
}

static int klua_kmnpserverpc_peer(lua_State* L)
{
    klua_kmnpserverpc_t* p_serve = to_klua_kmnpserverpc(L, 1);

    lua_pushstring(L, p_serve->peer);
    return 1;
}

void klua_kmnpserverpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kmnpserverpc_close },
        { "post",           klua_kmnpserverpc_post },
        { "notify",         klua_kmnpserverpc_notify },
        { "response",       klua_kmnpserverpc_response },
        { "co_recv",        klua_kmnpserverpc_co_recv },
        { "status",         klua_kmnpserverpc_status },
        { "peer",           klua_kmnpserverpc_peer },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmnpserverpc_close },
        { "__close",        klua_kmnpserverpc_close },
        { "__tostring",     klua_kmnpserverpc_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMNPSERVERPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static klua_kmnpserverpc_t* new_klua_kmnpserverpc_by_klb_socket(lua_State* L, klb_socket_t* p_socket)
{
    klua_kmnpserverpc_t* p_serve = new_klua_kmnpserverpc(L);
    klb_netconn_t* p_serve_conn = NULL;

    assert(NULL != p_socket);

    p_serve_conn = klb_mnpserverpc_conn_create(p_serve->p_netmulti, p_socket);
    klb_netconn_set_udata(p_serve_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve_conn, on_recv_data_klua_kmnpserverpc);
    klb_netmulti_push(p_serve->p_netmulti, p_serve_conn);
    p_serve->p_serve_conn = p_serve_conn;

    return p_serve;
}


static klua_kmnpserverpc_t* new_klua_kmnpserverpc_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_kmnpserverpc_t* p_serve = new_klua_kmnpserverpc_by_klb_socket(L, p_socket);

    klua_netlisten_format_peer_in(p_serve->peer, (int)sizeof(p_serve->peer), &p_listen_socket->addr);
    return p_serve;
}


//////////////////////////////////////////////////////////////////////////
// kmnp serve rpc listen

#define KLUA_KMNPSERVERPC_LISTEN_HANDLE     "KMNPSERVERPC_LISTEN_HANDLE*"


/// @struct klua_kmnpserverpc_listen_t
/// @brief  MNP 服务 RPC 监听
typedef struct klua_kmnpserverpc_listen_t_
{
    struct
    {
        lua_State*              L;              ///< (只读)L
        klua_env_t*             p_env;          ///< (只读)Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< (只读)Lua协程扩展
        klb_netmulti_t*         p_netmulti;     ///< (只读)复用
    };

    struct
    {
        lua_State*              co_accept;      ///< co_accept协程
        klb_nlist_t*            p_socket_nlist; ///< 待读取 socket; 存储 klb_netlisten_socket_t*
    };

    struct
    {
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };
}klua_kmnpserverpc_listen_t;


static klua_kmnpserverpc_listen_t* new_klua_kmnpserverpc_listen(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = (klua_kmnpserverpc_listen_t*)lua_newuserdata(L, sizeof(klua_kmnpserverpc_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_kmnpserverpc_listen_t));
    luaL_setmetatable(L, KLUA_KMNPSERVERPC_LISTEN_HANDLE);

    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_coex = klua_coroutine_get(p_listen->p_env);
    p_listen->p_netmulti = klua_netmulti_get(p_listen->p_env);
    p_listen->co_accept = NULL;
    p_listen->p_socket_nlist = klb_nlist_create();

    return p_listen;
}

static klua_kmnpserverpc_listen_t* to_klua_kmnpserverpc_listen(lua_State* L, int index)
{
    klua_kmnpserverpc_listen_t* p_listen = (klua_kmnpserverpc_listen_t*)luaL_checkudata(L, index, KLUA_KMNPSERVERPC_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'kmnpserverpc.listen' expected");
    return p_listen;
}

static int klua_kmnpserverpc_listen_tostring(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);
    int port = 0;

    if (NULL != p_listen->p_listen_conn)
    {
        port = klb_netlisten_conn_get_port(p_listen->p_listen_conn);
    }

    lua_pushfstring(L, "kmnpserverpc.listen:[%p],port:[%d]", p_listen, port);
    return 1;
}

static int klua_kmnpserverpc_listen_close(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);

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

static int call_co_accept_klua_kmnpserverpc_listen(klua_kmnpserverpc_listen_t* p_listen)
{
    lua_State* L = NULL;
    klb_netlisten_socket_t* p_listen_socket = NULL;
    int status = 0;

    assert(NULL != p_listen);

    if (NULL == p_listen->co_accept || 0 >= klb_nlist_size(p_listen->p_socket_nlist))
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
    p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
    new_klua_kmnpserverpc_by_socket(L, p_listen_socket);
    KLB_FREE(p_listen_socket);

    status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int call_co_accept_end_klua_kmnpserverpc_listen(klua_kmnpserverpc_listen_t* p_listen)
{
    lua_State* L = NULL;
    int status = 0;

    assert(NULL != p_listen);

    if (NULL == p_listen->co_accept)
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
    lua_pushlightuserdata(L, NULL);

    status = lua_pcall(L, 1, 0, 0);
    klua_env_report_by_L(L, status);

    if (LUA_OK == status)
    {
        return 0;
    }

    return 1;
}

static int on_accept_klua_kmnpserverpc_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_kmnpserverpc_listen_t* p_listen = (klua_kmnpserverpc_listen_t*)p_conn->p_udata;
    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);

    p_tmp->fd = fd;
    memcpy(&p_tmp->addr, p_addr, sizeof(struct sockaddr_in));
    p_tmp->tls = tls;

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);
    call_co_accept_klua_kmnpserverpc_listen(p_listen);

    return 0;
}

static int on_yield_accept_klua_kmnpserverpc_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kmnpserverpc_listen_t* p_listen = (klua_kmnpserverpc_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_kmnpserverpc_listen(p_listen);
    }

    return 0;
}

static int klua_kmnpserverpc_listen_co_accept(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);

    klua_check_coroutine(L, "kmnpserverpc.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        new_klua_kmnpserverpc_by_socket(L, p_listen_socket);
        KLB_FREE(p_listen_socket);
        return 1;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_kmnpserverpc_listen, p_listen);
}

static int klua_kmnpserverpc_listen_port(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_kmnpserverpc_listen_local_addr(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_kmnpserverpc_listen_native_fd(lua_State* L)
{
    klua_kmnpserverpc_listen_t* p_listen = to_klua_kmnpserverpc_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

void klua_kmnpserverpc_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kmnpserverpc_listen_close },
        { "co_accept",      klua_kmnpserverpc_listen_co_accept },
        { "port",           klua_kmnpserverpc_listen_port },
        { "local_addr",     klua_kmnpserverpc_listen_local_addr },
        { "native_fd",      klua_kmnpserverpc_listen_native_fd },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmnpserverpc_listen_close },
        { "__close",        klua_kmnpserverpc_listen_close },
        { "__tostring",     klua_kmnpserverpc_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMNPSERVERPC_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

int klua_kmnpserverpc_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_kmnpserverpc_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_kmnprpc(L, rc);
        }
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_kmnprpc(L, KLB_NETCODE_EINVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return push_err_klua_kmnprpc(L, rc);
    }

    p_listen = new_klua_kmnpserverpc_listen(L);
    klb_netconn_set_udata(p_listen_conn, p_listen);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_kmnpserverpc_listen, p_listen);
    p_listen->p_listen_conn = p_listen_conn;

    return 1;
}


/// @brief 由已探测 sock 创建 MNP-RPC 服务连接; 可选 `{cert,key}` 供 TLS(mnprpcs)
int klua_kmnpserverpc_new_serve(lua_State* L)
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
        return push_err_klua_kmnprpc(L, KLB_NETCODE_EINVAL);
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

    struct sockaddr_in peer_addr = p_listen_socket->addr;
    klua_kmnpserverpc_t* p_serve = NULL;

    fd = p_listen_socket->fd;
    KLB_FREE(p_listen_socket);

    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(
            p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnprpc(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnprpc(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnprpc(L, KLB_NETCODE_ENOMEM);
        }
    }

    p_serve = new_klua_kmnpserverpc_by_klb_socket(L, p_socket);
    klua_netlisten_format_peer_in(p_serve->peer, (int)sizeof(p_serve->peer), &peer_addr);
    return 1;
}

// end
