// Doc Encode : UTF-8 BOM, Unix(LF)
// kproxy: 代理链泵送 / 原始 TCP / 入口 listen Lua 绑定
#include "klua_kproxy_in.h"
#include "klbmem/klb_mem.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#endif


//////////////////////////////////////////////////////////////////////////
// 内部 tcp


klua_kproxy_tcp_t* klua_kproxy_tcp_new(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = (klua_kproxy_tcp_t*)lua_newuserdata(L, sizeof(klua_kproxy_tcp_t));
    KLB_MEMSET(p_tcp, 0, sizeof(klua_kproxy_tcp_t));
    p_tcp->fd = INVALID_SOCKET;
    luaL_setmetatable(L, KLUA_KPROXY_TCP_HANDLE);
    return p_tcp;
}


klua_kproxy_tcp_t* klua_kproxy_tcp_check(lua_State* L, int index)
{
    klua_kproxy_tcp_t* p_tcp = (klua_kproxy_tcp_t*)luaL_checkudata(L, index, KLUA_KPROXY_TCP_HANDLE);
    luaL_argcheck(L, NULL != p_tcp, index, "'kproxy.tcp' expected");
    return p_tcp;
}


static klua_kproxy_tcp_t* to_klua_kproxy_tcp(lua_State* L, int index)
{
    return klua_kproxy_tcp_check(L, index);
}


static void close_fd_klua_kproxy_tcp(klua_kproxy_tcp_t* p_tcp)
{
    if (NULL == p_tcp)
    {
        return;
    }

    if (p_tcp->closed)
    {
        return;
    }

    // 先卸异步 (closing netconn 会 destroy p_sock/p_tls)
    klua_kproxy_tcp_detach_async(p_tcp);

    if (NULL != p_tcp->p_tls)
    {
        klb_socket_destroy(p_tcp->p_tls);
        p_tcp->p_tls = NULL;
        p_tcp->fd = INVALID_SOCKET;
    }
    else if (INVALID_SOCKET != p_tcp->fd)
    {
        KLB_SOCKET_CLOSE(p_tcp->fd);
        p_tcp->fd = INVALID_SOCKET;
    }

    p_tcp->closed = true;
}


/// @brief 按 TLS socket status_rw 等待可读/可写
/// @return 0.就绪; -1.失败/超时
static int wait_rw_klua_kproxy_tcp(klb_socket_t* p_socket, int timeout_ms)
{
    fd_set rfds;
    fd_set wfds;
    struct timeval tv;
    struct timeval* p_tv = NULL;
    int nready = 0;

    if (NULL == p_socket || INVALID_SOCKET == p_socket->fd)
    {
        return -1;
    }

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    if (KLB_SOCKET_WAIT_WRITE == p_socket->status_rw)
    {
        FD_SET(p_socket->fd, &wfds);
    }
    else
    {
        FD_SET(p_socket->fd, &rfds);
    }

    if (0 <= timeout_ms)
    {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        p_tv = &tv;
    }

    nready = select((int)p_socket->fd + 1, &rfds, &wfds, NULL, p_tv);
    if (0 >= nready)
    {
        return -1;
    }

    return 0;
}


klua_kproxy_listen_t* klua_kproxy_listen_new(lua_State* L)
{
    klua_kproxy_listen_t* p_ls = (klua_kproxy_listen_t*)lua_newuserdata(L, sizeof(klua_kproxy_listen_t));
    KLB_MEMSET(p_ls, 0, sizeof(klua_kproxy_listen_t));
    p_ls->fd = INVALID_SOCKET;
    p_ls->pending_fd = INVALID_SOCKET;
    luaL_setmetatable(L, KLUA_KPROXY_LISTEN_HANDLE);
    return p_ls;
}


klua_kproxy_listen_t* klua_kproxy_listen_check(lua_State* L, int index)
{
    klua_kproxy_listen_t* p_ls = (klua_kproxy_listen_t*)luaL_checkudata(L, index, KLUA_KPROXY_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_ls, index, "'kproxy.listen' expected");
    return p_ls;
}


static klua_kproxy_listen_t* to_klua_kproxy_listen(lua_State* L, int index)
{
    return klua_kproxy_listen_check(L, index);
}


static void close_fd_klua_kproxy_listen(klua_kproxy_listen_t* p_ls)
{
    if (NULL == p_ls)
    {
        return;
    }

    if (p_ls->closed)
    {
        return;
    }

    klua_kproxy_listen_detach_async(p_ls);

    if (INVALID_SOCKET != p_ls->fd)
    {
        KLB_SOCKET_CLOSE(p_ls->fd);
        p_ls->fd = INVALID_SOCKET;
    }

    p_ls->closed = true;
}


