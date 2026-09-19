// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbws/core/klb_wsserve_conn.h"
#include "klbws/core/klb_websocket.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/http_parser.h"
#include "klbthird/sds.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include <assert.h>


/// @struct klb_wsserve_conn_t
/// @brief  WS服务连接
typedef struct klb_wsserve_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送列表
    };

    struct
    {
        bool                        http_over;          ///< HTTP握手是否结束
        int                         parser_status;
#define KLB_WSSERVE_PARSER_ERR                  (-1)
#define KLB_WSSERVE_PARSER                      0
#define KLB_WSSERVE_PARSER_OVER                 2

        http_parser                 parser;
        http_parser_settings        settings;
        bool                        req_line_crlf;

        klb_buf_t*                  p_read_buf;
        sds                         header;
        klb_buffer_t*               p_body;

        int                         ws_status;
#define KLB_WSSERVE_WS_HEAD                     0
#define KLB_WSSERVE_WS_BODY                     1
        klb_websocket_t             ws;
        uint64_t                    ws_remaining;
        klb_buffer_t*               p_ws_payload;
    };
}klb_wsserve_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_wsserve_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数

static void push_data_klb_wsserve_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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

static void push_netcode_klb_wsserve_conn(klb_netconn_t* p_conn, int code)
{
    assert(KLB_NETCODE_OK != code);

    if ((KLB_SOCKET_OK < code && code <= KLB_SOCKET_ERR_MAX) ||
        KLB_SOCKET_CLOSEING == code)
    {
        klb_socket_set_reading(p_conn->p_socket, false);
        klb_socket_set_writing(p_conn->p_socket, false);
    }

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, 0, NULL);
    }
}

static void push_http_pack_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    int head_len = 0;
    if (NULL != p_ws->header)
    {
        head_len = (int)sdslen(p_ws->header);
    }

    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    if (NULL != p_ws->p_body && 0 < klb_buffer_datalen(p_ws->p_body))
    {
        p_body_part = klb_buffer_join(p_ws->p_body, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_ws->p_body);
    }

    int total = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_data = klb_buf_malloc(total, false);

    klb_mnp_text_t com = { 0 };
    com.size = (uint32_t)total;
    com.head_size = (uint32_t)head_len;

    memcpy(p_data->p_buf, &com, sizeof(com));
    if (0 < head_len)
    {
        memcpy(p_data->p_buf + sizeof(com), p_ws->header, head_len);
    }

    if (0 < body_len)
    {
        memcpy(p_data->p_buf + sizeof(com) + head_len, p_body_part->p_buf + p_body_part->start, body_len);
    }

    p_data->start = 0;
    p_data->end = total;

    KLB_FREE_BY(p_body_part, klb_buf_unref);
    KLB_FREE_BY(p_ws->header, sdsfree);

    push_data_klb_wsserve_conn(p_conn, KLB_MNP_TEXT, p_data);
}

static void push_ws_payload_klb_wsserve_conn(klb_netconn_t* p_conn, int packtype)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    if (NULL != p_ws->p_ws_payload && 0 < klb_buffer_datalen(p_ws->p_ws_payload))
    {
        p_body_part = klb_buffer_join(p_ws->p_ws_payload, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_ws->p_ws_payload);
    }

    int head_size = (int)sizeof(klb_mnp_text_t);
    if (KLB_MNP_BINARY == packtype)
    {
        head_size = (int)sizeof(klb_mnp_binary_t);
    }

    int total = head_size + body_len;
    klb_buf_t* p_data = klb_buf_malloc(total, false);

    if (KLB_MNP_BINARY == packtype)
    {
        klb_mnp_binary_t com = { 0 };
        com.size = (uint32_t)total;
        memcpy(p_data->p_buf, &com, sizeof(com));
    }
    else
    {
        klb_mnp_text_t com = { 0 };
        com.size = (uint32_t)total;
        memcpy(p_data->p_buf, &com, sizeof(com));
    }

    if (0 < body_len)
    {
        memcpy(p_data->p_buf + head_size, p_body_part->p_buf + p_body_part->start, body_len);
    }

    p_data->start = 0;
    p_data->end = total;

    KLB_FREE_BY(p_body_part, klb_buf_unref);
    push_data_klb_wsserve_conn(p_conn, packtype, p_data);
}

