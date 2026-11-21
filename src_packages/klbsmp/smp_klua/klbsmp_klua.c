// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_crypto.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_seri.h"
#include "klbsmp/klbsmp.h"
#include "klbsmp/smp_klua/klbsmp_serve_klua.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klbsmp/core/klb_smpclient_conn.h"
#include "klbsmp/core/klb_smpclientrpc_conn.h"
#include "klbsmp/core/klb_smprpcer.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// ksmp client


#define KLUA_KSMPCLIENT_HANDLE          "KSMPCLIENT_HANDLE*"        ///< Lua meta标示


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_ksmp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @struct klua_ksmpclient_t
/// @brief  KLua-SMP客户端
typedef struct klua_ksmpclient_t_
{
    // Lua 相关
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
        klb_netconn_t*          p_smp_conn;     ///< smp 客户端连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
        klb_nlist_t*            p_binary_nlist; ///< 待读取的 二进制列表; 存储 klb_buf_t*
        klb_nlist_t*            p_media_nlist;  ///< 待读取的 媒体列表; 存储 klb_buf_t*

        int                     recv_code;      ///< 非0时, 下一次 co_recv 返回 error
    };
}klua_ksmpclient_t;


////////////////////////////////////////
static klua_ksmpclient_t* new_klua_ksmpclient(lua_State* L)
{
    klua_ksmpclient_t* p_client = (klua_ksmpclient_t*)lua_newuserdata(L, sizeof(klua_ksmpclient_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_ksmpclient_t));
    luaL_setmetatable(L, KLUA_KSMPCLIENT_HANDLE);
    return p_client;
}

static klua_ksmpclient_t* to_klua_ksmpclient(lua_State* L, int index)
{
    klua_ksmpclient_t* p_client = (klua_ksmpclient_t*)luaL_checkudata(L, index, KLUA_KSMPCLIENT_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'ksmpclient' expected");
    return p_client;
}

static int klua_ksmpclient_tostring(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);

    lua_pushfstring(L, "ksmpclient:%p", p_client);
    return 1;
}

static int call_co_recv_end_klua_ksmpclient(klua_ksmpclient_t* p_client);

static int klua_ksmpclient_close(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);

    /* wake pending co_recv before teardown (avoid hmap assert on destroy) */
    call_co_recv_end_klua_ksmpclient(p_client);

    // 清空 文本
    if (NULL != p_client->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    if (NULL != p_client->p_binary_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_binary_nlist))
        {
            klb_buf_t* p_bin = klb_nlist_pop_head(p_client->p_binary_nlist);
            KLB_FREE_BY(p_bin, klb_buf_unref);
        }
    }

    // 清空视频
    if (NULL != p_client->p_media_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_media_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_media_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_client->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_binary_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_media_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_smp_conn, klb_netconn_destroy);

    return 0;
}

////////////////////////////////////////

static int push_text_klua_ksmpclient(lua_State* L, const char* p_kind, klb_buf_t* p_data)
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

static int push_media_klua_ksmpclient(lua_State* L, klb_buf_t* p_data)
{
    lua_pushstring(L, "media");
    lua_pushlightuserdata(L, p_data);
    return 2;
}