/// @brief 在 host:port 上 listen
static klb_socket_fd listen_host_klua_kproxy(const char* p_host, int port, int backlog)
{
    char str_port[16] = { 0 };
    struct addrinfo hints;
    struct addrinfo* p_result = NULL;
    struct addrinfo* p_next = NULL;
    klb_socket_fd fd = INVALID_SOCKET;
    int ret = 0;

    KLB_MEMSET(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    snprintf(str_port, sizeof(str_port), "%d", port);
    str_port[sizeof(str_port) - 1] = '\0';

    ret = getaddrinfo(p_host, str_port, &hints, &p_result);
    if (0 != ret)
    {
        return INVALID_SOCKET;
    }

    p_next = p_result;
    while (NULL != p_next)
    {
        fd = socket(p_next->ai_family, p_next->ai_socktype, p_next->ai_protocol);
        if (INVALID_SOCKET == fd)
        {
            p_next = p_next->ai_next;
            continue;
        }

        if (0 != klb_socket_set_reuseaddr(fd))
        {
            KLB_SOCKET_CLOSE(fd);
            p_next = p_next->ai_next;
            continue;
        }

        if (0 != bind(fd, p_next->ai_addr, (int)p_next->ai_addrlen))
        {
            KLB_SOCKET_CLOSE(fd);
            fd = INVALID_SOCKET;
            p_next = p_next->ai_next;
            continue;
        }

        if (0 != listen(fd, backlog))
        {
            KLB_SOCKET_CLOSE(fd);
            fd = INVALID_SOCKET;
            p_next = p_next->ai_next;
            continue;
        }

        break;
    }

    KLB_FREE_BY(p_result, freeaddrinfo);
    return fd;
}


/// @brief 模块是否可用
static int klua_kproxy_available(lua_State* L)
{
    lua_pushboolean(L, 1);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// relay / listen_raw


/// @brief 双向泵送 a <-> b (select)
/// @note Lua: ok, err = kproxy.relay(a, b [, opts]); a/b 为 kproxy.tcp
static int klua_kproxy_relay(lua_State* L)
{
    klua_kproxy_tcp_t* p_a = NULL;
    klua_kproxy_tcp_t* p_b = NULL;
    char* p_buf = NULL;
    int timeout_ms = 0;
    fd_set rfds;
    struct timeval tv;
    struct timeval* p_tv = NULL;
    int maxfd = 0;
    int nready = 0;
    int n = 0;

    if (2 > lua_gettop(L))
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "need a and b");
        return 2;
    }

    p_a = to_klua_kproxy_tcp(L, 1);
    p_b = to_klua_kproxy_tcp(L, 2);

    if (3 <= lua_gettop(L) && LUA_TTABLE == lua_type(L, 3))
    {
        lua_getfield(L, 3, "timeout_ms");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            timeout_ms = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    else if (3 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 3) && LUA_TTABLE != lua_type(L, 3))
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "opts must be table");
        return 2;
    }

    if (p_a->closed || p_b->closed || INVALID_SOCKET == p_a->fd || INVALID_SOCKET == p_b->fd)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "closed");
        return 2;
    }

    // step1. 非阻塞便于 select
    klb_socket_set_block(p_a->fd, false);
    klb_socket_set_block(p_b->fd, false);

    p_buf = KLB_MALLOC(char, 16384, 0);

    // step2. 循环双向拷贝直至一端关闭
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(p_a->fd, &rfds);
        FD_SET(p_b->fd, &rfds);
        maxfd = (int)p_a->fd;
        if ((int)p_b->fd > maxfd)
        {
            maxfd = (int)p_b->fd;
        }

        p_tv = NULL;
        if (0 < timeout_ms)
        {
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            p_tv = &tv;
        }

        nready = select(maxfd + 1, &rfds, NULL, NULL, p_tv);
        if (0 > nready)
        {
#ifdef _WIN32
            KLB_FREE(p_buf);
            lua_pushboolean(L, 0);
            lua_pushstring(L, "select fail");
            return 2;
#else
            if (EINTR == errno)
            {
                continue;
            }

            KLB_FREE(p_buf);
            lua_pushboolean(L, 0);
            lua_pushstring(L, "select fail");
            return 2;
#endif
        }

        if (0 == nready)
        {
            KLB_FREE(p_buf);
            lua_pushboolean(L, 0);
            lua_pushstring(L, "timeout");
            return 2;
        }

        if (FD_ISSET(p_a->fd, &rfds))
        {
#ifdef _WIN32
            n = recv(p_a->fd, p_buf, 16384, 0);
#else
            n = (int)recv(p_a->fd, p_buf, 16384, 0);
#endif
            if (0 >= n)
            {
                break;
            }

#ifdef _WIN32
            if (0 > send(p_b->fd, p_buf, n, 0))
#else
            if (0 > send(p_b->fd, p_buf, (size_t)n, 0))
#endif
            {
                break;
            }
        }

        if (FD_ISSET(p_b->fd, &rfds))
        {
#ifdef _WIN32
            n = recv(p_b->fd, p_buf, 16384, 0);
#else
            n = (int)recv(p_b->fd, p_buf, 16384, 0);
#endif
            if (0 >= n)
            {
                break;
            }

#ifdef _WIN32
            if (0 > send(p_a->fd, p_buf, n, 0))
#else
            if (0 > send(p_a->fd, p_buf, (size_t)n, 0))
#endif
            {
                break;
            }
        }
    }

    KLB_FREE(p_buf);
    lua_pushboolean(L, 1);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// tcp userdata


