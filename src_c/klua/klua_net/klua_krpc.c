// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_log.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbnet/klb_nsp.h"
#include "klbnet/klb_ncm.h"
#include "klbnet/klb_nsc.h"
#include "klua/klua_util/klua_seri_json.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klbbase/klb_mnp.h"
#include "klbthird/cJSON.h"
#include <assert.h>


#define KLUA_KRPC_sequence_min          1000
#define KLUA_KRPC_sequence_max          0x7FFF0000


#define KLUA_KRPC_sequence_post         1
#define KLUA_KRPC_sequence_notify       2


typedef struct klua_krpc_data_t_
{
    int         protocol;
    int         id;
    int         packtype;

    klb_buf_t*  p_buf;
    cJSON*      p_json;
}klua_krpc_data_t;


//////////////////////////////////////////////////////////////////////////
// rpc lua
#define KLUA_KRPC_HANDLE                "KLUA_KRPC_HANDLE*"     ///< Lua meta标示


typedef struct klua_krpc_t_ klua_krpc_t;


// 参数
typedef struct klua_krpc_param_t_
{
    int     protocol;       ///< 协议: klb_protocol_e
    int     rbuf_max;       ///< 临时读取缓存最大值
    int     read_max;       ///< 缓存数据最大值后,暂停读取
    bool    tls;            ///< TLS
}klua_krpc_param_t;


typedef struct klua_krpc_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_recv;        ///< co_recv协程
        lua_State*              co_recv_notify; ///< co_recv_notify协程
    };

    struct
    {
        klua_ex_coroutine_t*    p_coroutine;    ///< 协程扩展
        klb_multiplex_t*        p_multi;        ///< 复用
        klb_nsc_t*              p_nsc;          ///< 单个网络连接(客户端)
        int                     protocol;       ///< 协议类型: klb_protocol_e
    };

    struct  
    {
        uint32_t                sequence;       ///< 当前call的序列号

        bool                    b_notify;       ///< 是否收取 "notify" 信息
        int32_t                 notify_num;     ///< 缓存的数据大小
        klb_nlist_t*            p_notify_list;  ///< 缓存的数据列表: klua_krpc_data_t*
    };

    uint32_t                    next_sequence;  ///< 下一个sequence序列号
}klua_krpc_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_co_recv_klua_krpc(klua_krpc_t* p_krpc, int id, uint32_t sequence, char* p_body, int body_len)
{
    if (NULL != p_krpc->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_krpc->p_env), p_krpc->co_recv);
        if (NULL == L) return -1; // 未处理

        p_krpc->co_recv = NULL; // 清空

        int num = (0 < body_len) ? luaseri_map_binary_unpack(L, 1, p_body, body_len) : 0;

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_co_recv_klua_krpc_json(klua_krpc_t* p_krpc, int id, uint32_t sequence, cJSON* p_json)
{
    if (NULL != p_krpc->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_krpc->p_env), p_krpc->co_recv);
        if (NULL == L) return -1; // 未处理

        p_krpc->co_recv = NULL; // 清空

        int num = luaseri_json_unpack(L, 1, p_json);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_co_recv_notify_klua_krpc(klua_krpc_t* p_krpc, int id, uint32_t sequence, char* p_body, int body_len)
{
    if (NULL != p_krpc->co_recv_notify)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_krpc->p_env), p_krpc->co_recv_notify);
        if (NULL == L) return -1; // 未处理

        p_krpc->co_recv_notify = NULL; // 清空

        int num = (0 < body_len) ? luaseri_map_binary_unpack(L, 1, p_body, body_len) : 0;

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_co_recv_notify_klua_krpc_json(klua_krpc_t* p_krpc, int id, uint32_t sequence, cJSON* p_json)
{
    if (NULL != p_krpc->co_recv_notify)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_krpc->p_env), p_krpc->co_recv_notify);
        if (NULL == L) return -1; // 未处理

        p_krpc->co_recv_notify = NULL; // 清空

        int num = luaseri_json_unpack(L, 1, p_json);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static uint32_t get_sequence_klua_krpc(klua_krpc_t* p_krpc)
{
    uint32_t seq = p_krpc->next_sequence;

    p_krpc->next_sequence++;
    if (KLUA_KRPC_sequence_max < p_krpc->next_sequence)
    {
        p_krpc->next_sequence = KLUA_KRPC_sequence_min;
    }

    assert(KLUA_KRPC_sequence_min <= seq);
    assert(seq <= KLUA_KRPC_sequence_max);
    return seq;
}

