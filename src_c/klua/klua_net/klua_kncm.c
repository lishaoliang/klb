#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbnet/klb_ncm.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>


#define KLUA_KNCM_HANDLE     "KLUA_KNCM_HANDLE*"


typedef struct klua_kncm_t_
{
    // lua 相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        int                     reg_on_recv;    ///< Lua脚本函数
    };

    struct
    {
        klua_ex_multiplex_t*    p_ex;           ///< 复用扩展
        klb_multiplex_t*        p_multi;        ///< 复用
        klb_ncm_t*              p_ncm;          ///< ncm模块
    };
}klua_kncm_t;

//////////////////////////////////////////////////////////////////////////

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_khttp       khttp指针
/// @param [in]  *p_msg         消息类型: "http", "header", "header_complete", "body"
/// @param [in]  *p_s1          字符串1
/// @param [in]  s1_len         字符串1长度
/// @param [in]  *p_s2          字符串2
/// @param [in]  s2_len         字符串2长度
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm(klua_kncm_t* p_kncm, const char* p_msg, const char* p_s1, int s1_len, const char* p_s2, int s2_len, int protocol, int id, int code)
{
    assert(NULL != p_kncm);
    if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

    lua_State* L = p_kncm->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
    lua_pushstring(L, p_msg);                                  /* 1st argument */
    lua_pushlstring(L, p_s1, s1_len);                          /* 2st argument */
    lua_pushlstring(L, p_s2, s2_len);                          /* 3st argument */
    lua_pushinteger(L, protocol);                              /* 4st argument */
    lua_pushinteger(L, id);                                    /* 5st argument */
    lua_pushinteger(L, code);                                  /* 6st argument */
    int status = lua_pcall(L, 6, 0, 0);                        /* do the call */
    klua_env_report(p_kncm->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int call_lua_reg_on_recv_klua_kncm_error(klua_kncm_t* p_kncm, const char* p_msg, const char* p_s1)
{
    assert(NULL != p_kncm);
    if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

    lua_State* L = p_kncm->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
    lua_pushstring(L, p_msg);                                   /* 1st argument */
    lua_pushstring(L, p_s1);                                    /* 2st argument */
    lua_pushstring(L, "");                                      /* 3st argument */
    int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
    klua_env_report(p_kncm->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/// @brief 连接解析收到数据时,回调
/// @return int 0
static int on_klb_ncm_recv_klua_kncm(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;

    if (0 == code)
    {
        if (KLB_MNP_TXT == packtype)
        {
            char* ptr = p_data->p_buf + p_data->start;
            int data_len = p_data->end - p_data->start;

            klb_mnp_common_t* p_common = (klb_mnp_common_t*)ptr;

            ptr += sizeof(klb_mnp_common_t);
            data_len -= sizeof(klb_mnp_common_t);

            call_lua_reg_on_recv_klua_kncm(p_kncm, "text",
                ptr, p_common->extra,
                ptr + p_common->extra, data_len - p_common->extra,
                protocol, id, code);
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
static klua_kncm_t* new_klua_kncm(lua_State* L)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)lua_newuserdata(L, sizeof(klua_kncm_t));
    KLB_MEMSET(p_kncm, 0, sizeof(klua_kncm_t));
    luaL_setmetatable(L, KLUA_KNCM_HANDLE);
    return p_kncm;
}

static klua_kncm_t* to_klua_kncm(lua_State* L, int index)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)luaL_checkudata(L, index, KLUA_KNCM_HANDLE);
    luaL_argcheck(L, NULL != p_kncm, index, "'kncm' expected");
    return p_kncm;
}

static void free_klua_kncm(klua_kncm_t* p_khttp)
{

}

//////////////////////////////////////////////////////////////////////////

static int klua_kncm_gc(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    return 0;
}

static int klua_kncm_tostring(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    lua_pushfstring(L, "kncm:%p", p_kncm);
    return 1;
}

static int klua_kncm_set_on_recv(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    p_kncm->reg_on_recv = klua_ref_registryindex(L, 2);

    return 0;
}

static int klua_kncm_connect(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    lua_Integer protocol = luaL_checkinteger(L, 2);
    const char* p_host = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);

    bool tls = false;
    if (lua_isboolean(L, 5))
    {
        tls = lua_toboolean(L, 5);
    }

    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushinteger(L, -1);
        return 1;
    }

    klb_socket_t* p_socket = NULL;
    if (tls)
    {
        p_socket = klb_socket_tls_async_create(fd);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    int id = klb_ncm_push(p_kncm->p_ncm, (int)protocol/*KLB_PROTOCOL_MNP*/, p_socket, NULL, 0);

    lua_pushinteger(L, id);
    return 1;
}

static int klua_kncm_close(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    return 0;
}

static int klua_kncm_send(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int id = (int)luaL_checkinteger(L, 2);

    size_t s1_len = 0;
    const char* p_s1 = luaL_checklstring(L, 3, &s1_len);

    size_t s2_len = 0;
    const char* p_s2 = luaL_checklstring(L, 4, &s2_len);

    uint32_t sequence = 0;
    uint32_t uid = 0;

    bool ok = false;
    if (0 < s1_len + s2_len)
    {
        if (0 == klb_ncm_send(p_kncm->p_ncm, id, sequence, uid, p_s1, s1_len, p_s2, s2_len))
        {
            ok = true;
        }
    }

    lua_pushboolean(L, ok);
    return 1;
}


static int klua_kncm_recv(lua_State* L)
{
    return 0;
}

static int klua_kncm_send_media(lua_State* L)
{
    return 0;
}

static int klua_kncm_recv_media(lua_State* L)
{
    return 0;
}

static void klua_kncm_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "set_on_recv",    klua_kncm_set_on_recv },

        { "connect",        klua_kncm_connect }, 
        { "close",          klua_kncm_close },

        { "send",           klua_kncm_send },
        { "recv",           klua_kncm_recv },

        { "send_media",     klua_kncm_send_media },
        { "recv_media",     klua_kncm_recv_media },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            klua_kncm_gc},
        {"__close",         klua_kncm_gc},
        {"__tostring",      klua_kncm_tostring},
        {NULL,              NULL}
    };

    luaL_newmetatable(L, KLUA_KNCM_HANDLE); /* metatable for KLUA_KNCM_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

static int klua_kncm_new(lua_State* L)
{
    klua_kncm_t* p_kncm = new_klua_kncm(L);

    p_kncm->L = L;
    p_kncm->p_env = klua_env_get_by_L(L);;
    p_kncm->p_ex = klua_ex_get_multiplex(p_kncm->p_env);
    p_kncm->p_multi = klua_ex_multiplex_get(p_kncm->p_ex);
    p_kncm->p_ncm = klb_ncm_create(p_kncm->p_multi);

    klb_ncm_add_receiver(p_kncm->p_ncm, on_klb_ncm_recv_klua_kncm, p_kncm);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_kncm(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new",        klua_kncm_new },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KNCM_HANDLE
    klua_kncm_createmeta(L);

    return 1;
}
