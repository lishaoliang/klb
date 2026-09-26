// Doc Encode : UTF-8 BOM, Unix(LF)
// 探测连接: 对已 accept 的 fd 嗅探 TLS / 明文协议
#include "klbnet/klblisten/klb_netprobe_conn.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_smp.h"
#include <string.h>
#include <assert.h>

#ifndef _WIN32
#include <errno.h>
#endif


/// @struct klb_netprobe_conn_t
/// @brief  探测连接
typedef struct klb_netprobe_conn_t_
{
    klb_netmulti_t*                     p_netmulti;         ///< 复用

    struct
    {
        bool                            is_open;            ///< 是否 open
        int                             timeout_ms;         ///< 超时(毫秒)
        int64_t                         begin_tc;           ///< 开始探测时刻
    };

    struct
    {
        uint8_t                         peek[KLB_NETPROBE_PEEK_MAX]; ///< PEEK 副本
        int                             peek_len;           ///< 副本长度
    };

    struct
    {
        klb_netprobe_conn_done_cb       cb_done;            ///< 探测结束
        void*                           ptr;                ///< ptr
    };
}klb_netprobe_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_netprobe_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数


static const uint8_t* find_crlf_klb_netprobe(const uint8_t* p_buf, int len)
{
    for (int i = 0; i + 1 < len; ++i)
    {
        if (('\r' == p_buf[i]) && ('\n' == p_buf[i + 1]))
        {
            return p_buf + i;
        }
    }

    return NULL;
}


static bool line_has_token_klb_netprobe(const uint8_t* p_line, int line_len, const char* p_token, int token_len)
{
    if (line_len < token_len)
    {
        return false;
    }

    for (int i = 0; i <= line_len - token_len; ++i)
    {
        if (0 == memcmp(p_line + i, p_token, token_len))
        {
            return true;
        }
    }

    return false;
}


static uint32_t load_u32_klb_netprobe(const uint8_t* p_buf)
{
    uint32_t v = 0;
    memcpy(&v, p_buf, sizeof(v));
    return v;
}


static bool is_tls_hello_klb_netprobe(const uint8_t* p_buf, int len)
{
    if (len < 3)
    {
        return false;
    }

    if (0x16 != p_buf[0])
    {
        return false;
    }

    if (0x03 != p_buf[1])
    {
        return false;
    }

    if (0x04 < p_buf[2])
    {
        return false;
    }

    return true;
}


static bool is_http_method_start_klb_netprobe(uint8_t c)
{
    if (('A' <= c) && (c <= 'Z'))
    {
        return true;
    }

    return false;
}


int klb_netprobe_check(const uint8_t* p_buf, int len, bool* p_tls, int* p_protocol)
{
    assert(NULL != p_tls);
    assert(NULL != p_protocol);
    assert(0 <= len);
    if (0 < len)
    {
        assert(NULL != p_buf);
    }

    *p_tls = false;
    *p_protocol = KLB_PROTOCOL_UNKOWN;

    if (len <= 0)
    {
        return KLB_NETPROBE_NEED_MORE;
    }

    // step1. TLS record / ClientHello
    if (0x16 == p_buf[0])
    {
        if (len < 3)
        {
            return KLB_NETPROBE_NEED_MORE;
        }

        if (is_tls_hello_klb_netprobe(p_buf, len))
        {
            *p_tls = true;
            *p_protocol = KLB_PROTOCOL_UNKOWN;
            return KLB_NETPROBE_OK;
        }

        return KLB_NETPROBE_UNKNOWN;
    }

    // step2. MNP / SMP magic
    if (len < 4)
    {
        if (is_http_method_start_klb_netprobe(p_buf[0]))
        {
            return KLB_NETPROBE_NEED_MORE;
        }

        return KLB_NETPROBE_NEED_MORE;
    }

    uint32_t magic = load_u32_klb_netprobe(p_buf);
    if (KLB_MNP_MAGIC == magic)
    {
        *p_protocol = KLB_PROTOCOL_MNP;

        if (sizeof(klb_mnp_t) <= (size_t)len)
        {
            klb_mnp_t mnp;
            memcpy(&mnp, p_buf, sizeof(mnp));

            if (KLB_MNP_RPC_LUA == mnp.packtype)
            {
                *p_protocol = KLB_PROTOCOL_RPC_MNP_LUA;
            }
            else if (KLB_MNP_RPC_JSON == mnp.packtype)
            {
                *p_protocol = KLB_PROTOCOL_RPC_MNP_JSON;
            }
        }

        return KLB_NETPROBE_OK;
    }
    else if (KLB_SMP_MAGIC == magic)
    {
        *p_protocol = KLB_PROTOCOL_SMP;
        return KLB_NETPROBE_OK;
    }

    // step3. 文本首行: RTSP / HTTP
    if (!is_http_method_start_klb_netprobe(p_buf[0]))
    {
        return KLB_NETPROBE_UNKNOWN;
    }

    const uint8_t* p_crlf = find_crlf_klb_netprobe(p_buf, len);
    if (NULL == p_crlf)
    {
        return KLB_NETPROBE_NEED_MORE;
    }

    int line_len = (int)(p_crlf - p_buf);
    if (line_has_token_klb_netprobe(p_buf, line_len, "RTSP/", 5))
    {
        *p_protocol = KLB_PROTOCOL_RTSP;
        return KLB_NETPROBE_OK;
    }
    else if (line_has_token_klb_netprobe(p_buf, line_len, "HTTP/", 5))
    {
        *p_protocol = KLB_PROTOCOL_HTTP;
        return KLB_NETPROBE_OK;
    }

    return KLB_NETPROBE_UNKNOWN;
}