static void close_klua_krpc(klua_krpc_t* p_krpc)
{
    // 断开连接
    if (NULL != p_krpc->p_nsc)
    {
        klb_nsc_close(p_krpc->p_nsc);
    }

    // co_call 返回
    call_lua_co_recv_klua_krpc(p_krpc, 0, 0, NULL, 0);

    // co_recv_notify 返回
    call_lua_co_recv_notify_klua_krpc(p_krpc, 0, 0, NULL, 0);

    // 清空
    if (NULL != p_krpc->p_notify_list)
    {
        while (0 < klb_nlist_size(p_krpc->p_notify_list))
        {
            klua_krpc_data_t* p_tmp = (klua_krpc_data_t*)klb_nlist_pop_head(p_krpc->p_notify_list);
            KLB_FREE_BY(p_tmp->p_buf, klb_buf_unref_next);
            KLB_FREE_BY(p_tmp->p_json, cJSON_Delete);
            KLB_FREE(p_tmp);
        }
    }
}

static void push_notify_data_klua_krpc(klua_krpc_t* p_krpc, int protocol, int id, int packtype, klb_buf_t* p_buf, cJSON* p_json)
{
    if (!p_krpc->b_notify)
    {
        return;
    }

    if (NULL != p_buf)
    {
        klb_buf_ref_next(p_buf);
    }

    klua_krpc_data_t* p_data = KLB_MALLOCZ(klua_krpc_data_t, 1, 0);

    p_data->protocol = protocol;
    p_data->id = id;
    p_data->packtype = packtype;

    p_data->p_buf = p_buf;
    p_data->p_json = p_json;

    klb_nlist_push_tail(p_krpc->p_notify_list, p_data);
}

static int cb_recv_klua_krpc(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_buf)
{
    klua_krpc_t* p_krpc = (klua_krpc_t*)ptr;

    if (KLB_SOCKET_CONNECT == code)
    {
        // 连接成功
    }
    else if (KLB_SOCKET_OK == code)
    {
        char* p_data = p_buf->p_buf + p_buf->start;
        int data_len = p_buf->end - p_buf->start;

        klb_mnp_common_t* p_com = (klb_mnp_common_t*)p_data;

        char* ptr = p_data + sizeof(klb_mnp_common_t) + p_com->head;
        data_len = data_len - sizeof(klb_mnp_common_t) - p_com->head;
        assert(data_len == p_com->size - p_com->head - sizeof(klb_mnp_common_t));
        assert(0 == p_buf->p_buf[p_buf->end]); // 连接层需要将非媒体数据末尾补0, 以便于文本解析

        if (KLB_NCM_PACK_RPC_LUA == packtype)
        {
            if (KLUA_KRPC_sequence_min <= p_com->sequence)
            {
                if (p_krpc->sequence == p_com->sequence)
                {
                    p_krpc->sequence = 0;
                    call_lua_co_recv_klua_krpc(p_krpc, id, p_com->sequence, ptr + p_com->head, data_len - p_com->head);
                }
            }
            else
            {
                if (call_lua_co_recv_notify_klua_krpc(p_krpc, id, p_com->sequence, ptr + p_com->head, data_len - p_com->head) < 0)
                {
                    push_notify_data_klua_krpc(p_krpc, protocol, id, packtype, p_buf, NULL);
                }
            }
        }
        else if(KLB_NCM_PACK_RPC_JSON == packtype)
        {
            const char* p_ep = NULL;
            cJSON* p_root = cJSON_Parse(ptr, &p_ep);
            cJSON* p_sequence = cJSON_GetObjectItem(p_root, "sequence");
            cJSON* p_response = cJSON_GetObjectItem(p_root, "response");
            cJSON* p_notify = cJSON_GetObjectItem(p_root, "notify");
            uint32_t sequence = ((NULL != p_sequence) && (cJSON_Int == p_sequence->type)) ? (uint32_t)p_sequence->valueint : 0;

            if (KLUA_KRPC_sequence_min <= p_com->sequence)
            {
                if (p_krpc->sequence == p_com->sequence)
                {
                    p_krpc->sequence = 0;
                    call_lua_co_recv_klua_krpc_json(p_krpc, id, sequence, p_response);
                }

                KLB_FREE_BY(p_root, cJSON_Delete);
            }
            else
            {
                if (call_lua_co_recv_notify_klua_krpc_json(p_krpc, id, sequence, p_notify) < 0)
                {
                    push_notify_data_klua_krpc(p_krpc, protocol, id, packtype, NULL, p_root);
                }
                else
                {
                    KLB_FREE_BY(p_root, cJSON_Delete);
                }
            }   
        }
    }
    else if (KLB_SOCKET_CLOSEING == code)
    {
        // 主动断开连接, 啥也不做
    }
    else
    {
        // 断开连接
        close_klua_krpc(p_krpc);
    }

    return 0;
}

////////////////////////////////////////
static klua_krpc_t* new_klua_krpc(lua_State* L)
{
    klua_krpc_t* p_krpc = (klua_krpc_t*)lua_newuserdata(L, sizeof(klua_krpc_t));
    KLB_MEMSET(p_krpc, 0, sizeof(klua_krpc_t));
    luaL_setmetatable(L, KLUA_KRPC_HANDLE);
    return p_krpc;
}

