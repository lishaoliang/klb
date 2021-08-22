#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbnet/klb_ncm.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_mnp.h"
#include "klua/klua_data.h"
#include "klbnet/klb_listen.h"
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

    struct
    {
        klb_listen_t*           p_listen;       ///< 监听
    };
}klua_kncm_t;

//////////////////////////////////////////////////////////////////////////

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_kncm        kncm指针
/// @param [in]  *p_msg         消息类型: "text", "binary"
/// @param [in]  *p_s1          字符串1
/// @param [in]  s1_len         字符串1长度
/// @param [in]  *p_s2          字符串2
/// @param [in]  s2_len         字符串2长度
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm(klua_kncm_t* p_kncm, const char* p_msg, int id, int protocol, const char* p_s1, int s1_len, const char* p_s2, int s2_len, uint32_t sequence, uint32_t uid)
{
    assert(NULL != p_kncm);
    if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

    lua_State* L = p_kncm->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
    lua_pushstring(L, p_msg);                                  /* 1st argument */
    lua_pushinteger(L, id);                                    /* 2st argument */
    lua_pushinteger(L, protocol);                              /* 3st argument */
    lua_pushlstring(L, p_s1, s1_len);                          /* 4st argument */
    lua_pushlstring(L, p_s2, s2_len);                          /* 5st argument */
    lua_pushinteger(L, sequence);                              /* 6st argument */
    lua_pushinteger(L, uid);                                   /* 7st argument */
    int status = lua_pcall(L, 7, 0, 0);                        /* do the call */
    klua_env_report(p_kncm->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_kncm        kncm指针
/// @param [in]  *p_msg         消息类型: "media"
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm_media(klua_kncm_t* p_kncm, const char* p_msg, int id, int protocol, klb_buf_t* p_data)
{
    assert(NULL != p_kncm);
    if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

    lua_State* L = p_kncm->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
    lua_pushstring(L, p_msg);                                  /* 1st argument */
    lua_pushinteger(L, id);                                    /* 2st argument */
    lua_pushinteger(L, protocol);                              /* 3st argument */
    lua_pushlstring(L, p_data->p_buf + p_data->start, p_data->end - p_data->start); /* 4st argument */
    int status = lua_pcall(L, 4, 0, 0);                        /* do the call */
    klua_env_report(p_kncm->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_kncm        kncm指针
/// @param [in]  *p_msg         消息类型: "connect", "disconnect"
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm_tip(klua_kncm_t* p_kncm, const char* p_msg, int id, int protocol, int code)
{
    assert(NULL != p_kncm);
    if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

    lua_State* L = p_kncm->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);     /* to call reg in protected mode */
    lua_pushstring(L, p_msg);                                   /* 1st argument */
    lua_pushinteger(L, id);                                     /* 2st argument */
    lua_pushinteger(L, protocol);                               /* 3st argument */
    lua_pushinteger(L, code);                                   /* 4st argument */
    int status = lua_pcall(L, 4, 0, 0);                         /* do the call */
    klua_env_report(p_kncm->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/// @brief 连接解析收到数据时,回调
/// @return int 0
static int on_klb_ncm_recv_klua_kncm(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;

    if (KLB_SOCKET_CONNECT == code)
    {
        call_lua_reg_on_recv_klua_kncm_tip(p_kncm, "connect", id, protocol, 0);
    }
    else if (KLB_SOCKET_OK == code)
    {
        char* ptr = p_data->p_buf + p_data->start;
        int data_len = p_data->end - p_data->start;

        if (KLB_NCM_PACK_TEXT == packtype)
        {
            klb_mnp_common_t* p_common = (klb_mnp_common_t*)ptr;

            ptr += sizeof(klb_mnp_common_t);
            data_len -= sizeof(klb_mnp_common_t);

            call_lua_reg_on_recv_klua_kncm(p_kncm, "text", id, protocol,
                ptr, p_common->extra,
                ptr + p_common->extra, data_len - p_common->extra,
                p_common->sequence, p_common->uid);
        }
        else if (KLB_NCM_PACK_BINARY == packtype)
        {
            klb_mnp_common_t* p_common = (klb_mnp_common_t*)ptr;

            ptr += sizeof(klb_mnp_common_t);
            data_len -= sizeof(klb_mnp_common_t);

            call_lua_reg_on_recv_klua_kncm(p_kncm, "binary", id, protocol,
                ptr, p_common->extra,
                ptr + p_common->extra, data_len - p_common->extra,
                p_common->sequence, p_common->uid);
        }
        else if (KLB_NCM_PACK_MEDIA == packtype)
        {
            call_lua_reg_on_recv_klua_kncm_media(p_kncm, "media", id, protocol, p_data);
        }
    }
    else if (KLB_SOCKET_CLOSEING == code)
    {
        // 主动断开连接, 啥也不做
    }
    else
    {
        // 需要断开连接
        call_lua_reg_on_recv_klua_kncm_tip(p_kncm, "disconnect", id, protocol, code);
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

static void free_klua_kncm(klua_kncm_t* p_kncm)
{

}

//////////////////////////////////////////////////////////////////////////

static int klua_kncm_gc(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    free_klua_kncm(p_kncm);

    return 0;
}

static int klua_kncm_tostring(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    lua_pushfstring(L, "kncm:%p", p_kncm);
    return 1;
}

static int klua_kncm_close(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    free_klua_kncm(p_kncm);

    return 0;
}

static int klua_kncm_set_on_recv(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    klua_unref_registryindex(L, p_kncm->reg_on_recv);
    p_kncm->reg_on_recv = klua_ref_registryindex(L, 2);

    return 0;
}

static int on_accept_klua_kncm_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;

    klb_socket_t* p_socket = klb_socket_async_create(fd);

    int id = klb_ncm_push(p_kncm->p_ncm, KLB_PROTOCOL_MNP, p_socket, NULL, 0);

    if (id < 0)
    {
        klb_socket_destroy(p_socket);
    }

    return 0;
}

static int klua_kncm_listen(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    p_kncm->p_listen = klb_listen_create(p_kncm->p_multi);

    klb_listen_set_accept(p_kncm->p_listen, on_accept_klua_kncm_listen, p_kncm);

    klb_listen_open(p_kncm->p_listen, port, 20);

    return 0;
}

static int klua_kncm_close_listen(lua_State* L)
{
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

static int klua_kncm_disconnect(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int id = (int)luaL_checkinteger(L, 2);

    if (0 == klb_ncm_close(p_kncm->p_ncm, id))
    {
        lua_pushboolean(L, true);
    }
    else
    {
        lua_pushboolean(L, false);
    }

    return 1;
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
    if (lua_isinteger(L, 5))
    {
        sequence = (uint32_t)luaL_checkinteger(L, 5);
    }

    uint32_t uid = 0;
    if (lua_isinteger(L, 6))
    {
        uid = (uint32_t)luaL_checkinteger(L, 6);
    }

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

/// @brief 对连接进行控制操作
static int klua_kncm_ctrl(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int id = (int)luaL_checkinteger(L, 2);

    int argc = lua_gettop(L);
    int data_num = argc - 2;

    klua_data_t data[LUA_MINSTACK];
    memset(data, 0, sizeof(klua_data_t) * LUA_MINSTACK);

    for (int i = 0; i < data_num; i++)
    {
        klua_checkdata(L, &data[i], i + 3);
    }

    klua_data_t* p_out = NULL;
    int out_num = 0;
    if (0 == klb_ncm_ctrl(p_kncm->p_ncm, id, &data, data_num, &p_out, &out_num))
    {
        lua_pushboolean(L, true);

        for (int i = 0; i < out_num; i++)
        {
            klua_pushdata(L, &p_out[i]);
        }
    }
    else
    {
        assert(0 == out_num);
        lua_pushboolean(L, false);
    }

    for (int i = 0; i < out_num; i++)
    {
        klua_emptydata(&p_out[i]);
    }

    KLB_FREE(p_out);
    return out_num + 1;
}

static void klua_kncm_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kncm_close },

        { "set_on_recv",    klua_kncm_set_on_recv },

        { "listen",         klua_kncm_listen },
        { "close_listen",   klua_kncm_close_listen },

        { "connect",        klua_kncm_connect },
        { "disconnect",     klua_kncm_disconnect },

        { "send",           klua_kncm_send },
        { "recv",           klua_kncm_recv },

        { "send_media",     klua_kncm_send_media },
        { "recv_media",     klua_kncm_recv_media },

        { "ctrl",           klua_kncm_ctrl },

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
