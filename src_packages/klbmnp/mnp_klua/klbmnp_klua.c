// Doc Encode : UTF-8 BOM, Unix(LF)
// klbmnp Lua 绑定: require("kmnp")
#include "klbmnp/klbmnp.h"
#include "klbmnp/core/klb_mnpclient_conn.h"
#include "klbmnp/core/klb_mnpserve_conn.h"
#include "klbmnp/mnp_klua/klbmnp_rpc_klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_crypto.h"
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
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// kmnp 连接 (client / serve 共用 userdata)

#define KLUA_KMNP_HANDLE                "KMNP_HANDLE*"              ///< Lua meta标示


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kmnp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @struct klua_kmnp_t
/// @brief  kmnp 连接
typedef struct klua_kmnp_t_
{
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用
        klb_netconn_t*          p_conn;         ///< MNP 连接
        bool                    is_client;      ///< true.客户端; false.服务端
        char                    peer[128];      ///< 对端 "ip:port"; 未知为空串
    };

    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< TEXT; 存储 klb_buf_t*
        klb_nlist_t*            p_binary_nlist; ///< BINARY; 存储 klb_buf_t*
        klb_nlist_t*            p_media_nlist;  ///< MEDIA; 存储 klb_buf_t*
        int                     recv_code;      ///< 最近一次非0网络码; 0.无
    };
}klua_kmnp_t;


////////////////////////////////////////
static klua_kmnp_t* new_klua_kmnp(lua_State* L)
{
    klua_kmnp_t* p_mnp = (klua_kmnp_t*)lua_newuserdata(L, sizeof(klua_kmnp_t));
    KLB_MEMSET(p_mnp, 0, sizeof(klua_kmnp_t));
    luaL_setmetatable(L, KLUA_KMNP_HANDLE);
    return p_mnp;
}

static klua_kmnp_t* to_klua_kmnp(lua_State* L, int index)
{
    klua_kmnp_t* p_mnp = (klua_kmnp_t*)luaL_checkudata(L, index, KLUA_KMNP_HANDLE);
    luaL_argcheck(L, NULL != p_mnp, index, "'kmnp' expected");
    return p_mnp;
}

static int klua_kmnp_tostring(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    lua_pushfstring(L, "kmnp:%p", p_mnp);
    return 1;
}

static int call_co_recv_end_klua_kmnp(klua_kmnp_t* p_mnp);