static int klua_kproxy_tcp_close(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    close_fd_klua_kproxy_tcp(p_tcp);
    return 0;
}


static int klua_kproxy_tcp_send(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    size_t len = 0;
    const char* p_data = NULL;
    size_t off = 0;

    p_data = luaL_checklstring(L, 2, &len);

    if (p_tcp->closed || INVALID_SOCKET == p_tcp->fd)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (0 == len)
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    // TLS 路径: 非阻塞 + WANT 重试
    if (NULL != p_tcp->p_tls)
    {
        while (off < len)
        {
            int close_status = KLB_SOCKET_DISCONNECT;
            int judge = 0;
            int n = klb_socket_send(p_tcp->p_tls, (const uint8_t*)(p_data + off), (int)(len - off));

            if (0 < n)
            {
                off += (size_t)n;
                continue;
            }

            judge = klb_socket_send_judge(p_tcp->p_tls, n, &close_status);
            if (KLB_SOCKET_SEND_AGAIN == judge)
            {
                if (0 != wait_rw_klua_kproxy_tcp(p_tcp->p_tls, 15000))
                {
                    lua_pushboolean(L, 0);
                    lua_pushstring(L, "tls send timeout");
                    return 2;
                }
                continue;
            }

            klb_socket_recv_closed(p_tcp->p_tls, close_status);
            lua_pushboolean(L, 0);
            lua_pushstring(L, "tls send fail");
            return 2;
        }

        lua_pushboolean(L, 1);
        return 1;
    }

    while (off < len)
    {
        int n = 0;
#ifdef _WIN32
        n = send(p_tcp->fd, p_data + off, (int)(len - off), 0);
#else
        n = (int)send(p_tcp->fd, p_data + off, len - off, 0);
#endif
        if (0 > n)
        {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "send fail");
            return 2;
        }

        if (0 == n)
        {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "send eof");
            return 2;
        }

        off += (size_t)n;
    }

    lua_pushboolean(L, 1);
    return 1;
}


/// @brief recv; 可选 timeout_ms (缺省阻塞)
/// @note Lua: data, err = tcp:recv(max_n [, timeout_ms]); timeout 时 data=nil, err="timeout"
static int klua_kproxy_tcp_recv(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    int max_n = (int)luaL_checkinteger(L, 2);
    int timeout_ms = -1;
    char* p_buf = NULL;
    int n = 0;

    if (3 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 3))
    {
        timeout_ms = (int)luaL_checkinteger(L, 3);
    }

    if (p_tcp->closed || INVALID_SOCKET == p_tcp->fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (1 > max_n)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad max");
        return 2;
    }

    if (65536 < max_n)
    {
        max_n = 65536;
    }

    // TLS 路径
    if (NULL != p_tcp->p_tls)
    {
        int wait_ms = (0 <= timeout_ms) ? timeout_ms : 15000;
        int close_status = KLB_SOCKET_DISCONNECT;

        p_buf = KLB_MALLOC(char, max_n, 0);
        for (;;)
        {
            int judge = 0;

            n = klb_socket_recv(p_tcp->p_tls, (uint8_t*)p_buf, max_n);
            judge = klb_socket_recv_judge(p_tcp->p_tls, n, &close_status);

            if (KLB_SOCKET_RECV_DATA == judge)
            {
                lua_pushlstring(L, p_buf, (size_t)n);
                KLB_FREE(p_buf);
                return 1;
            }

            // WANT: 按 status_rw 等就绪再试; 禁在 EOF 可读上自旋 (Bug.2026-7)
            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                if (0 != wait_rw_klua_kproxy_tcp(p_tcp->p_tls, wait_ms))
                {
                    KLB_FREE(p_buf);
                    lua_pushnil(L);
                    lua_pushstring(L, "timeout");
                    return 2;
                }
                continue;
            }

            KLB_FREE(p_buf);
            klb_socket_recv_closed(p_tcp->p_tls, close_status);
            lua_pushnil(L);
            lua_pushstring(L, (KLB_SOCKET_RECV_EOF == judge) ? "eof" : "tls recv fail");
            return 2;
        }
    }

    if (0 <= timeout_ms)
    {
        fd_set rfds;
        struct timeval tv;
        int nready = 0;

        FD_ZERO(&rfds);
        FD_SET(p_tcp->fd, &rfds);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        nready = select((int)p_tcp->fd + 1, &rfds, NULL, NULL, &tv);
        if (0 > nready)
        {
            lua_pushnil(L);
            lua_pushstring(L, "select fail");
            return 2;
        }

        if (0 == nready)
        {
            lua_pushnil(L);
            lua_pushstring(L, "timeout");
            return 2;
        }
    }

    p_buf = KLB_MALLOC(char, max_n, 0);

