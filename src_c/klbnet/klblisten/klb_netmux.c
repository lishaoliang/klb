// Doc Encode : UTF-8 BOM, Unix(LF)
// 共享 listen + probe 分发中枢
#include "klbnet/klblisten/klb_netmux.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klblisten/klb_netprobe_conn.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include <assert.h>
#include <string.h>


/// @brief 绑定地址 缓存长度
#define KLB_NETMUX_HOST_MAX     128


/// @struct klb_netmux_probe_t
/// @brief  探测上下文
typedef struct klb_netmux_probe_t_
{
    klb_netmux_t*                   p_mux;          ///< 所属 mux
    struct sockaddr_in              addr;           ///< accept 地址 (IPv4 折算)
    struct sockaddr_storage         addr_sa;        ///< 完整 accept 地址
}klb_netmux_probe_t;


/// @struct klb_netmux_t_
/// @brief  共享监听分发器
struct klb_netmux_t_
{
    klb_netmulti_t*                 p_netmulti;     ///< 复用
    klb_netconn_t*                  p_listen_conn;  ///< listen 连接

    bool                            is_open;        ///< 是否 open
    bool                            closed;         ///< 正在销毁/关闭
    bool                            tls_as_http;    ///< TLS 按 HTTP 投递
    bool                            tls_as_mnp;     ///< 非 HTTP/RTSP TLS 按 MNP 投递 (否则 SMP)
    bool                            tls_as_rtsp;    ///< 非 HTTP TLS 按 RTSP 投递 (优先于 mnp)
    int                             port;           ///< 端口
    int                             max_connect;    ///< listen backlog
    char                            host[KLB_NETMUX_HOST_MAX];  ///< 绑定地址; INADDR_ANY 为空串
    klb_netlisten_open_opts_t       open_opts;      ///< 族/套接字选项 (rebind 复用)

    klb_nlist_t*                    p_probe_nlist;  ///< 进行中的 probe
    klb_nlist_t*                    p_socket_nlist; ///< 待取走的 socket

    klb_netmux_ready_cb             cb_ready;       ///< 就绪回调
    void*                           ptr;            ///< 回调 ptr
};


//////////////////////////////////////////////////////////////////////////
// 内部


static void remove_probe_klb_netmux(klb_netmux_t* p_mux, klb_netconn_t* p_probe)
{
    assert(NULL != p_mux);
    assert(NULL != p_probe);

    if (NULL == p_mux->p_probe_nlist)
    {
        return;
    }

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_mux->p_probe_nlist);
    while (NULL != p_iter)
    {
        klb_nlist_iter_t* p_next = klb_nlist_next(p_iter);
        if (p_probe == (klb_netconn_t*)klb_nlist_data(p_iter))
        {
            klb_nlist_remove(p_mux->p_probe_nlist, p_iter);
            return;
        }

        p_iter = p_next;
    }
}


static void abort_probes_klb_netmux(klb_netmux_t* p_mux)
{
    assert(NULL != p_mux);

    if (NULL == p_mux->p_probe_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_mux->p_probe_nlist))
    {
        klb_netconn_t* p_probe = (klb_netconn_t*)klb_nlist_pop_head(p_mux->p_probe_nlist);
        klb_netmux_probe_t* p_ctx = (klb_netmux_probe_t*)p_probe->p_udata;

        klb_netconn_set_udata(p_probe, NULL);
        KLB_FREE(p_ctx);
        klb_netprobe_conn_free(p_probe);
    }
}


static void clear_socket_queue_klb_netmux(klb_netmux_t* p_mux)
{
    assert(NULL != p_mux);

    if (NULL == p_mux->p_socket_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_mux->p_socket_nlist))
    {
        klb_netmux_socket_t* p_tmp = (klb_netmux_socket_t*)klb_nlist_pop_head(p_mux->p_socket_nlist);
        if (NULL != p_tmp)
        {
            KLB_SOCKET_CLOSE(p_tmp->fd);
            KLB_FREE(p_tmp);
        }
    }
}