static int pop_recv_klua_ksmpclient(lua_State* L, klua_ksmpclient_t* p_client)
{
    klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
    if (NULL != p_txt)
    {
        int n = push_text_klua_ksmpclient(L, "text", p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_bin = klb_nlist_pop_head(p_client->p_binary_nlist);
    if (NULL != p_bin)
    {
        int n = push_text_klua_ksmpclient(L, "binary", p_bin);
        KLB_FREE_BY(p_bin, klb_buf_unref);
        return n;
    }

    klb_buf_t* p_media = klb_nlist_pop_head(p_client->p_media_nlist);
    if (NULL != p_media)
    {
        return push_media_klua_ksmpclient(L, p_media);
    }

    if (0 != p_client->recv_code)
    {
        int code = p_client->recv_code;
        p_client->recv_code = 0;

        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    return 0;
}

static bool has_recv_klua_ksmpclient(klua_ksmpclient_t* p_client)
{
    if (0 < klb_nlist_size(p_client->p_text_nlist) ||
        0 < klb_nlist_size(p_client->p_binary_nlist) ||
        0 < klb_nlist_size(p_client->p_media_nlist) ||
        0 != p_client->recv_code)
    {
        return true;
    }

    return false;
}

// 调用Lua协程
static int call_co_recv_klua_ksmpclient(klua_ksmpclient_t* p_client)
{
    assert(NULL != p_client);

    if (NULL == p_client->co_recv || !has_recv_klua_ksmpclient(p_client))
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

    int nargs = pop_recv_klua_ksmpclient(L, p_client);
    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

static int call_co_recv_end_klua_ksmpclient(klua_ksmpclient_t* p_client)
{
    assert(NULL != p_client);

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

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_ksmpclient(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpclient_t* p_client = p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_client->p_text_nlist, p_data);
            call_co_recv_klua_ksmpclient(p_client);
        }
        else if (KLB_MNP_BINARY == packtype)
        {
            klb_nlist_push_tail(p_client->p_binary_nlist, p_data);
            call_co_recv_klua_ksmpclient(p_client);
        }
        else if (KLB_MNP_MEDIA == packtype)
        {
            klb_nlist_push_tail(p_client->p_media_nlist, p_data);
            call_co_recv_klua_ksmpclient(p_client);
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
        p_client->recv_code = code;
        call_co_recv_klua_ksmpclient(p_client);
    }

    return 0;
}

////////////////////////////////////////

static int klua_ksmpclient_send_payload(lua_State* L, bool is_binary)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    const uint8_t* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3))
    {
        p_body = (const uint8_t*)luaL_checklstring(L, 3, &body_len);
    }

    int ret = 1;
    if (NULL != p_client->p_smp_conn)
    {
        if (is_binary)
        {
            ret = klb_netconn_send_binary(p_client->p_smp_conn, 0,
                (const uint8_t*)p_head, (int)head_len, p_body, (int)body_len);
        }
        else
        {
            ret = klb_netconn_send_text(p_client->p_smp_conn, 0,
                (const uint8_t*)p_head, (int)head_len, p_body, (int)body_len);
        }
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_ksmpclient_send_text(lua_State* L)
{
    return klua_ksmpclient_send_payload(L, false);
}

static int klua_ksmpclient_send_binary(lua_State* L)
{
    return klua_ksmpclient_send_payload(L, true);
}

static int klua_ksmpclient_send_media(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);
    klb_buf_t* p_buf = (klb_buf_t*)lua_touserdata(L, 2);

    int ret = 1;
    if (NULL != p_buf && NULL != p_client->p_smp_conn)
    {
        ret = klb_netconn_send_media(p_client->p_smp_conn, p_buf);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_ksmpclient_free_media(lua_State* L)
{
    to_klua_ksmpclient(L, 1);
    klb_buf_t* p_media = (klb_buf_t*)lua_touserdata(L, 2);

    KLB_FREE_BY(p_media, klb_buf_unref);
    return 0;
}

/// @brief 发送背压: 写缓存未发送字节数
/// @note Lua: n = ksmpclient:buffered_amount()
static int klua_ksmpclient_buffered_amount(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);
    int64_t n = 0;

    if (NULL != p_client->p_smp_conn)
    {
        n = klb_smpclient_wbuf_size(p_client->p_smp_conn);
    }

    lua_pushinteger(L, (lua_Integer)n);
    return 1;
}

static int on_yield_recv_klua_ksmpclient(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpclient_t* p_client = (klua_ksmpclient_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_ksmpclient(p_client);
    }

    return 0;
}

static int klua_ksmpclient_co_recv(lua_State* L)
{
    klua_ksmpclient_t* p_client = to_klua_ksmpclient(L, 1);
    klua_check_coroutine(L, "ksmpclient:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    if (has_recv_klua_ksmpclient(p_client))
    {
        return pop_recv_klua_ksmpclient(L, p_client);
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_ksmpclient, p_client);
}

////////////////////////////////////////


static int klua_ksmpclient_set_timeout(lua_State* L)
{
    klua_ksmpclient_t* p = to_klua_ksmpclient(L, 1);
    int r = (int)luaL_optinteger(L, 2, 0);
    int w = (int)luaL_optinteger(L, 3, 0);
    int i = (int)luaL_optinteger(L, 4, 0);
    if (NULL == p->p_smp_conn) { lua_pushinteger(L, KLB_NETCODE_EINVAL); return 1; }
    klb_smpclient_conn_set_timeout(p->p_smp_conn, r, w, i);
    lua_pushinteger(L, 0);
    return 1;
}

static int klua_ksmpclient_set_max_msg(lua_State* L)
{
    klua_ksmpclient_t* p = to_klua_ksmpclient(L, 1);
    int64_t m = (int64_t)luaL_optinteger(L, 2, 0);
    if (NULL == p->p_smp_conn) { lua_pushinteger(L, KLB_NETCODE_EINVAL); return 1; }
    klb_smpclient_conn_set_max_msg(p->p_smp_conn, m);
    lua_pushinteger(L, 0);
    return 1;
}


/// @brief 最近收包时刻 (含 PING/PONG); 供代理泵送刷新空闲计时
static int klua_ksmpclient_last_recv_ms(lua_State* L)
{
    klua_ksmpclient_t* p = to_klua_ksmpclient(L, 1);

    if (NULL == p->p_smp_conn || NULL == p->p_smp_conn->p_socket)
    {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)p->p_smp_conn->p_socket->last_recv_tc);
    return 1;
}

static void klua_ksmpclient_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",             klua_ksmpclient_close },

        { "send_text",              klua_ksmpclient_send_text },
        { "send_binary",            klua_ksmpclient_send_binary },
        { "send_media",             klua_ksmpclient_send_media },
        { "buffered_amount",        klua_ksmpclient_buffered_amount },

        { "co_recv",                klua_ksmpclient_co_recv },
        { "free_media",             klua_ksmpclient_free_media },
        { "set_timeout",            klua_ksmpclient_set_timeout },
        { "set_max_msg",            klua_ksmpclient_set_max_msg },
        { "last_recv_ms",           klua_ksmpclient_last_recv_ms },

        { NULL,                     NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",                NULL },  /* place holder */
        { "__gc",                   klua_ksmpclient_close },
        { "__close",                klua_ksmpclient_close },
        { "__tostring",             klua_ksmpclient_tostring },
        { NULL,                     NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPCLIENT_HANDLE);/* metatable for KLUA_KSMPCLIENT_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}


////////////////////////////////////////
// ksmp client

/// @brief 从 opts.crypto 解析连接加码策略; 无 crypto 字段则 *p_want=false
/// @return int 0.成功; 非0.字段非法
static int parse_opts_crypto_klua_ksmpclient(lua_State* L, int opts_idx,
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

    // format: string 名或 number
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

    lua_pop(L, 1); // crypto table

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


static int klua_ksmpclient_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号
    int code = 0;
    bool tls = false;                                               ///< @3. [可选] {tls=true, crypto={...}}
    bool crypto_want = false;
    uint32_t crypto_format = KLB_MNP_CRYPTO_NULL;
    uint32_t crypto_key_size = 0;
    const uint8_t* p_crypto_key = NULL;
    uint32_t crypto_rekey_after = 0;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_smp_conn = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);

        if (0 != parse_opts_crypto_klua_ksmpclient(L, 3, &crypto_want, &crypto_format,
            &crypto_key_size, &p_crypto_key, &crypto_rekey_after))
        {
            return push_err_klua_ksmp(L, KLB_NETCODE_EINVAL);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_ksmp(L, KLB_NETCODE_EINVAL);
    }

    p_smp_conn = klb_smpclient_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_smp_conn)
    {
        return push_err_klua_ksmp(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    if (crypto_want)
    {
        if (0 != klb_smpclient_conn_set_crypto(p_smp_conn, crypto_format,
            p_crypto_key, crypto_key_size, crypto_rekey_after))
        {
            klb_smpclient_conn_free(p_smp_conn);
            return push_err_klua_ksmp(L, KLB_NETCODE_EINVAL);
        }
    }

    {
        // 
        klua_ksmpclient_t* p_client = new_klua_ksmpclient(L);       ///< #1. meta: klua_ksmpclient_createmeta

        p_client->L = L;
        p_client->co_recv = NULL;
        p_client->p_env = p_env;
        p_client->p_coex = klua_coroutine_get(p_env);

        p_client->p_netmulti = p_netmulti;
        p_client->p_smp_conn = p_smp_conn;

        p_client->p_text_nlist = klb_nlist_create();
        p_client->p_binary_nlist = klb_nlist_create();
        p_client->p_media_nlist = klb_nlist_create();
        p_client->recv_code = 0;

        // 绑定数据接收
        p_smp_conn->p_udata = p_client;
        klb_netconn_bind_recv_data(p_smp_conn, on_recv_data_klua_ksmpclient);
    }

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// ksmp client rpc

#define KLUA_KSMPCLIENTRPC_HANDLE               "KSMPCLIENTRPC_HANDLE*"       ///< Lua meta标示


/// @struct klua_ksmpclientrpc_t
/// @brief  SMP-RPC客户端
typedef struct klua_ksmpclientrpc_t_
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
        lua_State*              co_recv;        ///< co_recv接收数据协程
        lua_State*              co_call;        ///< co_call协程
        lua_State*              co_wait;        ///< co_wait等待 协程
    };

    // 通用的 RPC数据
    struct
    {
        klb_nlist_t*            p_rpc_nlist;    ///< 待读取的 RPC数据列表; 存储 klb_buf_t*

        klb_mnp_rpc_t           rpc;            ///< 上一个RPC信息
        klb_buf_t*              p_rpc_buf;      ///< RPC数据
    };

    // KLB_MNP_RPC_RESPONSE 数据
    struct
    {
        int                     req_sequence;   ///< 请求的序列号

        klb_nlist_t*            p_res_nlist;    ///< 待读取的 RPC - RESPONSE 数据列表; 存储 klb_buf_t*

        klb_mnp_rpc_t           res;            ///< 上一个RPC信息
        klb_buf_t*              p_res_buf;      ///< RPC数据
    };

    // 网络相关
    struct
    {
        klb_netconn_t*          p_client_conn;  ///< RPC 客户端连接
    };
}klua_ksmpclientrpc_t;


////////////////////////////////////////
static klua_ksmpclientrpc_t* new_klua_ksmpclientrpc(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = (klua_ksmpclientrpc_t*)lua_newuserdata(L, sizeof(klua_ksmpclientrpc_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_ksmpclientrpc_t));
    luaL_setmetatable(L, KLUA_KSMPCLIENTRPC_HANDLE);

    // 初始化 只读 参数
    {
        p_client->L = L;
        p_client->p_env = klua_env_get_by_L(L);
        p_client->p_coex = klua_coroutine_get(p_client->p_env);
        p_client->p_netmulti = klua_netmulti_get(p_client->p_env);
    }

    // 初始化 其他参数
    {
        p_client->co_recv = NULL;
        p_client->co_call = NULL;
        p_client->co_wait = NULL;

        p_client->p_rpc_nlist = klb_nlist_create();
        p_client->p_res_nlist = klb_nlist_create();

        p_client->req_sequence = 99;
    }

    return p_client;
}

static klua_ksmpclientrpc_t* to_klua_ksmpclientrpc(lua_State* L, int index)
{
    klua_ksmpclientrpc_t* p_client = (klua_ksmpclientrpc_t*)luaL_checkudata(L, index, KLUA_KSMPCLIENTRPC_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'ksmpclientrpc' expected");
    return p_client;
}

static int klua_ksmpclientrpc_tostring(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);
    lua_pushfstring(L, "ksmpclientrpc:%p", p_client);
    return 1;
}

static void clear_buf_nlist_klua_ksmpclientrpc(klb_nlist_t* p_buf_nlist)
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

static int klua_ksmpclientrpc_close(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);

    // 关闭连接
    KLB_FREE_BY(p_client->p_client_conn, klb_smpclientrpc_conn_free);

    // 清空 RPC数据
    clear_buf_nlist_klua_ksmpclientrpc(p_client->p_rpc_nlist);
    clear_buf_nlist_klua_ksmpclientrpc(p_client->p_res_nlist);

    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_rpc_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);
    KLB_FREE_BY(p_client->p_res_nlist, klb_nlist_destroy);

    return 0;
}

////////////////////////////////////////

// 放入/更新 RPC 数据
static void update_rpc_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_rpc_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->rpc, p_rpc, sizeof(klb_mnp_rpc_t));

        p_client->p_rpc_buf = p_buf;
    }
}

static int push_rpc_body_klua_ksmpclientrpc(lua_State* L, klb_buf_t* p_buf)
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
static int call_co_recv_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_recv && 0 < klb_nlist_size(p_client->p_rpc_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行
        klb_buf_t* p_buf = klb_nlist_pop_head(p_client->p_rpc_nlist);
        int num = push_rpc_body_klua_ksmpclientrpc(L, p_buf);
        update_rpc_klua_ksmpclientrpc(p_client, p_buf);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程
static int call_co_recv_end_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_recv)
    {
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        lua_pushnil(L); // #1 空

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 放入/更新 RPC 数据
static void update_res_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client, klb_buf_t* p_buf)
{
    KLB_FREE_BY(p_client->p_res_buf, klb_buf_unref);

    if (NULL != p_buf)
    {
        klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_buf->p_buf + p_buf->start);
        memcpy(&p_client->res, p_rpc, sizeof(klb_mnp_rpc_t));

        p_client->p_res_buf = p_buf;
    }
}

// 调用Lua协程
static int call_co_res_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_call)
    {
        // 检查数据中, 是否有对应的sequence
        while (0 < klb_nlist_size(p_client->p_res_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_head(p_client->p_res_nlist);
            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_tmp->p_buf + p_tmp->start);

            if (KLB_MNP_RPC_RESPONSE == p_rpc->method && p_client->req_sequence == p_rpc->sequence)
            {
                break; // 对应上了, 保持列表首个, 即是我们需要的数据
            }

            // 未对应上的直接释放
            klb_nlist_pop_head(p_client->p_res_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);
        }

        if (klb_nlist_size(p_client->p_res_nlist) <= 0)
        {
            return -1; // 未处理
        }

        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_call));
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_call);
        if (NULL == L) return -1; // 未处理

        p_client->co_call = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行
        klb_buf_t* p_buf = klb_nlist_pop_head(p_client->p_res_nlist);
        int num = push_rpc_body_klua_ksmpclientrpc(L, p_buf);
        update_res_klua_ksmpclientrpc(p_client, p_buf);

        int status = lua_pcall(L, num, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程
static int call_co_res_end_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_call)
    {
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_call));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_call);
        if (NULL == L) return -1; // 未处理

        p_client->co_call = NULL; // 清空

        lua_pushnil(L); // #1 空

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程
static int call_co_wait_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    assert(NULL != p_client);

    if (NULL != p_client->co_wait)
    {
        assert(0 == klua_coroutine_debug_check(p_client->p_coex, p_client->co_wait));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_wait);
        if (NULL == L) return -1; // 未处理

        p_client->co_wait = NULL; // 清空

        lua_pushnil(L); // #1 空

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_ksmpclientrpc(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_ksmpclientrpc_t* p_client = p_conn->p_udata;

    if (KLB_NETCODE_OK == code)
    {
        bool need_free = true;

        if (KLB_MNP_RPC_LUA == packtype || 
            KLB_MNP_RPC_JSON == packtype)
        {
            klb_mnp_rpc_t* p_rpc = (klb_mnp_rpc_t*)(p_data->p_buf + p_data->start);

            if (KLB_MNP_RPC_POST == p_rpc->method ||
                KLB_MNP_RPC_NOTIFY == p_rpc->method)
            {
                need_free = false;

                klb_nlist_push_tail(p_client->p_rpc_nlist, p_data);
                call_co_recv_ksmpclientrpc(p_client);
            }
            else if (KLB_MNP_RPC_RESPONSE == p_rpc->method && p_client->req_sequence == p_rpc->sequence)
            {
                need_free = false;

                klb_nlist_push_tail(p_client->p_res_nlist, p_data);
                call_co_res_ksmpclientrpc(p_client);
            }
        }

        if (need_free) { KLB_FREE_BY(p_data, klb_buf_unref); }
    }
    else if (KLB_NETCODE_WBUF_EMPTY == code)
    {
        call_co_wait_klua_ksmpclientrpc(p_client);
    }
    else if (KLB_SOCKET_CONNECT != code)
    {
        call_co_recv_end_klua_ksmpclientrpc(p_client);
        call_co_res_end_klua_ksmpclientrpc(p_client);
        call_co_wait_klua_ksmpclientrpc(p_client);
    }

    return 0;
}

////////////////////////////////////////

/// @brief 获取下一个序号
static int get_next_sequence_klua_ksmpclientrpc(klua_ksmpclientrpc_t* p_client)
{
    // 值域范围: [0, 2^20=1048576), 参见: klb_mnp_rpc_t.sequence
    // 这里取范围: [1000, 1000000]
    int sequence = p_client->req_sequence + 1;

    if (sequence < 1000) { sequence = 1000; }
    if (1000000 < sequence) { sequence = 1000; }

    return sequence;
}

/// @brief 发送 RPC 数据
/// @param [in]     method              RPC方法: eg. KLB_MNP_RPC_POST, KLB_MNP_RPC_NOTIFY, KLB_MNP_RPC_REQUEST
/// @return int 错误码
static int klua_ksmpclientrpc_send(lua_State* L, klb_mnp_rpc_method_e method, klua_ksmpclientrpc_t** p_out, int* p_sequence)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self
    int rpctype = (int)luaL_checkinteger(L, 2);                     ///< @2 RPC类型: KLB_MNP_RPC_LUA / KLB_MNP_RPC_JSON

    int sequence = (KLB_MNP_RPC_REQUEST == method) ? get_next_sequence_klua_ksmpclientrpc(p_client) : 0;

    int ret = 1;

    if (NULL == p_client->p_client_conn)
    {
        if (NULL != p_out)
        {
            *p_out = p_client;
        }

        if (NULL != p_sequence)
        {
            *p_sequence = sequence;
        }

        return ret;
    }

    if (KLB_MNP_RPC_LUA == rpctype)
    {
        klb_buf_t* p_data = klua_seri_map_binary_pack(L, 2);        ///< @3 ~ @N 参数

        ret = klb_smpclientrpc_conn_send_buf(p_client->p_client_conn, KLB_MNP_RPC_LUA, method, sequence, p_data);

        if (0 != ret) { KLB_FREE_BY(p_data, klb_buf_unref); }
    }
    else if (KLB_MNP_RPC_JSON == rpctype)
    {
        size_t json_len = 0;
        const char* p_json = luaL_checklstring(L, 3, &json_len);    ///< @3 JSON数据

        ret = klb_smpclientrpc_conn_send(p_client->p_client_conn, KLB_MNP_RPC_JSON, method, sequence, (const uint8_t*)p_json, (int)json_len);
    }

    if (NULL != p_out) { *p_out = p_client; };
    if (NULL != p_sequence) { *p_sequence = sequence; }

    return ret;
}

// POST RPC, 无需等待回复
static int klua_ksmpclientrpc_post(lua_State* L)
{
    int ret = klua_ksmpclientrpc_send(L, KLB_MNP_RPC_POST, NULL, NULL);

    lua_pushinteger(L, ret);            ///< #1 错误码
    return 1;
}

// NOTIFY RPC, 无需等待回复
static int klua_ksmpclientrpc_notify(lua_State* L)
{
    int ret = klua_ksmpclientrpc_send(L, KLB_MNP_RPC_NOTIFY, NULL, NULL);

    lua_pushinteger(L, ret);            ///< #1 错误码
    return 1;
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_ksmpclientrpc(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_ksmpclientrpc_t* p_client = (klua_ksmpclientrpc_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_ksmpclientrpc(p_client);
        call_co_res_end_klua_ksmpclientrpc(p_client);
        call_co_wait_klua_ksmpclientrpc(p_client);
    }

    return 0;
}

// co recv 数据
static int klua_ksmpclientrpc_co_recv(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self
    klua_check_coroutine(L, "ksmpclientrpc:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    if (0 < klb_nlist_size(p_client->p_rpc_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_client->p_rpc_nlist);
        int num = push_rpc_body_klua_ksmpclientrpc(L, p_buf);

        update_rpc_klua_ksmpclientrpc(p_client, p_buf);

        return num;
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_ksmpclientrpc, p_client);
}

// co call 数据
static int klua_ksmpclientrpc_co_call(lua_State* L)
{   
    klua_check_coroutine(L, "ksmpclientrpc:co_call must in coroutine!");

    klua_ksmpclientrpc_t* p_client = NULL;
    int sequence = 0;
    int ret = klua_ksmpclientrpc_send(L, KLB_MNP_RPC_REQUEST, &p_client, &sequence);

    if (0 == ret)
    {
        assert(NULL == p_client->co_call);

        p_client->co_call = L;
        p_client->req_sequence = sequence; // 更新请求序号
        return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_ksmpclientrpc, p_client);
    }

    lua_pushnil(L);
    return 1;
}

// co wait 等待 待发送的数据放入 系统socket
static int klua_ksmpclientrpc_co_wait(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self
    klua_check_coroutine(L, "ksmpclientrpc:co_wait must in coroutine!");
    assert(NULL == p_client->co_wait);

    // 先检查是否还有数据 需要 写
    if (klb_smpclientrpc_wbuf_is_empty(p_client->p_client_conn))
    {
        lua_pushnil(L);
        return 1;
    }

    p_client->co_wait = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_ksmpclientrpc, p_client);
}

// 获取当前 状态
static int klua_ksmpclientrpc_status(lua_State* L)
{
    klua_ksmpclientrpc_t* p_client = to_klua_ksmpclientrpc(L, 1);   ///< @1 self

    lua_newtable(L);

    {
        klua_setfield_string(L, "rpctype", klb_smprpcer_to_rpctype_string(p_client->rpc.rpctype)); // RPC数据类型
        klua_setfield_string(L, "method", klb_smprpcer_to_method_string(p_client->rpc.method)); // 方法
        klua_setfield_integer(L, "sequence", p_client->rpc.sequence); // 序号

        klua_setfield_string(L, "res_rpctype", klb_smprpcer_to_rpctype_string(p_client->res.rpctype)); // RPC数据类型
        klua_setfield_string(L, "res_method", klb_smprpcer_to_method_string(p_client->res.method)); // 方法
        klua_setfield_integer(L, "res_sequence", p_client->res.sequence); // 序号
    }

    return 1; ///< #1 table
}

////////////////////////////////////////
// createmeta

static void klua_ksmpclientrpc_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ksmpclientrpc_close },             ///< 关闭连接

        { "post",           klua_ksmpclientrpc_post },              ///< 提交RPC数据
        { "notify",         klua_ksmpclientrpc_notify },            ///< 通知RPC数据

        { "co_recv",        klua_ksmpclientrpc_co_recv },           ///< 接收数据
        { "co_call",        klua_ksmpclientrpc_co_call },           ///< call 远程函数
        { "co_wait",        klua_ksmpclientrpc_co_wait },           ///< 等待(等待 待发送的数据放入 系统socket)

        { "status",         klua_ksmpclientrpc_status },            ///< 获取当前 状态

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_ksmpclientrpc_close },
        { "__close",         klua_ksmpclientrpc_close },
        { "__tostring",      klua_ksmpclientrpc_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KSMPCLIENTRPC_HANDLE);/* metatable for KLUA_KSMPCLIENTRPC_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

////////////////////////////////////////
// ksmp client rpc

static int klua_ksmpclient_connect_rpc(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号
    int code = 0;
    bool tls = false;                                               ///< @3. [可选] {tls=true}
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klb_netconn_t* p_client_conn = NULL;
    klua_ksmpclientrpc_t* p_client = NULL;

    if (klua_is_table(L, 3))
    {
        lua_getfield(L, 3, "tls");
        if (!lua_isnil(L, -1))
        {
            tls = klua_check_option_boolean(L, -1, false);
        }
        lua_pop(L, 1);
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        return push_err_klua_ksmp(L, KLB_NETCODE_EINVAL);
    }

    p_client_conn = klb_smpclientrpc_connect(p_netmulti, p_host, port, tls, &code);
    if (NULL == p_client_conn)
    {
        return push_err_klua_ksmp(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    p_client = new_klua_ksmpclientrpc(L);     ///< #1. meta: klua_ksmpclientrpc_createmeta

    klb_netconn_set_udata(p_client_conn, p_client);
    klb_netconn_bind_recv_data(p_client_conn, on_recv_data_klua_ksmpclientrpc);
    p_client->p_client_conn = p_client_conn;

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// "ksmp"
int klua_open_ksmp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",                klua_ksmpclient_connect },          ///< 客户端 发起连接
        { "connect_rpc",            klua_ksmpclient_connect_rpc },      ///< 客户端 发起 RPC 连接

        { "listen",                 klua_ksmpserve_listen },            ///< 服务端 监听
        { "listen_rpc",             klua_ksmpserverpc_listen },         ///< 服务端 RPC 监听
        { "new_serve",              klua_ksmpserve_new_serve },         ///< 由探测 sock 建服务连接
        { "new_serve_rpc",          klua_ksmpserverpc_new_serve },      ///< 由探测 sock 建 RPC 服务连接

        { NULL,                     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // k smp client
    klua_ksmpclient_createmeta(L);

    // k smp serve
    klua_ksmpserve_createmeta(L);

    // k smp serve listen
    klua_ksmpserve_listen_createmeta(L);

    // k smp client rpc
    klua_ksmpclientrpc_createmeta(L);

    // k smp serve rpc
    klua_ksmpserverpc_createmeta(L);

    // k smp serve rpc listen
    klua_ksmpserverpc_listen_createmeta(L);

    return 1;
}

//end
