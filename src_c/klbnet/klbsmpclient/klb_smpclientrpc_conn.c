// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbsmp/klb_smpclientrpc_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_nlist.h"
#include "klbnet/klbsmp/klb_smpparser.h"
#include <assert.h>

/// @struct klb_smpclientrpc_conn_t
/// @brief  SMP客户端RPC连接(SMP client RPC connect)
typedef struct klb_smpclientrpc_conn_t_
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
    };
}klb_smpclientrpc_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_smpclientrpc_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数


/// @brief 接收到数据 之后 放入数据
/// @param [in] code        错误码: klb_socket_status_e
/// @param [in] packtype    数包类型: klb_mnp_packtype_e
static void push_data_klb_smpclientrpc_conn(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

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
static void on_connect_timeout_klb_smpclientrpc_conn(klb_netconn_t* p_conn, int64_t now)
{

}

/// @brief connect 握手完成
static void on_connected_timeout_klb_smpclientrpc_conn(klb_netconn_t* p_conn, int64_t now)
{

}

/// @brief ticker 定时器消息
static void on_ticker_klb_smpclientrpc_conn(klb_netconn_t* p_conn, int64_t now)
{

}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_smpclientrpc_conn_destroy(klb_netconn_t* p_conn)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    // 移除
    if (NULL != p_smpclient->p_netmulti)
    {
        klb_netmulti_remove(p_smpclient->p_netmulti, p_conn);
        p_smpclient->p_netmulti = NULL;
    }

    // 关闭socket
    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    // 退出
    klb_smpclientrpc_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_smpclientrpc_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

/// @brief 发送常规 RPC-LUA 数据包
static int klb_smpclientrpc_conn_send_rpc_lua(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    int total_len = body_len;

    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = sequence;
    rpc.uid = uid;
    rpc.size = total_len + sizeof(klb_mnp_rpc_t);

    klb_smp_t smp = { 0 };
    smp.magic = KLB_SMP_MAGIC;
    smp.size = rpc.size + sizeof(klb_smp_t);
    smp.packtype = KLB_MNP_RPC_LUA;

    klb_buf_t* p_data = klb_buf_malloc(smp.size, false);
    klb_buf_write(p_data, (const char*)&smp, sizeof(smp)); // SMP 头部
    klb_buf_write(p_data, (const char*)&rpc, sizeof(rpc)); // RPC 头部
    klb_buf_write(p_data, p_body, body_len);  // 有效 数据体

    klb_nlist_push_tail(p_smpclient->p_write_nlist, p_data);
    klb_socket_set_writing(p_conn->p_socket, true);

    return 0;
}

/// @brief 发送常规 RPC-JSON 数据包
static int klb_smpclientrpc_conn_send_rpc_json(klb_netconn_t* p_conn, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    int total_len = body_len;

    klb_mnp_rpc_t rpc = { 0 };
    rpc.sequence = sequence;
    rpc.uid = uid;
    rpc.size = total_len + sizeof(klb_mnp_rpc_t);

    klb_smp_t smp = { 0 };
    smp.magic = KLB_SMP_MAGIC;
    smp.size = rpc.size + sizeof(klb_smp_t);
    smp.packtype = KLB_MNP_RPC_JSON;

    klb_buf_t* p_data = klb_buf_malloc(smp.size, false);
    klb_buf_write(p_data, (const char*)&smp, sizeof(smp)); // SMP 头部
    klb_buf_write(p_data, (const char*)&rpc, sizeof(rpc)); // RPC 头部
    klb_buf_write(p_data, p_body, body_len);  // 有效 数据体

    klb_nlist_push_tail(p_smpclient->p_write_nlist, p_data);
    klb_socket_set_writing(p_conn->p_socket, true);

    return 0;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_smpclientrpc_conn_send_normal(klb_netconn_t* p_conn, int packtype, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (KLB_MNP_RPC_LUA == packtype)
    {
        return klb_smpclientrpc_conn_send_rpc_lua(p_conn, sequence, uid, p_head, head_len, p_body, body_len);
    }
    else if(KLB_MNP_RPC_JSON == packtype)
    {
        return klb_smpclientrpc_conn_send_rpc_json(p_conn, sequence, uid, p_head, head_len, p_body, body_len);
    }

    return 1;
}

/// @brief 发送媒体数据包
/// @return int
static int klb_smpclientrpc_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_smpclientrpc_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_smpclient->p_write_nlist;
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

        if (0 < send_num)
        {
            p_buf->start += send_num;
            write_num += send_num;
        }
        else if (0 == send_num)
        {
            // 无法发送了
            break;
        }
        else
        {
            // 
            break;
        }
    }

    // 数据写完了, 无需读取
    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
    }

    return write_num;
}