static bool is_take_protocol_klb_netmux(klb_netmux_t* p_mux, int protocol, bool tls, int* p_out_protocol, bool* p_out_tls)
{
    assert(NULL != p_mux);
    assert(NULL != p_out_protocol);
    assert(NULL != p_out_tls);

    *p_out_tls = tls;
    *p_out_protocol = protocol;

    if (tls)
    {
        if (p_mux->tls_as_http)
        {
            // HTTPS/WSS: TLS ClientHello 按 HTTP 投递
            *p_out_protocol = KLB_PROTOCOL_HTTP;
            *p_out_tls = true;
            return true;
        }

        // rtsps: TLS 按 RTSP 投递 (new_serve 侧 TLS wrap); 与 https 互斥
        if (p_mux->tls_as_rtsp)
        {
            *p_out_protocol = KLB_PROTOCOL_RTSP;
            *p_out_tls = true;
            return true;
        }

        // smps/mnps: TLS 按 SMP 或 MNP 投递 (new_serve 侧 TLS wrap); 与 https/rtsps 互斥
        if (p_mux->tls_as_mnp)
        {
            *p_out_protocol = KLB_PROTOCOL_MNP;
        }
        else
        {
            *p_out_protocol = KLB_PROTOCOL_SMP;
        }

        *p_out_tls = true;
        return true;
    }

    if (KLB_PROTOCOL_HTTP == protocol
        || KLB_PROTOCOL_RTSP == protocol
        || KLB_PROTOCOL_SMP == protocol
        || KLB_PROTOCOL_MNP == protocol
        || KLB_PROTOCOL_RPC_MNP_LUA == protocol
        || KLB_PROTOCOL_RPC_MNP_JSON == protocol
        || KLB_PROTOCOL_RPC_SMP_LUA == protocol
        || KLB_PROTOCOL_RPC_SMP_JSON == protocol)
    {
        return true;
    }

    return false;
}


static void notify_ready_klb_netmux(klb_netmux_t* p_mux)
{
    assert(NULL != p_mux);

    if (NULL != p_mux->cb_ready)
    {
        p_mux->cb_ready(p_mux, p_mux->ptr);
    }
}


static int on_probe_done_klb_netmux(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, int protocol, bool tls, const uint8_t* p_peek, int peek_len)
{
    klb_netmux_probe_t* p_ctx = (klb_netmux_probe_t*)ptr;
    klb_netmux_t* p_mux = p_ctx->p_mux;

    (void)p_peek;
    (void)peek_len;

    remove_probe_klb_netmux(p_mux, p_conn);
    klb_netconn_set_udata(p_conn, NULL);

    int out_protocol = protocol;
    bool out_tls = tls;
    bool take = false;

    if (!p_mux->closed)
    {
        take = is_take_protocol_klb_netmux(p_mux, protocol, tls, &out_protocol, &out_tls);
    }

    if (!take)
    {
        KLB_SOCKET_CLOSE(fd);
        KLB_FREE(p_ctx);
        return 0;
    }

    klb_netmux_socket_t* p_tmp = KLB_MALLOCZ(klb_netmux_socket_t, 1, 0);
    p_tmp->fd = fd;
    p_tmp->addr = p_ctx->addr;
    p_tmp->addr_sa = p_ctx->addr_sa;
    p_tmp->tls = out_tls;
    p_tmp->protocol = out_protocol;
    p_tmp->err_code = 0;
    p_tmp->listen_port = p_mux->port;   // 组监听/移交用: 本口端口
    KLB_FREE(p_ctx);

    klb_nlist_push_tail(p_mux->p_socket_nlist, p_tmp);
    notify_ready_klb_netmux(p_mux);

    return 0;
}


/// @brief storage → sockaddr_in (IPv4 / IPv4-mapped)
static void to_v4_addr_klb_netmux(const struct sockaddr_storage* p_sa, struct sockaddr_in* p_out)
{
    assert(NULL != p_sa);
    assert(NULL != p_out);

    memset(p_out, 0, sizeof(struct sockaddr_in));

    if (AF_INET == p_sa->ss_family)
    {
        memcpy(p_out, p_sa, sizeof(struct sockaddr_in));
        return;
    }

#ifdef IN6_IS_ADDR_V4MAPPED
    if (AF_INET6 == p_sa->ss_family)
    {
        const struct sockaddr_in6* p_v6 = (const struct sockaddr_in6*)p_sa;

        if (IN6_IS_ADDR_V4MAPPED(&p_v6->sin6_addr))
        {
            p_out->sin_family = AF_INET;
            p_out->sin_port = p_v6->sin6_port;
            memcpy(&p_out->sin_addr, &p_v6->sin6_addr.s6_addr[12], 4);
        }
    }
#endif
}