#ifdef _WIN32
    n = recv(p_tcp->fd, p_buf, max_n, 0);
#else
    n = (int)recv(p_tcp->fd, p_buf, (size_t)max_n, 0);
#endif

    if (0 > n)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "recv fail");
        return 2;
    }

    if (0 == n)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "eof");
        return 2;
    }

    lua_pushlstring(L, p_buf, (size_t)n);
    KLB_FREE(p_buf);
    return 1;
}


/// @brief 从 Lua 表套 TLS 选项 (hostname/sni/verify/ca_pem/alpn/pin_sha256)
/// @return 0.ok; 非0 时 *pp_err 为静态错误串
int klua_kproxy_apply_tls_opts(lua_State* L, int idx, klb_socket_t* p_tls, const char** pp_err)
{
    const char* p_host = NULL;
    int verify = 0;
    int do_sni = 1;

    *pp_err = "tls opts bad";
    if (!lua_istable(L, idx))
    {
        return 1;
    }

    lua_getfield(L, idx, "hostname");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        p_host = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    if (NULL == p_host)
    {
        lua_getfield(L, idx, "sni");
        if (LUA_TSTRING == lua_type(L, -1))
        {
            p_host = lua_tostring(L, -1);
        }
        else if (LUA_TBOOLEAN == lua_type(L, -1) && 0 == lua_toboolean(L, -1))
        {
            do_sni = 0;
        }
        lua_pop(L, 1);
    }

    lua_getfield(L, idx, "verify");
    if (LUA_TBOOLEAN == lua_type(L, -1))
    {
        verify = (0 != lua_toboolean(L, -1)) ? 2 : 0;
    }
    else if (LUA_TNUMBER == lua_type(L, -1))
    {
        verify = (int)lua_tointeger(L, -1);
    }
    else if (LUA_TSTRING == lua_type(L, -1))
    {
        const char* p_v = lua_tostring(L, -1);
        if (NULL != p_v && (0 == strcmp(p_v, "required") || 0 == strcmp(p_v, "yes") || 0 == strcmp(p_v, "true")))
        {
            verify = 2;
        }
        else if (NULL != p_v && 0 == strcmp(p_v, "optional"))
        {
            verify = 1;
        }
        else
        {
            verify = 0;
        }
    }
    lua_pop(L, 1);

    if (0 != klb_socket_tls_set_verify(p_tls, verify))
    {
        *pp_err = "tls verify set fail";
        return 1;
    }

    lua_getfield(L, idx, "ca_pem");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        size_t ca_len = 0;
        const char* p_ca = lua_tolstring(L, -1, &ca_len);
        if (NULL == p_ca || 0 == ca_len || 0 != klb_socket_tls_set_ca_pem(p_tls, p_ca, (int)ca_len))
        {
            lua_pop(L, 1);
            *pp_err = "tls ca_pem bad";
            return 1;
        }
    }
    lua_pop(L, 1);

    if (2 <= verify)
    {
        // REQUIRED 且无 ca: 仍允许 (由对端校验失败); 文档要求宜配 ca_pem
    }

    lua_getfield(L, idx, "alpn");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        const char* p_alpn = lua_tostring(L, -1);
        if (0 != klb_socket_tls_set_alpn(p_tls, p_alpn))
        {
            lua_pop(L, 1);
            *pp_err = "tls alpn bad";
            return 1;
        }
    }
    else if (LUA_TTABLE == lua_type(L, -1))
    {
        // 拼 csv
        luaL_Buffer b;
        size_t n = lua_rawlen(L, -1);
        size_t i = 0;
        luaL_buffinit(L, &b);
        for (i = 1; i <= n; i++)
        {
            lua_rawgeti(L, -1, (lua_Integer)i);
            if (LUA_TSTRING == lua_type(L, -1))
            {
                if (1 < i)
                {
                    luaL_addchar(&b, ',');
                }
                luaL_addstring(&b, lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
        luaL_pushresult(&b);
        {
            const char* p_alpn = lua_tostring(L, -1);
            int rc_a = klb_socket_tls_set_alpn(p_tls, p_alpn);
            lua_pop(L, 1);
            if (0 != rc_a)
            {
                lua_pop(L, 1);
                *pp_err = "tls alpn bad";
                return 1;
            }
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, idx, "pin_sha256");
    if (LUA_TSTRING == lua_type(L, -1))
    {
        const char* p_pin = lua_tostring(L, -1);
        if (0 != klb_socket_tls_set_pin_sha256(p_tls, p_pin))
        {
            lua_pop(L, 1);
            *pp_err = "tls pin_sha256 bad";
            return 1;
        }
    }
    lua_pop(L, 1);

    if (0 != do_sni && NULL != p_host && '\0' != p_host[0])
    {
        (void)klb_socket_tls_set_hostname(p_tls, p_host);
    }

    *pp_err = NULL;
    return 0;
}


/// @brief 明文隧道就地升级 TLS (https over socks/CONNECT)
/// @note Lua: ok, err = tcp:upgrade_tls([hostname|opts] [, timeout_ms])
static int klua_kproxy_tcp_upgrade_tls(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    const char* p_host = NULL;
    int opts_idx = 0;
    int timeout_ms = 15000;
    klb_socket_t* p_tls = NULL;
    int top = lua_gettop(L);
    int64_t t0_ms = 0;
    int64_t now_ms = 0;
    const char* p_err = NULL;

    if (2 <= top && LUA_TTABLE == lua_type(L, 2))
    {
        opts_idx = 2;
        lua_getfield(L, 2, "hostname");
        if (LUA_TSTRING == lua_type(L, -1))
        {
            p_host = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        if (NULL == p_host)
        {
            lua_getfield(L, 2, "sni");
            if (LUA_TSTRING == lua_type(L, -1))
            {
                p_host = lua_tostring(L, -1);
            }
            lua_pop(L, 1);
        }
        lua_getfield(L, 2, "timeout_ms");
        if (LUA_TNUMBER == lua_type(L, -1))
        {
            timeout_ms = (int)lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    else if (2 <= top && LUA_TNIL != lua_type(L, 2) && LUA_TNUMBER != lua_type(L, 2))
    {
        p_host = luaL_checkstring(L, 2);
    }

    if (2 <= top && LUA_TNUMBER == lua_type(L, 2))
    {
        timeout_ms = (int)luaL_checkinteger(L, 2);
    }
    else if (3 <= top && LUA_TNIL != lua_type(L, 3))
    {
        timeout_ms = (int)luaL_checkinteger(L, 3);
    }

    if (1 > timeout_ms)
    {
        timeout_ms = 1;
    }

    if (p_tcp->closed || INVALID_SOCKET == p_tcp->fd)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (NULL != p_tcp->p_tls)
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    p_tls = klb_socket_async_create_tls(p_tcp->fd);
    if (NULL == p_tls)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "tls create fail");
        return 2;
    }

    if (0 != opts_idx)
    {
        if (0 != klua_kproxy_apply_tls_opts(L, opts_idx, p_tls, &p_err))
        {
            {
                klb_socket_fd fd = klb_socket_detach_fd(p_tls);
                klb_socket_destroy(p_tls);
                p_tcp->fd = fd;
            }
            lua_pushboolean(L, 0);
            lua_pushstring(L, (NULL != p_err) ? p_err : "tls opts bad");
            return 2;
        }
    }
    else if (NULL != p_host && '\0' != p_host[0])
    {
        (void)klb_socket_tls_set_hostname(p_tls, p_host);
    }

    klb_socket_set_connected(p_tls, true);

    // 握手循环 (非阻塞 + select)
    t0_ms = (int64_t)time(NULL) * 1000;
    for (;;)
    {
        int rc = klb_socket_tls_handshake(p_tls);
        if (0 == rc)
        {
            break;
        }

        if (0 > rc)
        {
            {
                klb_socket_fd fd = klb_socket_detach_fd(p_tls);
                klb_socket_destroy(p_tls);
                p_tcp->fd = fd;
            }
            lua_pushboolean(L, 0);
            lua_pushstring(L, "tls handshake fail");
            return 2;
        }

        now_ms = (int64_t)time(NULL) * 1000;
        if (now_ms - t0_ms > (int64_t)timeout_ms)
        {
            {
                klb_socket_fd fd = klb_socket_detach_fd(p_tls);
                klb_socket_destroy(p_tls);
                p_tcp->fd = fd;
            }
            lua_pushboolean(L, 0);
            lua_pushstring(L, "tls handshake timeout");
            return 2;
        }

        if (KLB_SOCKET_WAIT_READ == p_tls->status_rw || KLB_SOCKET_WAIT_WRITE == p_tls->status_rw)
        {
            if (0 != wait_rw_klua_kproxy_tcp(p_tls, timeout_ms))
            {
                {
                    klb_socket_fd fd = klb_socket_detach_fd(p_tls);
                    klb_socket_destroy(p_tls);
                    p_tcp->fd = fd;
                }
                lua_pushboolean(L, 0);
                lua_pushstring(L, "tls handshake timeout");
                return 2;
            }
        }
        else
        {
#ifdef _WIN32
            Sleep(1);
#else
            usleep(1000);
#endif
        }
    }

    if (0 != klb_socket_tls_check_pin(p_tls))
    {
        {
            klb_socket_fd fd = klb_socket_detach_fd(p_tls);
            klb_socket_destroy(p_tls);
            p_tcp->fd = fd;
        }
        lua_pushboolean(L, 0);
        lua_pushstring(L, "tls pin mismatch");
        return 2;
    }

    p_tcp->p_tls = p_tls;
    lua_pushboolean(L, 1);
    return 1;
}


/// @brief 窥探接收 (MSG_PEEK, 不消费); 供同口 demux 后把 fd 交给 ksmp/kmnp
/// @note Lua: data, err = tcp:recv_peek(max_n)
static int klua_kproxy_tcp_recv_peek(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    int max_n = (int)luaL_checkinteger(L, 2);
    char* p_buf = NULL;
    int n = 0;
    klb_socket_t* p_sock = NULL;

    if (p_tcp->closed)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (1 > max_n)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad max");
        return 2;
    }

    if (65536 < max_n)
    {
        max_n = 65536;
    }

    p_buf = KLB_MALLOC(char, max_n, 0);

    if (NULL != p_tcp->p_tls)
    {
        n = klb_socket_recv_peek(p_tcp->p_tls, (uint8_t*)p_buf, max_n);
    }
    else if (NULL != p_tcp->p_conn && NULL != p_tcp->p_conn->p_socket)
    {
        p_sock = p_tcp->p_conn->p_socket;
        n = klb_socket_recv_peek(p_sock, (uint8_t*)p_buf, max_n);
    }
    else if (INVALID_SOCKET != p_tcp->fd)
    {
#ifdef _WIN32
        n = recv(p_tcp->fd, p_buf, max_n, MSG_PEEK);
#else
        n = (int)recv(p_tcp->fd, p_buf, (size_t)max_n, MSG_PEEK);
#endif
    }
    else
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (0 > n)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "peek fail");
        return 2;
    }

    if (0 == n)
    {
        KLB_FREE(p_buf);
        lua_pushnil(L);
        lua_pushstring(L, "eof");
        return 2;
    }

    lua_pushlstring(L, p_buf, (size_t)n);
    KLB_FREE(p_buf);
    return 1;
}


/// @brief 取 tcp 当前有效 fd (异步 sock / tls / 裸 fd)
static klb_socket_fd fd_of_klua_kproxy_tcp(klua_kproxy_tcp_t* p_tcp)
{
    if (NULL == p_tcp || p_tcp->closed)
    {
        return INVALID_SOCKET;
    }

    if (NULL != p_tcp->p_conn && NULL != p_tcp->p_conn->p_socket)
    {
        return klb_socket_get_fd(p_tcp->p_conn->p_socket);
    }

    if (NULL != p_tcp->p_tls)
    {
        return klb_socket_get_fd(p_tcp->p_tls);
    }

    if (NULL != p_tcp->p_sock)
    {
        return klb_socket_get_fd(p_tcp->p_sock);
    }

    return p_tcp->fd;
}


/// @brief 回读已连接 tcp 本地地址 (getsockname)
/// @return ip[string|nil], port[number|nil]
static int klua_kproxy_tcp_local_addr(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    klb_socket_fd fd = fd_of_klua_kproxy_tcp(p_tcp);
    struct sockaddr_storage addr;
    char ip[64] = { 0 };
    int port = 0;

    KLB_MEMSET(&addr, 0, sizeof(addr));

    if (INVALID_SOCKET == fd
        || 0 != klb_socket_get_local_sa(fd, &addr, NULL))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    if (0 != klb_socket_format_addr(&addr, ip, (int)sizeof(ip), &port))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    return 2;
}


/// @brief 回读已连接 tcp 对端地址 (getpeername)
/// @return ip[string|nil], port[number|nil]
static int klua_kproxy_tcp_peer_addr(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    klb_socket_fd fd = fd_of_klua_kproxy_tcp(p_tcp);
    struct sockaddr_storage addr;
    char ip[64] = { 0 };
    int port = 0;

    KLB_MEMSET(&addr, 0, sizeof(addr));

    if (INVALID_SOCKET == fd
        || 0 != klb_socket_get_peer_sa(fd, &addr, NULL))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    if (0 != klb_socket_format_addr(&addr, ip, (int)sizeof(ip), &port))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    return 2;
}


/// @brief 取出裸 fd 并封装为 klb_netlisten_socket_t* (lightuserdata); tcp 置 closed 且不再关 fd
/// @note Lua: sock, err = tcp:steal_listen_socket(); 供 ksmp/kmnp.new_serve
static int klua_kproxy_tcp_steal_listen_socket(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = to_klua_kproxy_tcp(L, 1);
    klb_socket_fd fd = INVALID_SOCKET;
    klb_netlisten_socket_t* p_ls = NULL;

    if (p_tcp->closed)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (NULL != p_tcp->p_conn && NULL != p_tcp->p_conn->p_socket)
    {
        fd = klb_socket_detach_fd(p_tcp->p_conn->p_socket);
        klua_kproxy_tcp_detach_async(p_tcp);
    }
    else if (NULL != p_tcp->p_tls)
    {
        fd = klb_socket_detach_fd(p_tcp->p_tls);
        klb_socket_destroy(p_tcp->p_tls);
        p_tcp->p_tls = NULL;
        p_tcp->fd = INVALID_SOCKET;
    }
    else if (INVALID_SOCKET != p_tcp->fd)
    {
        fd = p_tcp->fd;
        p_tcp->fd = INVALID_SOCKET;
    }

    if (INVALID_SOCKET == fd)
    {
        p_tcp->closed = true;
        lua_pushnil(L);
        lua_pushstring(L, "no fd");
        return 2;
    }

    p_tcp->closed = true;
    p_tcp->fd = INVALID_SOCKET;
    p_tcp->p_sock = NULL;
    p_tcp->p_conn = NULL;
    p_tcp->p_tls = NULL;

    p_ls = KLB_MALLOCZ(klb_netlisten_socket_t, 1, 0);
    p_ls->fd = fd;
    p_ls->tls = false;
    KLB_MEMSET(&p_ls->addr, 0, sizeof(p_ls->addr));

    lua_pushlightuserdata(L, p_ls);
    return 1;
}


static void klua_kproxy_tcp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] =
    {
        { "close",                  klua_kproxy_tcp_close },
        { "send",                   klua_kproxy_tcp_send },
        { "recv",                   klua_kproxy_tcp_recv },
        { "recv_peek",              klua_kproxy_tcp_recv_peek },
        { "local_addr",             klua_kproxy_tcp_local_addr },
        { "peer_addr",              klua_kproxy_tcp_peer_addr },
        { "steal_listen_socket",    klua_kproxy_tcp_steal_listen_socket },
        { "upgrade_tls",            klua_kproxy_tcp_upgrade_tls },
        { "co_recv",                klua_kproxy_tcp_co_recv },
        { "co_send",                klua_kproxy_tcp_co_send },
        { "co_upgrade_tls",         klua_kproxy_tcp_co_upgrade_tls },
        { NULL,                     NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",    NULL },
        { "__gc",       klua_kproxy_tcp_close },
        { "__close",    klua_kproxy_tcp_close },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KPROXY_TCP_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


static int klua_kproxy_tcp_connect(lua_State* L)
{
    const char* p_host = NULL;
    int port = 0;
    int timeout_ms = 15000;
    klb_socket_fd fd = INVALID_SOCKET;
    klua_kproxy_tcp_t* p_tcp = NULL;

    p_host = luaL_checkstring(L, 1);
    port = (int)luaL_checkinteger(L, 2);

    if (3 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 3))
    {
        timeout_ms = (int)luaL_checkinteger(L, 3);
    }

    if (NULL == p_host || '\0' == p_host[0])
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad host");
        return 2;
    }

    if (1 > port || 65535 < port)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad port");
        return 2;
    }

    if (1 > timeout_ms)
    {
        timeout_ms = 1;
    }

    fd = klb_socket_connect(p_host, port, timeout_ms);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "connect fail");
        return 2;
    }

    klb_socket_set_block(fd, true);

    p_tcp = klua_kproxy_tcp_new(L);
    p_tcp->fd = fd;
    p_tcp->closed = false;
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// listen userdata