static klua_krpc_t* to_klua_krpc(lua_State* L, int index)
{
    klua_krpc_t* p_krpc = (klua_krpc_t*)luaL_checkudata(L, index, KLUA_KRPC_HANDLE);
    luaL_argcheck(L, NULL != p_krpc, index, "'krpc' expected");
    return p_krpc;
}

static int klua_krpc_tostring(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);

    lua_pushfstring(L, "krpc:%p", p_krpc);
    return 1;
}

static int klua_krpc_close(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);

    // 关闭
    close_klua_krpc(p_krpc);

    // 释放
    KLB_FREE_BY(p_krpc->p_nsc, klb_nsc_destroy);
    KLB_FREE_BY(p_krpc->p_notify_list, klb_nlist_destroy);

    return 0;
}

static int klua_krpc_status(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);

    int status = (NULL != p_krpc->p_nsc) ? klb_nsc_get_status(p_krpc->p_nsc) : -1;

    lua_pushboolean(L, (0 == status) ? true : false);
    lua_pushinteger(L, status);

    return 2;
}

static int send_pack_rpc_klua_krpc(klua_krpc_t* p_krpc, lua_State* L, int base_idx, uint32_t sequence)
{
    int ret = 1;

    if (NULL != p_krpc->p_nsc && 0 == klb_nsc_get_status(p_krpc->p_nsc))
    {
        if (KLB_PROTOCOL_RPC_MNP_LUA == p_krpc->protocol)
        {
            // pack buffer, rpc-lua
            klb_buf_t* p_buf = luaseri_map_binary_pack(L, base_idx/*1*/);

            ret = klb_nsc_send_rpc(p_krpc->p_nsc, sequence, 0, NULL, 0, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start);

            KLB_FREE(p_buf);
        }
        else if (KLB_PROTOCOL_RPC_MNP_JSON == p_krpc->protocol)
        {
            cJSON* p_json_param = luaseri_json_pack(L, base_idx);

            // pack buffer, rpc-json
            cJSON* p_root = cJSON_CreateObject();
            cJSON_AddItemToObject(p_root, "sequence", cJSON_CreateInt(sequence, 1));
            cJSON_AddItemToObject(p_root, "request", p_json_param);

            char* ptr = cJSON_PrintUnformatted(p_root);
            int size = strlen(ptr);

            ret = klb_nsc_send_rpc_json(p_krpc->p_nsc, sequence, 0, NULL, 0, (const uint8_t*)ptr, size);

            KLB_FREE(ptr);
            KLB_FREE_BY(p_root, cJSON_Delete);
        }
    }

    return ret;
}

static int klua_krpc_post(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);

    int ret = send_pack_rpc_klua_krpc(p_krpc, L, 1, KLUA_KRPC_sequence_post);

    lua_pushboolean(L, (0 == ret) ? true : false);
    return 1;
}

static int on_yield_klua_krpc_co_recv_notify(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krpc_t* p_krpc = (klua_krpc_t*)ptr;

    call_lua_co_recv_notify_klua_krpc(p_krpc, 0, 0, NULL, 0);

    return 0;
}


static int klua_krpc_co_recv_notify(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);
    klua_check_coroutine(L, "krpc.new():co_recv_notify() must in coroutine!");

    if (!p_krpc->b_notify)
    {
        return 0;
    }

    if (NULL != p_krpc->p_nsc && 0 == klb_nsc_get_status(p_krpc->p_nsc))
    {
        if (0 < klb_nlist_size(p_krpc->p_notify_list))
        {
            klua_krpc_data_t* p_data = (klua_krpc_data_t*)klb_nlist_pop_head(p_krpc->p_notify_list);

            int num = 0;

            if (KLB_NCM_PACK_RPC_LUA == p_data->packtype)
            {
                klb_buf_t* p_buf = p_data->p_buf;
                char* p_data = p_buf->p_buf + p_buf->start;
                int data_len = p_buf->end - p_buf->start;

                klb_mnp_common_t* p_com = (klb_mnp_common_t*)p_data;

                char* ptr = p_data + sizeof(klb_mnp_common_t) + p_com->head;
                data_len = data_len - sizeof(klb_mnp_common_t) - p_com->head;
                assert(data_len == p_com->size - p_com->head - sizeof(klb_mnp_common_t));
                assert(0 == p_buf->p_buf[p_buf->end]); // 连接层需要将非媒体数据末尾补0, 以便于文本解析

                num = luaseri_map_binary_unpack(L, 1, ptr, data_len);
            }
            else if(KLB_NCM_PACK_RPC_JSON == p_data->packtype)
            {
                cJSON* p_sequence = cJSON_GetObjectItem(p_data->p_json, "sequence");
                cJSON* p_notify = cJSON_GetObjectItem(p_data->p_json, "notify");
                uint32_t sequence = ((NULL != p_sequence) && (cJSON_Int == p_sequence->type)) ? (uint32_t)p_sequence->valueint : 0;

                luaseri_json_unpack(L, 1, p_notify);
            }

            return num;
        }

        // 等待通知数据
        assert(NULL == p_krpc->co_recv_notify);
        p_krpc->co_recv_notify = L;

        return klua_ex_coroutine_yield(p_krpc->p_coroutine, L, on_yield_klua_krpc_co_recv_notify, p_krpc);
        //return lua_yield(L, lua_gettop(L));
    }

    return 0;
}

