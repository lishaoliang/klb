// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsmp/core/klb_smpserverpc_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_nlist.h"
#include "klbsmp/core/klb_smpparser.h"
#include <assert.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <errno.h>
#endif


/// @brief 非阻塞套接字无数据时 recv 返回负值, 不是断线
static bool recv_would_block_klb_smpserverpc_conn(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();

    if (WSAEWOULDBLOCK == err || WSAEINTR == err)
    {
        return true;
    }
#else
    if (EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
    {
        return true;
    }
#endif

    return false;
}


/// @struct klb_smpserverpc_conn_t
/// @brief  SMP服务RPC连接(SMP serve RPC connect)
typedef struct klb_smpserverpc_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // 发送 数据体
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送 RPC数据 列表
    };

    // 读取 数据
    struct
    {
        int                         parser_status;      ///< 读取解析状态: klb_smpparser_status_e
        klb_smpparser_t             parser;             ///< 解析器

        klb_rbuf_t*                 p_read_head;        ///< 头部数据长度

        klb_buf_t*                  p_read_body;        ///< 读取到的数据
        int                         reading_len;        ///< 还需要读取body的数据长度

        int                         ping_interval_ms;   ///< 服务端 Ping 间隔; 0=关
        int                         ping_timeout_ms;    ///< 等 Pong 超时
        int64_t                     last_ping_tc;       ///< 上次 Ping 时刻; 0=未武装
        int64_t                     ping_deadline_tc;   ///< 等待 Pong 截止
        bool                        ping_waiting;       ///< 已发 Ping 待 Pong
    };
}klb_smpserverpc_conn_t;

#define KLB_SMPSERVERPC_PING_INTERVAL_MS_DEFAULT    15000
#define KLB_SMPSERVERPC_PING_TIMEOUT_MS_DEFAULT     30000


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_smpserverpc_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数

/// @brief 接收到数据 之后 放入数据
/// @param [in] code        错误码: klb_socket_status_e
/// @param [in] packtype    数包类型: klb_mnp_packtype_e
static void push_data_klb_smpserverpc_conn(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    if (KLB_SOCKET_OK != code && KLB_SOCKET_CONNECT != code)
    {
        // 关闭 socket 读写
        klb_socket_set_reading(p_conn->p_socket, false);
        klb_socket_set_writing(p_conn->p_socket, false);
    }

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, packtype, p_data);
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
    }
}


/// @brief connect 超时
static void on_connect_timeout_klb_smpserverpc_conn(klb_netconn_t* p_conn, int64_t now)
{

}

/// @brief connect 握手完成
static void on_connected_timeout_klb_smpserverpc_conn(klb_netconn_t* p_conn, int64_t now)
{
    int a = 0;


}

static void enqueue_ctrl_klb_smpserverpc_conn(klb_netconn_t* p_conn, int packtype)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_smp_t smp = { 0 };
    klb_buf_t* p_buf = NULL;

    smp.magic = KLB_SMP_MAGIC;
    smp.size = (uint32_t)sizeof(klb_smp_t);
    smp.packtype = (uint32_t)packtype;

    p_buf = klb_buf_malloc((int)sizeof(klb_smp_t), false);
    klb_buf_write(p_buf, (const char*)&smp, (int)sizeof(smp));
    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_buf);
    klb_socket_set_writing(p_socket, true);
}

static void normalize_ping_klb_smpserverpc_conn(klb_smpserverpc_conn_t* p_smpserve, int interval_ms, int timeout_ms)
{
    if (0 > interval_ms)
    {
        interval_ms = KLB_SMPSERVERPC_PING_INTERVAL_MS_DEFAULT;
    }

    if (0 == interval_ms)
    {
        p_smpserve->ping_interval_ms = 0;
        p_smpserve->ping_timeout_ms = 0;
        p_smpserve->ping_waiting = false;
        p_smpserve->ping_deadline_tc = 0;
        return;
    }

    if (0 >= timeout_ms)
    {
        timeout_ms = KLB_SMPSERVERPC_PING_TIMEOUT_MS_DEFAULT;
    }


    p_smpserve->ping_interval_ms = interval_ms;
    p_smpserve->ping_timeout_ms = timeout_ms;
}

