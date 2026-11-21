// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbonvif/klbonvif.h"
#include "klbonvif/core/klb_onvif_discovery.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include <assert.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// konvif client / serve (stub)


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_konvif(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


static int klua_konvif_connect(lua_State* L)
{
    return push_err_klua_konvif(L, KLB_NETCODE_EOPNOTSUPP);
}


static int klua_konvif_listen(lua_State* L)
{
    return push_err_klua_konvif(L, KLB_NETCODE_EOPNOTSUPP);
}


//////////////////////////////////////////////////////////////////////////
// konvif discovery userdata


#define KLUA_KONVIF_DISCOVERY_HANDLE    "KONVIF_DISCOVERY_HANDLE*"


/// @struct klua_konvif_discovery_t
/// @brief  Discovery Lua 包装
typedef struct klua_konvif_discovery_t_
{
    struct
    {
        lua_State*              L;
        lua_State*              co_recv;
        klua_env_t*             p_env;
        klua_ex_coroutine_t*    p_coex;
    };

    struct
    {
        klb_netmulti_t*         p_netmulti;
        klb_netconn_t*          p_conn;
        int                     role;
    };

    struct
    {
        klb_nlist_t*            p_text_nlist;
        int                     recv_code;
    };
}klua_konvif_discovery_t;


static klua_konvif_discovery_t* new_klua_konvif_discovery(lua_State* L)
{
    klua_konvif_discovery_t* p_d = (klua_konvif_discovery_t*)lua_newuserdata(L, sizeof(klua_konvif_discovery_t));
    KLB_MEMSET(p_d, 0, sizeof(klua_konvif_discovery_t));
    luaL_setmetatable(L, KLUA_KONVIF_DISCOVERY_HANDLE);
    return p_d;
}


static klua_konvif_discovery_t* to_klua_konvif_discovery(lua_State* L, int index)
{
    klua_konvif_discovery_t* p_d = (klua_konvif_discovery_t*)luaL_checkudata(L, index, KLUA_KONVIF_DISCOVERY_HANDLE);
    luaL_argcheck(L, NULL != p_d, index, "'konvif.discovery' expected");
    return p_d;
}


static int push_datagram_klua_konvif_discovery(lua_State* L, klb_buf_t* p_data)
{
    assert(NULL != p_data);

    int data_len = p_data->end - p_data->start;

    if (data_len < (int)sizeof(klb_mnp_text_t))
    {
        lua_pushstring(L, "datagram");
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

    lua_pushstring(L, "datagram");
    lua_pushlstring(L, p_head, (size_t)head_len);
    lua_pushlstring(L, p_head + head_len, (size_t)body_len);
    return 3;
}


static int call_co_recv_klua_konvif_discovery(klua_konvif_discovery_t* p_d)
{
    assert(NULL != p_d);

    if (NULL == p_d->co_recv)
    {
        return -1;
    }

    if (0 >= klb_nlist_size(p_d->p_text_nlist) && 0 == p_d->recv_code)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_d->p_coex, p_d->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_d->p_env), p_d->co_recv);

    if (NULL == L)
    {
        return -1;
    }

    p_d->co_recv = NULL;

    int nargs = 0;
    klb_buf_t* p_txt = klb_nlist_pop_head(p_d->p_text_nlist);

    if (NULL != p_txt)
    {
        nargs = push_datagram_klua_konvif_discovery(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
    }
    else
    {
        lua_pushstring(L, "error");
        lua_pushinteger(L, p_d->recv_code);
        p_d->recv_code = 0;
        nargs = 2;
    }

    int status = lua_pcall(L, nargs, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}


static int call_co_recv_end_klua_konvif_discovery(klua_konvif_discovery_t* p_d)
{
    if (NULL == p_d->co_recv)
    {
        return -1;
    }

    assert(0 == klua_coroutine_debug_check(p_d->p_coex, p_d->co_recv));

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_d->p_env), p_d->co_recv);

    if (NULL == L)
    {
        return -1;
    }

    p_d->co_recv = NULL;

    lua_pushstring(L, "exit");
    lua_pushstring(L, "");
    lua_pushstring(L, "");

    int status = lua_pcall(L, 3, 0, 0);
    klua_env_report_by_L(L, status);

    return (LUA_OK == status) ? 0 : 1;
}


static int on_recv_data_klua_konvif_discovery(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_konvif_discovery_t* p_d = (klua_konvif_discovery_t*)p_conn->p_udata;

    (void)packtype;

    if (NULL == p_d)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        return 0;
    }

    if (KLB_NETCODE_OK == code && NULL != p_data)
    {
        klb_nlist_push_tail(p_d->p_text_nlist, p_data);
        call_co_recv_klua_konvif_discovery(p_d);
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        p_d->recv_code = code;
        call_co_recv_klua_konvif_discovery(p_d);
    }

    return 0;
}


static void setup_klua_konvif_discovery(klua_konvif_discovery_t* p_d, lua_State* L, klb_netconn_t* p_conn, int role)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_d->L = L;
    p_d->co_recv = NULL;
    p_d->p_env = p_env;
    p_d->p_coex = klua_coroutine_get(p_env);

    p_d->p_netmulti = klua_netmulti_get(p_env);
    p_d->p_conn = p_conn;
    p_d->role = role;

    p_d->p_text_nlist = klb_nlist_create();
    p_d->recv_code = 0;

    klb_netconn_set_udata(p_conn, p_d);
    klb_netconn_bind_recv_data(p_conn, on_recv_data_klua_konvif_discovery);
}