static int klua_kmnp_close(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    /* wake pending co_recv before teardown (avoid hmap assert on destroy) */
    call_co_recv_end_klua_kmnp(p_mnp);

    if (NULL != p_mnp->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_mnp->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_mnp->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    if (NULL != p_mnp->p_binary_nlist)
    {
        while (0 < klb_nlist_size(p_mnp->p_binary_nlist))
        {
            klb_buf_t* p_bin = klb_nlist_pop_head(p_mnp->p_binary_nlist);
            KLB_FREE_BY(p_bin, klb_buf_unref);
        }
    }

    if (NULL != p_mnp->p_media_nlist)
    {
        while (0 < klb_nlist_size(p_mnp->p_media_nlist))
        {
            klb_buf_t* p_media = klb_nlist_pop_head(p_mnp->p_media_nlist);
            KLB_FREE_BY(p_media, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_mnp->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_mnp->p_binary_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_mnp->p_media_nlist, klb_nlist_destroy);

    if (NULL != p_mnp->p_conn)
    {
        klb_netconn_bind_recv_data(p_mnp->p_conn, NULL);
        klb_netconn_set_udata(p_mnp->p_conn, NULL);

        if (p_mnp->is_client)
        {
            klb_mnpclient_conn_free(p_mnp->p_conn);
        }
        else
        {
            klb_mnpserve_conn_free(p_mnp->p_conn);
        }

        p_mnp->p_conn = NULL;
    }

    return 0;
}

////////////////////////////////////////

static int push_mnp_text_klua_kmnp(lua_State* L, const char* p_kind, klb_buf_t* p_data)
{
    assert(NULL != p_data);

    int data_len = p_data->end - p_data->start;
    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        lua_pushstring(L, p_kind);
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    klb_mnp_text_t* p_txt = (klb_mnp_text_t*)(p_data->p_buf + p_data->start);
    int remain = data_len - (int)sizeof(klb_mnp_text_t);
    int head_len = (int)p_txt->head_size;
    int body_len = (int)p_txt->size - head_len - (int)sizeof(klb_mnp_text_t);
    const char* p_head = (const char*)(p_txt + 1);

    if (head_len < 0 || head_len > remain)
    {
        lua_pushstring(L, p_kind);
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        return 3;
    }

    if (body_len < 0)
    {
        body_len = 0;
    }
    else if (body_len > remain - head_len)
    {
        body_len = remain - head_len;
    }

    lua_pushstring(L, p_kind);
    lua_pushlstring(L, p_head, (size_t)head_len);
    lua_pushlstring(L, p_head + head_len, (size_t)body_len);
    return 3;
}

static int push_mnp_media_klua_kmnp(lua_State* L, klb_buf_t* p_data)
{
    lua_pushstring(L, "media");
    lua_pushlightuserdata(L, p_data);
    return 2;
}

static int call_co_recv_klua_kmnp(klua_kmnp_t* p_mnp)
{
    assert(NULL != p_mnp);

    if (NULL == p_mnp->co_recv)
    {
        return -1;
    }

    if (0 >= klb_nlist_size(p_mnp->p_text_nlist) &&
        0 >= klb_nlist_size(p_mnp->p_binary_nlist) &&
        0 >= klb_nlist_size(p_mnp->p_media_nlist) &&
        0 == p_mnp->recv_code)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_mnp->p_coex, p_mnp->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_mnp->p_env), p_mnp->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_mnp->co_recv = NULL;

    int nargs = 0;
    klb_buf_t* p_txt = klb_nlist_pop_head(p_mnp->p_text_nlist);
    if (NULL != p_txt)
    {
        nargs = push_mnp_text_klua_kmnp(L, "text", p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
    }
    else
    {
        klb_buf_t* p_bin = klb_nlist_pop_head(p_mnp->p_binary_nlist);
        if (NULL != p_bin)
        {
            nargs = push_mnp_text_klua_kmnp(L, "binary", p_bin);
            KLB_FREE_BY(p_bin, klb_buf_unref);
        }
        else
        {
            klb_buf_t* p_media = klb_nlist_pop_head(p_mnp->p_media_nlist);
            if (NULL != p_media)
            {
                nargs = push_mnp_media_klua_kmnp(L, p_media);
            }
            else
            {
                lua_pushstring(L, "error");
                lua_pushinteger(L, p_mnp->recv_code);
                p_mnp->recv_code = 0;
                nargs = 2;
            }
        }
    }

    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_kmnp(klua_kmnp_t* p_mnp)
{
    assert(NULL != p_mnp);

    if (NULL == p_mnp->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_mnp->p_coex, p_mnp->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_mnp->p_env), p_mnp->co_recv);
    if (NULL == L)
    {
        return -1;
    }

    p_mnp->co_recv = NULL;

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int on_recv_data_klua_kmnp(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_kmnp_t* p_mnp = (klua_kmnp_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_mnp->p_text_nlist, p_data);
            call_co_recv_klua_kmnp(p_mnp);
        }
        else if (KLB_MNP_BINARY == packtype)
        {
            klb_nlist_push_tail(p_mnp->p_binary_nlist, p_data);
            call_co_recv_klua_kmnp(p_mnp);
        }
        else if (KLB_MNP_MEDIA == packtype)
        {
            klb_nlist_push_tail(p_mnp->p_media_nlist, p_data);
            call_co_recv_klua_kmnp(p_mnp);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
        }
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref);

        if (KLB_SOCKET_CONNECT != code && KLB_NETCODE_WBUF_EMPTY != code)
        {
            p_mnp->recv_code = code;
            call_co_recv_klua_kmnp(p_mnp);
        }
    }

    return 0;
}

static void setup_klua_kmnp(klua_kmnp_t* p_mnp, lua_State* L, klb_netconn_t* p_conn, bool is_client)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_mnp->L = L;
    p_mnp->co_recv = NULL;
    p_mnp->p_env = p_env;
    p_mnp->p_coex = klua_coroutine_get(p_env);

    p_mnp->p_netmulti = klua_netmulti_get(p_env);
    p_mnp->p_conn = p_conn;
    p_mnp->is_client = is_client;

    p_mnp->p_text_nlist = klb_nlist_create();
    p_mnp->p_binary_nlist = klb_nlist_create();
    p_mnp->p_media_nlist = klb_nlist_create();
    p_mnp->recv_code = 0;

    klb_netconn_set_udata(p_conn, p_mnp);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_kmnp);
}

