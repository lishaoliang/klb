// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_socket.h"      // Bug. MINGW32/MINGW64 必须先引用 <winsock2.h>, 再 引用 <windows.h>
#include "klbnet/klb_ncm.h"
#include "klbnet/klb_listen.h"
#include "klbnet/klb_nsp.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_mnp.h"
#include <stdlib.h>
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
        lua_State*              co_recv;        ///< "recv"函数对应的协程
    };

    struct
    {
        klua_ex_multiplex_t*    p_ex;           ///< 复用扩展
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    struct
    {
        klb_listen_t*           p_listen;       ///< 监听
        klb_nsp_t*              p_nsp;          ///< 连接预处理: 识别协议类型
        klb_ncm_t*              p_ncm;          ///< ncm模块
    };

    bool                        is_close;       ///< 是否关闭: true.关闭; false.未关闭
}klua_kncm_t;

//////////////////////////////////////////////////////////////////////////

// KLB_PROTOCOL_MAX
static const char* s_klua_kncm_protocols[] = { 
    "unkown", "mnp", "mnps", "rtmp", "rtsp", 
    "http", "https", "ws", "wss", "http-mnp",
    "http-flv", "ws-mnp", "ws-flv", NULL };

static int check_protocol_klua_kncm(const char* p_protocol)
{
    for (int i = 0; s_klua_kncm_protocols[i]; i++)
    {
        if (strcmp(s_klua_kncm_protocols[i], p_protocol) == 0)
        {
            return i;
        }
    }

    return KLB_PROTOCOL_UNKOWN;
}

static const char* get_protocol_klua_kncm(int protocol, char* p_buf, int buf_len)
{
    if (0 <= protocol && protocol < KLB_PROTOCOL_MAX)
    {
        return s_klua_kncm_protocols[protocol];
    }

    snprintf(p_buf, buf_len - 1, "%d", protocol);
    return p_buf;
}

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

    if (p_kncm->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kncm->p_env), p_kncm->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_kncm->co_recv = NULL; // 清空

        char buf[32] = { 0 };

        lua_pushstring(L, p_msg);                                  /* 1st argument */
        lua_pushinteger(L, id);                                    /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));/* 3st argument */
        lua_pushlstring(L, p_s1, s1_len);                          /* 4st argument */
        lua_pushlstring(L, p_s2, s2_len);                          /* 5st argument */
        lua_pushinteger(L, sequence);                              /* 6st argument */
        lua_pushinteger(L, uid);                                   /* 7st argument */
        int status = lua_pcall(L, 7, 0, 0);                        /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

        char buf[32] = { 0 };

        lua_State* L = p_kncm->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
        lua_pushstring(L, p_msg);                                  /* 1st argument */
        lua_pushinteger(L, id);                                    /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));/* 3st argument */
        lua_pushlstring(L, p_s1, s1_len);                          /* 4st argument */
        lua_pushlstring(L, p_s2, s2_len);                          /* 5st argument */
        lua_pushinteger(L, sequence);                              /* 6st argument */
        lua_pushinteger(L, uid);                                   /* 7st argument */
        int status = lua_pcall(L, 7, 0, 0);                        /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_kncm        kncm指针
/// @param [in]  *p_msg         消息类型: "media"
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm_media(klua_kncm_t* p_kncm, const char* p_msg, int id, int protocol, klb_buf_t* p_data)
{
    assert(NULL != p_kncm);

    if (p_kncm->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kncm->p_env), p_kncm->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_kncm->co_recv = NULL; // 清空

        char buf[32] = { 0 };

        lua_pushstring(L, p_msg);                                  /* 1st argument */
        lua_pushinteger(L, id);                                    /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));  /* 3st argument */
        lua_pushlightuserdata(L, p_data);                          /* 4st argument */
        int status = lua_pcall(L, 4, 0, 0);                        /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

        char buf[32] = { 0 };

        lua_State* L = p_kncm->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);    /* to call reg in protected mode */
        lua_pushstring(L, p_msg);                                  /* 1st argument */
        lua_pushinteger(L, id);                                    /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));  /* 3st argument */
        lua_pushlightuserdata(L, p_data);                          /* 4st argument */
        int status = lua_pcall(L, 4, 0, 0);                        /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_kncm        kncm指针