/// @brief 接收网络数据
/// @return int
static int do_recv_klb_smpclientrpc_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SMPPARSER_head == p_smpclient->parser_status)
    {
        klb_rbuf_t* p_head = p_smpclient->p_read_head;

        int idle_len = 0;
        char* p_idle = klb_rbuf_idle(p_head, 0, &idle_len);
        int recv_len = klb_socket_recv(p_socket, p_idle, idle_len);

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_head, recv_len);
            *p_read_num += recv_len;
        }
        else if (0 == recv_len)
        {
            return 1; // 无数据可以读
        }
        else
        {
            // 连接断开等错误
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
            if (KLB_MNP_PONG == parser.packtype ||
                KLB_MNP_PING == parser.packtype)
            {
                p_smpclient->parser_status = KLB_SMPPARSER_head;
            }
            else if (KLB_MNP_RPC_LUA == parser.packtype ||
                KLB_MNP_RPC_JSON == parser.packtype)
            {
                klb_buf_t* p_buf = klb_buf_malloc(parser.pack_len, false);
                klb_buf_write(p_buf, (const char*)&parser.rpc, sizeof(klb_mnp_rpc_t));
                p_smpclient->reading_len = parser.pack_len - parser.head_len;

                int body_len = 0;
                char* p_body = klb_rbuf_data(p_head, &body_len);
                int cp_len = MIN(p_smpclient->reading_len, body_len);
                if (0 < cp_len)
                {
                    klb_buf_write(p_buf, p_body, cp_len);

                    klb_rbuf_use(p_head, cp_len);
                    p_smpclient->reading_len -= cp_len;
                }

                if (0 < p_smpclient->reading_len)
                {
                    p_smpclient->p_read_body = p_buf;

                    p_smpclient->parser = parser;
                    p_smpclient->parser_status = KLB_SMPPARSER_body;
                }
                else
                {
                    // 完整的数据包
                    p_smpclient->parser_status = KLB_SMPPARSER_head;

                    // 完整数据
                    push_data_klb_smpclientrpc_conn(p_conn, KLB_SOCKET_OK, parser.packtype, p_buf);
                }
            }
            else
            {
                // 不支持
                p_smpclient->parser_status = KLB_SMPPARSER_null;
            }

            klb_rbuf_memmove(p_head);
        }
        else if (0 < ret_parse)
        {
            // 数据不足
            return 0;
        }
        else
        {
            // 协议错误
            return -1;
        }
    }
    else if (KLB_SMPPARSER_body == p_smpclient->parser_status)
    {
        klb_buf_t* p_buf = p_smpclient->p_read_body;
        int idle_len = MIN(p_smpclient->reading_len, klb_buf_idle_len(p_buf));
        char* p_idle = p_buf->p_buf + p_buf->end;

        int recv_len = klb_socket_recv(p_socket, p_idle, idle_len);
        if (0 < recv_len)
        {
            p_buf->end += recv_len;
            p_smpclient->reading_len -= recv_len;
            *p_read_num += recv_len;

            if (p_smpclient->reading_len <= 0)
            {
                p_smpclient->p_read_body = NULL;
                p_smpclient->parser_status = KLB_SMPPARSER_head;

                // 完整数据
                push_data_klb_smpclientrpc_conn(p_conn, KLB_SOCKET_OK, p_smpclient->parser.packtype, p_buf);
            }
        }
        else if(0 == recv_len)
        {
            // 无数据可读
            return 1;
        }
        else
        {
            // 连接错误
            return -1;
        }
    }

    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_smpclientrpc_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_smpclientrpc_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }
    
    return read_num;
}

/// @brief 当网络上有消息传来时
/// @param [in] msg             消息类型: klb_netconn_msg_e
static int klb_smpclientrpc_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_smpclientrpc_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_smpclientrpc_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_smpclientrpc_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数



//////////////////////////////////////////////////////////////////////////
// init / quit

/// @brief 初始化
static int klb_smpclientrpc_conn_init(klb_netconn_t* p_conn)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    // 初始化 vtable
    {
        p_conn->vtable.destroy = NULL; // 销毁
        p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

        p_conn->vtable.ioctrl = klb_smpclientrpc_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
        p_conn->vtable.send_normal = klb_smpclientrpc_conn_send_normal; // 调用者 发送常规数据包
        p_conn->vtable.send_media = klb_smpclientrpc_conn_send_media; // 调用者 发送媒体数据包
        p_conn->vtable.on_send = klb_smpclientrpc_conn_on_send; // 当网络上可以发送数据时
        p_conn->vtable.on_recv = klb_smpclientrpc_conn_on_recv; // 当网络上可以接收数据时
        p_conn->vtable.on_msg = klb_smpclientrpc_conn_on_msg; // 当网络上有消息传来时
    }


    // 初始化
    p_smpclient->p_write_nlist = klb_nlist_create();
    p_smpclient->p_read_head = klb_rbuf_malloc(KLB_SMP_HEAD_MAX);

    // 初始化其他
    p_smpclient->parser_status = KLB_SMPPARSER_head;

    p_smpclient->p_read_body = NULL;
    p_smpclient->reading_len = 0;

    return 0;
}

/// @brief 退出
static void klb_smpclientrpc_conn_quit(klb_netconn_t* p_conn)
{
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_smpclient->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_smpclient->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_smpclient->p_read_head, klb_rbuf_free);
    KLB_FREE_BY(p_smpclient->p_read_body, klb_buf_unref);
    KLB_FREE_BY(p_smpclient->p_write_nlist, klb_nlist_destroy);
}

//////////////////////////////////////////////////////////////////////////
/// 连接目标

/// @brief SMP连接目标
klb_netconn_t* klb_smpclientrpc_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port)
{
    // 连接 socket
    klb_socket_fd fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        return NULL;
    }

    // 创建 klb_socket_t*
    klb_socket_t* p_socket = klb_socket_async_create(fd);

    // 创建 klb_netconn_t*
    klb_netconn_t* p_netconn = klb_smpclientrpc_conn_create(p_netmulti, p_socket);

    {
        // 放入复用模块
        klb_netmulti_push(p_netmulti, p_netconn);
    }

    return p_netconn;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建SMP连接
/// @return klb_netconn_t*
klb_netconn_t* klb_smpclientrpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_smpclientrpc_conn_t));
    klb_smpclientrpc_conn_t* p_smpclient = (klb_smpclientrpc_conn_t*)p_conn->extra;

    klb_smpclientrpc_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_smpclientrpc_conn_destroy;


    // socket
    {
        p_smpclient->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        // 开启读写
        //klb_socket_set_writing(p_socket, true);
        klb_socket_set_reading(p_socket, true);
    }

    return p_conn;
}

//end
