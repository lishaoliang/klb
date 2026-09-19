// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbhttp/core/klb_httpserve_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/http_parser.h"
#include "klbthird/sds.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include <assert.h>


/// @struct klb_httpserve_conn_t
/// @brief  HTTP服务连接(HTTP serve connect)
typedef struct klb_httpserve_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // 发送 数据体
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送 HTTP数据 列表
    };

    // 读取 数据
    struct
    {
        int                         parser_status;      ///< 读取解析状态
#define KLB_HTTPSERVE_PARSER_ERR                (-1)
#define KLB_HTTPSERVE_PARSER                    0
#define KLB_HTTPSERVE_PARSER_OVER               2

        http_parser                 parser;             ///< HTTP解析器
        http_parser_settings        settings;           ///< 解析器设置
        bool                        req_line_crlf;      ///< 请求行是否已补 HTTP/x.x CRLF

        klb_buf_t*                  p_read_buf;         ///< 读取缓存

        sds                         header;             ///< 已解析的请求头
        klb_buffer_t*               p_body;             ///< 已解析的请求体
    };
}klb_httpserve_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_httpserve_conn_quit(klb_netconn_t* p_conn);
static void reset_msg_klb_httpserve_conn(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数


/// @brief 接收到数据 之后 放入数据
/// @param [in] packtype    数包类型: klb_mnp_packtype_e
static void push_data_klb_httpserve_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
{
    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, KLB_NETCODE_OK, packtype, p_data);
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
    }
}

/// @brief 接收到数据 之后 放入数据
/// @param [in] code        错误码: klb_netcode_e
static void push_netcode_klb_httpserve_conn(klb_netconn_t* p_conn, int code)
{
    assert(KLB_NETCODE_OK != code);

    if ((KLB_SOCKET_OK < code && code <= KLB_SOCKET_ERR_MAX) ||
        KLB_SOCKET_CLOSEING == code)
    {
        // 关闭 socket 读写
        klb_socket_set_reading(p_conn->p_socket, false);
        klb_socket_set_writing(p_conn->p_socket, false);
    }

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, 0, NULL);
    }
}

/// @brief 组装完整HTTP请求, 按 TEXT 包上送
static void push_pack_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    int head_len = 0;
    if (NULL != p_httpserve->header)
    {
        head_len = (int)sdslen(p_httpserve->header);
    }

    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    if (NULL != p_httpserve->p_body && 0 < klb_buffer_datalen(p_httpserve->p_body))
    {
        p_body_part = klb_buffer_join(p_httpserve->p_body, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_httpserve->p_body);
    }

    int total = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_data = klb_buf_malloc(total, false);

    klb_mnp_text_t com = { 0 };
    com.size = (uint32_t)total;
    com.head_size = (uint32_t)head_len;

    memcpy(p_data->p_buf, &com, sizeof(com));
    if (0 < head_len)
    {
        memcpy(p_data->p_buf + sizeof(com), p_httpserve->header, head_len);
    }

    if (0 < body_len)
    {
        memcpy(p_data->p_buf + sizeof(com) + head_len, p_body_part->p_buf + p_body_part->start, body_len);
    }

    p_data->start = 0;
    p_data->end = total;

    KLB_FREE_BY(p_body_part, klb_buf_unref);
    KLB_FREE_BY(p_httpserve->header, sdsfree);

    push_data_klb_httpserve_conn(p_conn, KLB_MNP_TEXT, p_data);
}

/// @brief connect 超时
static void on_connect_timeout_klb_httpserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

/// @brief connect 握手完成
static void on_connected_timeout_klb_httpserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

/// @brief ticker 定时器消息
static void on_ticker_klb_httpserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}


//////////////////////////////////////////////////////////////////////////
// http_parser 回调

static int on_message_begin_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    KLB_FREE_BY(p_httpserve->header, sdsfree);
    p_httpserve->header = sdsnew(http_method_str((enum http_method)p_parser->method));
    p_httpserve->header = sdscat(p_httpserve->header, " ");
    p_httpserve->req_line_crlf = false;

    return 0;
}

static int on_url_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    if (NULL == p_httpserve->header)
    {
        p_httpserve->header = sdsnew("");
    }

    if (0 < length)
    {
        p_httpserve->header = sdscatlen(p_httpserve->header, at, length);
    }

    return 0;
}

static int on_status_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    return 0;
}