////////////////////////////////////////

static int klua_kmnp_send_text(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    const uint8_t* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3))
    {
        p_body = (const uint8_t*)luaL_checklstring(L, 3, &body_len);
    }

    int ret = 1;
    if (NULL != p_mnp->p_conn)
    {
        ret = klb_netconn_send_text(p_mnp->p_conn, 0, (const uint8_t*)p_head, (int)head_len, p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kmnp_send_binary(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    const uint8_t* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3))
    {
        p_body = (const uint8_t*)luaL_checklstring(L, 3, &body_len);
    }

    int ret = 1;
    if (NULL != p_mnp->p_conn)
    {
        ret = klb_netconn_send_binary(p_mnp->p_conn, 0, (const uint8_t*)p_head, (int)head_len, p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_kmnp_peer(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    lua_pushstring(L, p_mnp->peer);
    return 1;
}

static int klua_kmnp_free_media(lua_State* L)
{
    to_klua_kmnp(L, 1);
    klb_buf_t* p_media = (klb_buf_t*)lua_touserdata(L, 2);

    KLB_FREE_BY(p_media, klb_buf_unref);
    return 0;
}

static int on_yield_recv_klua_kmnp(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kmnp_t* p_mnp = (klua_kmnp_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_kmnp(p_mnp);
    }

    return 0;
}

static int klua_kmnp_co_recv(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);
    klua_check_coroutine(L, "kmnp:co_recv must in coroutine!");
    assert(NULL == p_mnp->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_mnp->p_text_nlist);
    if (NULL != p_txt)
    {
        klb_nlist_pop_head(p_mnp->p_text_nlist);
        int n = push_mnp_text_klua_kmnp(L, "text", p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_bin = klb_nlist_head(p_mnp->p_binary_nlist);
    if (NULL != p_bin)
    {
        klb_nlist_pop_head(p_mnp->p_binary_nlist);
        int n = push_mnp_text_klua_kmnp(L, "binary", p_bin);
        KLB_FREE_BY(p_bin, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_media = klb_nlist_head(p_mnp->p_media_nlist);
    if (NULL != p_media)
    {
        klb_nlist_pop_head(p_mnp->p_media_nlist);
        return push_mnp_media_klua_kmnp(L, p_media);
    }

    if (0 != p_mnp->recv_code)
    {
        int code = p_mnp->recv_code;
        p_mnp->recv_code = 0;

        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    p_mnp->co_recv = L;
    return klua_coroutine_yield(p_mnp->p_coex, L, on_yield_recv_klua_kmnp, p_mnp);
}


static int klua_kmnp_set_timeout(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);
    int read_ms = 0;
    int write_ms = 0;
    int idle_ms = 0;

    if (NULL == p_mnp->p_conn)
    {
        lua_pushinteger(L, KLB_NETCODE_INVAL);
        return 1;
    }

    if (LUA_TTABLE == lua_type(L, 2))
    {
        lua_getfield(L, 2, "read");
        read_ms = (int)luaL_optinteger(L, -1, 0);
        lua_pop(L, 1);
        lua_getfield(L, 2, "write");
        write_ms = (int)luaL_optinteger(L, -1, 0);
        lua_pop(L, 1);
        lua_getfield(L, 2, "idle");
        idle_ms = (int)luaL_optinteger(L, -1, 0);
        lua_pop(L, 1);
    }
    else
    {
        read_ms = (int)luaL_optinteger(L, 2, 0);
        write_ms = (int)luaL_optinteger(L, 3, 0);
        idle_ms = (int)luaL_optinteger(L, 4, 0);
    }

    if (p_mnp->is_client)
    {
        klb_mnpclient_conn_set_timeout(p_mnp->p_conn, read_ms, write_ms, idle_ms);
    }
    else
    {
        klb_mnpserve_conn_set_timeout(p_mnp->p_conn, read_ms, write_ms, idle_ms);
    }

    lua_pushinteger(L, 0);
    return 1;
}


static int klua_kmnp_set_max_msg(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);
    int64_t max_msg = (int64_t)luaL_optinteger(L, 2, 0);

    if (NULL == p_mnp->p_conn)
    {
        lua_pushinteger(L, KLB_NETCODE_INVAL);
        return 1;
    }

    if (p_mnp->is_client)
    {
        klb_mnpclient_conn_set_max_msg(p_mnp->p_conn, max_msg);
    }
    else
    {
        klb_mnpserve_conn_set_max_msg(p_mnp->p_conn, max_msg);
    }

    lua_pushinteger(L, 0);
    return 1;
}


/// @brief 最近收包时刻 (含 PING/PONG); 供代理泵送刷新空闲计时
static int klua_kmnp_last_recv_ms(lua_State* L)
{
    klua_kmnp_t* p_mnp = to_klua_kmnp(L, 1);

    if (NULL == p_mnp->p_conn || NULL == p_mnp->p_conn->p_socket)
    {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)p_mnp->p_conn->p_socket->last_recv_tc);
    return 1;
}

static void klua_kmnp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kmnp_close },

        { "send_text",      klua_kmnp_send_text },
        { "send_binary",    klua_kmnp_send_binary },

        { "co_recv",        klua_kmnp_co_recv },
        { "free_media",     klua_kmnp_free_media },
        { "peer",           klua_kmnp_peer },
        { "set_timeout",    klua_kmnp_set_timeout },
        { "set_max_msg",    klua_kmnp_set_max_msg },
        { "last_recv_ms",   klua_kmnp_last_recv_ms },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmnp_close },
        { "__close",        klua_kmnp_close },
        { "__tostring",     klua_kmnp_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMNP_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

////////////////////////////////////////
// kmnp client

/// @brief 从 opts.crypto 解析连接加码策略; 无 crypto 字段则 *p_want=false
static int parse_opts_crypto_klua_kmnp(lua_State* L, int opts_idx,
    bool* p_want, uint32_t* p_format, uint32_t* p_key_size,
    const uint8_t** pp_key, uint32_t* p_rekey_after)
{
    size_t key_len = 0;
    const char* p_key_str = NULL;
    bool has_key = false;
    bool has_key_size = false;

    *p_want = false;
    *p_format = KLB_MNP_CRYPTO_NULL;
    *p_key_size = 0;
    *pp_key = NULL;
    *p_rekey_after = 0;

    if (!klua_is_table(L, opts_idx))
    {
        return 0;
    }

    lua_getfield(L, opts_idx, "crypto");
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return 0;
    }

    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "format");
    if (lua_isstring(L, -1))
    {
        if (0 != klb_mnp_crypto_format_from_name(lua_tostring(L, -1), p_format))
        {
            lua_pop(L, 2);
            return 1;
        }
    }
    else if (lua_isnumber(L, -1))
    {
        *p_format = (uint32_t)lua_tointeger(L, -1);
    }
    else
    {
        lua_pop(L, 2);
        return 1;
    }
    lua_pop(L, 1);

    if (KLB_MNP_CRYPTO_NULL == *p_format)
    {
        lua_pop(L, 1);
        return 1;
    }

    lua_getfield(L, -1, "key");
    if (!lua_isnil(L, -1))
    {
        if (!lua_isstring(L, -1))
        {
            lua_pop(L, 2);
            return 1;
        }

        p_key_str = lua_tolstring(L, -1, &key_len);
        has_key = true;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "key_size");
    if (!lua_isnil(L, -1))
    {
        if (!lua_isnumber(L, -1))
        {
            lua_pop(L, 2);
            return 1;
        }

        *p_key_size = (uint32_t)lua_tointeger(L, -1);
        has_key_size = true;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "rekey_after");
    if (!lua_isnil(L, -1))
    {
        if (!lua_isnumber(L, -1))
        {
            lua_pop(L, 2);
            return 1;
        }

        *p_rekey_after = (uint32_t)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pop(L, 1);

    if (has_key)
    {
        if (0 == key_len || KLB_MNP_CRYPTO_KEY_SIZE_MAX < key_len)
        {
            return 1;
        }

        if (has_key_size && *p_key_size != (uint32_t)key_len)
        {
            return 1;
        }

        *p_key_size = (uint32_t)key_len;
        *pp_key = (const uint8_t*)p_key_str;
    }
    else if (!has_key_size || 0 == *p_key_size)
    {
        return 1;
    }

    *p_want = true;
    return 0;
}


static int klua_kmnp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    int code = 0;
    bool tls = false;
    bool crypto_want = false;
    uint32_t crypto_format = KLB_MNP_CRYPTO_NULL;
    uint32_t crypto_key_size = 0;
    const uint8_t* p_crypto_key = NULL;
    uint32_t crypto_rekey_after = 0;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);

        if (0 != parse_opts_crypto_klua_kmnp(L, 3, &crypto_want, &crypto_format,
            &crypto_key_size, &p_crypto_key, &crypto_rekey_after))
        {
            return push_err_klua_kmnp(L, KLB_NETCODE_INVAL);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_kmnp(L, KLB_NETCODE_INVAL);
    }

    p_conn = klb_mnpclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_conn)
    {
        return push_err_klua_kmnp(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    if (crypto_want)
    {
        if (0 != klb_mnpclient_conn_set_crypto(p_conn, crypto_format,
            p_crypto_key, crypto_key_size, crypto_rekey_after))
        {
            klb_mnpclient_conn_free(p_conn);
            return push_err_klua_kmnp(L, KLB_NETCODE_INVAL);
        }
    }

    {
        klua_kmnp_t* p_mnp = new_klua_kmnp(L);
        setup_klua_kmnp(p_mnp, L, p_conn, true);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// kmnp listen

#define KLUA_KMNP_LISTEN_HANDLE         "KMNP_LISTEN_HANDLE*"       ///< Lua meta标示


/// @struct klua_kmnp_listen_t
/// @brief  kmnp 服务监听
typedef struct klua_kmnp_listen_t_
{
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< co_accept协程

        klua_env_t*             p_env;          ///< Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };

    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待 accept 的 socket
    };
}klua_kmnp_listen_t;


////////////////////////////////////////
static klua_kmnp_listen_t* new_klua_kmnp_listen(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = (klua_kmnp_listen_t*)lua_newuserdata(L, sizeof(klua_kmnp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_kmnp_listen_t));
    luaL_setmetatable(L, KLUA_KMNP_LISTEN_HANDLE);
    return p_listen;
}

static klua_kmnp_listen_t* to_klua_kmnp_listen(lua_State* L, int index)
{
    klua_kmnp_listen_t* p_listen = (klua_kmnp_listen_t*)luaL_checkudata(L, index, KLUA_KMNP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'kmnp listen' expected");
    return p_listen;
}

static int klua_kmnp_listen_tostring(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);

    lua_pushfstring(L, "kmnp listen:%p", p_listen);
    return 1;
}

static int klua_kmnp_listen_close(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);

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

static klua_kmnp_t* new_klua_kmnp_by_socket(lua_State* L, klb_netlisten_socket_t* p_listen_socket)
{
    klb_socket_t* p_socket = klb_socket_async_create(p_listen_socket->fd);
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = klb_mnpserve_conn_create(p_netmulti, p_socket);
    klua_kmnp_t* p_mnp = new_klua_kmnp(L);

    setup_klua_kmnp(p_mnp, L, p_conn, false);
    klua_netlisten_format_peer_in(p_mnp->peer, (int)sizeof(p_mnp->peer), &p_listen_socket->addr);
    return p_mnp;
}


/// @brief 由已探测 sock 创建 MNP 服务连接; 可选 `{cert,key}` 供 TLS(mnps)
static int klua_kmnp_new_serve(lua_State* L)
{
    klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)lua_touserdata(L, 1);

    if (NULL == p_listen_socket)
    {
        return push_err_klua_kmnp(L, KLB_NETCODE_INVAL);
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

    struct sockaddr_in peer_addr = p_listen_socket->addr;
    klb_socket_fd fd = p_listen_socket->fd;
    klb_socket_t* p_socket = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_kmnp_t* p_mnp = NULL;

    KLB_FREE(p_listen_socket);

    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(
            p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnp(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnp(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_kmnp(L, KLB_NETCODE_NOMEM);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    p_conn = klb_mnpserve_conn_create(p_netmulti, p_socket);

    if (has_ping)
    {
        klb_mnpserve_conn_set_ping(p_conn, ping_interval_ms, ping_timeout_ms);
    }

    p_mnp = new_klua_kmnp(L);
    setup_klua_kmnp(p_mnp, L, p_conn, false);
    klua_netlisten_format_peer_in(p_mnp->peer, (int)sizeof(p_mnp->peer), &peer_addr);
    return 1;
}

static int call_co_accept_klua_kmnp_listen(klua_kmnp_listen_t* p_listen)
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
    new_klua_kmnp_by_socket(L, p_listen_socket);
    KLB_FREE(p_listen_socket);

    lua_pushstring(L, "ok");

    int status = lua_pcall(L, 2, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_accept_end_klua_kmnp_listen(klua_kmnp_listen_t* p_listen)
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

static int on_accept_klua_kmnp_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_kmnp_listen_t* p_listen = (klua_kmnp_listen_t*)p_conn->p_udata;

    klb_netlisten_socket_t* p_tmp = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);
    p_tmp->fd = fd;
    p_tmp->addr = *p_addr;
    p_tmp->tls = tls;

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);
    call_co_accept_klua_kmnp_listen(p_listen);

    return 0;
}

static int on_yield_accept_klua_kmnp_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kmnp_listen_t* p_listen = (klua_kmnp_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_kmnp_listen(p_listen);
    }

    return 0;
}

static int klua_kmnp_listen_co_accept(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);
    klua_check_coroutine(L, "kmnp.listen:co_accept must in coroutine!");
    assert(NULL == p_listen->co_accept);

    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klb_netlisten_socket_t* p_listen_socket = (klb_netlisten_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);
        new_klua_kmnp_by_socket(L, p_listen_socket);
        KLB_FREE(p_listen_socket);

        lua_pushstring(L, "ok");
        return 2;
    }

    p_listen->co_accept = L;
    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_kmnp_listen, p_listen);
}