static void on_connect_timeout_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

static void on_connected_timeout_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

static void on_ticker_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}


//////////////////////////////////////////////////////////////////////////
// http_parser 回调

static int on_message_begin_klb_wsserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    KLB_FREE_BY(p_ws->header, sdsfree);
    p_ws->header = sdsnew(http_method_str((enum http_method)p_parser->method));
    p_ws->header = sdscat(p_ws->header, " ");
    p_ws->req_line_crlf = false;

    return 0;
}

static int on_url_klb_wsserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (NULL == p_ws->header)
    {
        p_ws->header = sdsnew("");
    }

    if (0 < length)
    {
        p_ws->header = sdscatlen(p_ws->header, at, length);
    }

    return 0;
}

static int on_status_klb_wsserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    return 0;
}

static void finish_req_line_klb_wsserve_conn(http_parser* p_parser, klb_wsserve_conn_t* p_ws)
{
    if (p_ws->req_line_crlf)
    {
        return;
    }

    if (NULL == p_ws->header)
    {
        p_ws->header = sdsnew("");
    }

    p_ws->header = sdscatfmt(p_ws->header, " HTTP/%u.%u\r\n",
        p_parser->http_major, p_parser->http_minor);
    p_ws->req_line_crlf = true;
}

static int on_header_field_klb_wsserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    finish_req_line_klb_wsserve_conn(p_parser, p_ws);
    p_ws->header = sdscatlen(p_ws->header, at, length);
    p_ws->header = sdscat(p_ws->header, ": ");

    return 0;
}

static int on_header_value_klb_wsserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    p_ws->header = sdscatlen(p_ws->header, at, length);
    p_ws->header = sdscat(p_ws->header, "\r\n");

    return 0;
}

static int on_headers_complete_klb_wsserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    finish_req_line_klb_wsserve_conn(p_parser, p_ws);
    p_ws->header = sdscat(p_ws->header, "\r\n");

    return 0;
}

static int on_body_klb_wsserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (0 == length)
    {
        return 0;
    }

    if (NULL == p_ws->p_body)
    {
        p_ws->p_body = klb_buffer_create(4096);
    }

    klb_buffer_write(p_ws->p_body, at, (int)length);

    return 0;
}

static int on_message_complete_klb_wsserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    p_ws->parser_status = KLB_WSSERVE_PARSER_OVER;

    return 0;
}

static void compact_read_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
{
    klb_buf_t* p_buf = p_ws->p_read_buf;

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

static void finish_ws_frame_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    uint8_t opcode = p_ws->ws.opcode;

    if (KLB_WEBSOCKET_OPCODE_TEXT == opcode)
    {
        push_ws_payload_klb_wsserve_conn(p_conn, KLB_MNP_TEXT);
    }
    else if (KLB_WEBSOCKET_OPCODE_BINARY == opcode)
    {
        push_ws_payload_klb_wsserve_conn(p_conn, KLB_MNP_BINARY);
    }
    else if (KLB_WEBSOCKET_OPCODE_PING == opcode)
    {
        push_data_klb_wsserve_conn(p_conn, KLB_MNP_PING, NULL);
        if (NULL != p_ws->p_ws_payload)
        {
            klb_buffer_reset(p_ws->p_ws_payload);
        }
    }
    else if (KLB_WEBSOCKET_OPCODE_PONG == opcode)
    {
        push_data_klb_wsserve_conn(p_conn, KLB_MNP_PONG, NULL);
        if (NULL != p_ws->p_ws_payload)
        {
            klb_buffer_reset(p_ws->p_ws_payload);
        }
    }
    else if (KLB_WEBSOCKET_OPCODE_CLOSE == opcode)
    {
        if (NULL != p_ws->p_ws_payload)
        {
            klb_buffer_reset(p_ws->p_ws_payload);
        }

        push_netcode_klb_wsserve_conn(p_conn, KLB_SOCKET_CLOSEING);
    }
    else
    {
        if (NULL != p_ws->p_ws_payload)
        {
            klb_buffer_reset(p_ws->p_ws_payload);
        }
    }

    p_ws->ws_status = KLB_WSSERVE_WS_HEAD;
    p_ws->ws_remaining = 0;
}

