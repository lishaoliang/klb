// Doc Encode : UTF-8 BOM, Unix(LF)
// kproxy co_*: netmulti 异步就绪 + kco yield (N6 M1.b)
#include "klua_kproxy_in.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <string.h>

/// @struct klua_kproxy_stream_extra_t
/// @brief  stream netconn 扩展 (指向 tcp userdata)
typedef struct klua_kproxy_stream_extra_t_
{
    klua_kproxy_tcp_t*          p_tcp;
}klua_kproxy_stream_extra_t;


/// @struct klua_kproxy_lconn_extra_t
/// @brief  listen netconn 扩展
typedef struct klua_kproxy_lconn_extra_t_
{
    klua_kproxy_listen_t*       p_ls;
}klua_kproxy_lconn_extra_t;


//////////////////////////////////////////////////////////////////////////
// 前置


static int on_yield_listen_klua_kproxy(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt);


//////////////////////////////////////////////////////////////////////////
// 唤醒辅助


static int push_str2_klua_kproxy(lua_State* L, const char* a, const char* b)
{
    lua_pushstring(L, a);
    lua_pushstring(L, NULL != b ? b : "");
    return 2;
}


//////////////////////////////////////////////////////////////////////////
// stream netconn


static void destroy_stream_klua_kproxy(klb_netconn_t* p_conn)
{
    klua_kproxy_stream_extra_t* p_ex = (klua_kproxy_stream_extra_t*)p_conn->extra;

    if (NULL != p_ex->p_tcp)
    {
        // socket 将在下方 destroy; 同步清空 tcp 侧指针
        if (p_ex->p_tcp->p_tls == p_conn->p_socket)
        {
            p_ex->p_tcp->p_tls = NULL;
        }
        p_ex->p_tcp->p_conn = NULL;
        p_ex->p_tcp->p_sock = NULL;
        p_ex->p_tcp->fd = INVALID_SOCKET;
        p_ex->p_tcp = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    KLB_FREE(p_conn);
}


static int push_recv_ok_klua_kproxy(lua_State* L, const char* p_data, int n)
{
    lua_pushlstring(L, p_data, (size_t)n);
    return 1;
}


/// @brief 推进 TLS 握手 (读/写就绪都可能推进)
/// @note  Bug.2026-7: 首包写出后多为 WANT_READ, 只有读就绪可推进;
///        读侧缺此推进时 co_tls 永不恢复, https over tunnel 挂起
/// @return int 1.已唤醒 co_tls (成功或失败); 0.仍 WANT 或未在握手
static int advance_tls_klua_kproxy(klua_kproxy_tcp_t* p_tcp)
{
    klb_socket_t* p_tls = NULL;
    int rc = 0;

    if (!p_tcp->tls_want || NULL == p_tcp->p_tls || NULL == p_tcp->co_tls)
    {
        return 0;
    }

    p_tls = p_tcp->p_tls;
    rc = klb_socket_tls_handshake(p_tls);

    if (0 == rc)
    {
        lua_State* co = NULL;

        p_tcp->tls_want = false;
        klb_socket_set_writing(p_tls, false);
        klb_socket_set_reading(p_tls, true);
        co = p_tcp->co_tls;
        p_tcp->co_tls = NULL;
        if (NULL != co)
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);

            if (NULL != L)
            {
                if (0 != klb_socket_tls_check_pin(p_tls))
                {
                    lua_pushboolean(L, 0);
                    lua_pushstring(L, "tls pin mismatch");
                    lua_pcall(L, 2, 0, 0);
                }
                else
                {
                    lua_pushboolean(L, 1);
                    lua_pcall(L, 1, 0, 0);
                }
            }
        }

        return 1;
    }

    if (0 > rc)
    {
        lua_State* co = NULL;

        p_tcp->tls_want = false;
        co = p_tcp->co_tls;
        p_tcp->co_tls = NULL;
        if (NULL != co)
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);

            if (NULL != L)
            {
                lua_pushboolean(L, 0);
                lua_pushstring(L, "tls handshake fail");
                lua_pcall(L, 2, 0, 0);
            }
        }

        return 1;
    }

    // WANT: 按 status_rw 调整 reading/writing
    if (KLB_SOCKET_WAIT_WRITE == p_tls->status_rw)
    {
        klb_socket_set_writing(p_tls, true);
        klb_socket_set_reading(p_tls, false);
    }
    else
    {
        klb_socket_set_writing(p_tls, false);
        klb_socket_set_reading(p_tls, true);
    }

    return 0;
}