/// @brief ticker 定时器消息
static void on_ticker_klb_smpserverpc_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    if (0 >= p_smpserve->ping_interval_ms)
    {
        return;
    }

    if (0 == p_smpserve->last_ping_tc)
    {
        p_smpserve->last_ping_tc = now;
        return;
    }

    if (p_smpserve->ping_waiting)
    {
        if (now >= p_smpserve->ping_deadline_tc)
        {
            push_data_klb_smpserverpc_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL, 0, NULL);
        }

        return;
    }

    if ((int64_t)p_smpserve->ping_interval_ms <= ABS_SUB(now, p_smpserve->last_ping_tc))
    {
        enqueue_ctrl_klb_smpserverpc_conn(p_conn, KLB_MNP_PING);
        p_smpserve->last_ping_tc = now;
        p_smpserve->ping_waiting = true;
        p_smpserve->ping_deadline_tc = now + (int64_t)p_smpserve->ping_timeout_ms;
    }
}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_smpserverpc_conn_destroy(klb_netconn_t* p_conn)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    // 移除
    if (NULL != p_smpserve->p_netmulti)
    {
        klb_netmulti_remove(p_smpserve->p_netmulti, p_conn);
        p_smpserve->p_netmulti = NULL;
    }

    // 关闭socket
    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    // 退出
    klb_smpserverpc_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_smpserverpc_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_smpserverpc_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return 1;
}

/// @brief 发送媒体数据包
/// @return int
static int klb_smpserverpc_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_smpserverpc_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_smpserve->p_write_nlist;
    int write_num = 0;

    while (true)
    {
        klb_buf_t* p_buf = klb_nlist_head(p_write_nlist);
        if (NULL == p_buf)
        {
            break;
        }

        int data_len = klb_buf_data_len(p_buf);
        if (data_len <= 0)
        {
            klb_buf_t* p_tmp = klb_nlist_pop_head(p_write_nlist);
            KLB_FREE_BY(p_tmp, klb_buf_unref);

            continue;
        }

        uint8_t* p_data = p_buf->p_buf + p_buf->start;
        int send_num = klb_socket_send(p_socket, p_data, data_len);
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        if (0 < send_num)
        {
            p_buf->start += send_num;
            write_num += send_num;
            continue;
        }

        judge = klb_socket_send_judge(p_socket, send_num, &close_status);
        if (KLB_SOCKET_SEND_AGAIN == judge)
        {
            break;
        }

        klb_socket_recv_closed(p_socket, close_status);
        return write_num;
    }

    // 数据写完了, 无需读取
    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
    }

    return write_num;
}

static bool is_rpc_pack_klb_smpserverpc_conn(int packtype)
{
    if (KLB_MNP_RPC_LUA == packtype || KLB_MNP_RPC_JSON == packtype)
    {
        return true;
    }

    return false;
}

static void skip_body_klb_smpserverpc_conn(klb_netconn_t* p_conn, klb_smpparser_t* p_parser, klb_rbuf_t* p_head)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    int body_need = p_parser->pack_len - p_parser->head_len;

    if (0 > body_need)
    {
        body_need = 0;
    }

    int body_len = 0;
    klb_rbuf_data(p_head, &body_len);

    int cp_len = MIN(body_need, body_len);
    if (0 < cp_len)
    {
        klb_rbuf_use(p_head, cp_len);
        body_need -= cp_len;
    }

    if (0 < body_need)
    {
        p_smpserve->p_read_body = klb_buf_malloc(body_need, false);
        p_smpserve->reading_len = body_need;
        p_smpserve->parser = *p_parser;
        p_smpserve->parser_status = KLB_SMPPARSER_body;
    }
    else
    {
        p_smpserve->parser_status = KLB_SMPPARSER_head;
    }
}