static int on_accept_sa_klb_netmux(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd,
    const struct sockaddr_storage* p_addr, bool tls)
{
    klb_netmux_t* p_mux = (klb_netmux_t*)ptr;

    (void)p_conn;
    (void)tls;

    if (NULL == p_mux || p_mux->closed || !p_mux->is_open)
    {
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klb_netmux_probe_t* p_ctx = KLB_MALLOCZ(klb_netmux_probe_t, 1, 0);
    p_ctx->p_mux = p_mux;
    p_ctx->addr_sa = *p_addr;
    to_v4_addr_klb_netmux(p_addr, &p_ctx->addr);

    klb_netconn_t* p_probe = klb_netprobe_conn_create(p_mux->p_netmulti);
    klb_netconn_set_udata(p_probe, p_ctx);
    klb_netprobe_conn_set_done(p_probe, on_probe_done_klb_netmux, p_ctx);

    if (0 != klb_netprobe_conn_open(p_probe, fd, 0))
    {
        klb_netconn_set_udata(p_probe, NULL);
        KLB_FREE(p_ctx);
        klb_netprobe_conn_free(p_probe);
        KLB_SOCKET_CLOSE(fd);
        return 0;
    }

    klb_nlist_push_tail(p_mux->p_probe_nlist, p_probe);
    return 0;
}


static int on_accept_err_klb_netmux(klb_netconn_t* p_conn, void* ptr, int code)
{
    klb_netmux_t* p_mux = (klb_netmux_t*)ptr;
    klb_netmux_socket_t* p_tmp = NULL;

    (void)p_conn;

    if (NULL == p_mux || p_mux->closed || !p_mux->is_open)
    {
        return 0;
    }

    p_tmp = KLB_MALLOCZ(klb_netmux_socket_t, 1, 0);
    p_tmp->fd = INVALID_SOCKET;
    p_tmp->err_code = (0 != code) ? code : KLB_NETCODE_ACCEPT_FAIL;
    p_tmp->listen_port = p_mux->port;   // 错误哨兵仍带本口端口, 便于上层定位

    klb_nlist_push_tail(p_mux->p_socket_nlist, p_tmp);
    notify_ready_klb_netmux(p_mux);

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出


klb_netmux_t* klb_netmux_create(klb_netmulti_t* p_multi)
{
    assert(NULL != p_multi);

    klb_netmux_t* p_mux = KLB_MALLOCZ(klb_netmux_t, 1, 0);

    p_mux->p_netmulti = p_multi;
    p_mux->p_listen_conn = NULL;
    p_mux->is_open = false;
    p_mux->closed = false;
    p_mux->tls_as_http = true;
    p_mux->tls_as_mnp = false;
    p_mux->tls_as_rtsp = false;
    p_mux->port = 0;
    p_mux->max_connect = 0;
    p_mux->host[0] = '\0';
    p_mux->open_opts.family = AF_INET;
    p_mux->open_opts.reuseport = false;
    p_mux->open_opts.nodelay = false;
    p_mux->open_opts.keepalive = false;
    p_mux->p_probe_nlist = klb_nlist_create();
    p_mux->p_socket_nlist = klb_nlist_create();
    p_mux->cb_ready = NULL;
    p_mux->ptr = NULL;

    return p_mux;
}


void klb_netmux_destroy(klb_netmux_t* p_mux)
{
    if (NULL == p_mux)
    {
        return;
    }

    p_mux->closed = true;
    p_mux->cb_ready = NULL;
    p_mux->ptr = NULL;

    abort_probes_klb_netmux(p_mux);
    clear_socket_queue_klb_netmux(p_mux);

    if (NULL != p_mux->p_listen_conn)
    {
        klb_netlisten_conn_set_accept(p_mux->p_listen_conn, NULL, NULL);
        klb_netlisten_conn_set_accept_sa(p_mux->p_listen_conn, NULL, NULL);
        klb_netlisten_conn_set_accept_err(p_mux->p_listen_conn, NULL, NULL);
        klb_netlisten_conn_free(p_mux->p_listen_conn);
        p_mux->p_listen_conn = NULL;
    }

    p_mux->is_open = false;
    p_mux->port = 0;
    p_mux->max_connect = 0;
    p_mux->host[0] = '\0';
    p_mux->p_netmulti = NULL;

    KLB_FREE_BY(p_mux->p_probe_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_mux->p_socket_nlist, klb_nlist_destroy);
    KLB_FREE(p_mux);
}


/// @brief 以 host 打开监听 (调用前 须 未 open)
static int do_open_klb_netmux(klb_netmux_t* p_mux, const char* p_host, int port, int max_connect,
    const klb_netlisten_open_opts_t* p_opts)
{
    klb_netconn_t* p_listen_conn = NULL;
    klb_netlisten_open_opts_t opts = { 0 };
    int rc = 0;

    if (NULL != p_host && '\0' != p_host[0] && strlen(p_host) >= sizeof(p_mux->host))
    {
        return KLB_NETCODE_EINVAL;
    }

    opts.family = AF_INET;
    if (NULL != p_opts)
    {
        opts = *p_opts;
        if (0 == opts.family)
        {
            opts.family = AF_UNSPEC;
        }
    }

    p_listen_conn = klb_netlisten_conn_create(p_mux->p_netmulti);
    klb_netlisten_conn_set_accept_sa(p_listen_conn, on_accept_sa_klb_netmux, p_mux);
    klb_netlisten_conn_set_accept_err(p_listen_conn, on_accept_err_klb_netmux, p_mux);

    rc = klb_netlisten_conn_open_addr_ex(p_listen_conn, p_host, port, max_connect, &opts);
    if (0 != rc)
    {
        klb_netlisten_conn_free(p_listen_conn);
        return rc;
    }

    if (NULL != p_host && '\0' != p_host[0])
    {
        strncpy(p_mux->host, p_host, sizeof(p_mux->host) - 1);
        p_mux->host[sizeof(p_mux->host) - 1] = '\0';
    }
    else
    {
        p_mux->host[0] = '\0';
    }

    p_mux->p_listen_conn = p_listen_conn;
    p_mux->port = port;
    p_mux->max_connect = max_connect;
    p_mux->open_opts = opts;
    p_mux->is_open = true;

    return 0;
}


int klb_netmux_open(klb_netmux_t* p_mux, int port, int max_connect)
{
    return klb_netmux_open_addr(p_mux, NULL, port, max_connect);
}


int klb_netmux_open_addr(klb_netmux_t* p_mux, const char* p_host, int port, int max_connect)
{
    return klb_netmux_open_addr_ex(p_mux, p_host, port, max_connect, NULL);
}


int klb_netmux_open_addr_ex(klb_netmux_t* p_mux, const char* p_host, int port, int max_connect,
    const klb_netlisten_open_opts_t* p_opts)
{
    assert(NULL != p_mux);

    if (NULL == p_mux)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (p_mux->is_open || p_mux->closed)
    {
        return KLB_NETCODE_EBUSY;
    }

    if (NULL == p_mux->p_netmulti)
    {
        return KLB_NETCODE_EINVAL;
    }

    return do_open_klb_netmux(p_mux, p_host, port, max_connect, p_opts);
}


int klb_netmux_rebind(klb_netmux_t* p_mux, const char* p_host)
{
    return klb_netmux_rebind_ex(p_mux, p_host, NULL);
}


int klb_netmux_rebind_ex(klb_netmux_t* p_mux, const char* p_host, const klb_netlisten_open_opts_t* p_opts)
{
    struct sockaddr_storage addr = { 0 };
    int port = 0;
    int max_connect = 0;
    const klb_netlisten_open_opts_t* p_use = NULL;

    assert(NULL != p_mux);

    if (NULL == p_mux)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (!p_mux->is_open || p_mux->closed)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (NULL != p_host && '\0' != p_host[0] && strlen(p_host) >= sizeof(p_mux->host))
    {
        return KLB_NETCODE_EINVAL;
    }

    // 回读 实际端口 (port=0 时端口由内核分配)
    port = p_mux->port;
    if (0 == klb_netlisten_conn_get_local_sa(p_mux->p_listen_conn, &addr, NULL))
    {
        if (AF_INET == addr.ss_family)
        {
            port = (int)ntohs(((struct sockaddr_in*)&addr)->sin_port);
        }
        else if (AF_INET6 == addr.ss_family)
        {
            port = (int)ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
        }
    }

    max_connect = p_mux->max_connect;
    p_use = (NULL != p_opts) ? p_opts : &p_mux->open_opts;

    // 同步关闭 旧 listen (既有已 accept 连接不受影响)
    klb_netlisten_conn_close_sync(p_mux->p_listen_conn);
    p_mux->p_listen_conn = NULL;
    p_mux->is_open = false;

    return do_open_klb_netmux(p_mux, p_host, port, max_connect, p_use);
}


const char* klb_netmux_get_host(klb_netmux_t* p_mux)
{
    if (NULL == p_mux || !p_mux->is_open)
    {
        return NULL;
    }

    return p_mux->host;
}


int klb_netmux_get_local_addr(klb_netmux_t* p_mux, struct sockaddr_in* p_addr)
{
    if (NULL == p_mux || NULL == p_mux->p_listen_conn)
    {
        return -1;
    }

    return klb_netlisten_conn_get_local_addr(p_mux->p_listen_conn, p_addr);
}


int klb_netmux_get_local_sa(klb_netmux_t* p_mux, struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    if (NULL == p_mux || NULL == p_mux->p_listen_conn)
    {
        return -1;
    }

    return klb_netlisten_conn_get_local_sa(p_mux->p_listen_conn, p_addr, p_addr_len);
}


klb_socket_fd klb_netmux_native_fd(klb_netmux_t* p_mux)
{
    if (NULL == p_mux || NULL == p_mux->p_listen_conn)
    {
        return INVALID_SOCKET;
    }

    return klb_netlisten_conn_native_fd(p_mux->p_listen_conn);
}


bool klb_netmux_is_open(klb_netmux_t* p_mux)
{
    if (NULL == p_mux)
    {
        return false;
    }

    return p_mux->is_open && !p_mux->closed;
}


int klb_netmux_get_port(klb_netmux_t* p_mux)
{
    if (NULL == p_mux)
    {
        return 0;
    }

    return p_mux->port;
}


int klb_netmux_set_tls_as_http(klb_netmux_t* p_mux, bool enable)
{
    assert(NULL != p_mux);

    p_mux->tls_as_http = enable;
    return 0;
}


int klb_netmux_set_tls_as_mnp(klb_netmux_t* p_mux, bool enable)
{
    assert(NULL != p_mux);

    p_mux->tls_as_mnp = enable;
    return 0;
}


int klb_netmux_set_tls_as_rtsp(klb_netmux_t* p_mux, bool enable)
{
    assert(NULL != p_mux);

    p_mux->tls_as_rtsp = enable;
    return 0;
}


int klb_netmux_set_ready(klb_netmux_t* p_mux, klb_netmux_ready_cb cb_ready, void* ptr)
{
    assert(NULL != p_mux);

    p_mux->cb_ready = cb_ready;
    p_mux->ptr = ptr;
    return 0;
}


klb_netmux_socket_t* klb_netmux_pop(klb_netmux_t* p_mux)
{
    assert(NULL != p_mux);

    if (NULL == p_mux->p_socket_nlist)
    {
        return NULL;
    }

    if (0 >= klb_nlist_size(p_mux->p_socket_nlist))
    {
        return NULL;
    }

    return (klb_netmux_socket_t*)klb_nlist_pop_head(p_mux->p_socket_nlist);
}


int klb_netmux_queue_size(klb_netmux_t* p_mux)
{
    if (NULL == p_mux || NULL == p_mux->p_socket_nlist)
    {
        return 0;
    }

    return klb_nlist_size(p_mux->p_socket_nlist);
}

// end