/// @param [in]  *p_msg         消息类型: "connect", "disconnect"
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kncm_tip(klua_kncm_t* p_kncm, const char* p_msg, int id, int protocol, int code)
{
    assert(NULL != p_kncm);

    if (p_kncm->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kncm->p_env), p_kncm->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_kncm->co_recv = NULL; // 清空

        char buf[32] = { 0 };

        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushinteger(L, id);                                     /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));  /* 3st argument */
        lua_pushinteger(L, code);                                   /* 4st argument */
        int status = lua_pcall(L, 4, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {

        if (p_kncm->reg_on_recv <= 0) return EXIT_FAILURE;

        char buf[32] = { 0 };

        lua_State* L = p_kncm->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_kncm->reg_on_recv);     /* to call reg in protected mode */
        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushinteger(L, id);                                     /* 2st argument */
        lua_pushstring(L, get_protocol_klua_kncm(protocol, buf, sizeof(buf)));  /* 3st argument */
        lua_pushinteger(L, code);                                   /* 4st argument */
        int status = lua_pcall(L, 4, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

/// @brief 连接解析收到数据时,回调
/// @return int 0
static int on_klb_ncm_recv_klua_kncm(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;

    if (KLB_SOCKET_CONNECT == code)
    {
        // 仅仅表示 创建socket成功, 连接中
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
                ptr, p_common->head,
                ptr + p_common->head, data_len - p_common->head,
                p_common->sequence, p_common->uid);
        }
        else if (KLB_NCM_PACK_BINARY == packtype)
        {
            klb_mnp_common_t* p_common = (klb_mnp_common_t*)ptr;

            ptr += sizeof(klb_mnp_common_t);
            data_len -= sizeof(klb_mnp_common_t);

            call_lua_reg_on_recv_klua_kncm(p_kncm, "binary", id, protocol,
                ptr, p_common->head,
                ptr + p_common->head, data_len - p_common->head,
                p_common->sequence, p_common->uid);
        }
        else if (KLB_NCM_PACK_MEDIA == packtype)
        {
            //klb_mnp_media_t* p_media = (klb_mnp_media_t*)ptr;
            //KLB_LOG("dtype=%d,vtype=%d,size=%d\n", p_media->dtype, p_media->vtype, p_media->size);
            call_lua_reg_on_recv_klua_kncm_media(p_kncm, "media", id, protocol, p_data);
        }
    }
    else if (KLB_SOCKET_CLOSEING == code)
    {
        // 主动断开连接, 啥也不做
    }
    else
    {
        // 断开连接
        klb_ncm_close(p_kncm->p_ncm, id);

        // 通知脚本断开连接
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

static void close_klua_kncm(klua_kncm_t* p_kncm)
{
    if (!p_kncm->is_close)
    {
        klb_listen_close(p_kncm->p_listen);

        KLB_FREE_BY(p_kncm->p_listen, klb_listen_destroy);
        KLB_FREE_BY(p_kncm->p_nsp, klb_nsp_destroy);
        KLB_FREE_BY(p_kncm->p_ncm, klb_ncm_destroy);

        klua_unref_registryindex(p_kncm->L, p_kncm->reg_on_recv);

        p_kncm->is_close = true;
    }
}

//////////////////////////////////////////////////////////////////////////

static int klua_kncm_gc(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    close_klua_kncm(p_kncm);

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

    close_klua_kncm(p_kncm);

    return 0;
}

static int klua_kncm_on_recv(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    klua_unref_registryindex(L, p_kncm->reg_on_recv);
    p_kncm->reg_on_recv = klua_ref_registryindex(L, 2);

    return 0;
}

static int on_accept_klua_kncm_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;
    klb_socket_t* p_socket = NULL;
    
    if (tls)
    {
        p_socket = klb_socket_tls_async_create(fd, false, p_tls_param);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    if (NULL != p_socket)
    {
        klb_socket_set_connected(p_socket, true);
        klb_nsp_push(p_kncm->p_nsp, p_socket);
    }
    else
    {
        KLB_SOCKET_CLOSE(fd);
    }

    return 0;
}

static int on_accept_klua_kncm_nsp(void* ptr, int protocol, klb_socket_t* p_socket, const klb_buf_t* p_buf)
{
    klua_kncm_t* p_kncm = (klua_kncm_t*)ptr;

    uint8_t* p_data = (uint8_t*)(p_buf->p_buf + p_buf->start);
    int data_len = p_buf->end - p_buf->start;

    int id = klb_ncm_push(p_kncm->p_ncm, protocol, p_socket, p_data, data_len);
    
    if (id < 0)
    {
        klb_socket_destroy(p_socket);
    }

    return 0;
}

static int klua_kncm_listen(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int port = (int)luaL_checkinteger(L, 2);

    klb_listen_open(p_kncm->p_listen, port, 20);

    return 0;
}

static int klua_kncm_close_listen(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    klb_listen_close(p_kncm->p_listen);
    return 0;
}

static int klua_kncm_check_protocol(lua_State* L, int idx)
{
    int protocol = KLB_PROTOCOL_MNP;
    if (lua_isstring(L, idx))
    {
        protocol = check_protocol_klua_kncm(luaL_checkstring(L, idx));
    }
    else if(lua_isinteger(L, idx))
    {
        protocol = (int)luaL_checkinteger(L, idx);
    }

    return protocol;
}

static int klua_kncm_route(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    const char* p_path = luaL_checkstring(L, 2);
    int protocol = klua_kncm_check_protocol(L, 3); // klb_protocol_e

    klb_nsp_route(p_kncm->p_nsp, p_path, protocol);

    return 0;
}

static int klua_kncm_connect(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int protocol = klua_kncm_check_protocol(L, 2); // klb_protocol_e
    const char* p_host = luaL_checkstring(L, 3);
    lua_Integer port = luaL_checkinteger(L, 4);

    bool tls = false;

    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushinteger(L, -1);
        return 1;
    }

    klb_socket_t* p_socket = NULL;
    if (tls)
    {
        p_socket = klb_socket_tls_async_create(fd, false, NULL);
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

static int klua_kncm_send_text(lua_State* L)
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
        if (0 == klb_ncm_send_text(p_kncm->p_ncm, id, sequence, uid, (const uint8_t*)p_s1, s1_len, (const uint8_t*)p_s2, s2_len))
        {
            ok = true;
        }
    }

    lua_pushboolean(L, ok);
    return 1;
}

static int klua_kncm_send_binary(lua_State* L)
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
        if (0 == klb_ncm_send_binary(p_kncm->p_ncm, id, sequence, uid, (const uint8_t*)p_s1, s1_len, (const uint8_t*)p_s2, s2_len))
        {
            ok = true;
        }
    }

    lua_pushboolean(L, ok);
    return 1;
}