static int klua_kproxy_listen_close(lua_State* L)
{
    klua_kproxy_listen_t* p_ls = to_klua_kproxy_listen(L, 1);
    close_fd_klua_kproxy_listen(p_ls);
    return 0;
}


/// @brief accept; 可选 timeout_ms (缺省阻塞)
/// @note Lua: tcp, err = listener:accept([timeout_ms]); timeout 时 tcp=nil, err="timeout"
static int klua_kproxy_listen_accept(lua_State* L)
{
    klua_kproxy_listen_t* p_ls = to_klua_kproxy_listen(L, 1);
    int timeout_ms = -1;
    struct sockaddr_in addr;
    klb_socket_fd fd = INVALID_SOCKET;
    klua_kproxy_tcp_t* p_tcp = NULL;

    if (2 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 2))
    {
        timeout_ms = (int)luaL_checkinteger(L, 2);
    }

    if (p_ls->closed || INVALID_SOCKET == p_ls->fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    if (0 <= timeout_ms)
    {
        fd_set rfds;
        struct timeval tv;
        int nready = 0;

        FD_ZERO(&rfds);
        FD_SET(p_ls->fd, &rfds);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        nready = select((int)p_ls->fd + 1, &rfds, NULL, NULL, &tv);
        if (0 > nready)
        {
            lua_pushnil(L);
            lua_pushstring(L, "select fail");
            return 2;
        }

        if (0 == nready)
        {
            lua_pushnil(L);
            lua_pushstring(L, "timeout");
            return 2;
        }
    }

    KLB_MEMSET(&addr, 0, sizeof(addr));
    fd = klb_socket_accept(p_ls->fd, &addr);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "accept fail");
        return 2;
    }

    klb_socket_set_block(fd, true);

    p_tcp = klua_kproxy_tcp_new(L);
    p_tcp->fd = fd;
    p_tcp->closed = false;
    return 1;
}


