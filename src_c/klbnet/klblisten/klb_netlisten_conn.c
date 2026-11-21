// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klblisten/klb_netlisten_conn.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"


/// @struct klb_netlisten_conn_t
/// @brief  listen 连接
typedef struct klb_netlisten_conn_t_
{
    klb_netmulti_t*                     p_netmulti;         ///< 复用


    struct
    {
        bool                            is_open;            ///< 是否 open
        int                             port;               ///< 端口
        bool                            nodelay;            ///< accept 后 TCP_NODELAY
        bool                            keepalive;          ///< accept 后 SO_KEEPALIVE
    };
    
    // 回调函数
    struct
    {
        klb_netlisten_conn_accept_cb    cb_accept;          ///< 当有连接进入 (IPv4)
        klb_netlisten_conn_accept_sa_cb cb_accept_sa;       ///< 当有连接进入 (storage; 优先)
        klb_netlisten_conn_accept_err_cb cb_accept_err;     ///< accept 系统错误
        void*                           ptr;                ///< ptr
        void*                           ptr_err;            ///< accept_err ptr
    };
}klb_netlisten_conn_t;



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_netlisten_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数


/// @brief storage → sockaddr_in (IPv4 或 IPv4-mapped); 纯 IPv6 则清零
static void to_v4_addr_klb_netlisten(const struct sockaddr_storage* p_sa, struct sockaddr_in* p_out)
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


// accept
static int klb_netlisten_conn_do_accept(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_fd listen_fd = klb_socket_get_fd(p_conn->p_socket);

    klb_netlisten_conn_accept_cb cb_accept = p_listen->cb_accept;
    klb_netlisten_conn_accept_sa_cb cb_accept_sa = p_listen->cb_accept_sa;
    void* ptr = p_listen->ptr;

    int num = 0;
    while (true)
    {
        struct sockaddr_storage addr = { 0 };
        socklen_t addr_len = 0;
        klb_socket_fd fd = klb_socket_accept_sa(listen_fd, &addr, &addr_len);

        if (INVALID_SOCKET != fd)
        {
            num++;

            if (p_listen->nodelay)
            {
                klb_socket_set_tcp_nodelay(fd, true);
            }

            if (p_listen->keepalive)
            {
                klb_socket_set_keepalive(fd, true);
            }

            if (cb_accept_sa)
            {
                cb_accept_sa(p_conn, ptr, fd, &addr, false);
            }
            else if (cb_accept)
            {
                struct sockaddr_in v4 = { 0 };

                to_v4_addr_klb_netlisten(&addr, &v4);
                cb_accept(p_conn, ptr, fd, &v4, false);
            }
            else
            {
                KLB_SOCKET_CLOSE(fd)
            }
        }
        else
        {
            int code = klb_netcode_last_syserr();

            // 非阻塞耗尽 / 中断: 正常结束本轮
            if (KLB_NETCODE_AGAIN == code
                || KLB_NETCODE_INTR == code
                || KLB_NETCODE_INPROGRESS == code)
            {
                break;
            }

            // 内存类错误 fail-fast
            assert(KLB_NETCODE_NOMEM != code);

            if (p_listen->cb_accept_err)
            {
                int report = code;

                if (KLB_NETCODE_IO == report || KLB_NETCODE_ERR == report)
                {
                    report = KLB_NETCODE_ACCEPT_FAIL;
                }

                p_listen->cb_accept_err(p_conn, p_listen->ptr_err, report);
            }

            break;
        }
    }

    return num;
}

//////////////////////////////////////////////////////////////////////////
// msg

// 握手 连接超时
static void on_msg_connect_timeout_klb_netlisten_conn(klb_netconn_t* p_conn)
{

}

// 握手完成
static void on_msg_connected_klb_netlisten_conn(klb_netconn_t* p_conn)
{

}

