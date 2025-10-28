// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klbws/klb_wsclient_conn.h"
#include "klbmem/klb_mem.h"


/// @struct klb_wsclient_conn_t
/// @brief  WS客户端连接
typedef struct klb_wsclient_conn_t_
{
    int a;
}klb_wsclient_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_wsclient_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数




//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_wsclient_conn_destroy(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_wsclient = (klb_wsclient_conn_t*)p_conn->extra;

    klb_wsclient_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_wsclient_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 0;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_wsclient_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return 0;
}

/// @brief 发送媒体数据包
/// @return int
static int klb_wsclient_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 0;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_wsclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_wsclient_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    return 0;
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_wsclient_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数



//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_wsclient_conn_init(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_wsclient = (klb_wsclient_conn_t*)p_conn->extra;

    // 初始化 vtable
    p_conn->vtable.destroy = NULL; // 销毁
    p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

    p_conn->vtable.ioctrl = klb_wsclient_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
    p_conn->vtable.send_normal = klb_wsclient_conn_send_normal; // 调用者 发送常规数据包
    p_conn->vtable.send_media = klb_wsclient_conn_send_media; // 调用者 发送媒体数据包
    p_conn->vtable.on_send = klb_wsclient_conn_on_send; // 当网络上可以发送数据时
    p_conn->vtable.on_recv = klb_wsclient_conn_on_recv; // 当网络上可以接收数据时
    p_conn->vtable.on_msg = klb_wsclient_conn_on_msg; // 当网络上有消息传来时

    return 0;
}

static void klb_wsclient_conn_quit(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_wsclient = (klb_wsclient_conn_t*)p_conn->extra;

}

//////////////////////////////////////////////////////////////////////////
/// 连接目标

klb_netconn_t* klb_wsclient_connect()
{
    return NULL;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建WS连接
/// @return klb_netconn_t*
klb_netconn_t* klb_wsclient_conn_create()
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_wsclient_conn_t));

    klb_wsclient_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_wsclient_conn_destroy;

    return p_conn;
}

//end
