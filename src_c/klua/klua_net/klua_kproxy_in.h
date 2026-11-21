// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
/// @file    klua_kproxy_in.h
/// @brief   kproxy 内部: tcp/listen userdata 与 co_* 异步路径
/// @version 0.1
/// @history
///  \n [2026-10] N6 M1.b: 异步 co_* (netmulti)
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_KPROXY_IN_H__
#define __KLUA_KPROXY_IN_H__

#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_netmulti.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netmulti.h"
#include <stdbool.h>

#define KLUA_KPROXY_TCP_HANDLE          "KPROXY_TCP_HANDLE*"
#define KLUA_KPROXY_LISTEN_HANDLE       "KPROXY_LISTEN_HANDLE*"

/// @struct klua_kproxy_tcp_t
/// @brief  TCP 会话 (同步或异步; 可选 TLS)
typedef struct klua_kproxy_tcp_t_
{
    klb_socket_fd               fd;             ///< 裸 fd (无 p_sock 时)
    bool                        closed;         ///< 已 close
    klb_socket_t*               p_tls;          ///< TLS 包装 (可与 p_sock 互斥)

    klua_env_t*                 p_env;          ///< Lua 环境
    klua_ex_coroutine_t*        p_coex;         ///< 协程扩展
    klb_netmulti_t*             p_netmulti;     ///< 复用
    klb_netconn_t*              p_conn;         ///< stream netconn
    klb_socket_t*               p_sock;         ///< 异步 socket (明文路径)

    lua_State*                  co_recv;        ///< co_recv
    lua_State*                  co_send;        ///< co_send
    lua_State*                  co_connect;     ///< co_connect 等待已连接
    lua_State*                  co_tls;         ///< co_upgrade_tls

    char*                       p_wbuf;         ///< co_send 残留
    int                         wbuf_len;
    int                         wbuf_off;
    int                         recv_want;      ///< co_recv 期望最大字节
    char                        tls_host[256];  ///< SNI
    bool                        tls_want;       ///< 正在握手
}klua_kproxy_tcp_t;

/// @struct klua_kproxy_listen_t
/// @brief  TCP 监听
typedef struct klua_kproxy_listen_t_
{
    klb_socket_fd               fd;             ///< listen fd
    bool                        closed;         ///< 已 close

    klua_env_t*                 p_env;
    klua_ex_coroutine_t*        p_coex;
    klb_netmulti_t*             p_netmulti;
    klb_netconn_t*              p_conn;         ///< listen netconn
    klb_socket_t*               p_sock;

    lua_State*                  co_accept;      ///< co_accept
    klb_socket_fd               pending_fd;     ///< 已 accept 待取
}klua_kproxy_listen_t;

klua_kproxy_tcp_t* klua_kproxy_tcp_check(lua_State* L, int index);
klua_kproxy_listen_t* klua_kproxy_listen_check(lua_State* L, int index);
klua_kproxy_tcp_t* klua_kproxy_tcp_new(lua_State* L);
klua_kproxy_listen_t* klua_kproxy_listen_new(lua_State* L);

void klua_kproxy_tcp_detach_async(klua_kproxy_tcp_t* p_tcp);
void klua_kproxy_listen_detach_async(klua_kproxy_listen_t* p_ls);

int klua_kproxy_tcp_co_recv(lua_State* L);
int klua_kproxy_tcp_co_send(lua_State* L);
int klua_kproxy_tcp_co_upgrade_tls(lua_State* L);
int klua_kproxy_co_connect(lua_State* L);
int klua_kproxy_listen_co_accept(lua_State* L);

int klua_kproxy_tcp_ensure_async(klua_kproxy_tcp_t* p_tcp, lua_State* L);
int klua_kproxy_listen_ensure_async(klua_kproxy_listen_t* p_ls, lua_State* L);

/// @brief 从 Lua opts 表套客户端 TLS 选项 (upgrade_tls / co_upgrade_tls)
int klua_kproxy_apply_tls_opts(lua_State* L, int idx, klb_socket_t* p_tls, const char** pp_err);

#endif // __KLUA_KPROXY_IN_H__

// end