static void finish_req_line_klb_httpserve_conn(http_parser* p_parser, klb_httpserve_conn_t* p_httpserve)
{
    if (p_httpserve->req_line_crlf)
    {
        return;
    }

    if (NULL == p_httpserve->header)
    {
        p_httpserve->header = sdsnew("");
    }

    p_httpserve->header = sdscatfmt(p_httpserve->header, " HTTP/%u.%u\r\n",
        p_parser->http_major, p_parser->http_minor);
    p_httpserve->req_line_crlf = true;
}

static int on_header_field_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    finish_req_line_klb_httpserve_conn(p_parser, p_httpserve);

    p_httpserve->header = sdscatlen(p_httpserve->header, at, length);
    p_httpserve->header = sdscat(p_httpserve->header, ": ");

    return 0;
}

static int on_header_value_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->header = sdscatlen(p_httpserve->header, at, length);
    p_httpserve->header = sdscat(p_httpserve->header, "\r\n");

    return 0;
}

static int on_headers_complete_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    finish_req_line_klb_httpserve_conn(p_parser, p_httpserve);
    p_httpserve->header = sdscat(p_httpserve->header, "\r\n");

    return 0;
}

static int on_body_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    if (0 == length)
    {
        return 0;
    }

    if (NULL == p_httpserve->p_body)
    {
        p_httpserve->p_body = klb_buffer_create(4096);
    }

    klb_buffer_write(p_httpserve->p_body, at, (int)length);

    return 0;
}

static int on_message_complete_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->parser_status = KLB_HTTPSERVE_PARSER_OVER;

    return 0;
}

/// @brief 复位单条消息收集状态(keep-alive 下 parser 本身不 init)
static void reset_msg_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->parser_status = KLB_HTTPSERVE_PARSER;
    p_httpserve->req_line_crlf = false;

    KLB_FREE_BY(p_httpserve->header, sdsfree);
    if (NULL != p_httpserve->p_body)
    {
        klb_buffer_reset(p_httpserve->p_body);
    }
}

/// @brief 压缩读缓存
static void compact_read_klb_httpserve_conn(klb_httpserve_conn_t* p_httpserve)
{
    klb_buf_t* p_buf = p_httpserve->p_read_buf;

    if (p_buf->end <= p_buf->start)
    {
        p_buf->start = 0;
        p_buf->end = 0;
    }
    else if (0 < p_buf->start)
    {
        memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        p_buf->end = p_buf->end - p_buf->start;
        p_buf->start = 0;
    }
}

/// @brief 解析读缓存中的HTTP请求
static int parser_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_httpserve->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        size_t have = (size_t)(p_buf->end - p_buf->start);
        size_t nparsed = http_parser_execute(&p_httpserve->parser, &p_httpserve->settings,
            p_buf->p_buf + p_buf->start, have);

        if (0 != p_httpserve->parser.http_errno)
        {
            p_httpserve->parser_status = KLB_HTTPSERVE_PARSER_ERR;
            compact_read_klb_httpserve_conn(p_httpserve);
            return -1;
        }

        if (0 < nparsed)
        {
            p_buf->start += (int)nparsed;
        }

        if (KLB_HTTPSERVE_PARSER_OVER == p_httpserve->parser_status)
        {
            push_pack_klb_httpserve_conn(p_conn);
            reset_msg_klb_httpserve_conn(p_conn);
            continue;
        }

        if (0 == nparsed)
        {
            break;
        }

        if (nparsed < have)
        {
            break;
        }
    }

    compact_read_klb_httpserve_conn(p_httpserve);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_httpserve_conn_destroy(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    // 之前 需要 调用 klb_httpserve_conn_free 函数
    assert(NULL == p_httpserve->p_netmulti);

    // 关闭socket
    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    // 退出
    klb_httpserve_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_httpserve_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_httpserve_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_httpserve_conn_send(p_conn, p_head, head_len, p_body, body_len);
}

/// @brief 发送媒体数据包
/// @return int
static int klb_httpserve_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_httpserve_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_httpserve->p_write_nlist;
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

        uint8_t* p_data = (uint8_t*)(p_buf->p_buf + p_buf->start);
        int send_num = klb_socket_send(p_socket, p_data, data_len);

        if (0 < send_num)
        {
            p_buf->start += send_num;
            write_num += send_num;
        }
        else if (0 == send_num)
        {
            break;
        }
        else
        {
            break;
        }
    }

    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
        push_netcode_klb_httpserve_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

