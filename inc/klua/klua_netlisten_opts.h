// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_netlisten_opts.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   Lua listen cfg → klb_netlisten_open_opts / host / backlog 解析辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_NETLISTEN_OPTS_H__
#define __KLUA_NETLISTEN_OPTS_H__

#include "klbnet/klb_socket.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klua/klua.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief family 串 → AF_*; "dual"→AF_UNSPEC; 空/NULL/"ipv4"→AF_INET
/// @return 0 成功; -1 非法
static inline int klua_netlisten_parse_family(const char* p_family, int* p_out)
{
    assert(NULL != p_out);

    if (NULL == p_family || '\0' == p_family[0] || 0 == strcmp(p_family, "ipv4"))
    {
        *p_out = AF_INET;
        return 0;
    }

    if (0 == strcmp(p_family, "ipv6"))
    {
        *p_out = AF_INET6;
        return 0;
    }

    if (0 == strcmp(p_family, "dual"))
    {
        *p_out = AF_UNSPEC;
        return 0;
    }

    return -1;
}


/// @brief 从 cfg 表填充 open_opts; 非法 family 时 family=-1
static inline void klua_netlisten_fill_open_opts(lua_State* L, int idx, klb_netlisten_open_opts_t* p_opts)
{
    assert(NULL != p_opts);

    p_opts->family = AF_INET;
    p_opts->reuseport = false;
    p_opts->nodelay = false;
    p_opts->keepalive = false;

    if (!klua_is_table(L, idx))
    {
        return;
    }

    lua_getfield(L, idx, "family");
    if (lua_isstring(L, -1))
    {
        int family = AF_INET;

        if (0 != klua_netlisten_parse_family(lua_tostring(L, -1), &family))
        {
            p_opts->family = -1;
        }
        else
        {
            p_opts->family = family;
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "reuseport");
    if (!lua_isnil(L, -1))
    {
        p_opts->reuseport = klua_check_option_boolean(L, -1, false);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "nodelay");
    if (!lua_isnil(L, -1))
    {
        p_opts->nodelay = klua_check_option_boolean(L, -1, false);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "keepalive");
    if (!lua_isnil(L, -1))
    {
        p_opts->keepalive = klua_check_option_boolean(L, -1, false);
    }
    lua_pop(L, 1);
}


/// @brief 解析 bind 相关: host / max_connect / family / reuseport / nodelay / keepalive
/// @param [in,out] *p_max_connect  入参为默认 backlog; 下限钳到 1
/// @return 0 成功; 非0 klb_netcode_e
static inline int klua_netlisten_parse_bind_cfg(lua_State* L, int idx,
    char* p_host, int host_cap, int* p_max_connect, klb_netlisten_open_opts_t* p_opts)
{
    assert(NULL != p_host);
    assert(0 < host_cap);
    assert(NULL != p_max_connect);
    assert(NULL != p_opts);

    p_host[0] = '\0';
    p_opts->family = AF_INET;
    p_opts->reuseport = false;
    p_opts->nodelay = false;
    p_opts->keepalive = false;

    if (!klua_is_table(L, idx))
    {
        if (1 > *p_max_connect)
        {
            *p_max_connect = 1;
        }
        return 0;
    }

    lua_getfield(L, idx, "max_connect");
    if (!lua_isnil(L, -1))
    {
        *p_max_connect = (int)klua_check_option_integer(L, -1, *p_max_connect);
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "host");
    if (!lua_isnil(L, -1))
    {
        const char* p = luaL_checkstring(L, -1);

        if ((int)strlen(p) >= host_cap)
        {
            lua_pop(L, 1);
            return KLB_NETCODE_INVAL;
        }

        if ('\0' != p[0])
        {
            strncpy(p_host, p, (size_t)host_cap - 1);
            p_host[host_cap - 1] = '\0';
        }
    }
    lua_pop(L, 1);

    klua_netlisten_fill_open_opts(L, idx, p_opts);
    if (-1 == p_opts->family)
    {
        return KLB_NETCODE_INVAL;
    }

    if (1 > *p_max_connect)
    {
        *p_max_connect = 1;
    }

    return 0;
}


/// @brief 压入 local_addr: ip[string|nil], port[number|nil]
static inline int klua_netlisten_push_local_addr(lua_State* L, klb_netconn_t* p_conn)
{
    struct sockaddr_storage addr = { 0 };
    char ip[64] = { 0 };
    int port = 0;

    if (NULL == p_conn
        || 0 != klb_netlisten_conn_get_local_sa(p_conn, &addr, NULL)
        || 0 != klb_socket_format_addr(&addr, ip, (int)sizeof(ip), &port))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    return 2;
}


/// @brief 压入 native_fd[number|nil]
static inline int klua_netlisten_push_native_fd(lua_State* L, klb_netconn_t* p_conn)
{
    klb_socket_fd fd = INVALID_SOCKET;

    if (NULL != p_conn)
    {
        fd = klb_netlisten_conn_native_fd(p_conn);
    }

    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, (lua_Integer)fd);
    return 1;
}


/// @brief 压入监听端口 (get_port; 未 open 为 0)
static inline int klua_netlisten_push_port(lua_State* L, klb_netconn_t* p_conn)
{
    int port = 0;

    if (NULL != p_conn)
    {
        port = klb_netlisten_conn_get_port(p_conn);
    }

    lua_pushinteger(L, port);
    return 1;
}


/// @brief 格式化对端 "ip:port" (IPv4 sockaddr_in; IPv6 映射已折算)
static inline void klua_netlisten_format_peer_in(char* p_dst, int cap, const struct sockaddr_in* p_addr)
{
    unsigned int ip = 0;
    int port = 0;
    int n = 0;

    assert(NULL != p_dst);
    assert(0 < cap);
    assert(NULL != p_addr);

    ip = ntohl(p_addr->sin_addr.s_addr);
    port = (int)ntohs(p_addr->sin_port);
    n = snprintf(p_dst, (size_t)cap, "%u.%u.%u.%u:%d",
        (ip >> 24) & 0xFFu,
        (ip >> 16) & 0xFFu,
        (ip >> 8) & 0xFFu,
        ip & 0xFFu,
        port);

    if (0 > n || n >= cap)
    {
        p_dst[0] = '\0';
    }

    p_dst[cap - 1] = '\0';
}


#if defined(__cplusplus)
}
#endif

#endif // __KLUA_NETLISTEN_OPTS_H__

// end
