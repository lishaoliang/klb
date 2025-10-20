// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klblisten/klb_netlisten_conn.h"
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
    };
    
    // 回调函数
    struct
    {
        klb_netlisten_conn_accept_cb    cb_accept;          ///< 当有连接进入
        void*                           ptr;                ///< ptr
    };
}klb_netlisten_conn_t;



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_netlisten_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数


// accept
static int klb_netlisten_conn_do_accept(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_fd listen_fd = klb_socket_get_fd(p_conn->p_socket);

    klb_netlisten_conn_accept_cb cb_accept = p_listen->cb_accept;
    void* ptr = p_listen->ptr;

    int num = 0;
    while (true)
    {
        struct sockaddr_in addr = { 0 };
        klb_socket_fd fd = klb_socket_accept(listen_fd, &addr);

        if (INVALID_SOCKET != fd)
        {
            num++;

            if (cb_accept)
            {
                cb_accept(p_conn, ptr, fd, &addr, false, NULL);
            }
            else
            {
                KLB_SOCKET_CLOSE(fd)
            }
        }
        else
        {
            break; // 都取完了
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

    // 关闭监听
    klb_netlisten_conn_close(p_conn);

    // 退出
    klb_netlisten_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_netlisten_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_rtspclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

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

/// @brief 监听: 端口
int klb_netlisten_conn_open(klb_netconn_t* p_conn, int port, int max_connect)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    // 关闭
    klb_netlisten_conn_close(p_conn);

    // 创建 socket
    klb_socket_fd fd = klb_socket_listen(port, max_connect);
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = klb_socket_async_create(fd);
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
    p_listen->is_open = true;

    return 0;
}

/// @brief 开启TCP监听: unix路径
int klb_netlisten_conn_open_unix(klb_netconn_t* p_conn, const char* p_path, int max_connect)
{
    return 0;
}

/// @brief 关闭
int klb_netlisten_conn_close(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    if (p_listen->is_open)
    {
        p_listen->is_open = false;
        p_listen->port = 0;

        klb_socket_t* p_socket = p_conn->p_socket;
        klb_socket_set_reading(p_socket, false);

        // 直接移除
        klb_netmulti_remove(p_listen->p_netmulti, p_conn);

        p_conn->p_socket = NULL;
        KLB_FREE_BY(p_socket, klb_socket_destroy);
    }

    return 0;
}

/// @brief 获取监听端口
int klb_netlisten_conn_get_port(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    return p_listen->port;
}

/// @brief 设置当监听到socket连接建立时回调
int klb_netlisten_conn_set_accept(klb_netconn_t* p_conn, klb_netlisten_conn_accept_cb cb_accept, void* ptr)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

    p_listen->cb_accept = cb_accept;
    p_listen->ptr = ptr;

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
    }

    return 0;
}

static void klb_netlisten_conn_quit(klb_netconn_t* p_conn)
{
    klb_netlisten_conn_t* p_listen = (klb_netlisten_conn_t*)p_conn->extra;

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

//end