static int parser_ws_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        uint8_t* p_data = (uint8_t*)(p_buf->p_buf + p_buf->start);
        int data_len = p_buf->end - p_buf->start;

        if (KLB_WSSERVE_WS_HEAD == p_ws->ws_status)
        {
            klb_websocket_t ws = { 0 };
            int ret = klb_websocket_parse(&ws, p_data, data_len);
            if (0 != ret)
            {
                break;
            }

            p_buf->start += ws.head_len;
            p_ws->ws = ws;
            p_ws->ws_remaining = ws.payload_len;
            p_ws->ws_status = KLB_WSSERVE_WS_BODY;

            if (0 == p_ws->ws_remaining)
            {
                finish_ws_frame_klb_wsserve_conn(p_conn);
            }
        }
        else
        {
            if (data_len <= 0)
            {
                break;
            }

            int cp_len = (int)MIN(p_ws->ws_remaining, (uint64_t)data_len);
            if (0 != p_ws->ws.mask)
            {
                klb_websocket_mask(p_ws->ws.mask_key, p_data, cp_len);
            }

            if (NULL == p_ws->p_ws_payload)
            {
                p_ws->p_ws_payload = klb_buffer_create(4096);
            }

            klb_buffer_write(p_ws->p_ws_payload, (const char*)p_data, cp_len);
            p_buf->start += cp_len;
            p_ws->ws_remaining -= (uint64_t)cp_len;

            if (0 == p_ws->ws_remaining)
            {
                finish_ws_frame_klb_wsserve_conn(p_conn);
            }
        }
    }

    compact_read_klb_wsserve_conn(p_ws);
    return 0;
}

static int parser_http_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        size_t have = (size_t)(p_buf->end - p_buf->start);
        size_t nparsed = http_parser_execute(&p_ws->parser, &p_ws->settings,
            p_buf->p_buf + p_buf->start, have);

        if (0 != p_ws->parser.http_errno)
        {
            p_ws->parser_status = KLB_WSSERVE_PARSER_ERR;
            compact_read_klb_wsserve_conn(p_ws);
            return -1;
        }

        if (0 < nparsed)
        {
            p_buf->start += (int)nparsed;
        }

        if (KLB_WSSERVE_PARSER_OVER == p_ws->parser_status || 0 != p_ws->parser.upgrade)
        {
            push_http_pack_klb_wsserve_conn(p_conn);
            p_ws->http_over = true;
            p_ws->ws_status = KLB_WSSERVE_WS_HEAD;
            compact_read_klb_wsserve_conn(p_ws);
            return 0;
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

    compact_read_klb_wsserve_conn(p_ws);
    return 0;
}

static int parser_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (!p_ws->http_over)
    {
        int ret = parser_http_klb_wsserve_conn(p_conn);
        if (0 != ret)
        {
            return ret;
        }
    }

    if (p_ws->http_over)
    {
        return parser_ws_klb_wsserve_conn(p_conn);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klb_wsserve_conn_destroy(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    assert(NULL == p_ws->p_netmulti);

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    klb_wsserve_conn_quit(p_conn);
    KLB_FREE(p_conn);
}

static int klb_wsserve_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

static int klb_wsserve_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_wsserve_conn_send(p_conn, p_head, head_len, p_body, body_len);
}

static int klb_wsserve_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

static int klb_wsserve_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_ws->p_write_nlist;
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
        if (klb_socket_is_tls(p_socket) && (KLB_SOCKET_WAIT_WRITE == p_socket->status_rw))
        {
            klb_socket_send(p_socket, NULL, 0);
            if (KLB_SOCKET_WAIT_WRITE == p_socket->status_rw)
            {
                return write_num;
            }

            klb_socket_set_writing(p_socket, false);
            return write_num;
        }

        klb_socket_set_writing(p_socket, false);
        push_netcode_klb_wsserve_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

static int do_recv_klb_wsserve_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    if (KLB_WSSERVE_PARSER_ERR == p_ws->parser_status)
    {
        return -1;
    }

    int idle_len = klb_buf_idle_len(p_buf);
    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_wsserve_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_wsserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_wsserve_conn(p_conn, KLB_NETCODE_RBUF_FULL);
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

    int ret_parse = parser_klb_wsserve_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_wsserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 == recv_len)
    {
        return 1;
    }

    return 0;
}