static int klua_kmnp_listen_port(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_kmnp_listen_local_addr(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_kmnp_listen_native_fd(lua_State* L)
{
    klua_kmnp_listen_t* p_listen = to_klua_kmnp_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

static void klua_kmnp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kmnp_listen_close },

        { "co_accept",      klua_kmnp_listen_co_accept },
        { "port",           klua_kmnp_listen_port },
        { "local_addr",     klua_kmnp_listen_local_addr },
        { "native_fd",      klua_kmnp_listen_native_fd },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmnp_listen_close },
        { "__close",        klua_kmnp_listen_close },
        { "__tostring",     klua_kmnp_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMNP_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static int klua_kmnp_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_kmnp_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_kmnp(L, rc);
        }
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_kmnp(L, KLB_NETCODE_INVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_kmnp_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return push_err_klua_kmnp(L, rc);
    }

    p_listen = new_klua_kmnp_listen(L);

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

int klua_open_kmnp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",        klua_kmnp_connect },
        { "connect_rpc",    klua_kmnpclient_connect_rpc },

        { "listen",         klua_kmnp_listen },
        { "listen_rpc",     klua_kmnpserverpc_listen },
        { "new_serve",      klua_kmnp_new_serve },
        { "new_serve_rpc",  klua_kmnpserverpc_new_serve },

        { NULL,             NULL }
    };

    luaL_newlib(L, lib);

    klua_kmnp_createmeta(L);
    klua_kmnp_listen_createmeta(L);

    klua_kmnpclientrpc_createmeta(L);
    klua_kmnpserverpc_createmeta(L);
    klua_kmnpserverpc_listen_createmeta(L);

    return 1;
}

// end