/// @brief 接收网络数据
/// @return int
static int do_recv_klb_smpserverpc_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SMPPARSER_head == p_smpserve->parser_status)
    {
        klb_rbuf_t* p_head = p_smpserve->p_read_head;

        int idle_len = 0;
        char* p_idle = klb_rbuf_idle(p_head, 0, &idle_len);
        int recv_len = klb_socket_recv(p_socket, p_idle, idle_len);

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_head, recv_len);
            *p_read_num += recv_len;
        }
        else
        {
            int close_status = KLB_SOCKET_DISCONNECT;
            int judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                return 1;
            }
            klb_socket_recv_closed(p_socket, close_status);
            return -1;
        }

        // 解析
        int data_len = 0;
        char* p_data = klb_rbuf_data(p_head, &data_len);

        klb_smpparser_t parser = { 0 };
        int ret_parse = klb_smpparser_parse(&parser, p_data, data_len);

        if (0 == ret_parse)
        {
            klb_rbuf_use(p_head, parser.head_len);

            // 解析成功
            if (KLB_MNP_PING == parser.packtype)
            {
                enqueue_ctrl_klb_smpserverpc_conn(p_conn, KLB_MNP_PONG);
                p_smpserve->parser_status = KLB_SMPPARSER_head;
                klb_rbuf_memmove(p_head);
            }
            else if (KLB_MNP_PONG == parser.packtype)
            {
                // C 层吞掉 Pong; 不推 Lua
                p_smpserve->ping_waiting = false;
                p_smpserve->ping_deadline_tc = 0;
                p_smpserve->parser_status = KLB_SMPPARSER_head;
                klb_rbuf_memmove(p_head);
            }
            else if (KLB_MNP_RPC_LUA == parser.packtype ||
                KLB_MNP_RPC_JSON == parser.packtype)
            {
                klb_buf_t* p_buf = klb_buf_malloc(parser.pack_len, false);
                klb_buf_write(p_buf, (const char*)&parser.rpc, sizeof(klb_mnp_rpc_t));
                p_smpserve->reading_len = parser.pack_len - parser.head_len;

                int body_len = 0;
                char* p_body = klb_rbuf_data(p_head, &body_len);
                int cp_len = MIN(p_smpserve->reading_len, body_len);
                if (0 < cp_len)
                {
                    klb_buf_write(p_buf, p_body, cp_len);

                    klb_rbuf_use(p_head, cp_len);
                    p_smpserve->reading_len -= cp_len;
                }

                if (0 < p_smpserve->reading_len)
                {
                    p_smpserve->p_read_body = p_buf;

                    p_smpserve->parser = parser;
                    p_smpserve->parser_status = KLB_SMPPARSER_body;
                    klb_rbuf_memmove(p_head);
                }
                else
                {
                    // 完整的数据包
                    p_smpserve->parser_status = KLB_SMPPARSER_head;

                    // 先整理 rbuf, 再回调; 避免 Lua disconnect 后 memmove 悬空
                    klb_rbuf_memmove(p_head);
                    push_data_klb_smpserverpc_conn(p_conn, KLB_SOCKET_OK, parser.packtype, p_buf);
                }
            }
            else
            {
                skip_body_klb_smpserverpc_conn(p_conn, &parser, p_head);
                klb_rbuf_memmove(p_head);
            }
        }
        else if (0 < ret_parse)
        {
            // 数据不足
            return 0;
        }
        else
        {
            // 协议错误
            return -2;
        }
    }
    else if (KLB_SMPPARSER_body == p_smpserve->parser_status)
    {
        klb_buf_t* p_buf = p_smpserve->p_read_body;
        int idle_len = MIN(p_smpserve->reading_len, klb_buf_idle_len(p_buf));
        char* p_idle = p_buf->p_buf + p_buf->end;

        int recv_len = klb_socket_recv(p_socket, p_idle, idle_len);
        if (0 < recv_len)
        {
            p_buf->end += recv_len;
            p_smpserve->reading_len -= recv_len;
            *p_read_num += recv_len;

            if (p_smpserve->reading_len <= 0)
            {
                p_smpserve->p_read_body = NULL;
                p_smpserve->parser_status = KLB_SMPPARSER_head;

                if (is_rpc_pack_klb_smpserverpc_conn(p_smpserve->parser.packtype))
                {
                    push_data_klb_smpserverpc_conn(p_conn, KLB_SOCKET_OK, p_smpserve->parser.packtype, p_buf);
                }
                else
                {
                    KLB_FREE_BY(p_buf, klb_buf_unref);
                }
            }
        }
        else
        {
            int close_status = KLB_SOCKET_DISCONNECT;
            int judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                return 1;
            }

            klb_socket_recv_closed(p_socket, close_status);
            return -1;
        }
    }

    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_smpserverpc_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_smpserverpc_conn(p_conn, &read_num);

        if (0 > ret)
        {
            int code = (int)klb_socket_get_status(p_socket);

            if (-2 == ret)
            {
                code = KLB_SOCKET_ERR_PROTOCOL;
                klb_socket_recv_closed(p_socket, code);
            }
            else if (KLB_SOCKET_OK == code)
            {
                code = KLB_SOCKET_DISCONNECT;
                klb_socket_recv_closed(p_socket, code);
            }

            push_data_klb_smpserverpc_conn(p_conn, code, 0, NULL);
            break;
        }

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @param [in] msg             消息类型: klb_netconn_msg_e
static int klb_smpserverpc_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_smpserverpc_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_smpserverpc_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_smpserverpc_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_smpserverpc_conn_free(klb_netconn_t* p_conn)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_smpserve->p_netmulti, p_conn);
    p_smpserve->p_netmulti = NULL;
}