static bool is_recv_wait_klb_netprobe(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();
    if ((WSAEWOULDBLOCK == err) || (WSAEINTR == err))
    {
        return true;
    }
#else
    if ((EAGAIN == errno) || (EWOULDBLOCK == errno) || (EINTR == errno))
    {
        return true;
    }
#endif

    return false;
}


static void finish_klb_netprobe_conn(klb_netconn_t* p_conn, int protocol, bool tls)
{
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    if (!p_probe->is_open)
    {
        return;
    }

    klb_netprobe_conn_done_cb cb_done = p_probe->cb_done;
    void* ptr = p_probe->ptr;
    uint8_t* p_peek = p_probe->peek;
    int peek_len = p_probe->peek_len;
    klb_netmulti_t* p_netmulti = p_probe->p_netmulti;

    p_probe->cb_done = NULL;
    p_probe->ptr = NULL;
    p_probe->is_open = false;
    p_probe->p_netmulti = NULL;

    klb_socket_set_reading(p_conn->p_socket, false);
    klb_socket_fd fd = klb_socket_detach_fd(p_conn->p_socket);

    if (NULL != cb_done)
    {
        cb_done(p_conn, ptr, fd, protocol, tls, p_peek, peek_len);
    }
    else
    {
        KLB_SOCKET_CLOSE(fd);
    }

    klb_netmulti_closing(p_netmulti, p_conn);
}


//////////////////////////////////////////////////////////////////////////
// msg

static void on_msg_connect_timeout_klb_netprobe_conn(klb_netconn_t* p_conn)
{

}

static void on_msg_connected_klb_netprobe_conn(klb_netconn_t* p_conn)
{

}