static int klua_konvif_discovery_tostring(lua_State* L)
{
    klua_konvif_discovery_t* p_d = to_klua_konvif_discovery(L, 1);
    lua_pushfstring(L, "konvif.discovery:%p", p_d);
    return 1;
}


static int klua_konvif_discovery_close(lua_State* L)
{
    klua_konvif_discovery_t* p_d = to_klua_konvif_discovery(L, 1);

    if (NULL != p_d->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_d->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_d->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    KLB_FREE_BY(p_d->p_text_nlist, klb_nlist_destroy);

    if (NULL != p_d->p_conn)
    {
        klb_netconn_bind_recv_data(p_d->p_conn, NULL);
        klb_netconn_set_udata(p_d->p_conn, NULL);
        klb_onvif_discovery_free(p_d->p_conn);
        p_d->p_conn = NULL;
    }

    return 0;
}


static int klua_konvif_discovery_send(lua_State* L)
{
    klua_konvif_discovery_t* p_d = to_klua_konvif_discovery(L, 1);

    size_t data_len = 0;
    const char* p_data = luaL_checklstring(L, 2, &data_len);

    const char* p_ip = NULL;
    uint16_t port = 0;

    if (3 <= lua_gettop(L) && lua_isstring(L, 3))
    {
        p_ip = lua_tostring(L, 3);
    }

    if (4 <= lua_gettop(L) && lua_isnumber(L, 4))
    {
        port = (uint16_t)lua_tointeger(L, 4);
    }

    int ret = 1;

    if (NULL != p_d->p_conn)
    {
        ret = klb_onvif_discovery_send(p_d->p_conn, (const uint8_t*)p_data, (int)data_len, p_ip, port);
    }

    lua_pushinteger(L, ret);
    return 1;
}


static int on_yield_recv_klua_konvif_discovery(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_konvif_discovery_t* p_d = (klua_konvif_discovery_t*)ptr;

    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_klua_konvif_discovery(p_d);
    }

    return 0;
}


static int klua_konvif_discovery_co_recv(lua_State* L)
{
    klua_konvif_discovery_t* p_d = to_klua_konvif_discovery(L, 1);

    klua_check_coroutine(L, "konvif.discovery:co_recv must in coroutine!");
    assert(NULL == p_d->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_d->p_text_nlist);

    if (NULL != p_txt)
    {
        klb_nlist_pop_head(p_d->p_text_nlist);
        int n = push_datagram_klua_konvif_discovery(L, p_txt);
        KLB_FREE_BY(p_txt, klb_buf_unref);
        return n;
    }

    if (0 != p_d->recv_code)
    {
        int code = p_d->recv_code;
        p_d->recv_code = 0;
        lua_pushstring(L, "error");
        lua_pushinteger(L, code);
        return 2;
    }

    p_d->co_recv = L;
    return klua_coroutine_yield(p_d->p_coex, L, on_yield_recv_klua_konvif_discovery, p_d);
}


static void klua_konvif_discovery_createmeta(lua_State* L)
{
    static luaL_Reg meta[] =
    {
        { "__gc",           klua_konvif_discovery_close },
        { "__tostring",     klua_konvif_discovery_tostring },
        { "close",          klua_konvif_discovery_close },
        { "send",           klua_konvif_discovery_send },
        { "co_recv",        klua_konvif_discovery_co_recv },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KONVIF_DISCOVERY_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta, 0);
    lua_pop(L, 1);
}


static int open_discovery_klua_konvif(lua_State* L, int role, uint16_t port)
{
    int code = 0;
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_conn = NULL;

    if (NULL == p_netmulti)
    {
        return push_err_klua_konvif(L, KLB_NETCODE_EINVAL);
    }

    p_conn = klb_onvif_discovery_open(p_netmulti, role, port, &code);
    if (NULL == p_conn)
    {
        return push_err_klua_konvif(L, (0 != code) ? code : KLB_NETCODE_BIND_FAIL);
    }

    {
        klua_konvif_discovery_t* p_d = new_klua_konvif_discovery(L);
        setup_klua_konvif_discovery(p_d, L, p_conn, role);
    }

    return 1;
}


/// @brief 搜索端: 发 Probe 组播, 收 ProbeMatches
static int klua_konvif_discover(lua_State* L)
{
    return open_discovery_klua_konvif(L, KLB_ONVIF_DISCOVERY_SEARCH, 0);
}


/// @brief 服务端: 加入组播, 收 Probe, 回 ProbeMatches
static int klua_konvif_discover_listen(lua_State* L)
{
    uint16_t port = 0;

    if (1 <= lua_gettop(L) && lua_isnumber(L, 1))
    {
        port = (uint16_t)lua_tointeger(L, 1);
    }

    return open_discovery_klua_konvif(L, KLB_ONVIF_DISCOVERY_SERVE, port);
}


//////////////////////////////////////////////////////////////////////////


int klua_open_konvif(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",            klua_konvif_connect },
        { "listen",             klua_konvif_listen },
        { "discover",           klua_konvif_discover },
        { "discover_listen",    klua_konvif_discover_listen },
        { NULL,                 NULL }
    };

    klua_konvif_discovery_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}


// end