/// @brief 回读 listen 本地绑定 (getsockname)
/// @return ip[string|nil], port[number|nil]
static int klua_kproxy_listen_local_addr(lua_State* L)
{
    klua_kproxy_listen_t* p_ls = to_klua_kproxy_listen(L, 1);
    struct sockaddr_storage addr;
    char ip[64] = { 0 };
    int port = 0;

    KLB_MEMSET(&addr, 0, sizeof(addr));

    if (NULL == p_ls || p_ls->closed || INVALID_SOCKET == p_ls->fd
        || 0 != klb_socket_get_local_sa(p_ls->fd, &addr, NULL))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    if (0 != klb_socket_format_addr(&addr, ip, (int)sizeof(ip), &port))
    {
        lua_pushnil(L);
        lua_pushnil(L);
        return 2;
    }

    lua_pushstring(L, ip);
    lua_pushinteger(L, port);
    return 2;
}


static void klua_kproxy_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] =
    {
        { "close",      klua_kproxy_listen_close },
        { "accept",     klua_kproxy_listen_accept },
        { "co_accept",  klua_kproxy_listen_co_accept },
        { "local_addr", klua_kproxy_listen_local_addr },
        { NULL,         NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",    NULL },
        { "__gc",       klua_kproxy_listen_close },
        { "__close",    klua_kproxy_listen_close },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KPROXY_LISTEN_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


/// @brief TCP listen
/// @note Lua: listener, err = kproxy.listen(host, port [, backlog])
static int klua_kproxy_listen(lua_State* L)
{
    const char* p_host = NULL;
    int port = 0;
    int backlog = 128;
    klb_socket_fd fd = INVALID_SOCKET;
    klua_kproxy_listen_t* p_ls = NULL;

    p_host = luaL_checkstring(L, 1);
    port = (int)luaL_checkinteger(L, 2);

    if (3 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 3))
    {
        backlog = (int)luaL_checkinteger(L, 3);
    }

    if (NULL == p_host || '\0' == p_host[0])
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad host");
        return 2;
    }

    // port=0: 内核分配临时口; 经 listener:local_addr 回读
    if (0 > port || 65535 < port)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad port");
        return 2;
    }

    if (1 > backlog)
    {
        backlog = 1;
    }

    fd = listen_host_klua_kproxy(p_host, port, backlog);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "listen fail");
        return 2;
    }

    p_ls = klua_kproxy_listen_new(L);
    p_ls->fd = fd;
    p_ls->closed = false;
    p_ls->pending_fd = INVALID_SOCKET;
    return 1;
}