static void on_msg_ticker_klb_netprobe_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    if (!p_probe->is_open)
    {
        return;
    }

    if ((int64_t)p_probe->timeout_ms < ABS_SUB(now, p_probe->begin_tc))
    {
        finish_klb_netprobe_conn(p_conn, KLB_PROTOCOL_UNKOWN, false);
    }
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_netprobe_conn_destroy(klb_netconn_t* p_conn)
{
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    assert(NULL == p_probe->p_netmulti);

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    klb_netprobe_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_netprobe_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_conn;
    (void)p_in;
    (void)p_out;

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_netprobe_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    (void)p_conn;
    (void)now;

    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_netprobe_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    (void)now;
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (!p_probe->is_open)
    {
        return 0;
    }

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int ret = klb_socket_recv_peek(p_socket, p_probe->peek, KLB_NETPROBE_PEEK_MAX);
    if (ret < 0)
    {
        if (is_recv_wait_klb_netprobe())
        {
            return 0;
        }

        finish_klb_netprobe_conn(p_conn, KLB_PROTOCOL_UNKOWN, false);
        return 0;
    }
    else if (0 == ret)
    {
        finish_klb_netprobe_conn(p_conn, KLB_PROTOCOL_UNKOWN, false);
        return 0;
    }

    p_probe->peek_len = ret;

    bool tls = false;
    int protocol = KLB_PROTOCOL_UNKOWN;
    int result = klb_netprobe_check(p_probe->peek, p_probe->peek_len, &tls, &protocol);

    if (KLB_NETPROBE_OK == result)
    {
        finish_klb_netprobe_conn(p_conn, protocol, tls);
    }
    else if (KLB_NETPROBE_UNKNOWN == result)
    {
        finish_klb_netprobe_conn(p_conn, KLB_PROTOCOL_UNKOWN, false);
    }
    else if (KLB_NETPROBE_PEEK_MAX <= p_probe->peek_len)
    {
        finish_klb_netprobe_conn(p_conn, KLB_PROTOCOL_UNKOWN, false);
    }

    return 0;
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_netprobe_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_msg_connect_timeout_klb_netprobe_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_connected:
        on_msg_connected_klb_netprobe_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_msg_ticker_klb_netprobe_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_netprobe_conn_free(klb_netconn_t* p_conn)
{
    assert(NULL != p_conn);
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    klb_netprobe_conn_set_done(p_conn, NULL, NULL);

    if (p_probe->is_open)
    {
        p_probe->is_open = false;
        p_probe->timeout_ms = 0;
        p_probe->begin_tc = 0;

        klb_netmulti_closing(p_probe->p_netmulti, p_conn);
        p_probe->p_netmulti = NULL;
    }
    else
    {
        p_probe->p_netmulti = NULL;

        klb_netconn_destroy(p_conn);
    }
}

int klb_netprobe_conn_open(klb_netconn_t* p_conn, klb_socket_fd fd, int timeout_ms)
{
    assert(NULL != p_conn);
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    if (p_probe->is_open)
    {
        return 1;
    }

    if (INVALID_SOCKET == fd)
    {
        return 1;
    }

    klb_socket_t* p_socket = klb_socket_async_create(fd);
    p_conn->p_socket = p_socket;

    klb_netmulti_push(p_probe->p_netmulti, p_conn);

    klb_socket_set_connected(p_socket, true);
    klb_socket_set_reading(p_socket, true);

    p_probe->timeout_ms = (0 < timeout_ms) ? timeout_ms : KLB_NETPROBE_TIMEOUT_DEFAULT;
    p_probe->begin_tc = p_socket->last_send_tc;
    p_probe->peek_len = 0;
    p_probe->is_open = true;

    return 0;
}

int klb_netprobe_conn_set_done(klb_netconn_t* p_conn, klb_netprobe_conn_done_cb cb_done, void* ptr)
{
    assert(NULL != p_conn);
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    p_probe->cb_done = cb_done;
    p_probe->ptr = ptr;

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_netprobe_conn_init(klb_netconn_t* p_conn)
{
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    p_conn->vtable.destroy = NULL;
    p_conn->vtable.recv_data = NULL;

    p_conn->vtable.ioctrl = klb_netprobe_conn_ioctrl;
    p_conn->vtable.send_normal = NULL;
    p_conn->vtable.send_media = NULL;
    p_conn->vtable.on_send = klb_netprobe_conn_on_send;
    p_conn->vtable.on_recv = klb_netprobe_conn_on_recv;
    p_conn->vtable.on_msg = klb_netprobe_conn_on_msg;

    p_probe->is_open = false;

    return 0;
}

static void klb_netprobe_conn_quit(klb_netconn_t* p_conn)
{
    (void)p_conn;
}


//////////////////////////////////////////////////////////////////////////

/// @brief 创建探测连接
/// @return klb_netconn_t*
klb_netconn_t* klb_netprobe_conn_create(klb_netmulti_t* p_netmulti)
{
    assert(NULL != p_netmulti);

    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_netprobe_conn_t));
    klb_netprobe_conn_t* p_probe = (klb_netprobe_conn_t*)p_conn->extra;

    klb_netprobe_conn_init(p_conn);

    p_conn->vtable.destroy = klb_netprobe_conn_destroy;

    p_probe->p_netmulti = p_netmulti;
    p_conn->p_socket = NULL;

    return p_conn;
}

// end