static int on_recv_stream_klua_kproxy(klb_netconn_t* p_conn, int64_t now)
{
    klua_kproxy_stream_extra_t* p_ex = (klua_kproxy_stream_extra_t*)p_conn->extra;
    klua_kproxy_tcp_t* p_tcp = p_ex->p_tcp;
    klb_socket_t* p_sock = NULL;
    char* p_buf = NULL;
    int max_n = 0;
    int n = 0;
    int judge = 0;
    int close_status = KLB_SOCKET_DISCONNECT;
    (void)now;

    if (NULL == p_tcp)
    {
        return 0;
    }

    // TLS 握手未完成: 只推进握手, 禁把握手记录当应用数据排空 (Bug.2026-7)
    if (p_tcp->tls_want && NULL != p_tcp->p_tls)
    {
        advance_tls_klua_kproxy(p_tcp);
        return 1;
    }

    p_sock = (NULL != p_tcp->p_tls) ? p_tcp->p_tls : p_conn->p_socket;
    if (NULL == p_sock)
    {
        return 0;
    }

    // Bug.2026-20: 无 co_recv 时仍可能 reading=1 + 对端 FIN → select 空转
    // 无消费者: 排空至 AGAIN 后清 reading; EOF/ERROR 走 recv_closed
    if (NULL == p_tcp->co_recv)
    {
        if (KLB_SOCKET_OK == klb_socket_get_status(p_sock))
        {
            char tmp[256];

            while (true)
            {
                n = klb_socket_recv(p_sock, (uint8_t*)tmp, (int)sizeof(tmp));
                judge = klb_socket_recv_judge(p_sock, n, &close_status);

                if (KLB_SOCKET_RECV_DATA == judge)
                {
                    continue;
                }

                if (KLB_SOCKET_RECV_AGAIN == judge)
                {
                    klb_socket_set_reading(p_sock, false);
                    break;
                }

                klb_socket_recv_closed(p_sock, close_status);
                break;
            }
        }
        else
        {
            klb_socket_set_reading(p_sock, false);
            klb_socket_set_writing(p_sock, false);
        }

        return 0;
    }

    max_n = p_tcp->recv_want;
    if (1 > max_n)
    {
        max_n = 4096;
    }

    if (65536 < max_n)
    {
        max_n = 65536;
    }

    p_buf = KLB_MALLOC(char, max_n, 0);
    n = klb_socket_recv(p_sock, (uint8_t*)p_buf, max_n);
    judge = klb_socket_recv_judge(p_sock, n, &close_status);

    if (KLB_SOCKET_RECV_DATA == judge)
    {
        lua_State* co = p_tcp->co_recv;

        p_tcp->co_recv = NULL;
        if (NULL != co)
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);

            if (NULL != L)
            {
                push_recv_ok_klua_kproxy(L, p_buf, n);
                lua_pcall(L, 1, 0, 0);
            }
        }

        KLB_FREE(p_buf);
        return n;
    }

    KLB_FREE(p_buf);

    if (KLB_SOCKET_RECV_AGAIN == judge)
    {
        // EAGAIN / TLS WANT: 保持 co_recv, 继续等读就绪
        klb_socket_set_reading(p_sock, true);
        return 0;
    }

    // EOF / ERROR: 清 reading/writing 并唤醒
    klb_socket_recv_closed(p_sock, close_status);
    {
        lua_State* co = p_tcp->co_recv;
        const char* err = (KLB_SOCKET_RECV_EOF == judge) ? "eof" : "recv fail";

        p_tcp->co_recv = NULL;
        if (NULL != co)
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);

            if (NULL != L)
            {
                push_str2_klua_kproxy(L, NULL, err);
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    return 0;
}