static int on_yield_klua_krpc_co_call(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krpc_t* p_krpc = (klua_krpc_t*)ptr;

    call_lua_co_recv_klua_krpc(p_krpc, 0, 0, NULL, 0);

    return 0;
}

static int klua_krpc_co_call(lua_State* L)
{
    klua_krpc_t* p_krpc = to_klua_krpc(L, 1);
    klua_check_coroutine(L, "krpc.new():co_call() must in coroutine!");

    // 获取序列号
    uint32_t sequence = get_sequence_klua_krpc(p_krpc);

    // 发送请求
    if (0 == send_pack_rpc_klua_krpc(p_krpc, L, 1, sequence))
    {
        p_krpc->sequence = sequence;
    }
    else
    {
        return 0;
    }

    // 等待结果
    assert(NULL == p_krpc->co_recv);
    p_krpc->co_recv = L;

    return klua_ex_coroutine_yield(p_krpc->p_coroutine, L, on_yield_klua_krpc_co_call, p_krpc);
    //return lua_yield(L, lua_gettop(L));
}

static void klua_krpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krpc_close },

        { "status",         klua_krpc_status },

        { "post",           klua_krpc_post },
        { "co_call",        klua_krpc_co_call },

        { "co_recv_notify", klua_krpc_co_recv_notify },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krpc_close },
        { "__close",         klua_krpc_close },
        { "__tostring",      klua_krpc_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRPC_HANDLE); /* metatable for KLUA_KRPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


#define KLUA_KRPC_RBUF_MAX          (1024 * 8)              ///< 临时读取缓存最大值
#define KLUA_KRPC_READ_MAX          (1024 * 512)            ///< 缓存最大值后,暂停读取

static void default_param_klua_krpc(klua_krpc_param_t* p_param)
{
    p_param->rbuf_max = KLUA_KRPC_RBUF_MAX; // 临时读取缓存最大值
    p_param->read_max = KLUA_KRPC_READ_MAX; // 缓存最大值后,暂停读取
    p_param->tls = false;
    p_param->protocol = KLB_PROTOCOL_RPC_MNP_LUA;
}

static void check_param_klua_krpc(lua_State* L, int from, klua_krpc_param_t* p_param)
{
    static const char* options[] = { "rbuf_max", "read_max", "tls", "protocol", NULL };

    int num = lua_gettop(L) - from;

    for (int i = 1; i < num; i += 2)
    {
        int idx = luaL_checkoption(L, from + i, NULL, options);
        int ud = from + i + 1;
        switch (idx)
        {
        case 0:
            p_param->rbuf_max = (int)luaL_checkinteger(L, ud);
            break;
        case 1:
            p_param->read_max = (int)luaL_checkinteger(L, ud);
            break;
        case 2:
            p_param->tls = luaL_checkboolean(L, ud);
            break;
        case 3:
            {
                const char* p_protocol = luaL_checkstring(L, ud);
                if (0 == strcmp(p_protocol, "mnp-lua")) { p_param->protocol = KLB_PROTOCOL_RPC_MNP_LUA; }
                else if (0 == strcmp(p_protocol, "mnp-json")) { p_param->protocol = KLB_PROTOCOL_RPC_MNP_JSON; }
                else if (0 == strcmp(p_protocol, "http-lua")) { p_param->protocol = KLB_PROTOCOL_RPC_HTTP_LUA; }
                else if (0 == strcmp(p_protocol, "http-json")) { p_param->protocol = KLB_PROTOCOL_RPC_HTTP_JSON; }
                else if (0 == strcmp(p_protocol, "ws-lua")) { p_param->protocol = KLB_PROTOCOL_RPC_WS_LUA; }
                else if (0 == strcmp(p_protocol, "ws-json")) { p_param->protocol = KLB_PROTOCOL_RPC_WS_JSON; }
            }
            break;
        default:
            break;
        }
    }
}

klua_krpc_t* new_connect_klua_krpc(lua_State* L, klua_krpc_param_t* p_param, klb_socket_fd fd, const klb_socket_tls_param_t* p_tls_param)
{
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = NULL;
    if (p_param->tls)
    {
        p_socket = klb_socket_tls_async_create(fd, false, p_tls_param);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    klua_krpc_t* p_krpc = new_klua_krpc(L);
    p_krpc->L = L;
    p_krpc->p_env = klua_env_get_by_L(L);
    p_krpc->p_coroutine = klua_ex_get_coroutine(p_krpc->p_env);
    p_krpc->p_multi = klua_ex_multiplex_get_by_L(L);
    p_krpc->p_nsc = klb_nsc_create(p_krpc->p_multi);
    p_krpc->protocol = p_param->protocol;
    p_krpc->next_sequence = KLUA_KRPC_sequence_min;

    p_krpc->p_notify_list = klb_nlist_create();
    p_krpc->b_notify = false;

    klb_nsc_set_receiver(p_krpc->p_nsc, cb_recv_klua_krpc, p_krpc);
    klb_nsc_push(p_krpc->p_nsc, p_krpc->protocol, p_socket, NULL, 0);

    return p_krpc;
}

static int lib_klua_krpc_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    // param
    klua_krpc_param_t param = { 0 };
    default_param_klua_krpc(&param);
    check_param_klua_krpc(L, 2, &param);

    // socket fd
    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    // new connect
    new_connect_klua_krpc(L, &param, fd, NULL);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

#define KLUA_KRPC_MODULE_HANDLE    "KLUA_KRPC_MODULE_HANDLE*"

typedef struct klua_krpc_module_param_t_
{
    int max_connect;                            ///< 操作系统监听socket, 能缓存的最大连接数
}klua_krpc_module_param_t;

typedef struct klua_krpc_module_t_
{
    klua_krpc_module_param_t    param;          ///< 参数

    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境
        lua_State*              co_recv;        ///< sync的"co_recv"函数对应的协程
    };

    struct
    {
        klua_ex_coroutine_t*    p_coroutine;    ///< 协程扩展
        klb_multiplex_t*        p_multi;        ///< 复用

        klb_listen_t*           p_listen;       ///< 监听
        klb_nsp_t*              p_nsp;          ///< 连接预处理: 识别协议类型
        klb_ncm_t*              p_ncm;          ///< ncm模块

        int32_t                 recv_num;       ///< 缓存的数据大小
        klb_nlist_t*            p_recv_list;    ///< 缓存的数据列表: klua_krpc_data_t*
    };
}klua_krpc_module_t;


/////////////////////////////////////////
static int call_lua_co_recv_klua_krpc_module(klua_krpc_module_t* p_mo, int protocol, int id, uint32_t sequence, char* p_body, int body_len);


////////////////////////////////////////
static klua_krpc_module_t* new_klua_krpc_module(lua_State* L)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)lua_newuserdata(L, sizeof(klua_krpc_module_t));
    KLB_MEMSET(p_mo, 0, sizeof(klua_krpc_module_t));
    luaL_setmetatable(L, KLUA_KRPC_MODULE_HANDLE);
    return p_mo;
}

static klua_krpc_module_t* to_klua_krpc_module(lua_State* L, int index)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)luaL_checkudata(L, index, KLUA_KRPC_MODULE_HANDLE);
    luaL_argcheck(L, NULL != p_mo, index, "'krpc module' expected");
    return p_mo;
}

static int klua_krpc_module_tostring(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);

    lua_pushfstring(L, "krpc.new_module():%p,port:%d", p_mo, klb_listen_port(p_mo->p_listen));
    return 1;
}

static int klua_krpc_module_close(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);

    if (NULL != p_mo->p_listen)
    {
        klb_listen_close(p_mo->p_listen);     
    }

    if (NULL != p_mo->p_recv_list)
    {
        while (0 < klb_nlist_size(p_mo->p_recv_list))
        {
            klua_krpc_data_t* p_tmp = (klua_krpc_data_t*)klb_nlist_pop_head(p_mo->p_recv_list);
            klb_buf_unref_next(p_tmp->p_buf);
            KLB_FREE(p_tmp);
        }
    }

    KLB_FREE_BY(p_mo->p_listen, klb_listen_destroy);
    KLB_FREE_BY(p_mo->p_nsp, klb_nsp_destroy);
    KLB_FREE_BY(p_mo->p_ncm, klb_ncm_destroy);
    KLB_FREE_BY(p_mo->p_recv_list, klb_nlist_destroy);

    // 清理 co_recv
    call_lua_co_recv_klua_krpc_module(p_mo, 0, 0, 0, NULL, 0);

    return 0;
}

static int klua_krpc_module_status(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);

    lua_pushboolean(L, (NULL != p_mo->p_ncm) ? true : false);
    lua_pushinteger(L, (NULL != p_mo->p_ncm) ? 0 : -1);
    return 2;
}

static int klua_krpc_module_route(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    const char* p_protocol = luaL_checkstring(L, 3);


    return 0;
}

static int on_yield_klua_krpc_module_co_recv(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)ptr;

    call_lua_co_recv_klua_krpc_module(p_mo, 0, 0, 0, NULL, 0);

    return 0;
}


// "co_recv"接收消息
// 接收由客户端发起的 co_call(request)请求, post消息
// sequence序列号: 小于1000: 为无需回应的(post)消息; 
//      大于等于1000: 为必须回应的co_call(request)请求
static int klua_krpc_module_co_recv(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);
    klua_check_coroutine(L, "krpc.module().co_recv must in coroutine!");

    if (NULL == p_mo->p_ncm)
    {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        return 3;
    }

    if (0 < klb_nlist_size(p_mo->p_recv_list))
    {
        klua_krpc_data_t* p_recv = (klua_krpc_data_t*)klb_nlist_pop_head(p_mo->p_recv_list);
        klb_buf_t* p_buf = p_recv->p_buf;

        char* p_data = p_buf->p_buf + p_buf->start;
        int data_len = p_buf->end - p_buf->start;

        klb_mnp_common_t* p_com = (klb_mnp_common_t*)p_data;

        char* ptr = p_data + sizeof(klb_mnp_common_t) + p_com->head;
        data_len = data_len - sizeof(klb_mnp_common_t) - p_com->head;
        assert(data_len == p_com->size - p_com->head - sizeof(klb_mnp_common_t));
        assert(0 == p_buf->p_buf[p_buf->end]); // 连接层需要将非媒体数据末尾补0, 以便于文本解析

        int ret = 0;

        if (KLB_NCM_PACK_RPC_LUA == p_recv->packtype)
        {
            lua_pushinteger(L, p_recv->protocol);
            lua_pushinteger(L, p_recv->id);
            lua_pushinteger(L, p_com->sequence);
            int n = luaseri_map_binary_unpack(L, 4, ptr, data_len);

            ret = n + 3;
        }
        else if(KLB_NCM_PACK_RPC_JSON == p_recv->packtype)
        {
            const char* p_ep = NULL;
            cJSON* p_root = cJSON_Parse(ptr, &p_ep);
            cJSON* p_request = cJSON_GetObjectItem(p_root, "request");
            cJSON* p_sequence = cJSON_GetObjectItem(p_root, "sequence");
            uint32_t sequence = ((NULL != p_sequence) && (cJSON_Int == p_sequence->type)) ? (uint32_t)p_sequence->valueint : 0;

            lua_pushinteger(L, p_recv->protocol);
            lua_pushinteger(L, p_recv->id);
            lua_pushinteger(L, sequence);
            int n = luaseri_json_unpack(L, 4, p_request);
            KLB_FREE_BY(p_root, cJSON_Delete);

            ret = n + 3;
        }
        else
        {
            assert(false);
        }

        klb_buf_unref_next(p_recv->p_buf);
        KLB_FREE(p_recv);
        return ret;
    }

    assert(NULL == p_mo->co_recv);
    p_mo->co_recv = L;

    return klua_ex_coroutine_yield(p_mo->p_coroutine, L, on_yield_klua_krpc_module_co_recv, p_mo);
    //return lua_yield(L, lua_gettop(L));
}

// "response"回复
// 由客户端发起"co_call"(request), 服务处理后回应
static int klua_krpc_module_response(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);
    int protocol = (int)luaL_checkinteger(L, 2);
    int id = (int)luaL_checkinteger(L, 3);
    uint32_t sequence = (uint32_t)luaL_checkinteger(L, 4);

    if (NULL == p_mo->p_ncm)
    {
        return 0;
    }

    if (sequence < KLUA_KRPC_sequence_min)
    {
        // call(request)/response 大于等于 1000
        return 0;
    }

    if (KLB_PROTOCOL_RPC_MNP_LUA == protocol)
    {
        klb_buf_t* p_buf = luaseri_map_binary_pack(L, 4);

        klb_ncm_send_rpc(p_mo->p_ncm, id, sequence, 0, NULL, 0, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start);

        KLB_FREE(p_buf);
    }
    else if(KLB_PROTOCOL_RPC_MNP_JSON == protocol)
    {
        cJSON* p_json_param = luaseri_json_pack(L, 4);

        cJSON* p_root = cJSON_CreateObject();
        cJSON_AddItemToObject(p_root, "sequence", cJSON_CreateInt(sequence, 1));
        cJSON_AddItemToObject(p_root, "response", p_json_param);

        char* ptr = cJSON_PrintUnformatted(p_root);

        klb_ncm_send_rpc_json(p_mo->p_ncm, id, sequence, 0, NULL, 0, (const uint8_t*)ptr, strlen(ptr));

        KLB_FREE(ptr);
        KLB_FREE_BY(p_root, cJSON_Delete);
    }

    return 0;
}

// "notify"通知
// 由服务端, 直接发送给客户端的信息
// 业务层可以做成"订阅"机制, 或直接"推送"机制
static int klua_krpc_module_notify(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);
    int protocol = (int)luaL_checkinteger(L, 2);
    int id = (int)luaL_checkinteger(L, 3);

    if (NULL == p_mo->p_ncm)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    uint32_t sequence = KLUA_KRPC_sequence_notify; // "notify"(通知)固定2
    int ret = 1;

    if (KLB_PROTOCOL_RPC_MNP_LUA == protocol)
    {
        klb_buf_t* p_buf = luaseri_map_binary_pack(L, 3);

        ret = klb_ncm_send_rpc(p_mo->p_ncm, id, sequence, 0, NULL, 0, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start);

        KLB_FREE(p_buf);
    }
    else if (KLB_PROTOCOL_RPC_MNP_JSON == protocol)
    {
        cJSON* p_json_param = luaseri_json_pack(L, 3);

        cJSON* p_root = cJSON_CreateObject();
        cJSON_AddItemToObject(p_root, "sequence", cJSON_CreateInt(sequence, 1));
        cJSON_AddItemToObject(p_root, "notify", p_json_param);

        char* ptr = cJSON_PrintUnformatted(p_root);

        ret = klb_ncm_send_rpc_json(p_mo->p_ncm, id, sequence, 0, NULL, 0, (const uint8_t*)ptr, strlen(ptr));

        KLB_FREE(ptr);
        KLB_FREE_BY(p_root, cJSON_Delete);
    }

    lua_pushboolean(L, (0 == ret) ? true : false);
    return 1;
}

static int klua_krpc_module_notify_all(lua_State* L)
{
    klua_krpc_module_t* p_mo = to_klua_krpc_module(L, 1);

    if (NULL == p_mo->p_ncm)
    {
        return 0;
    }

    uint32_t sequence = KLUA_KRPC_sequence_notify; // "notify"(通知)固定2

    klb_buf_t* p_buf = luaseri_map_binary_pack(L, 1);

    klb_ncm_send_rpc(p_mo->p_ncm, 0, sequence, 0, NULL, 0, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start);
    KLB_FREE(p_buf);

    cJSON* p_json_param = luaseri_json_pack(L, 1);
    cJSON* p_root = cJSON_CreateObject();
    cJSON_AddItemToObject(p_root, "sequence", cJSON_CreateInt(sequence, 1));
    cJSON_AddItemToObject(p_root, "notify", p_json_param);
    char* p_json = cJSON_PrintUnformatted(p_root);

    klb_ncm_send_rpc_json(p_mo->p_ncm, 0, sequence, 0, NULL, 0, (const uint8_t*)p_json, strlen(p_json));

    KLB_FREE(p_json);
    KLB_FREE_BY(p_root, cJSON_Delete);

    return 0;
}

//////////////////////////////////////////////////

static void klua_krpc_module_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krpc_module_close },       ///< 

        { "status",         klua_krpc_module_status },

        { "route",          klua_krpc_module_route },       ///< 

        { "co_recv",        klua_krpc_module_co_recv },     ///< 
        { "response",       klua_krpc_module_response },    ///<

        { "notify",         klua_krpc_module_notify },
        { "notify_all",     klua_krpc_module_notify_all },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krpc_module_close },
        { "__close",         klua_krpc_module_close },
        { "__tostring",      klua_krpc_module_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRPC_MODULE_HANDLE);  /* metatable for KLUA_KRPC_MODULE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}

///////////////////////////////////////////////

static int call_lua_co_recv_klua_krpc_module(klua_krpc_module_t* p_mo, int protocol, int id, uint32_t sequence, char* p_body, int body_len)
{
    if (NULL != p_mo->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_mo->p_env), p_mo->co_recv);
        if (NULL == L) return -1; // 未处理

        p_mo->co_recv = NULL; // 清空

        lua_pushinteger(L, protocol);
        lua_pushinteger(L, id);
        lua_pushinteger(L, sequence);

        int n = 0;
        if (0 < body_len)
        {
            n = luaseri_map_binary_unpack(L, 4, p_body, body_len);
        }

        int status = lua_pcall(L, n + 3, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_co_recv_klua_krpc_module_json(klua_krpc_module_t* p_mo, int protocol, int id, char* p_body, int body_len)
{
    if (NULL != p_mo->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_mo->p_env), p_mo->co_recv);
        if (NULL == L) return -1; // 未处理

        p_mo->co_recv = NULL; // 清空

        const char* p_ep = NULL;
        cJSON* p_root = cJSON_Parse(p_body, &p_ep);
        cJSON* p_request = cJSON_GetObjectItem(p_root, "request");
        cJSON* p_sequence = cJSON_GetObjectItem(p_root, "sequence");
        uint32_t sequence = ((NULL != p_sequence) && (cJSON_Int == p_sequence->type)) ? (uint32_t)p_sequence->valueint : 0;

        lua_pushinteger(L, protocol);
        lua_pushinteger(L, id);
        lua_pushinteger(L, sequence);
        int n = luaseri_json_unpack(L, 4, p_request);
        KLB_FREE_BY(p_root, cJSON_Delete);

        int status = lua_pcall(L, n + 3, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static void push_recv_data_klua_krpc_module(klua_krpc_module_t* p_mo, int protocol, int id, int packtype, klb_buf_t* p_buf)
{
    klb_buf_ref_next(p_buf);

    klua_krpc_data_t* p_data = KLB_MALLOCZ(klua_krpc_data_t, 1, 0);
    p_data->protocol = protocol;
    p_data->id = id;
    p_data->packtype = packtype;
    p_data->p_buf = p_buf;

    klb_nlist_push_tail(p_mo->p_recv_list, p_data);
}

/// @brief 连接解析收到数据时,回调
/// @return int 0
static int on_recv_klua_krpc_module(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_buf)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)ptr;

    if (KLB_SOCKET_CONNECT == code)
    {
        // 新连接成功
    }
    else if (KLB_SOCKET_OK == code)
    {
        char* p_data = p_buf->p_buf + p_buf->start;
        int data_len = p_buf->end - p_buf->start;

        klb_mnp_common_t* p_com = (klb_mnp_common_t*)p_data;

        char* ptr = p_data + sizeof(klb_mnp_common_t) + p_com->head;
        data_len = data_len - sizeof(klb_mnp_common_t) - p_com->head;
        assert(data_len == p_com->size - p_com->head - sizeof(klb_mnp_common_t));
        assert(0 == p_buf->p_buf[p_buf->end]); // 连接层需要将非媒体数据末尾补0, 以便于文本解析

        if (KLB_NCM_PACK_RPC_LUA == packtype)
        {
            if (call_lua_co_recv_klua_krpc_module(p_mo, protocol, id, p_com->sequence, ptr + p_com->head, data_len - p_com->head) < 0)
            {
                // 未调用lua处理, 则缓存起来
                push_recv_data_klua_krpc_module(p_mo, protocol, id, packtype, p_buf);
            }
        }
        else if (KLB_NCM_PACK_RPC_JSON == packtype)
        {
            if (call_lua_co_recv_klua_krpc_module_json(p_mo, protocol, id, ptr, data_len) < 0)
            {
                // 未调用lua处理, 则缓存起来
                push_recv_data_klua_krpc_module(p_mo, protocol, id, packtype, p_buf);
            }
        }
    }
    else if (KLB_SOCKET_CLOSEING == code)
    {
        // 主动断开连接, 啥也不做
    }
    else
    {
        // 断开连接
        klb_ncm_close(p_mo->p_ncm, id);
    }

    return 0;
}

static int on_accept_klua_krpc_nsp(void* ptr, int protocol, klb_socket_t* p_socket, const klb_buf_t* p_buf)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)ptr;

    uint8_t* p_data = (uint8_t*)(p_buf->p_buf + p_buf->start);
    int data_len = p_buf->end - p_buf->start;

    int id = klb_ncm_push(p_mo->p_ncm, protocol, p_socket, p_data, data_len);

    if (id < 0)
    {
        klb_socket_destroy(p_socket);
    }

    return 0;
}

static int on_accept_klua_krpc_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_krpc_module_t* p_mo = (klua_krpc_module_t*)ptr;
    klb_socket_t* p_socket = NULL;

    if (tls)
    {
        p_socket = klb_socket_tls_async_create(fd, true, p_tls_param);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    if (NULL != p_socket)
    {
        klb_socket_set_connected(p_socket, true); // 设置已连接
        klb_nsp_push(p_mo->p_nsp, p_socket);
    }
    else
    {
        KLB_SOCKET_CLOSE(fd);
    }

    return 0;
}

////////////////////////////////////////////////

static int lib_klua_krpc_new_module(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_krpc_module_t* p_mo = new_klua_krpc_module(L);
    p_mo->L = L;
    p_mo->p_env = klua_env_get_by_L(L);

    p_mo->p_coroutine = klua_ex_get_coroutine(p_mo->p_env);
    p_mo->p_multi = klua_ex_multiplex_get_by_L(L);
    p_mo->p_listen = klb_listen_create(p_mo->p_multi);
    p_mo->p_nsp = klb_nsp_create(p_mo->p_multi);
    p_mo->p_ncm = klb_ncm_create(p_mo->p_multi, KLB_PROTOCOL_LOAD_RPC);

    p_mo->recv_num = 0;
    p_mo->p_recv_list = klb_nlist_create();

    // 数据
    klb_ncm_add_receiver(p_mo->p_ncm, on_recv_klua_krpc_module, p_mo);

    // nsp预处理
    klb_nsp_set_accept(p_mo->p_nsp, on_accept_klua_krpc_nsp, p_mo);

    // 监听之后的socket处理
    klb_listen_set_accept(p_mo->p_listen, on_accept_klua_krpc_listen, p_mo);

    // 监听端口
    klb_listen_open(p_mo->p_listen, port, 20);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// rpc lua
int klua_open_krpc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new_module", lib_klua_krpc_new_module },

        { "new",        lib_klua_krpc_connect },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // krpc module, 提供服务模块
    klua_krpc_module_createmeta(L);

    // krpc
    klua_krpc_createmeta(L);

    return 1;
}
