// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbrtsp/rtsp_klua/klbrtsp_serve_klua.h"
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_netlisten_opts.h"
#include "klua/klua_coroutine.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klua/klua_env.h"
#include "klbrtsp/core/klb_rtspserve_conn.h"
#include "klbutil/klb_nlist.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_krtspserve(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @struct klua_krtspserve_listen_socket_t
/// @brief  监听得到 socket
typedef struct klua_krtspserve_listen_socket_t_
{
    klb_socket_fd               fd;             ///< socket fd
    struct sockaddr_in          addr;           ///< 地址
    bool                        tls;            ///< tls
}klua_krtspserve_listen_socket_t;


//////////////////////////////////////////////////////////////////////////
// klua rtsp serve

#define KLUA_KRTSPSERVE_HANDLE          "KRTSPSERVE_HANDLE*"


/// @struct klua_krtspserve_t
/// @brief  RTSP服务连接
typedef struct klua_krtspserve_t_
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
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 服务连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*

        bool                    closed;         ///< 连接已断开或协议错误
    };
}klua_krtspserve_t;


////////////////////////////////////////
static klua_krtspserve_t* new_klua_krtspserve(lua_State* L)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)lua_newuserdata(L, sizeof(klua_krtspserve_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_krtspserve_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_HANDLE);
    return p_serve;
}

static klua_krtspserve_t* to_klua_krtspserve(lua_State* L, int index)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'krtspserve' expected");
    return p_serve;
}

static int call_co_recv_end_text_klua_krtspserve(klua_krtspserve_t* p_serve);

static int klua_krtspserve_tostring(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    lua_pushfstring(L, "krtspserve:%p", p_serve);
    return 1;
}

static int klua_krtspserve_close(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    // 先唤醒等待中的 co_recv, 再释放 nlist, 避免退出回调对空表取 size
    p_serve->closed = true;
    call_co_recv_end_text_klua_krtspserve(p_serve);

    if (NULL != p_serve->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_serve->p_text_nlist))
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_serve->p_text_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_serve->p_rtsp_conn, klb_netconn_destroy);
    KLB_FREE_BY(p_serve->p_text_nlist, klb_nlist_destroy);

    return 0;
}

//////////////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_krtspserve(klua_krtspserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv
        && NULL != p_serve->p_text_nlist
        && 0 < klb_nlist_size(p_serve->p_text_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv 函数; 这里会存在执行函数的交替执行

        klb_buf_t* p_txt = klb_nlist_pop_head(p_serve->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // #1 body
            lua_pushstring(L, "text"); // #2 "text"
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 调用Lua协程 结束
static int call_co_recv_end_text_klua_krtspserve(klua_krtspserve_t* p_serve)
{
    assert(NULL != p_serve);

    if (NULL != p_serve->co_recv)
    {
        assert(0 == klua_coroutine_debug_check(p_serve->p_coex, p_serve->co_recv));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_serve->p_env), p_serve->co_recv);
        if (NULL == L) return -1; // 未处理

        p_serve->co_recv = NULL; // 清空

        lua_pushstring(L, ""); // #1 body
        lua_pushstring(L, "exit"); // #2 "text"

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包时触发
static int on_recv_data_klua_krtspserve(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            if (NULL == p_serve->p_text_nlist)
            {
                KLB_FREE_BY(p_data, klb_buf_unref_next);
                return 0;
            }

            klb_nlist_push_tail(p_serve->p_text_nlist, p_data);

            call_co_recv_text_klua_krtspserve(p_serve);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }
    else if (KLB_SOCKET_CONNECT == code || KLB_NETCODE_WBUF_EMPTY == code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }
    else
    {
        p_serve->closed = true;
        call_co_recv_end_text_klua_krtspserve(p_serve);
        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }

    return 0;
}

//////////////////////////////////////////////////
// 

static int klua_krtspserve_send_text(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);                  // @1. self

    size_t head_len = 0;
    const char* p_head = (const char*)luaL_checklstring(L, 2, &head_len);   // @2. head

    const char* p_body = NULL;
    size_t body_len = 0;
    if (klua_is_string(L, 3)) { p_body = (const char*)luaL_checklstring(L, 3, &body_len);} // @3. body

    int ret = 1;

    if (NULL != p_serve->p_rtsp_conn)
    {
        ret = klb_netconn_send_text(p_serve->p_rtsp_conn, 0, (const uint8_t*)p_head, (int)head_len, (const uint8_t*)p_body, (int)body_len);
    }

    lua_pushinteger(L, ret);            // #1.  错误码
    return 1;
}