static int on_send_stream_klua_kproxy(klb_netconn_t* p_conn, int64_t now)
{
    klua_kproxy_stream_extra_t* p_ex = (klua_kproxy_stream_extra_t*)p_conn->extra;
    klua_kproxy_tcp_t* p_tcp = p_ex->p_tcp;
    klb_socket_t* p_sock = NULL;
    int n = 0;
    (void)now;

    if (NULL == p_tcp)
    {
        return 0;
    }

    // TLS 握手推进 (写就绪侧)
    if (p_tcp->tls_want && NULL != p_tcp->p_tls)
    {
        advance_tls_klua_kproxy(p_tcp);
        return 1;
    }

    if (NULL == p_tcp->co_send || NULL == p_tcp->p_wbuf)
    {
        return 0;
    }

    p_sock = (NULL != p_tcp->p_tls) ? p_tcp->p_tls : p_conn->p_socket;
    if (NULL == p_sock)
    {
        return 0;
    }

    while (p_tcp->wbuf_off < p_tcp->wbuf_len)
    {
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        n = klb_socket_send(p_sock,
            (const uint8_t*)(p_tcp->p_wbuf + p_tcp->wbuf_off),
            p_tcp->wbuf_len - p_tcp->wbuf_off);

        if (0 < n)
        {
            p_tcp->wbuf_off += n;
            continue;
        }

        judge = klb_socket_send_judge(p_sock, n, &close_status);
        if (KLB_SOCKET_SEND_AGAIN == judge)
        {
            klb_socket_set_writing(p_sock, true);
            return 0;
        }

        KLB_FREE(p_tcp->p_wbuf);
        p_tcp->p_wbuf = NULL;
        p_tcp->wbuf_len = 0;
        p_tcp->wbuf_off = 0;
        klb_socket_recv_closed(p_sock, close_status);
        {
            lua_State* co = p_tcp->co_send;

            p_tcp->co_send = NULL;
            if (NULL != co)
            {
                lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);

                if (NULL != L)
                {
                    lua_pushboolean(L, 0);
                    lua_pushstring(L, "send fail");
                    lua_pcall(L, 2, 0, 0);
                }
            }
        }
        return 0;
    }

    KLB_FREE(p_tcp->p_wbuf);
    p_tcp->p_wbuf = NULL;
    p_tcp->wbuf_len = 0;
    p_tcp->wbuf_off = 0;
    klb_socket_set_writing(p_sock, false);

    {
        lua_State* co = p_tcp->co_send;
        p_tcp->co_send = NULL;
        if (NULL != co)
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
            if (NULL != L)
            {
                lua_pushboolean(L, 1);
                lua_pcall(L, 1, 0, 0);
            }
        }
    }

    return 1;
}


static int on_msg_stream_klua_kproxy(klb_netconn_t* p_conn, int msg, int64_t now)
{
    klua_kproxy_stream_extra_t* p_ex = (klua_kproxy_stream_extra_t*)p_conn->extra;
    klua_kproxy_tcp_t* p_tcp = p_ex->p_tcp;
    (void)now;

    if (NULL == p_tcp)
    {
        return 0;
    }

    if (KLB_NETCONN_MSG_connected == msg)
    {
        klb_socket_set_reading(p_conn->p_socket, true);
        klb_socket_set_writing(p_conn->p_socket, false);

        if (NULL != p_tcp->co_connect)
        {
            lua_State* co = p_tcp->co_connect;
            int ref = p_tcp->recv_want;
            p_tcp->co_connect = NULL;
            p_tcp->recv_want = 0;
            {
                lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
                if (NULL != L)
                {
                    // resume 返回值 = tcp userdata (registry)
                    if (0 < ref)
                    {
                        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
                        luaL_unref(L, LUA_REGISTRYINDEX, ref);
                        lua_pcall(L, 1, 0, 0);
                    }
                    else
                    {
                        lua_pushnil(L);
                        lua_pushstring(L, "connect lost ref");
                        lua_pcall(L, 2, 0, 0);
                    }
                }
            }
        }
    }
    else if (KLB_NETCONN_MSG_connect_timeout == msg)
    {
        if (NULL != p_tcp->co_connect)
        {
            lua_State* co = p_tcp->co_connect;
            int ref = p_tcp->recv_want;
            p_tcp->co_connect = NULL;
            p_tcp->recv_want = 0;
            {
                lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
                if (NULL != L)
                {
                    if (0 < ref)
                    {
                        luaL_unref(L, LUA_REGISTRYINDEX, ref);
                    }
                    lua_pushnil(L);
                    lua_pushstring(L, "connect timeout");
                    lua_pcall(L, 2, 0, 0);
                }
            }
        }
    }

    return 0;
}