/// @brief 发送数据
int klb_smpserverpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    int total_len = body_len;

    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = sequence;
    rpc.rpctype = rpctype; // KLB_MNP_RPC_LUA;
    rpc.method = method; // KLB_MNP_RPC_POST;
    rpc.encode = KLB_MNP_ENCODE_NULL;
    rpc.size = total_len + sizeof(klb_mnp_rpc_t);

    klb_smp_t smp = { 0 };
    smp.magic = KLB_SMP_MAGIC;
    smp.size = rpc.size + sizeof(klb_smp_t);
    smp.packtype = rpctype; // KLB_MNP_RPC_LUA;

    klb_buf_t* p_data = klb_buf_malloc(smp.size, false);
    klb_buf_write(p_data, (const char*)&smp, sizeof(smp)); // SMP 头部
    klb_buf_write(p_data, (const char*)&rpc, sizeof(rpc)); // RPC 头部
    klb_buf_write(p_data, p_body, body_len);  // 有效 数据体

    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_data);

    klb_socket_set_writing(p_socket, true);

    return 0;
}

/// @brief 按 buf 发送数据
int klb_smpserverpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    int total_len = klb_buf_data_len(p_data);

    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = sequence;
    rpc.rpctype = rpctype; // KLB_MNP_RPC_LUA;
    rpc.method = method; // KLB_MNP_RPC_POST;
    rpc.encode = KLB_MNP_ENCODE_NULL;
    rpc.size = total_len + sizeof(klb_mnp_rpc_t);

    klb_smp_t smp = { 0 };
    smp.magic = KLB_SMP_MAGIC;
    smp.size = rpc.size + sizeof(klb_smp_t);
    smp.packtype = rpctype; // KLB_MNP_RPC_LUA;

    klb_buf_t* p_head = klb_buf_malloc(sizeof(klb_smp_t) + sizeof(klb_mnp_rpc_t), false);
    klb_buf_write(p_head, (const char*)&smp, sizeof(smp)); // SMP 头部
    klb_buf_write(p_head, (const char*)&rpc, sizeof(rpc)); // RPC 头部

    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_head);
    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_data);

    klb_socket_set_writing(p_socket, true);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_smpserverpc_conn_init(klb_netconn_t* p_conn)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    // 初始化 vtable
    {
        p_conn->vtable.destroy = NULL; // 销毁
        p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

        p_conn->vtable.ioctrl = klb_smpserverpc_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
        p_conn->vtable.send_normal = klb_smpserverpc_conn_send_normal; // 调用者 发送常规数据包
        p_conn->vtable.send_media = klb_smpserverpc_conn_send_media; // 调用者 发送媒体数据包
        p_conn->vtable.on_send = klb_smpserverpc_conn_on_send; // 当网络上可以发送数据时
        p_conn->vtable.on_recv = klb_smpserverpc_conn_on_recv; // 当网络上可以接收数据时
        p_conn->vtable.on_msg = klb_smpserverpc_conn_on_msg; // 当网络上有消息传来时
    }


    // 初始化其他
    p_smpserve->p_write_nlist = klb_nlist_create();
    p_smpserve->p_read_head = klb_rbuf_malloc(KLB_SMP_HEAD_MAX);

    // 初始化其他
    p_smpserve->parser_status = KLB_SMPPARSER_head;

    p_smpserve->p_read_body = NULL;
    p_smpserve->reading_len = 0;
    normalize_ping_klb_smpserverpc_conn(p_smpserve, KLB_SMPSERVERPC_PING_INTERVAL_MS_DEFAULT, KLB_SMPSERVERPC_PING_TIMEOUT_MS_DEFAULT);
    p_smpserve->last_ping_tc = 0;
    p_smpserve->ping_deadline_tc = 0;
    p_smpserve->ping_waiting = false;

    return 0;
}

static void klb_smpserverpc_conn_quit(klb_netconn_t* p_conn)
{
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_smpserve->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_smpserve->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_smpserve->p_read_head, klb_rbuf_free);
    KLB_FREE_BY(p_smpserve->p_read_body, klb_buf_unref);
    KLB_FREE_BY(p_smpserve->p_write_nlist, klb_nlist_destroy);
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建SMP连接
/// @return klb_netconn_t*
klb_netconn_t* klb_smpserverpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_smpserverpc_conn_t));
    klb_smpserverpc_conn_t* p_smpserve = (klb_smpserverpc_conn_t*)p_conn->extra;

    klb_smpserverpc_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_smpserverpc_conn_destroy;

    // socket
    {
        p_smpserve->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        // 开启读写
        //klb_socket_set_writing(p_socket, true);
        klb_socket_set_reading(p_socket, true);
    }

    // 
    {
        // 放入复用模块
        klb_netmulti_push(p_netmulti, p_conn);
    }

    return p_conn;
}

//end