static int klua_krtspserve_setup_tcp(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int rtp_ch = (int)luaL_checkinteger(L, 2);

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_setup_tcp(p_serve->p_rtsp_conn, rtp_ch);
    }

    return 0;
}


static int klua_krtspserve_setup_udp(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int client_port = (int)luaL_checkinteger(L, 2);
    int server_port = 0;
    int ret = 1;

    if (NULL != p_serve->p_rtsp_conn)
    {
        ret = klb_rtspserve_conn_setup_udp(p_serve->p_rtsp_conn, client_port, &server_port);
    }

    if (0 != ret)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, server_port);
    return 1;
}


static int klua_krtspserve_set_timeout(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int read_ms = (int)luaL_optinteger(L, 2, 0);
    int write_ms = (int)luaL_optinteger(L, 3, 0);
    int idle_ms = (int)luaL_optinteger(L, 4, 0);

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_set_timeout(p_serve->p_rtsp_conn, read_ms, write_ms, idle_ms);
    }

    return 0;
}


static int klua_krtspserve_set_video_pt(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int pt = (int)luaL_optinteger(L, 2, 0);

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_set_video_pt(p_serve->p_rtsp_conn, pt);
    }

    return 0;
}


static int klua_krtspserve_set_audio_pt(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int pt = (int)luaL_optinteger(L, 2, 0);

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_set_audio_pt(p_serve->p_rtsp_conn, pt);
    }

    return 0;
}

static int klua_krtspserve_set_track_pt(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int track = (int)luaL_checkinteger(L, 2);
    int pt = (int)luaL_optinteger(L, 3, 0);
    bool is_audio = (0 != lua_toboolean(L, 4));

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_set_track_pt(p_serve->p_rtsp_conn, track, pt, is_audio);
    }

    return 0;
}

static int klua_krtspserve_setup_tcp_track(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int track = (int)luaL_checkinteger(L, 2);
    int rtp_ch = (int)luaL_checkinteger(L, 3);

    if (NULL != p_serve->p_rtsp_conn)
    {
        klb_rtspserve_conn_setup_tcp_track(p_serve->p_rtsp_conn, track, rtp_ch);
    }

    return 0;
}

static int klua_krtspserve_setup_udp_track(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    int track = (int)luaL_checkinteger(L, 2);
    int client_port = (int)luaL_checkinteger(L, 3);
    int server_port = 0;
    int ret = 1;

    if (NULL != p_serve->p_rtsp_conn)
    {
        ret = klb_rtspserve_conn_setup_udp_track(p_serve->p_rtsp_conn, track, client_port, &server_port);
    }

    if (0 != ret)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, server_port);
    return 1;
}


static int klua_krtspserve_send_media(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);      // @1. self
    klb_buf_t* p_buf = lua_touserdata(L, 2);                    // @2. 媒体数据

    int ret = 1;
    if (NULL != p_buf)
    {
        ret = klb_netconn_send_media(p_serve->p_rtsp_conn, p_buf);
    }

    lua_pushinteger(L, ret);            // #1.  错误码
    return 1;
}

// 当需要退出等的 退出函数
static int on_yield_recv_klua_klua_krtspserve(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_end_text_klua_krtspserve(p_serve);
    }

    return 0;
}