// 定时器消息
static void on_msg_ticker_klb_netlisten_conn(klb_netconn_t* p_conn)
{

}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_netlisten_conn_destroy(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    // 需要先调用 klb_netlisten_conn_free
    assert(NULL == p_listen->p_netmulti);

    // 关闭 socket
    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    // 退出
    klb_netlisten_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_netlisten_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    (void)p_listen;
    (void)p_in;
    (void)p_out;

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_rtspclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    (void)p_listen;
    (void)now;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    return klb_netlisten_conn_do_accept(p_conn);
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_netlisten_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    (void)p_listen;
    (void)now;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    return klb_netlisten_conn_do_accept(p_conn);
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_netlisten_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    (void)now;

    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_msg_connect_timeout_klb_netlisten_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_connected:
        on_msg_connected_klb_netlisten_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_msg_ticker_klb_netlisten_conn(p_conn);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_netlisten_conn_free(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    // 清空
    klb_netlisten_conn_set_accept(p_conn, NULL, NULL);
    klb_netlisten_conn_set_accept_sa(p_conn, NULL, NULL);
    klb_netlisten_conn_set_accept_err(p_conn, NULL, NULL);

    // 删除
    if (p_listen->is_open)
    {
        // 若 已经 open, 则需要 将连接 移交给 复用模块 去关闭
        p_listen->is_open = false;
        p_listen->port = 0;

        klb_netmulti_closing(p_listen->p_netmulti, p_conn);
        p_listen->p_netmulti = NULL;
    }
    else
    {
        // 若 没有 open, 则 直接销毁
        p_listen->p_netmulti = NULL;

        klb_netconn_destroy(p_conn);
    }
}

/// @brief 监听: 端口
int klb_netlisten_conn_open(klb_netconn_t* p_conn, int port, int max_connect)
{
    return klb_netlisten_conn_open_addr(p_conn, NULL, port, max_connect);
}

/// @brief 监听: 指定绑定地址
int klb_netlisten_conn_open_addr(klb_netconn_t* p_conn, const char* p_host, int port, int max_connect)
{
    return klb_netlisten_conn_open_addr_ex(p_conn, p_host, port, max_connect, NULL);
}

/// @brief 监听: 指定绑定地址 + 族/选项
int klb_netlisten_conn_open_addr_ex(klb_netconn_t* p_conn, const char* p_host, int port,
    int max_connect, const klb_netlisten_open_opts_t* p_opts)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    struct sockaddr_storage addr = { 0 };
    socklen_t addr_len = 0;
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;
    int code = 0;
    int family = AF_INET;
    bool reuseport = false;
    bool ipv6only = true;
    bool nodelay = false;
    bool keepalive = false;

    if (NULL == p_conn || NULL == p_listen || NULL == p_listen->p_netmulti)
    {
        return KLB_NETCODE_INVAL;
    }

    if (p_listen->is_open)
    {
        return KLB_NETCODE_BUSY;
    }

    if (NULL != p_opts)
    {
        family = p_opts->family;
        reuseport = p_opts->reuseport;
        nodelay = p_opts->nodelay;
        keepalive = p_opts->keepalive;

        if (0 == family)
        {
            family = AF_UNSPEC;
        }
        else if (AF_INET != family && AF_INET6 != family && AF_UNSPEC != family)
        {
            return KLB_NETCODE_INVAL;
        }
    }

    // dual: 通配绑 :: 且 V6ONLY=0; 纯 ipv6: V6ONLY=1
    if (AF_UNSPEC == family)
    {
        ipv6only = false;
    }
    else if (AF_INET6 == family)
    {
        ipv6only = true;
    }

    // 解析绑定地址 (host 非法时 fail-fast)
    if (0 != klb_socket_resolve_sa(p_host, port, family, &addr, &addr_len))
    {
        return KLB_NETCODE_INVAL;
    }

    // 创建 socket
    fd = klb_socket_listen_sa(&addr, addr_len, max_connect, reuseport, ipv6only);
    if (INVALID_SOCKET == fd)
    {
        code = klb_netcode_last_syserr();
        if (KLB_NETCODE_IO == code || KLB_NETCODE_ERR == code)
        {
            code = KLB_NETCODE_LISTEN_FAIL;
        }

        return code;
    }

    p_socket = klb_socket_async_create(fd);
    p_conn->p_socket = p_socket;

    // 放入复用模块
    klb_netmulti_push(p_listen->p_netmulti, p_conn);

    // 初始化设置
    {
        klb_socket_set_connected(p_socket, true);
        klb_socket_set_reading(p_socket, true);
    }

    // 标记
    p_listen->port = port;
    p_listen->nodelay = nodelay;
    p_listen->keepalive = keepalive;
    p_listen->is_open = true;

    return 0;
}

/// @brief 开启TCP监听: unix路径
int klb_netlisten_conn_open_unix(klb_netconn_t* p_conn, const char* p_path, int max_connect)
{
    (void)p_conn;
    (void)p_path;
    (void)max_connect;

    return 0;
}

/// @brief 获取监听端口
int klb_netlisten_conn_get_port(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    return p_listen->port;
}