static int klua_kncm_co_recv(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);

    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "kncm.new():recv() must in coroutine!");
        return 0;
    }

    // todo. 未能及时 recv 数据时的处理
    assert(NULL == p_kncm->co_recv);
    p_kncm->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static int klua_kncm_send_media(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int id = (int)luaL_checkinteger(L, 2);
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 3);

    int ret = klb_ncm_send_media(p_kncm->p_ncm, id, p_frame);

    lua_pushinteger(L, ret);
    return 1;
}

/// @brief 对连接进行控制操作
static int klua_kncm_ctrl(lua_State* L)
{
    klua_kncm_t* p_kncm = to_klua_kncm(L, 1);
    int id = (int)luaL_checkinteger(L, 2);

    int argc = lua_gettop(L);
    int data_num = argc - 2;

    return 0;
}

static void klua_kncm_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kncm_close },

        //{ "on_recv",        klua_kncm_on_recv },

        { "listen",         klua_kncm_listen },
        { "close_listen",   klua_kncm_close_listen },

        { "route",          klua_kncm_route },

        { "connect",        klua_kncm_connect },
        { "disconnect",     klua_kncm_disconnect },

        { "send_text",      klua_kncm_send_text },
        { "send_binary",    klua_kncm_send_binary },
        { "send_media",     klua_kncm_send_media },

        { "co_recv",        klua_kncm_co_recv },

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
    p_kncm->p_env = klua_env_get_by_L(L);
    p_kncm->p_ex = klua_ex_get_multiplex(p_kncm->p_env);
    p_kncm->p_multi = klua_ex_multiplex_get(p_kncm->p_ex);

    p_kncm->p_listen = klb_listen_create(p_kncm->p_multi);
    p_kncm->p_nsp = klb_nsp_create(p_kncm->p_multi);
    p_kncm->p_ncm = klb_ncm_create(p_kncm->p_multi, KLB_PROTOCOL_LOAD_STD);
    p_kncm->is_close = false;

    // 数据
    klb_ncm_add_receiver(p_kncm->p_ncm, on_klb_ncm_recv_klua_kncm, p_kncm);

    // nsp预处理
    klb_nsp_set_accept(p_kncm->p_nsp, on_accept_klua_kncm_nsp, p_kncm);

    // 监听
    klb_listen_set_accept(p_kncm->p_listen, on_accept_klua_kncm_listen, p_kncm);

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