static int klua_krtspserve_co_recv(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);
    klua_check_coroutine(L, "krtspserve:co_recv must in coroutine!");
    assert(NULL == p_serve->co_recv);

    if (NULL != p_serve->p_text_nlist)
    {
        klb_buf_t* p_txt = klb_nlist_head(p_serve->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // #1. body
            lua_pushstring(L, "text"); // #2. "text"

            klb_nlist_pop_head(p_serve->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref_next);

            return 2;
        }
    }

    if (p_serve->closed)
    {
        lua_pushstring(L, "");
        lua_pushstring(L, "exit");
        return 2;
    }

    p_serve->co_recv = L;
    return klua_coroutine_yield(p_serve->p_coex, L, on_yield_recv_klua_klua_krtspserve, p_serve);
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtspserve_close },            ///< 关闭连接

        { "send_text",      klua_krtspserve_send_text },        ///< 发送文本数据
        { "send_media",     klua_krtspserve_send_media },       ///< 发送媒体数据
        { "setup_tcp",      klua_krtspserve_setup_tcp },        ///< 媒体改走 TCP interleaved
        { "setup_udp",      klua_krtspserve_setup_udp },        ///< 媒体改走 UDP
        { "set_timeout",    klua_krtspserve_set_timeout },      ///< 设置读写/空闲超时(毫秒)
        { "set_video_pt",   klua_krtspserve_set_video_pt },     ///< 设置视频 RTP 载荷类型
        { "set_audio_pt",   klua_krtspserve_set_audio_pt },     ///< 设置音频 RTP 载荷类型; 0 关闭音频
        { "set_track_pt",   klua_krtspserve_set_track_pt },     ///< 设置指定轨 RTP 载荷类型
        { "setup_tcp_track", klua_krtspserve_setup_tcp_track }, ///< 指定轨改走 TCP interleaved
        { "setup_udp_track", klua_krtspserve_setup_udp_track }, ///< 指定轨改走 UDP; 返回本端端口

        { "co_recv",        klua_krtspserve_co_recv },          ///< 接收数据

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_close },
        { "__close",        klua_krtspserve_close },
        { "__tostring",     klua_krtspserve_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_HANDLE);   /* metatable for KLUA_KRTSPSERVE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}

//////////////////////////////////////////////////

// 创建
/// @brief 由已探测 sock 创建 RTSP 服务连接; 可选 `{cert,key}` 供 TLS(rtsps)
int klua_krtspserve_new_serve(lua_State* L)
{
    klua_krtspserve_listen_socket_t* p_listen_socket = (klua_krtspserve_listen_socket_t*)lua_touserdata(L, 1);
    bool tls = false;
    const char* p_cert = NULL;
    size_t cert_len = 0;
    const char* p_key = NULL;
    size_t key_len = 0;
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_netmulti = NULL;
    klua_krtspserve_t* p_serve = NULL;

    // close 唤醒 co_accept 时可能传入 NULL lightuserdata
    if (NULL == p_listen_socket)
    {
        return push_err_klua_krtspserve(L, KLB_NETCODE_INVAL);
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

    // 取走 fd 后释放队列节点 (与 klisten / klb_netmux_socket_t 前三字段布局兼容)
    fd = p_listen_socket->fd;
    KLB_FREE(p_listen_socket);

    if (tls)
    {
        klb_socket_tls_server_ctx_t* p_tls_ctx = klb_socket_tls_server_ctx_create(
            p_cert, (int)cert_len, p_key, (int)key_len);
        if (NULL == p_tls_ctx)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_krtspserve(L, KLB_NETCODE_TLS_CERT);
        }

        p_socket = klb_socket_async_create_tls_server(fd, p_tls_ctx);
        klb_socket_tls_server_ctx_destroy(p_tls_ctx);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_krtspserve(L, KLB_NETCODE_TLS_HANDSHAKE);
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            return push_err_klua_krtspserve(L, KLB_NETCODE_NOMEM);
        }
    }

    p_env = klua_env_get_by_L(L);
    p_netmulti = klua_netmulti_get(p_env);
    if (NULL == p_netmulti)
    {
        KLB_FREE_BY(p_socket, klb_socket_destroy);
        return push_err_klua_krtspserve(L, KLB_NETCODE_INVAL);
    }

    p_serve = new_klua_krtspserve(L);

    p_serve->L = L;
    p_serve->co_recv = NULL;
    p_serve->p_env = p_env;
    p_serve->p_coex = klua_coroutine_get(p_env);

    p_serve->p_netmulti = p_netmulti;

    p_serve->p_text_nlist = klb_nlist_create();

    p_serve->p_rtsp_conn = klb_rtspserve_conn_create(p_serve->p_netmulti, p_socket);

    klb_netconn_set_udata(p_serve->p_rtsp_conn, p_serve);
    klb_netconn_bind_recv_data(p_serve->p_rtsp_conn, on_recv_data_klua_krtspserve);

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// klua rtsp serve listen

#define KLUA_KRTSPSERVE_LISTEN_HANDLE    "KRTSPSERVE_LISTEN_HANDLE*"


/// @struct klua_krtspserve_listen_t
/// @brief  rtsp 服务监听
typedef struct klua_krtspserve_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_accept;      ///< sync的"co_accept"函数对应的协程

        klua_env_t*             p_env;          ///< Lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
    };

    // 监听
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_listen_conn;  ///< 监听连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_socket_nlist; ///< 待读取的 socket 列表; 存储 klua_krtspserve_listen_socket_t*
    };
}klua_krtspserve_listen_t;

////////////////////////////////////////
static klua_krtspserve_listen_t* new_klua_krtspserve_listen(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)lua_newuserdata(L, sizeof(klua_krtspserve_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_krtspserve_listen_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    return p_listen;
}

static klua_krtspserve_listen_t* to_klua_krtspserve_listen(lua_State* L, int index)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'krtspserve listen' expected");
    return p_listen;
}

static int klua_krtspserve_listen_tostring(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    lua_pushfstring(L, "krtspserve listen:%p", p_listen);
    return 1;
}

static int call_co_accept_end_klua_krtspserve_listen(klua_krtspserve_listen_t* p_listen);

static int klua_krtspserve_listen_close(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    // 先唤醒等待中的 co_accept (退出条件勿依赖 nlist 非空)
    call_co_accept_end_klua_krtspserve_listen(p_listen);

    // 释放 监听连接
    KLB_FREE_BY(p_listen->p_listen_conn, klb_netlisten_conn_free);

    // 释放 已经 监听得到的 socket
    if (NULL != p_listen->p_socket_nlist)
    {
        while (0 < klb_nlist_size(p_listen->p_socket_nlist))
        {
            klua_krtspserve_listen_socket_t* p_tmp = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

            KLB_SOCKET_CLOSE(p_tmp->fd);
            KLB_FREE(p_tmp);
        }
    }

    KLB_FREE_BY(p_listen->p_socket_nlist, klb_nlist_destroy);

    return 0;
}

//////////////////////////////////////////////////
// 

// call lua 协程
static int call_co_accept_klua_krtspserve_listen(klua_krtspserve_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept
        && NULL != p_listen->p_socket_nlist
        && 0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_accept 函数; 这里会存在执行函数的交替执行

        klua_krtspserve_listen_socket_t* p_listen_socket = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        lua_pushlightuserdata(L, p_listen_socket); // #1. lua - userdata
        lua_pushstring(L, "ok"); // #2. 状态消息

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// call lua 协程 结束: 退出时即使 nlist 已空/已销毁也须唤醒
static int call_co_accept_end_klua_krtspserve_listen(klua_krtspserve_listen_t* p_listen)
{
    assert(NULL != p_listen);

    if (NULL != p_listen->co_accept)
    {
        assert(0 == klua_coroutine_debug_check(p_listen->p_coex, p_listen->co_accept));

        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) return -1; // 未处理

        p_listen->co_accept = NULL; // 清空

        lua_pushlightuserdata(L, NULL); // #1. lua - userdata
        lua_pushstring(L, "exit"); // #2. 状态消息

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

/// @brief 当监听到socket连接建立时回调函数
static int on_accept_klua_krtspserve_listen(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)p_conn->p_udata;

    if (NULL == p_listen->p_socket_nlist)
    {
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klua_krtspserve_listen_socket_t* p_tmp = KLB_MALLOCZ(klua_krtspserve_listen_socket_t, 1, 0);

    {
        p_tmp->fd = fd;
        memcpy(&p_tmp->addr, p_addr, sizeof(struct sockaddr_in));
        p_tmp->tls = tls;
    }

    klb_nlist_push_tail(p_listen->p_socket_nlist, p_tmp);

    // call lua
    call_co_accept_klua_krtspserve_listen(p_listen);

    return 0;
}

//////////////////////////////////////////////////

// 当需要退出等的 退出函数
static int on_yield_accept_klua_krtspserve_listen(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_accept_end_klua_krtspserve_listen(p_listen);
    }

    return 0;
}

static int klua_krtspserve_listen_co_accept(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    klua_check_coroutine(L, "krtspserve:co_accept must in coroutine!");

    // 监听已关闭
    if (NULL == p_listen->p_socket_nlist || NULL == p_listen->p_listen_conn)
    {
        lua_pushlightuserdata(L, NULL);
        lua_pushstring(L, "exit");
        return 2;
    }

    // 若列表中有数据, 则直接返回
    if (0 < klb_nlist_size(p_listen->p_socket_nlist))
    {
        klua_krtspserve_listen_socket_t* p_listen_socket = (klua_krtspserve_listen_socket_t*)klb_nlist_pop_head(p_listen->p_socket_nlist);

        lua_pushlightuserdata(L, p_listen_socket); // #1. lua - userdata
        lua_pushstring(L, "ok"); // #2. 状态消息

        return 2;
    }

    // 若列表中无数据, 则执行协程等待
    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return klua_coroutine_yield(p_listen->p_coex, L, on_yield_accept_klua_krtspserve_listen, p_listen);
}

static int klua_krtspserve_listen_port(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    return klua_netlisten_push_port(L, p_listen->p_listen_conn);
}

static int klua_krtspserve_listen_local_addr(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    return klua_netlisten_push_local_addr(L, p_listen->p_listen_conn);
}

static int klua_krtspserve_listen_native_fd(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    return klua_netlisten_push_native_fd(L, p_listen->p_listen_conn);
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krtspserve_listen_close },     ///< 关闭监听

        { "co_accept",      klua_krtspserve_listen_co_accept }, ///< 接收 新连接
        { "port",           klua_krtspserve_listen_port },
        { "local_addr",     klua_krtspserve_listen_local_addr },
        { "native_fd",      klua_krtspserve_listen_native_fd },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_listen_close },
        { "__close",        klua_krtspserve_listen_close },
        { "__tostring",     klua_krtspserve_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);/* metatable for KLUA_KRTSPSERVE_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////


/// @brief 监听端口
int klua_krtspserve_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);                        ///< @1. 端口号
    int rc = 0;
    int max_connect = 20;
    char host[128] = { 0 };
    klb_netlisten_open_opts_t open_opts = { 0 };
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_listen_conn = NULL;
    klua_krtspserve_listen_t* p_listen = NULL;

    open_opts.family = AF_INET;

    if (klua_is_table(L, 2))
    {
        rc = klua_netlisten_parse_bind_cfg(L, 2, host, (int)sizeof(host), &max_connect, &open_opts);
        if (0 != rc)
        {
            return push_err_klua_krtspserve(L, rc);
        }
    }

    if (NULL == p_netmulti)
    {
        return push_err_klua_krtspserve(L, KLB_NETCODE_INVAL);
    }

    p_listen_conn = klb_netlisten_conn_create(p_netmulti);
    klb_netlisten_conn_set_accept(p_listen_conn, on_accept_klua_krtspserve_listen, NULL);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, ('\0' == host[0]) ? NULL : host,
        port, max_connect, &open_opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return push_err_klua_krtspserve(L, rc);
    }

    p_listen = new_klua_krtspserve_listen(L); ///< #1. 监听对象

    // 初始化
    {
        p_listen->L = L;
        p_listen->co_accept = NULL;
        p_listen->p_env = p_env;
        p_listen->p_coex = klua_coroutine_get(p_env);

        p_listen->p_netmulti = p_netmulti;
        p_listen->p_listen_conn = p_listen_conn;

        p_listen->p_socket_nlist = klb_nlist_create();

        klb_netconn_set_udata(p_listen_conn, p_listen);
    }

    return 1;
}

//end