/// @brief 回读监听本地地址 (getsockname)
int klb_netlisten_conn_get_local_addr(klb_netconn_t* p_conn, struct sockaddr_in* p_addr)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_fd fd = INVALID_SOCKET;

    if (NULL == p_conn || NULL == p_listen || !p_listen->is_open)
    {
        return -1;
    }

    fd = klb_socket_get_fd(p_conn->p_socket);
    if (INVALID_SOCKET == fd)
    {
        return -1;
    }

    return klb_socket_get_local_addr(fd, p_addr);
}

int klb_netlisten_conn_get_local_sa(klb_netconn_t* p_conn, struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_fd fd = INVALID_SOCKET;

    if (NULL == p_conn || NULL == p_listen || !p_listen->is_open)
    {
        return -1;
    }

    fd = klb_socket_get_fd(p_conn->p_socket);
    if (INVALID_SOCKET == fd)
    {
        return -1;
    }

    return klb_socket_get_local_sa(fd, p_addr, p_addr_len);
}

klb_socket_fd klb_netlisten_conn_native_fd(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = NULL;

    if (NULL == p_conn)
    {
        return INVALID_SOCKET;
    }

    p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    if (NULL == p_listen || !p_listen->is_open || NULL == p_conn->p_socket)
    {
        return INVALID_SOCKET;
    }

    return klb_socket_get_fd(p_conn->p_socket);
}

/// @brief 同步关闭监听 (立即关闭 fd, 不经复用模块延迟关闭)
void klb_netlisten_conn_close_sync(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    if (NULL == p_conn || NULL == p_listen)
    {
        return;
    }

    klb_netlisten_conn_set_accept(p_conn, NULL, NULL);
    klb_netlisten_conn_set_accept_sa(p_conn, NULL, NULL);
    klb_netlisten_conn_set_accept_err(p_conn, NULL, NULL);

    if (!p_listen->is_open)
    {
        p_listen->p_netmulti = NULL;
        klb_netconn_destroy(p_conn);
        return;
    }

    // 同步 从复用模块 移除 (不使用延迟关闭路径)
    klb_netmulti_remove(p_listen->p_netmulti, p_conn);

    p_listen->is_open = false;
    p_listen->port = 0;
    p_listen->p_netmulti = NULL;

    // 内部 关闭 socket fd
    klb_netconn_destroy(p_conn);
}

/// @brief 设置当监听到socket连接建立时回调
int klb_netlisten_conn_set_accept(klb_netconn_t* p_conn, klb_netlisten_conn_accept_cb cb_accept, void* ptr)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    p_listen->cb_accept = cb_accept;
    p_listen->ptr = ptr;

    return 0;
}

int klb_netlisten_conn_set_accept_sa(klb_netconn_t* p_conn, klb_netlisten_conn_accept_sa_cb cb_accept, void* ptr)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    p_listen->cb_accept_sa = cb_accept;
    p_listen->ptr = ptr;

    return 0;
}

int klb_netlisten_conn_set_accept_err(klb_netconn_t* p_conn, klb_netlisten_conn_accept_err_cb cb_err, void* ptr)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    p_listen->cb_accept_err = cb_err;
    p_listen->ptr_err = ptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_netlisten_conn_init(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    // 初始化 vtable
    p_conn->vtable.destroy = NULL; // 销毁
    p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

    p_conn->vtable.ioctrl = klb_netlisten_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
    p_conn->vtable.send_normal = NULL; // 调用者 发送常规数据包
    p_conn->vtable.send_media = NULL; // 调用者 发送媒体数据包
    p_conn->vtable.on_send = klb_rtspclient_conn_on_send; // 当网络上可以发送数据时
    p_conn->vtable.on_recv = klb_netlisten_conn_on_recv; // 当网络上可以接收数据时
    p_conn->vtable.on_msg = klb_netlisten_conn_on_msg; // 当网络上有消息传来时

    // 其他参数
    {
        p_listen->is_open = false;
        p_listen->nodelay = false;
        p_listen->keepalive = false;
    }

    return 0;
}

static void klb_netlisten_conn_quit(klb_netconn_t* p_conn)
{
    (void)p_conn;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建rtsp连接
/// @return klb_netconn_t*
klb_netconn_t* klb_netlisten_conn_create(klb_netmulti_t* p_netmulti)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_netlisten_conn_t));
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    klb_netlisten_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_netlisten_conn_destroy;

    // socket
    {
        p_listen->p_netmulti = p_netmulti;
        p_conn->p_socket = NULL;
    }

    return p_conn;
}

// end