static klb_netconn_t* create_stream_conn_klua_kproxy(klua_kproxy_tcp_t* p_tcp, klb_socket_t* p_sock)
{
    klb_netconn_t* p_conn = NULL;
    klua_kproxy_stream_extra_t* p_ex = NULL;

    p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klua_kproxy_stream_extra_t));
    p_ex = (klua_kproxy_stream_extra_t*)p_conn->extra;
    p_ex->p_tcp = p_tcp;

    p_conn->vtable.destroy = destroy_stream_klua_kproxy;
    p_conn->vtable.on_recv = on_recv_stream_klua_kproxy;
    p_conn->vtable.on_send = on_send_stream_klua_kproxy;
    p_conn->vtable.on_msg = on_msg_stream_klua_kproxy;
    p_conn->protocol = KLB_PROTOCOL_user;
    p_conn->p_socket = p_sock;
    p_conn->p_udata = p_tcp;

    return p_conn;
}


void klua_kproxy_tcp_detach_async(klua_kproxy_tcp_t* p_tcp)
{
    if (NULL == p_tcp)
    {
        return;
    }

    // 唤醒挂起协程
    if (NULL != p_tcp->co_recv && NULL != p_tcp->p_env)
    {
        lua_State* co = p_tcp->co_recv;
        p_tcp->co_recv = NULL;
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
            if (NULL != L)
            {
                push_str2_klua_kproxy(L, NULL, "closed");
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    if (NULL != p_tcp->co_send && NULL != p_tcp->p_env)
    {
        lua_State* co = p_tcp->co_send;
        p_tcp->co_send = NULL;
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
            if (NULL != L)
            {
                lua_pushboolean(L, 0);
                lua_pushstring(L, "closed");
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    if (NULL != p_tcp->co_connect && NULL != p_tcp->p_env)
    {
        lua_State* co = p_tcp->co_connect;
        p_tcp->co_connect = NULL;
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
            if (NULL != L)
            {
                lua_pushboolean(L, 0);
                lua_pushstring(L, "closed");
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    if (NULL != p_tcp->co_tls && NULL != p_tcp->p_env)
    {
        lua_State* co = p_tcp->co_tls;
        p_tcp->co_tls = NULL;
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_tcp->p_env), co);
            if (NULL != L)
            {
                lua_pushboolean(L, 0);
                lua_pushstring(L, "closed");
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    KLB_FREE(p_tcp->p_wbuf);
    p_tcp->p_wbuf = NULL;
    p_tcp->wbuf_len = 0;
    p_tcp->wbuf_off = 0;

    if (NULL != p_tcp->p_conn && NULL != p_tcp->p_netmulti)
    {
        // closing 会 destroy; 清空 back-ref
        {
            klua_kproxy_stream_extra_t* p_ex = (klua_kproxy_stream_extra_t*)p_tcp->p_conn->extra;
            p_ex->p_tcp = NULL;
        }
        klb_netmulti_closing(p_tcp->p_netmulti, p_tcp->p_conn);
        p_tcp->p_conn = NULL;
        p_tcp->p_sock = NULL;
        p_tcp->fd = INVALID_SOCKET;
    }
}


int klua_kproxy_tcp_ensure_async(klua_kproxy_tcp_t* p_tcp, lua_State* L)
{
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_multi = NULL;
    klb_socket_t* p_sock = NULL;
    klb_netconn_t* p_conn = NULL;

    if (NULL == p_tcp || p_tcp->closed)
    {
        return -1;
    }

    if (NULL != p_tcp->p_conn)
    {
        return 0;
    }

    p_env = klua_env_get_by_L(L);
    p_multi = klua_netmulti_get(p_env);
    if (NULL == p_env || NULL == p_multi)
    {
        return -1;
    }

    if (INVALID_SOCKET == p_tcp->fd && NULL == p_tcp->p_tls)
    {
        return -1;
    }

    // 已有 TLS: 用 TLS socket 挂 multi
    if (NULL != p_tcp->p_tls)
    {
        p_sock = p_tcp->p_tls;
        p_tcp->fd = klb_socket_get_fd(p_sock);
    }
    else
    {
        klb_socket_set_block(p_tcp->fd, false);
        p_sock = klb_socket_async_create(p_tcp->fd);
        if (NULL == p_sock)
        {
            return -1;
        }
        klb_socket_set_connected(p_sock, true);
        klb_socket_set_reading(p_sock, true);
        p_tcp->p_sock = p_sock;
    }

    p_conn = create_stream_conn_klua_kproxy(p_tcp, p_sock);
    klb_netmulti_push(p_multi, p_conn);

    p_tcp->p_env = p_env;
    p_tcp->p_coex = klua_coroutine_get(p_env);
    p_tcp->p_netmulti = p_multi;
    p_tcp->p_conn = p_conn;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// listen netconn


static void destroy_lconn_klua_kproxy(klb_netconn_t* p_conn)
{
    klua_kproxy_lconn_extra_t* p_ex = (klua_kproxy_lconn_extra_t*)p_conn->extra;

    if (NULL != p_ex->p_ls)
    {
        p_ex->p_ls->p_conn = NULL;
        p_ex->p_ls->p_sock = NULL;
        p_ex->p_ls = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    KLB_FREE(p_conn);
}


static int on_recv_lconn_klua_kproxy(klb_netconn_t* p_conn, int64_t now)
{
    klua_kproxy_lconn_extra_t* p_ex = (klua_kproxy_lconn_extra_t*)p_conn->extra;
    klua_kproxy_listen_t* p_ls = p_ex->p_ls;
    klb_socket_fd listen_fd = INVALID_SOCKET;
    int num = 0;
    (void)now;

    if (NULL == p_ls)
    {
        return 0;
    }

    listen_fd = klb_socket_get_fd(p_conn->p_socket);

    while (true)
    {
        struct sockaddr_in addr;
        klb_socket_fd fd = INVALID_SOCKET;

        KLB_MEMSET(&addr, 0, sizeof(addr));
        fd = klb_socket_accept(listen_fd, &addr);
        if (INVALID_SOCKET == fd)
        {
            break;
        }

        num++;

        if (NULL != p_ls->co_accept)
        {
            lua_State* co = p_ls->co_accept;
            p_ls->co_accept = NULL;
            {
                lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_ls->p_env), co);
                if (NULL != L)
                {
                    klua_kproxy_tcp_t* p_tcp = klua_kproxy_tcp_new(L);
                    p_tcp->fd = fd;
                    p_tcp->closed = false;
                    (void)klua_kproxy_tcp_ensure_async(p_tcp, L);
                    lua_pcall(L, 1, 0, 0);
                }
                else
                {
                    KLB_SOCKET_CLOSE(fd);
                }
            }
        }
        else if (INVALID_SOCKET == p_ls->pending_fd)
        {
            p_ls->pending_fd = fd;
        }
        else
        {
            KLB_SOCKET_CLOSE(fd);
        }
    }

    return num;
}


static int on_msg_lconn_klua_kproxy(klb_netconn_t* p_conn, int msg, int64_t now)
{
    (void)p_conn;
    (void)msg;
    (void)now;
    return 0;
}


void klua_kproxy_listen_detach_async(klua_kproxy_listen_t* p_ls)
{
    if (NULL == p_ls)
    {
        return;
    }

    if (NULL != p_ls->co_accept && NULL != p_ls->p_env)
    {
        lua_State* co = p_ls->co_accept;
        p_ls->co_accept = NULL;
        {
            lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_ls->p_env), co);
            if (NULL != L)
            {
                push_str2_klua_kproxy(L, NULL, "closed");
                lua_pcall(L, 2, 0, 0);
            }
        }
    }

    if (INVALID_SOCKET != p_ls->pending_fd)
    {
        KLB_SOCKET_CLOSE(p_ls->pending_fd);
        p_ls->pending_fd = INVALID_SOCKET;
    }

    if (NULL != p_ls->p_conn && NULL != p_ls->p_netmulti)
    {
        {
            klua_kproxy_lconn_extra_t* p_ex = (klua_kproxy_lconn_extra_t*)p_ls->p_conn->extra;
            p_ex->p_ls = NULL;
        }
        klb_netmulti_closing(p_ls->p_netmulti, p_ls->p_conn);
        p_ls->p_conn = NULL;
        p_ls->p_sock = NULL;
        p_ls->fd = INVALID_SOCKET;
    }
}


int klua_kproxy_listen_ensure_async(klua_kproxy_listen_t* p_ls, lua_State* L)
{
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_multi = NULL;
    klb_socket_t* p_sock = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_kproxy_lconn_extra_t* p_ex = NULL;

    if (NULL == p_ls || p_ls->closed)
    {
        return -1;
    }

    if (NULL != p_ls->p_conn)
    {
        return 0;
    }

    p_env = klua_env_get_by_L(L);
    p_multi = klua_netmulti_get(p_env);
    if (NULL == p_env || NULL == p_multi || INVALID_SOCKET == p_ls->fd)
    {
        return -1;
    }

    klb_socket_set_block(p_ls->fd, false);
    p_sock = klb_socket_async_create(p_ls->fd);
    if (NULL == p_sock)
    {
        return -1;
    }

    klb_socket_set_connected(p_sock, true);
    klb_socket_set_reading(p_sock, true);

    p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klua_kproxy_lconn_extra_t));
    p_ex = (klua_kproxy_lconn_extra_t*)p_conn->extra;
    p_ex->p_ls = p_ls;
    p_conn->vtable.destroy = destroy_lconn_klua_kproxy;
    p_conn->vtable.on_recv = on_recv_lconn_klua_kproxy;
    p_conn->vtable.on_msg = on_msg_lconn_klua_kproxy;
    p_conn->protocol = KLB_PROTOCOL_user;
    p_conn->p_socket = p_sock;
    p_conn->p_udata = p_ls;

    klb_netmulti_push(p_multi, p_conn);

    p_ls->p_env = p_env;
    p_ls->p_coex = klua_coroutine_get(p_env);
    p_ls->p_netmulti = p_multi;
    p_ls->p_conn = p_conn;
    p_ls->p_sock = p_sock;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// Lua co_* API


static int on_yield_tcp_klua_kproxy(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kproxy_tcp_t* p_tcp = (klua_kproxy_tcp_t*)ptr;
    (void)p_ex;
    (void)p_co;

    if (KLUA_ENV_EX_quit == opt && NULL != p_tcp)
    {
        if (p_tcp->co_recv == p_co)
        {
            p_tcp->co_recv = NULL;
        }
        if (p_tcp->co_send == p_co)
        {
            p_tcp->co_send = NULL;
        }
        if (p_tcp->co_connect == p_co)
        {
            p_tcp->co_connect = NULL;
        }
        if (p_tcp->co_tls == p_co)
        {
            p_tcp->co_tls = NULL;
        }
    }

    return 0;
}


int klua_kproxy_tcp_co_recv(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = klua_kproxy_tcp_check(L, 1);
    int max_n = (int)luaL_checkinteger(L, 2);

    klua_check_coroutine(L, "kproxy.tcp:co_recv must in coroutine!");
    assert(NULL == p_tcp->co_recv);

    if (1 > max_n)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad max");
        return 2;
    }

    if (0 != klua_kproxy_tcp_ensure_async(p_tcp, L))
    {
        lua_pushnil(L);
        lua_pushstring(L, "async attach fail");
        return 2;
    }

    p_tcp->recv_want = max_n;
    // 立即尝试一次
    {
        klb_socket_t* p_sock = (NULL != p_tcp->p_tls) ? p_tcp->p_tls : p_tcp->p_conn->p_socket;
        char* p_buf = NULL;
        int n = 0;
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        if (65536 < max_n)
        {
            max_n = 65536;
        }

        p_buf = KLB_MALLOC(char, max_n, 0);
        n = klb_socket_recv(p_sock, (uint8_t*)p_buf, max_n);
        judge = klb_socket_recv_judge(p_sock, n, &close_status);

        if (KLB_SOCKET_RECV_DATA == judge)
        {
            lua_pushlstring(L, p_buf, (size_t)n);
            KLB_FREE(p_buf);
            return 1;
        }

        KLB_FREE(p_buf);

        if (KLB_SOCKET_RECV_AGAIN != judge)
        {
            klb_socket_recv_closed(p_sock, close_status);
            lua_pushnil(L);
            lua_pushstring(L, (KLB_SOCKET_RECV_EOF == judge) ? "eof" : "recv fail");
            return 2;
        }
        // AGAIN: 落入下方 yield 等待
    }

    klb_socket_set_reading(
        (NULL != p_tcp->p_tls) ? p_tcp->p_tls : p_tcp->p_conn->p_socket, true);
    p_tcp->co_recv = L;
    return klua_coroutine_yield(p_tcp->p_coex, L, on_yield_tcp_klua_kproxy, p_tcp);
}


int klua_kproxy_tcp_co_send(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = klua_kproxy_tcp_check(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);
    klb_socket_t* p_sock = NULL;
    int n = 0;
    size_t off = 0;

    klua_check_coroutine(L, "kproxy.tcp:co_send must in coroutine!");
    assert(NULL == p_tcp->co_send);

    if (0 != klua_kproxy_tcp_ensure_async(p_tcp, L))
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "async attach fail");
        return 2;
    }

    if (0 == len)
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    p_sock = (NULL != p_tcp->p_tls) ? p_tcp->p_tls : p_tcp->p_conn->p_socket;

    while (off < len)
    {
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        n = klb_socket_send(p_sock, (const uint8_t*)(p_data + off), (int)(len - off));

        if (0 < n)
        {
            off += (size_t)n;
            continue;
        }

        judge = klb_socket_send_judge(p_sock, n, &close_status);
        if (KLB_SOCKET_SEND_AGAIN == judge)
        {
            break;
        }

        klb_socket_recv_closed(p_sock, close_status);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "send fail");
        return 2;
    }

    if (off >= len)
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    // 残留入队后 yield
    p_tcp->wbuf_len = (int)(len - off);
    p_tcp->wbuf_off = 0;
    p_tcp->p_wbuf = KLB_MALLOC(char, p_tcp->wbuf_len, 0);
    memcpy(p_tcp->p_wbuf, p_data + off, (size_t)p_tcp->wbuf_len);
    klb_socket_set_writing(p_sock, true);
    p_tcp->co_send = L;
    return klua_coroutine_yield(p_tcp->p_coex, L, on_yield_tcp_klua_kproxy, p_tcp);
}


