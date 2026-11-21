// Doc Encode : UTF-8 BOM, Unix(LF)
// klbws Lua 绑定: require("kws")
#include "klbws/klbws.h"
#include "klbws/core/klb_wsclient_conn.h"
#include "klbws/core/klb_wsserve_conn.h"
#include "klbws/core/klb_websocket.h"
#include "klbws/ws_klua/klbws_rpc_klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbthird/sds.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// kws 连接 (client / serve 共用 userdata)

#define KLUA_KWS_HANDLE                 "KWS_HANDLE*"               ///< Lua meta标示


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kws(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


#define KLUA_KWS_EVT_OPEN               1
#define KLUA_KWS_EVT_CLOSE              2
#define KLUA_KWS_EVT_PONG               3


/// @struct klua_kws_evt_t
/// @brief  排队中的 open/close/pong
typedef struct klua_kws_evt_t_
{
    int                     kind;           ///< KLUA_KWS_EVT_*
    int                     code;           ///< close 码
    klb_buf_t*              p_body;         ///< pong 载荷或 close reason
}klua_kws_evt_t;


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
        klb_nlist_t*            p_evt_nlist;    ///< open/close/pong; 存储 klua_kws_evt_t*
        int                     recv_code;      ///< 最近一次非0网络码; 0.无
        char                    peer[128];      ///< 对端 "ip:port"
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

    if (NULL != p_kws->p_evt_nlist)
    {
        while (0 < klb_nlist_size(p_kws->p_evt_nlist))
        {
            klua_kws_evt_t* p_evt = (klua_kws_evt_t*)klb_nlist_pop_head(p_kws->p_evt_nlist);
            if (NULL != p_evt)
            {
                KLB_FREE_BY(p_evt->p_body, klb_buf_unref);
                KLB_FREE(p_evt);
            }
        }
    }

    KLB_FREE_BY(p_kws->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_kws->p_binary_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_kws->p_evt_nlist, klb_nlist_destroy);

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

static int push_evt_klua_kws(lua_State* L, klua_kws_evt_t* p_evt)
{
    if (KLUA_KWS_EVT_OPEN == p_evt->kind)
    {
        lua_pushstring(L, "open");
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    if (KLUA_KWS_EVT_CLOSE == p_evt->kind)
    {
        lua_pushstring(L, "close");
        lua_pushinteger(L, p_evt->code);
        if (NULL != p_evt->p_body && 0 < klb_buf_data_len(p_evt->p_body))
        {
            lua_pushlstring(L, p_evt->p_body->p_buf + p_evt->p_body->start, (size_t)klb_buf_data_len(p_evt->p_body));
        }
        else
        {
            lua_pushstring(L, "");
        }

        return 3;
    }

    lua_pushstring(L, "pong");
    lua_pushstring(L, "");
    if (NULL != p_evt->p_body && 0 < klb_buf_data_len(p_evt->p_body))
    {
        lua_pushlstring(L, p_evt->p_body->p_buf + p_evt->p_body->start, (size_t)klb_buf_data_len(p_evt->p_body));
    }
    else
    {
        lua_pushstring(L, "");
    }

    return 3;
}

static void queue_evt_klua_kws(klua_kws_t* p_kws, int kind, int code, klb_buf_t* p_body)
{
    klua_kws_evt_t* p_evt = KLB_MALLOCZ(klua_kws_evt_t, 1, 0);
    p_evt->kind = kind;
    p_evt->code = code;
    p_evt->p_body = p_body;
    klb_nlist_push_tail(p_kws->p_evt_nlist, p_evt);
}

static void free_evt_klua_kws(klua_kws_evt_t* p_evt)
{
    if (NULL == p_evt)
    {
        return;
    }

    KLB_FREE_BY(p_evt->p_body, klb_buf_unref);
    KLB_FREE(p_evt);
}

// 唤醒 co_recv
static int call_co_recv_klua_kws(klua_kws_t* p_kws)
{
    assert(NULL != p_kws);

    if (NULL == p_kws->co_recv)
    {
        return -1;
    }

    if (0 >= klb_nlist_size(p_kws->p_evt_nlist) &&
        0 >= klb_nlist_size(p_kws->p_text_nlist) &&
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
    klua_kws_evt_t* p_evt = (klua_kws_evt_t*)klb_nlist_pop_head(p_kws->p_evt_nlist);
    if (NULL != p_evt)
    {
        nargs = push_evt_klua_kws(L, p_evt);
        free_evt_klua_kws(p_evt);
    }
    else
    {
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
        else if (KLB_WEBSOCKET_PACK_OPEN == packtype)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            queue_evt_klua_kws(p_kws, KLUA_KWS_EVT_OPEN, 0, NULL);
            call_co_recv_klua_kws(p_kws);
        }
        else if (KLB_WEBSOCKET_PACK_PONG == packtype)
        {
            queue_evt_klua_kws(p_kws, KLUA_KWS_EVT_PONG, 0, p_data);
            call_co_recv_klua_kws(p_kws);
        }
        else if (KLB_WEBSOCKET_PACK_CLOSE == packtype)
        {
            int ccode = KLB_WEBSOCKET_CLOSE_NOSTATUS;
            klb_buf_t* p_reason = NULL;
            if (NULL != p_data && 2 <= klb_buf_data_len(p_data))
            {
                const uint8_t* p = (const uint8_t*)(p_data->p_buf + p_data->start);
                int n = klb_buf_data_len(p_data);
                ccode = (int)KLB_RB16(p);
                if (2 < n)
                {
                    p_reason = klb_buf_malloc(n - 2, false);
                    memcpy(p_reason->p_buf, p + 2, (size_t)(n - 2));
                    p_reason->start = 0;
                    p_reason->end = n - 2;
                }
            }

            KLB_FREE_BY(p_data, klb_buf_unref);
            queue_evt_klua_kws(p_kws, KLUA_KWS_EVT_CLOSE, ccode, p_reason);
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
    p_kws->p_evt_nlist = klb_nlist_create();
    p_kws->recv_code = 0;
    p_kws->peer[0] = '\0';

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
        uint8_t mask_key[4] = { 0 };
        klb_websocket_mask_key(mask_key);
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

static int send_frame_klua_kws(klua_kws_t* p_kws, uint8_t opcode, const char* p_data, int data_len)
{
    if (NULL == p_kws->p_conn)
    {
        return 1;
    }

    uint8_t mask_key[4] = { 0 };
    const uint8_t* p_mask = NULL;
    if (p_kws->is_client)
    {
        klb_websocket_mask_key(mask_key);
        p_mask = mask_key;
    }

    klb_buf_t* p_frame = klb_websocket_pack_fin(opcode, p_mask, p_data, data_len);
    if (p_kws->is_client)
    {
        return klb_wsclient_conn_send_buf(p_kws->p_conn, p_frame);
    }

    return klb_wsserve_conn_send_buf(p_kws->p_conn, p_frame);
}

static int klua_kws_send_ping(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    size_t data_len = 0;
    const char* p_data = NULL;
    if (lua_isstring(L, 2))
    {
        p_data = lua_tolstring(L, 2, &data_len);
    }

    int ret = 1;
    if (125 >= (int)data_len)
    {
        ret = send_frame_klua_kws(p_kws, KLB_WEBSOCKET_OPCODE_PING, p_data, (int)data_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kws_send_close(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);

    int code = KLB_WEBSOCKET_CLOSE_NORMAL;
    if (lua_isinteger(L, 2))
    {
        code = (int)lua_tointeger(L, 2);
    }

    size_t reason_len = 0;
    const char* p_reason = NULL;
    if (lua_isstring(L, 3))
    {
        p_reason = lua_tolstring(L, 3, &reason_len);
    }

    int ret = 1;
    if (klb_websocket_close_code_ok(code) && 123 >= (int)reason_len &&
        (NULL == p_reason || klb_websocket_utf8_ok((const uint8_t*)p_reason, (int)reason_len)))
    {
        char payload[125];
        payload[0] = (char)((code >> 8) & 0xFF);
        payload[1] = (char)(code & 0xFF);
        if (0 < reason_len && NULL != p_reason)
        {
            memcpy(payload + 2, p_reason, reason_len);
        }

        ret = send_frame_klua_kws(p_kws, KLB_WEBSOCKET_OPCODE_CLOSE, payload, 2 + (int)reason_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kws_peer(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);
    lua_pushstring(L, p_kws->peer);
    return 1;
}

/// @brief 发送背压: 写缓存未发送字节数
/// @note Lua: n = kws:buffered_amount()
static int klua_kws_buffered_amount(lua_State* L)
{
    klua_kws_t* p_kws = to_klua_kws(L, 1);
    int64_t n = 0;

    if (NULL != p_kws->p_conn)
    {
        if (p_kws->is_client)
        {
            n = klb_wsclient_wbuf_size(p_kws->p_conn);
        }
        else
        {
            n = klb_wsserve_wbuf_size(p_kws->p_conn);
        }
    }

    lua_pushinteger(L, (lua_Integer)n);
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

    klua_kws_evt_t* p_evt = (klua_kws_evt_t*)klb_nlist_head(p_kws->p_evt_nlist);
    if (NULL != p_evt)
    {
        klb_nlist_pop_head(p_kws->p_evt_nlist);
        int n = push_evt_klua_kws(L, p_evt);
        free_evt_klua_kws(p_evt);
        return n;
    }

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


static int klua_kws_set_timeout(lua_State* L)
{
    klua_kws_t* p = to_klua_kws(L, 1);
    int r = (int)luaL_optinteger(L, 2, 0);
    int w = (int)luaL_optinteger(L, 3, 0);
    int i = (int)luaL_optinteger(L, 4, 0);
    if (NULL == p->p_conn) { lua_pushinteger(L, KLB_NETCODE_INVAL); return 1; }
    if (p->is_client) { klb_wsclient_conn_set_timeout(p->p_conn, r, w, i); }
    lua_pushinteger(L, 0);
    return 1;
}

static int klua_kws_set_max_payload(lua_State* L)
{
    klua_kws_t* p = to_klua_kws(L, 1);
    int64_t m = (int64_t)luaL_optinteger(L, 2, 0);
    if (NULL == p->p_conn) { lua_pushinteger(L, KLB_NETCODE_INVAL); return 1; }
    if (p->is_client) { klb_wsclient_conn_set_max_payload(p->p_conn, m); }
    lua_pushinteger(L, 0);
    return 1;
}


/// @brief 最近收包时刻 (含 PING/PONG); 供代理泵送刷新空闲计时
static int klua_kws_last_recv_ms(lua_State* L)
{
    klua_kws_t* p = to_klua_kws(L, 1);

    if (NULL == p->p_conn || NULL == p->p_conn->p_socket)
    {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)p->p_conn->p_socket->last_recv_tc);
    return 1;
}

static void klua_kws_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kws_close },

        { "send_text",      klua_kws_send_text },
        { "send_binary",    klua_kws_send_binary },
        { "send_ping",      klua_kws_send_ping },
        { "send_close",     klua_kws_send_close },
        { "peer",           klua_kws_peer },
        { "buffered_amount", klua_kws_buffered_amount },

        { "co_recv",        klua_kws_co_recv },
        { "set_timeout",    klua_kws_set_timeout },
        { "set_max_payload", klua_kws_set_max_payload },
        { "last_recv_ms",   klua_kws_last_recv_ms },

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

static bool text_has_ctl_klua_kws(const char* p, size_t n)
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

static bool header_reserved_klua_kws(const char* p_name, size_t n)
{
    char tmp[64];
    if (0 == n || n >= sizeof(tmp))
    {
        return true;
    }

    for (size_t i = 0; i < n; ++i)
    {
        char c = p_name[i];
        if (':' == c || ' ' == c)
        {
            return true;
        }

        if ('A' <= c && 'Z' >= c)
        {
            c = (char)(c - 'A' + 'a');
        }

        tmp[i] = c;
    }

    tmp[n] = '\0';

    static const char* reserved[] = {
        "host",
        "connection",
        "upgrade",
        "sec-websocket-key",
        "sec-websocket-version",
        "sec-websocket-protocol",
        "sec-websocket-accept",
        "content-length",
        NULL
    };

    for (int i = 0; NULL != reserved[i]; ++i)
    {
        if (0 == strcmp(tmp, reserved[i]))
        {
            return true;
        }
    }

    return false;
}

static bool append_headers_klua_kws(lua_State* L, int opts_idx, sds* p_str)
{
    if (!klua_is_table(L, opts_idx))
    {
        return true;
    }

    lua_getfield(L, opts_idx, "headers");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return true;
    }

    int headers_idx = lua_gettop(L);
    lua_pushnil(L);
    while (0 != lua_next(L, headers_idx))
    {
        if (!lua_isstring(L, -2) || !lua_isstring(L, -1))
        {
            lua_pop(L, 1);
            continue;
        }

        size_t name_len = 0;
        size_t val_len = 0;
        const char* p_name = lua_tolstring(L, -2, &name_len);
        const char* p_val = lua_tolstring(L, -1, &val_len);
        if (text_has_ctl_klua_kws(p_name, name_len) || text_has_ctl_klua_kws(p_val, val_len))
        {
            lua_pop(L, 2);
            lua_pop(L, 1);
            return false;
        }

        if (!header_reserved_klua_kws(p_name, name_len))
        {
            *p_str = sdscatlen(*p_str, p_name, name_len);
            *p_str = sdscat(*p_str, ": ");
            *p_str = sdscatlen(*p_str, p_val, val_len);
            *p_str = sdscat(*p_str, "\r\n");
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static void append_host_klua_kws(sds* p_str, const char* p_host, int port, bool tls)
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
        str = sdscatfmt(str, ":%i", port);
    }

    str = sdscat(str, "\r\n");
    *p_str = str;
}

static void format_peer_klua_kws(char* p_dst, int cap, const struct sockaddr_in* p_addr)
{
    klua_netlisten_format_peer_in(p_dst, cap, p_addr);
}

static sds build_upgrade_klua_kws(lua_State* L, int opts_idx, const char* p_host, int port, const char* p_path,
    bool tls, const char* p_protocol, char* p_key, int key_cap)
{
    uint8_t nonce[16];
    klb_websocket_random(nonce, 16);
    if (0 != klb_websocket_b64(nonce, 16, p_key, key_cap))
    {
        return NULL;
    }

    sds str = sdsnew("");
    str = sdscatfmt(str, "GET %s HTTP/1.1\r\n", p_path);
    append_host_klua_kws(&str, p_host, port, tls);
    str = sdscat(str, "Connection: Upgrade\r\n");
    str = sdscat(str, "Upgrade: websocket\r\n");
    str = sdscat(str, "Sec-WebSocket-Version: 13\r\n");
    str = sdscatfmt(str, "Sec-WebSocket-Key: %s\r\n", p_key);
    if (NULL != p_protocol && '\0' != p_protocol[0])
    {
        str = sdscatfmt(str, "Sec-WebSocket-Protocol: %s\r\n", p_protocol);
    }

    if (!append_headers_klua_kws(L, opts_idx, &str))
    {
        KLB_FREE_BY(str, sdsfree);
        return NULL;
    }

    str = sdscat(str, "\r\n");
    return str;
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

    if (text_has_ctl_klua_kws(p_path, strlen(p_path)))
    {
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    bool tls = false;
    char protocol[128] = { 0 };
    int code = 0;
    int opts_idx = klua_is_table(L, 3) ? 3 : 4;
    if (klua_is_table(L, opts_idx))
    {
        lua_getfield(L, opts_idx, "tls");
        tls = klua_check_option_boolean(L, -1, false);
        lua_pop(L, 1);

        lua_getfield(L, opts_idx, "protocol");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_proto = lua_tolstring(L, -1, &n);
            if (0 < n && n < sizeof(protocol))
            {
                if (text_has_ctl_klua_kws(p_proto, n))
                {
                    lua_pop(L, 1);
                    return push_err_klua_kws(L, KLB_NETCODE_INVAL);
                }

                memcpy(protocol, p_proto, n + 1);
            }
            else if (0 < n)
            {
                lua_pop(L, 1);
                return push_err_klua_kws(L, KLB_NETCODE_INVAL);
            }
        }
        lua_pop(L, 1);
    }

    char key[32] = { 0 };
    sds req = build_upgrade_klua_kws(L, opts_idx, p_host, port, p_path, tls, protocol, key, (int)sizeof(key));
    if (NULL == req)
    {
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = NULL;

    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(req, sdsfree);
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    p_conn = klb_wsclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_conn)
    {
        KLB_FREE_BY(req, sdsfree);
        return push_err_klua_kws(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    // step1. 绑定 Lua userdata
    klua_kws_t* p_kws = new_klua_kws(L);
    setup_klua_kws(p_kws, L, p_conn, true);
    klb_wsclient_conn_set_key(p_conn, key);
    klb_wsclient_conn_set_protocol(p_conn, protocol);

    int npeer = snprintf(p_kws->peer, sizeof(p_kws->peer), "%s:%d", p_host, port);
    if (0 > npeer || (size_t)npeer >= sizeof(p_kws->peer))
    {
        p_kws->peer[0] = '\0';
    }
    p_kws->peer[sizeof(p_kws->peer) - 1] = '\0';

    // step2. 发送 WS 握手
    klb_wsclient_conn_send(p_conn, (const uint8_t*)req, (int)sdslen(req), NULL, 0);
    KLB_FREE_BY(req, sdsfree);

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
        bool                    tls;            ///< 是否 TLS
        klb_socket_tls_server_ctx_t* p_tls_ctx; ///< TLS 服务端上下文
        char                    path[256];      ///< 路径过滤; 空为任意
        char                    protocol[128];  ///< 选定子协议; 空为不协商
        bool                    has_ping;       ///< listen opts 是否显式配心跳
        int                     ping_interval_ms;   ///< 服务端 Ping 间隔; 0=关
        int                     ping_timeout_ms;    ///< 等 Pong 超时
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

static klua_kws_t* new_klua_kws_by_socket(lua_State* L, klua_kws_listen_t* p_listen, klb_netlisten_socket_t* p_listen_socket)
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

    if (p_listen->has_ping)
    {
        klb_wsserve_conn_set_ping(p_conn, p_listen->ping_interval_ms, p_listen->ping_timeout_ms);
    }

    klua_kws_t* p_kws = new_klua_kws(L);
    setup_klua_kws(p_kws, L, p_conn, false);
    format_peer_klua_kws(p_kws->peer, (int)sizeof(p_kws->peer), &p_listen_socket->addr);
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
    if (NULL == new_klua_kws_by_socket(L, p_listen, p_listen_socket))
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
    p_tmp->tls = p_listen->tls;
    (void)tls;

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

static int klua_kws_listen_port(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_kws_listen_local_addr(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_kws_listen_native_fd(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static int klua_kws_listen_co_accept(lua_State* L)
{
    klua_kws_listen_t* p_listen = to_klua_kws_listen(L, 1);
    klua_check_coroutine(L, "kws.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        if (NULL == new_klua_kws_by_socket(L, p_listen, p_listen_socket))
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
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_kws_listen, p_listen);
}

static void klua_kws_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kws_listen_close },

        { "co_accept",      klua_kws_listen_co_accept },
        { "port",           klua_kws_listen_port },
        { "local_addr",     klua_kws_listen_local_addr },
        { "native_fd",      klua_kws_listen_native_fd },

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

    bool tls = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    char path[256] = { 0 };
    char protocol[128] = { 0 };
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    int rc = 0;
    bool has_ping = false;
    int ping_interval_ms = -1;
    int ping_timeout_ms = -1;

    open_opts.family = AF_INET;

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
            if (n >= sizeof(path))
            {
                lua_pop(L, 1);
                return push_err_klua_kws(L, KLB_NETCODE_INVAL);
            }

            if (0 < n)
            {
                if (text_has_ctl_klua_kws(p_path, n))
                {
                    lua_pop(L, 1);
                    return push_err_klua_kws(L, KLB_NETCODE_INVAL);
                }

                memcpy(path, p_path, n + 1);
            }
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "protocol");
        if (lua_isstring(L, -1))
        {
            size_t n = 0;
            const char* p_proto = lua_tolstring(L, -1, &n);
            if (n >= sizeof(protocol))
            {
                lua_pop(L, 1);
                return push_err_klua_kws(L, KLB_NETCODE_INVAL);
            }

            if (0 < n)
            {
                if (text_has_ctl_klua_kws(p_proto, n))
                {
                    lua_pop(L, 1);
                    return push_err_klua_kws(L, KLB_NETCODE_INVAL);
                }

                memcpy(protocol, p_proto, n + 1);
            }
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

        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_kws(L, rc);
        }
    }

    klb_socket_tls_server_ctx_t* p_tls_ctx = NULL;
    if (tls)
    {
        p_tls_ctx = klb_socket_tls_server_ctx_create(p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            return push_err_klua_kws(L, KLB_NETCODE_TLS_CERT);
        }
    }

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_kws_listen_t* p_listen = NULL;

    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_kws_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        KLB_FREE_BY(p_tls_ctx, klb_socket_tls_server_ctx_destroy);
        return push_err_klua_kws(L, rc);
    }

    p_listen = new_klua_kws_listen(L);

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
    p_listen->has_ping = has_ping;
    p_listen->ping_interval_ms = ping_interval_ms;
    p_listen->ping_timeout_ms = ping_timeout_ms;

    p_listen->p_socket_nlist = klb_nlist_create();

    klb_netconn_set_udata(p_listen_conn, p_listen);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

static int klua_kws_accept(lua_State* L)
{
    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 1, &key_len);
    char accept[64];

    if (0 == key_len || 0 != klb_websocket_accept(p_key, accept, (int)sizeof(accept)))
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, accept);
    return 1;
}


/// @brief 由已发 101 的探测 sock 接管为 WS 服务连接; 帧/心跳在 C
/// @note Lua: conn, code = kws.adopt(lightuserdata); 参数为 klb_netlisten_socket_t* (取得 fd 并释放)
static int klua_kws_adopt(lua_State* L)
{
    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)lua_touserdata(L, 1);
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_kws_t* p_kws = NULL;
    bool has_ping = false;
    int ping_interval_ms = -1;
    int ping_timeout_ms = -1;

    if (NULL == p_listen_socket)
    {
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    if (klua_is_table(L, 2))
    {
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

    fd = p_listen_socket->fd;
    KLB_FREE(p_listen_socket);

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_env || NULL == p_netmulti)
    {
        KLB_SOCKET_CLOSE(fd);
        return push_err_klua_kws(L, KLB_NETCODE_INVAL);
    }

    p_socket = klb_socket_async_create(fd);
    if (NULL == p_socket)
    {
        KLB_SOCKET_CLOSE(fd);
        return push_err_klua_kws(L, KLB_NETCODE_NOMEM);
    }

    p_conn = klb_wsserve_conn_create(p_netmulti, p_socket);
    if (NULL == p_conn)
    {
        klb_socket_destroy(p_socket);
        return push_err_klua_kws(L, KLB_NETCODE_NOMEM);
    }

    if (has_ping)
    {
        klb_wsserve_conn_set_ping(p_conn, ping_interval_ms, ping_timeout_ms);
    }

    p_kws = new_klua_kws(L);
    setup_klua_kws(p_kws, L, p_conn, false);
    klb_wsserve_conn_adopt_tail(p_conn, NULL);
    return 1;
}

static int adopt_http_klua_kws(void* ptr, klb_netmulti_t* p_multi, klb_socket_t* p_socket, klb_buf_t* p_tail, const char* p_peer)
{
    lua_State* L = (lua_State*)ptr;
    klb_netconn_t* p_conn = klb_wsserve_conn_create(p_multi, p_socket);
    klua_kws_t* p_kws = new_klua_kws(L);

    setup_klua_kws(p_kws, L, p_conn, false);
    if (NULL != p_peer && '\0' != p_peer[0])
    {
        strncpy(p_kws->peer, p_peer, sizeof(p_kws->peer) - 1);
        p_kws->peer[sizeof(p_kws->peer) - 1] = '\0';
    }

    klb_wsserve_conn_adopt_tail(p_conn, p_tail);
    return 1;
}

int klua_open_kws(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",        klua_kws_connect },
        { "connect_rpc",    klua_kws_connect_rpc },

        { "listen",         klua_kws_listen },
        { "listen_rpc",     klua_kws_listen_rpc },

        { "accept",         klua_kws_accept },
        { "adopt",          klua_kws_adopt },

        { NULL,         NULL }
    };

    luaL_newlib(L, lib);

    klua_kws_createmeta(L);
    klua_kws_listen_createmeta(L);

    klua_kwsclientrpc_createmeta(L);
    klua_kwsserverpc_createmeta(L);
    klua_kwsserverpc_listen_createmeta(L);

    klb_netmulti_set_ws_adopt(adopt_http_klua_kws);

    return 1;
}

// end