/// @brief 接收网络数据
/// @return int
static int do_recv_klb_httpserve_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_httpserve->p_read_buf;

    if (KLB_HTTPSERVE_PARSER_ERR == p_httpserve->parser_status)
    {
        return -1;
    }

    int idle_len = klb_buf_idle_len(p_buf);
    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_httpserve_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_httpserve_conn(p_conn, KLB_NETCODE_RBUF_FULL);
            return -1;
        }
    }

    int recv_len = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), idle_len);
    if (recv_len < 0)
    {
        return -1;
    }
    else if (0 < recv_len)
    {
        p_buf->end += recv_len;
        *p_read_num += recv_len;
    }
    else
    {
        if (p_buf->end <= p_buf->start)
        {
            return 1;
        }
    }

    int ret_parse = parser_klb_httpserve_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 == recv_len)
    {
        return 1;
    }

    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_httpserve_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_httpserve_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @param [in] msg             消息类型: klb_netconn_msg_e
static int klb_httpserve_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_httpserve_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_httpserve_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_httpserve_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

/// @brief 关闭连接
void klb_httpserve_conn_free(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_httpserve->p_netmulti, p_conn);
    p_httpserve->p_netmulti = NULL;
}

/// @brief 发送HTTP响应
int klb_httpserve_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    int total_len = 0;
    if (0 < head_len)
    {
        total_len += head_len;
    }

    if (0 < body_len)
    {
        total_len += body_len;
    }

    if (total_len <= 0)
    {
        return 0;
    }

    klb_buf_t* p_data = klb_buf_malloc(total_len, false);
    if (0 < head_len && NULL != p_head)
    {
        klb_buf_write(p_data, (const char*)p_head, head_len);
    }

    if (0 < body_len && NULL != p_body)
    {
        klb_buf_write(p_data, (const char*)p_body, body_len);
    }

    klb_nlist_push_tail(p_httpserve->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

/// @brief 按 buf 发送HTTP数据
int klb_httpserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    klb_nlist_push_tail(p_httpserve->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

/// @brief 写缓存 是否为空
bool klb_httpserve_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_httpserve->p_write_nlist))
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

/// @brief 初始化
static int klb_httpserve_conn_init(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    {
        p_conn->vtable.destroy = NULL;
        p_conn->vtable.recv_data = NULL;

        p_conn->vtable.ioctrl = klb_httpserve_conn_ioctrl;
        p_conn->vtable.send_normal = klb_httpserve_conn_send_normal;
        p_conn->vtable.send_media = klb_httpserve_conn_send_media;
        p_conn->vtable.on_send = klb_httpserve_conn_on_send;
        p_conn->vtable.on_recv = klb_httpserve_conn_on_recv;
        p_conn->vtable.on_msg = klb_httpserve_conn_on_msg;
    }

    p_conn->protocol = KLB_PROTOCOL_HTTP;

    p_httpserve->p_write_nlist = klb_nlist_create();
    p_httpserve->p_read_buf = klb_buf_malloc(1024 * 16, false);

    http_parser_init(&p_httpserve->parser, HTTP_REQUEST);
    http_parser_settings_init(&p_httpserve->settings);
    p_httpserve->parser.data = p_conn;

    p_httpserve->settings.on_message_begin = on_message_begin_klb_httpserve_conn;
    p_httpserve->settings.on_url = on_url_klb_httpserve_conn;
    p_httpserve->settings.on_status = on_status_klb_httpserve_conn;
    p_httpserve->settings.on_header_field = on_header_field_klb_httpserve_conn;
    p_httpserve->settings.on_header_value = on_header_value_klb_httpserve_conn;
    p_httpserve->settings.on_headers_complete = on_headers_complete_klb_httpserve_conn;
    p_httpserve->settings.on_body = on_body_klb_httpserve_conn;
    p_httpserve->settings.on_message_complete = on_message_complete_klb_httpserve_conn;

    p_httpserve->parser_status = KLB_HTTPSERVE_PARSER;
    p_httpserve->req_line_crlf = false;
    p_httpserve->header = NULL;
    p_httpserve->p_body = NULL;

    return 0;
}

/// @brief 退出
static void klb_httpserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_httpserve->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_httpserve->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_httpserve->header, sdsfree);
    KLB_FREE_BY(p_httpserve->p_body, klb_buffer_destroy);
    KLB_FREE_BY(p_httpserve->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_httpserve->p_write_nlist, klb_nlist_destroy);
}


//////////////////////////////////////////////////////////////////////////

/// @brief 创建HTTP服务连接
klb_netconn_t* klb_httpserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_httpserve_conn_t));
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    klb_httpserve_conn_init(p_conn);

    p_conn->vtable.destroy = klb_httpserve_conn_destroy;

    {
        p_httpserve->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        klb_socket_set_reading(p_socket, true);
    }

    {
        klb_netmulti_push(p_netmulti, p_conn);
    }

    return p_conn;
}

// end