int klua_kproxy_tcp_co_upgrade_tls(lua_State* L)
{
    klua_kproxy_tcp_t* p_tcp = klua_kproxy_tcp_check(L, 1);
    const char* p_host = NULL;
    int opts_idx = 0;
    klb_socket_t* p_tls = NULL;
    int rc = 0;
    const char* p_err = NULL;

    klua_check_coroutine(L, "kproxy.tcp:co_upgrade_tls must in coroutine!");
    assert(NULL == p_tcp->co_tls);

    if (2 <= lua_gettop(L) && LUA_TTABLE == lua_type(L, 2))
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
    }
    else if (2 <= lua_gettop(L) && LUA_TNIL != lua_type(L, 2) && LUA_TNUMBER != lua_type(L, 2))
    {
        p_host = luaL_checkstring(L, 2);
    }

    if (0 != klua_kproxy_tcp_ensure_async(p_tcp, L))
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "async attach fail");
        return 2;
    }

    if (NULL != p_tcp->p_tls)
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    // 从 async sock 取 fd 建 TLS (fd 归属转 TLS)
    {
        klb_socket_fd fd = klb_socket_get_fd(p_tcp->p_conn->p_socket);
        p_tls = klb_socket_async_create_tls(fd);
        if (NULL == p_tls)
        {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "tls create fail");
            return 2;
        }

        (void)klb_socket_detach_fd(p_tcp->p_conn->p_socket);
        klb_socket_destroy(p_tcp->p_conn->p_socket);
        p_tcp->p_conn->p_socket = p_tls;
        p_tcp->p_sock = p_tls;
        p_tcp->p_tls = p_tls;
        p_tcp->fd = fd;
    }

    if (0 != opts_idx)
    {
        if (0 != klua_kproxy_apply_tls_opts(L, opts_idx, p_tls, &p_err))
        {
            lua_pushboolean(L, 0);
            lua_pushstring(L, (NULL != p_err) ? p_err : "tls opts bad");
            return 2;
        }
    }
    else if (NULL != p_host && '\0' != p_host[0])
    {
        strncpy(p_tcp->tls_host, p_host, sizeof(p_tcp->tls_host) - 1);
        p_tcp->tls_host[sizeof(p_tcp->tls_host) - 1] = '\0';
        klb_socket_tls_set_hostname(p_tls, p_host);
    }

    if (NULL != p_host && '\0' != p_host[0])
    {
        strncpy(p_tcp->tls_host, p_host, sizeof(p_tcp->tls_host) - 1);
        p_tcp->tls_host[sizeof(p_tcp->tls_host) - 1] = '\0';
    }

    klb_socket_set_connected(p_tls, true);
    rc = klb_socket_tls_handshake(p_tls);
    if (0 == rc)
    {
        klb_socket_set_reading(p_tls, true);
        if (0 != klb_socket_tls_check_pin(p_tls))
        {
            lua_pushboolean(L, 0);
            lua_pushstring(L, "tls pin mismatch");
            return 2;
        }
        lua_pushboolean(L, 1);
        return 1;
    }

    if (0 > rc)
    {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "tls handshake fail");
        return 2;
    }

    p_tcp->tls_want = true;
    if (KLB_SOCKET_WAIT_WRITE == p_tls->status_rw)
    {
        klb_socket_set_writing(p_tls, true);
        klb_socket_set_reading(p_tls, false);
    }
    else
    {
        klb_socket_set_writing(p_tls, false);
        klb_socket_set_reading(p_tls, true);
    }

    p_tcp->co_tls = L;
    return klua_coroutine_yield(p_tcp->p_coex, L, on_yield_tcp_klua_kproxy, p_tcp);
}