/// @brief C-3: 与 listen 同义 (返回 listener)
static int klua_kproxy_listen_raw(lua_State* L)
{
    return klua_kproxy_listen(L);
}


/// @brief C-4: 自有隧道 connect (加密帧 N3-2; 现行 Lua 明文帧走 tcp_connect)
/// @note Lua: tcp, err = kproxy.tunnel_connect(...); 正式握手在 packages/proxy/tunnel
/// @note 保留导出供日后 C 侧加密; 现行返回 not_implemented 以免绕过 Lua 帧
static int klua_kproxy_tunnel_connect(lua_State* L)
{
    (void)L;
    lua_pushnil(L);
    lua_pushstring(L, "use_lua_tunnel_frame");
    return 2;
}


int klua_open_kproxy(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "available",      klua_kproxy_available },
        { "relay",          klua_kproxy_relay },
        { "listen",         klua_kproxy_listen },
        { "listen_raw",     klua_kproxy_listen_raw },
        { "tcp_connect",    klua_kproxy_tcp_connect },
        { "co_connect",     klua_kproxy_co_connect },
        { "tunnel_connect", klua_kproxy_tunnel_connect },
        { NULL,             NULL }
    };

    klua_kproxy_tcp_createmeta(L);
    klua_kproxy_listen_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}

// end
