// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbrtsp/klb_rtspserve_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_nlist.h"
#include "klbnet/klbrtsp/klb_rtspparser.h"


/// @def   KLB_RTSPSERVE_rbuf_min
/// @brief 临时接收数据最小长度
#define KLB_RTSPSERVE_rbuf_min    (4096)

/// @def   KLB_RTSPSERVE_rbuf_max
/// @brief 临时接收数据最大长度
#define KLB_RTSPSERVE_rbuf_max    (65536)


/// @struct klb_rtspserve_conn_t
/// @brief  RTSP客户端连接
typedef struct klb_rtspserve_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // write
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 待发送数据列表
    };
 
    // read
    struct
    {
        klb_rbuf_t*                 p_read_rbuf;        ///< 临时读取缓存
    };
}klb_rtspserve_conn_t;



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_rtspserve_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////

static void push_data_klb_rtspserve_conn(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

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
        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }
}

//////////////////////////////////////////////////////////////////////////
// 


// 解析从网络上接收到的数据
static void parse_recv_data_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_rbuf_t* p_read_rbuf = p_rtsp->p_read_rbuf;

    while (true)
    {
        // 直接读取的数据
        int data_len = 0;
        char* p_data = klb_rbuf_data(p_read_rbuf, &data_len);

        if (data_len <= 0)
        {
            break; // 数据长度不足
        }

        // 解析数据
        klb_rtspparser_t parser = { 0 };
        int ret = klb_rtspparser_parse(&parser, p_data, data_len);

        if (0 == ret)
        {
            int pack_len = parser.pack_len;
            char* ptr = p_data;

            if (KLB_RTSPTYPE_rtsp == parser.rtsptype)
            {
                // rtsp 完整数据包
                klb_buf_t* p_buf = klb_buf_malloc(pack_len, false);
                klb_buf_write(p_buf, ptr, pack_len);

                push_data_klb_rtspserve_conn(p_conn, 0, KLB_MNP_TEXT, p_buf);
            }
            else if(KLB_RTSPTYPE_rtp == parser.rtsptype)
            {
                // rtp 包
            }
            else if(KLB_RTSPTYPE_rtcp == parser.rtsptype)
            {
                // rtcp 包
            }

            klb_rbuf_use(p_read_rbuf, parser.pack_len);
        }
        else if(0 < ret)
        {
            // 数据长度不足
            break;
        }
        else
        {
            // 协议解析错误
            assert(false);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// msg

// 握手 连接超时
static void on_msg_connect_timeout_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
}

// 握手完成
static void on_msg_connected_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_CONNECT, 0, NULL);
}

// 定时器消息
static void on_msg_ticker_klb_rtspserve_conn(klb_netconn_t* p_conn)
{

}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_rtspserve_conn_destroy(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (NULL != p_rtsp->p_netmulti)
    {
        klb_netmulti_remove(p_rtsp->p_netmulti, p_conn);
        p_rtsp->p_netmulti = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    klb_rtspserve_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_rtspserve_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    return 0;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_rtspserve_conn_send_normal(klb_netconn_t* p_conn, int packtype, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (KLB_MNP_TEXT == packtype)
    {
        klb_buf_t* p_buf = klb_buf_malloc(head_len + body_len, false);

        if (0 < head_len) { klb_buf_write(p_buf, p_head, head_len); }
        if (0 < body_len) { klb_buf_write(p_buf, p_body, body_len); }


        klb_nlist_push_tail(p_rtsp->p_write_nlist, p_buf);
        klb_socket_set_writing(p_conn->p_socket, true);
    }

    return 0;
}

/// @brief 发送媒体数据包
/// @return int
static int klb_rtspserve_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_rtspserve_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_rtsp->p_write_nlist;
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
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);

            continue;
        }

        uint8_t* p_data = p_buf->p_buf + p_buf->start;
        int send_num = klb_socket_send(p_socket, p_data, data_len);

        if (0 < send_num)
        {
            p_buf->start += send_num;
            write_num += send_num;
        }
        else if(0 == send_num)
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

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_rtspserve_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_rbuf_t* p_read_rbuf = p_rtsp->p_read_rbuf;
    int read_num = 0;

    while (true)
    {
        int idle_len = 0;
        char* p_idle = klb_rbuf_idle(p_read_rbuf, 0, &idle_len);

        if (NULL == p_idle)
        {
            // 缓存满了
            break;
        }

        int recv_len = klb_socket_recv(p_socket, (uint8_t*)p_idle, idle_len);

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_read_rbuf, recv_len);
            read_num += recv_len;

            // 解析数据
            parse_recv_data_klb_rtspserve_conn(p_conn);

            klb_rbuf_memmove(p_read_rbuf);
        }
        else if (0 == recv_len)
        {
            break; // 无数据了
        }
        else
        {
            break;
        }
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_rtspserve_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_msg_connect_timeout_klb_rtspserve_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_connected:
        on_msg_connected_klb_rtspserve_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_msg_ticker_klb_rtspserve_conn(p_conn);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_rtspserve_conn_init(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 初始化 vtable
    p_conn->vtable.destroy = NULL; // 销毁
    p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

    p_conn->vtable.ioctrl = klb_rtspserve_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
    p_conn->vtable.send_normal = klb_rtspserve_conn_send_normal; // 调用者 发送常规数据包
    p_conn->vtable.send_media = klb_rtspserve_conn_send_media; // 调用者 发送媒体数据包
    p_conn->vtable.on_send = klb_rtspserve_conn_on_send; // 当网络上可以发送数据时
    p_conn->vtable.on_recv = klb_rtspserve_conn_on_recv; // 当网络上可以接收数据时
    p_conn->vtable.on_msg = klb_rtspserve_conn_on_msg; // 当网络上有消息传来时

    // 初始化其他

    p_rtsp->p_write_nlist = klb_nlist_create();

    // 读
    p_rtsp->p_read_rbuf = klb_rbuf_malloc(KLB_RTSPSERVE_rbuf_min);

    return 0;
}

static void klb_rtspserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 释放内存
    while (0 < klb_nlist_size(p_rtsp->p_write_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_rtsp->p_write_nlist);
        KLB_FREE_BY(p_buf, klb_buf_unref_next);
    }

    // 销毁
    KLB_FREE_BY(p_rtsp->p_write_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_rtsp->p_read_rbuf, klb_rbuf_datalen);
}


//////////////////////////////////////////////////////////////////////////

/// @brief 创建rtsp连接
/// @return klb_netconn_t*
klb_netconn_t* klb_rtspserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_rtspserve_conn_t));
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    klb_rtspserve_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_rtspserve_conn_destroy;

    // socket
    {
        p_rtsp->p_netmulti = p_netmulti;
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