int klua_kproxy_co_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    klb_socket_fd fd = INVALID_SOCKET;
    klua_kproxy_tcp_t* p_tcp = NULL;
    klb_socket_t* p_sock = NULL;
    klb_netconn_t* p_conn = NULL;
    klua_env_t* p_env = NULL;
    klb_netmulti_t* p_multi = NULL;

    klua_check_coroutine(L, "kproxy.co_connect must in coroutine!");

    if (NULL == p_host || '\0' == p_host[0] || 1 > port || 65535 < port)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad host/port");
        return 2;
    }

    p_env = klua_env_get_by_L(L);
    p_multi = klua_netmulti_get(p_env);
    if (NULL == p_env || NULL == p_multi)
    {
        lua_pushnil(L);
        lua_pushstring(L, "no netmulti");
        return 2;
    }

    // 非阻塞 connect
    fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        lua_pushstring(L, "connect fail");
        return 2;
    }

    p_sock = klb_socket_async_create(fd);
    if (NULL == p_sock)
    {
        KLB_SOCKET_CLOSE(fd);
        lua_pushnil(L);
        lua_pushstring(L, "socket create fail");
        return 2;
    }

    // userdata 先入栈
    p_tcp = klua_kproxy_tcp_new(L);
    p_tcp->fd = fd;
    p_tcp->closed = false;
    p_tcp->p_sock = p_sock;
    p_tcp->p_env = p_env;
    p_tcp->p_coex = klua_coroutine_get(p_env);
    p_tcp->p_netmulti = p_multi;

    p_conn = create_stream_conn_klua_kproxy(p_tcp, p_sock);
    p_tcp->p_conn = p_conn;

    klb_socket_set_reading(p_sock, true);
    klb_netmulti_push(p_multi, p_conn);

    if (klb_socket_is_connected(p_sock))
    {
        // 极少: 立即完成
        return 1;
    }

    // userdata 入 registry; connected 时作为 co_connect 返回值
    {
        int ref = 0;
        lua_pushvalue(L, -1);
        ref = luaL_ref(L, LUA_REGISTRYINDEX);
        p_tcp->recv_want = ref;
        p_tcp->co_connect = L;
    }

    return klua_coroutine_yield(p_tcp->p_coex, L, on_yield_tcp_klua_kproxy, p_tcp);
}