static int klb_wsserve_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_wsserve_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

static int klb_wsserve_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_wsserve_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_wsserve_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_wsserve_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_wsserve_conn_free(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_ws->p_netmulti, p_conn);
    p_ws->p_netmulti = NULL;
}

int klb_wsserve_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
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

    if (!p_ws->http_over)
    {
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

        klb_nlist_push_tail(p_ws->p_write_nlist, p_data);
        klb_socket_set_writing(p_socket, true);
        return 0;
    }

    klb_buf_t* p_payload = NULL;
    const char* p_send = NULL;
    int send_len = 0;

    if (0 < head_len && 0 < body_len)
    {
        p_payload = klb_buf_malloc(total_len, false);
        klb_buf_write(p_payload, (const char*)p_head, head_len);
        klb_buf_write(p_payload, (const char*)p_body, body_len);
        p_send = p_payload->p_buf;
        send_len = total_len;
    }
    else if (0 < head_len)
    {
        p_send = (const char*)p_head;
        send_len = head_len;
    }
    else if (0 < body_len)
    {
        p_send = (const char*)p_body;
        send_len = body_len;
    }

    klb_buf_t* p_frame = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_TEXT, NULL, p_send, send_len);
    KLB_FREE_BY(p_payload, klb_buf_unref);

    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

int klb_wsserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    klb_nlist_push_tail(p_ws->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

bool klb_wsserve_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_ws->p_write_nlist))
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_wsserve_conn_init(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    {
        p_conn->vtable.destroy = NULL;
        p_conn->vtable.recv_data = NULL;
        p_conn->vtable.ioctrl = klb_wsserve_conn_ioctrl;
        p_conn->vtable.send_normal = klb_wsserve_conn_send_normal;
        p_conn->vtable.send_media = klb_wsserve_conn_send_media;
        p_conn->vtable.on_send = klb_wsserve_conn_on_send;
        p_conn->vtable.on_recv = klb_wsserve_conn_on_recv;
        p_conn->vtable.on_msg = klb_wsserve_conn_on_msg;
    }

    p_conn->protocol = KLB_PROTOCOL_WS;

    p_ws->p_write_nlist = klb_nlist_create();
    p_ws->p_read_buf = klb_buf_malloc(1024 * 16, false);

    http_parser_init(&p_ws->parser, HTTP_REQUEST);
    http_parser_settings_init(&p_ws->settings);
    p_ws->parser.data = p_conn;

    p_ws->settings.on_message_begin = on_message_begin_klb_wsserve_conn;
    p_ws->settings.on_url = on_url_klb_wsserve_conn;
    p_ws->settings.on_status = on_status_klb_wsserve_conn;
    p_ws->settings.on_header_field = on_header_field_klb_wsserve_conn;
    p_ws->settings.on_header_value = on_header_value_klb_wsserve_conn;
    p_ws->settings.on_headers_complete = on_headers_complete_klb_wsserve_conn;
    p_ws->settings.on_body = on_body_klb_wsserve_conn;
    p_ws->settings.on_message_complete = on_message_complete_klb_wsserve_conn;

    p_ws->http_over = false;
    p_ws->parser_status = KLB_WSSERVE_PARSER;
    p_ws->req_line_crlf = false;
    p_ws->header = NULL;
    p_ws->p_body = NULL;
    p_ws->ws_status = KLB_WSSERVE_WS_HEAD;
    p_ws->ws_remaining = 0;
    p_ws->p_ws_payload = NULL;

    return 0;
}

static void klb_wsserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_ws->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_ws->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_ws->header, sdsfree);
    KLB_FREE_BY(p_ws->p_body, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_ws_payload, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_ws->p_write_nlist, klb_nlist_destroy);
}

klb_netconn_t* klb_wsserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_wsserve_conn_t));
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    klb_wsserve_conn_init(p_conn);
    p_conn->vtable.destroy = klb_wsserve_conn_destroy;

    {
        p_ws->p_netmulti = p_netmulti;
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