// 修正 co_connect 的 connected 唤醒: 返回 userdata
// 在 on_msg_stream 里特判


int klua_kproxy_listen_co_accept(lua_State* L)
{
    klua_kproxy_listen_t* p_ls = klua_kproxy_listen_check(L, 1);
    klua_kproxy_tcp_t* p_tcp = NULL;

    klua_check_coroutine(L, "kproxy.listen:co_accept must in coroutine!");
    assert(NULL == p_ls->co_accept);

    if (0 != klua_kproxy_listen_ensure_async(p_ls, L))
    {
        lua_pushnil(L);
        lua_pushstring(L, "async attach fail");
        return 2;
    }

    if (INVALID_SOCKET != p_ls->pending_fd)
    {
        klb_socket_fd fd = p_ls->pending_fd;
        p_ls->pending_fd = INVALID_SOCKET;
        klb_socket_set_block(fd, false);
        p_tcp = klua_kproxy_tcp_new(L);
        p_tcp->fd = fd;
        p_tcp->closed = false;
        if (0 != klua_kproxy_tcp_ensure_async(p_tcp, L))
        {
            // 仍返回裸 tcp, sync 可用
        }
        return 1;
    }

    p_ls->co_accept = L;
    return klua_coroutine_yield(p_ls->p_coex, L, on_yield_listen_klua_kproxy, p_ls);
}


static int on_yield_listen_klua_kproxy(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_kproxy_listen_t* p_ls = (klua_kproxy_listen_t*)ptr;
    (void)p_ex;

    if (KLUA_ENV_EX_quit == opt && NULL != p_ls && p_ls->co_accept == p_co)
    {
        p_ls->co_accept = NULL;
    }

    return 0;
}


// end
