// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbws/core/klb_wsclient_conn.h"
#include "klbws/core/klb_websocket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/http_parser.h"
#include "klbthird/sds.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include "klbplatform/klb_time.h"
#include <assert.h>


/// @struct klb_wsclient_conn_t
/// @brief  WS客户端连接
typedef struct klb_wsclient_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送列表
        klb_nlist_t*                p_pending_nlist;    ///< 握手完成前暂存的帧
    };

    struct
    {
        bool                        http_over;          ///< HTTP握手是否结束
        int                         parser_status;      ///< HTTP解析状态
#define KLB_WSCLIENT_PARSER_ERR                 (-1)
#define KLB_WSCLIENT_PARSER                     0
#define KLB_WSCLIENT_PARSER_OVER                2

        http_parser                 parser;             ///< HTTP解析器(握手)
        http_parser_settings        settings;           ///< 解析器设置
        bool                        status_prefix;
        bool                        status_crlf;

        klb_buf_t*                  p_read_buf;         ///< 读取缓存
        sds                         header;
        klb_buffer_t*               p_body;

        int                         ws_status;          ///< WS解析: 0.head; 1.body
#define KLB_WSCLIENT_WS_HEAD                    0
#define KLB_WSCLIENT_WS_BODY                    1
        klb_websocket_t             ws;
        uint64_t                    ws_remaining;
        klb_buffer_t*               p_ws_payload;
        klb_buffer_t*               p_msg;              ///< 分片重组
        uint8_t                     msg_opcode;         ///< 0xFF.空闲; 否则 text/binary
        bool                        ws_dead;            ///< 已结束, 不再解析

        char                        accept_expect[40];  ///< 期望的 Accept
        char                        protocol[128];      ///< 期望的子协议; 空为不校验
        bool                        upgrade_sent;       ///< 已发出握手请求
        bool                        handshake_ok;       ///< 握手已通过
        int                         read_timeout_ms;
        int                         write_timeout_ms;
        int                         idle_timeout_ms;
        int64_t                     max_payload;
    };
}klb_wsclient_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_wsclient_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数

static void push_data_klb_wsclient_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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

static void push_netcode_klb_wsclient_conn(klb_netconn_t* p_conn, int code)
{
    assert(KLB_NETCODE_OK != code);

    // 断联/超时/协议错/RBUF_FULL 等均须移出 select; 仅 WBUF_EMPTY/CONNECT 保持读写标记
    if (KLB_NETCODE_WBUF_EMPTY != code && KLB_SOCKET_CONNECT != code)
    {
        klb_socket_set_reading(p_conn->p_socket, false);
        klb_socket_set_writing(p_conn->p_socket, false);
    }

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, 0, NULL);
    }
}

static void drop_http_klb_wsclient_conn(klb_wsclient_conn_t* p_ws)
{
    KLB_FREE_BY(p_ws->header, sdsfree);
    if (NULL != p_ws->p_body)
    {
        klb_buffer_reset(p_ws->p_body);
    }
}

static void clear_pending_klb_wsclient_conn(klb_wsclient_conn_t* p_ws)
{
    if (NULL == p_ws->p_pending_nlist)
    {
        return;
    }

    while (0 < klb_nlist_size(p_ws->p_pending_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_ws->p_pending_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }
}

static void flush_pending_klb_wsclient_conn(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    bool any = false;

    while (0 < klb_nlist_size(p_ws->p_pending_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_ws->p_pending_nlist);
        klb_nlist_push_tail(p_ws->p_write_nlist, p_buf);
        any = true;
    }

    if (any)
    {
        klb_socket_set_writing(p_conn->p_socket, true);
    }
}

static void queue_ctrl_klb_wsclient_conn(klb_netconn_t* p_conn, uint8_t opcode)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    const char* p_data = NULL;
    int data_len = 0;
    klb_buf_t* p_part = NULL;
    if (NULL != p_ws->p_ws_payload && 0 < klb_buffer_datalen(p_ws->p_ws_payload))
    {
        p_part = klb_buffer_join(p_ws->p_ws_payload, NULL, NULL);
        if (NULL != p_part)
        {
            p_data = p_part->p_buf + p_part->start;
            data_len = klb_buf_data_len(p_part);
        }
    }

    uint8_t mask_key[4] = { 0 };
    klb_websocket_mask_key(mask_key);
    klb_buf_t* p_frame = klb_websocket_pack_fin(opcode, mask_key, p_data, data_len);
    KLB_FREE_BY(p_part, klb_buf_unref);

    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_conn->p_socket, true);
}

static void push_buffer_klb_wsclient_conn(klb_netconn_t* p_conn, int packtype, klb_buffer_t* p_src)
{
    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    if (NULL != p_src && 0 < klb_buffer_datalen(p_src))
    {
        p_body_part = klb_buffer_join(p_src, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_src);
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
    push_data_klb_wsclient_conn(p_conn, packtype, p_data);
}

static void on_connect_timeout_klb_wsclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
}

static void on_connected_timeout_klb_wsclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_CONNECT);
}

static void on_ticker_klb_wsclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    bool writing = false;
    bool reading = false;
    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket)) { return; }
    writing = (0 < klb_nlist_size(p_ws->p_write_nlist)) || (0 < klb_nlist_size(p_ws->p_pending_nlist));
    reading = !p_ws->handshake_ok || (NULL != p_ws->p_msg) || (0 < p_ws->ws_remaining);
    if (writing && 0 < p_ws->write_timeout_ms && (int64_t)p_ws->write_timeout_ms <= ABS_SUB(now, p_socket->last_send_tc)) { push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_TIMEOUT); return; }
    if (reading && 0 < p_ws->read_timeout_ms && (int64_t)p_ws->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc)) { push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_TIMEOUT); return; }
    if (!writing && !reading && 0 < p_ws->idle_timeout_ms && (int64_t)p_ws->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc)) { push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_TIMEOUT); return; }
}


//////////////////////////////////////////////////////////////////////////
// http_parser 回调

static int on_message_begin_klb_wsclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    KLB_FREE_BY(p_ws->header, sdsfree);
    p_ws->header = sdsnew("");
    p_ws->status_prefix = false;
    p_ws->status_crlf = false;

    return 0;
}

static int on_url_klb_wsclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    return 0;
}

static int on_status_klb_wsclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (NULL == p_ws->header)
    {
        p_ws->header = sdsnew("");
    }

    if (!p_ws->status_prefix)
    {
        p_ws->header = sdscatfmt(p_ws->header, "HTTP/%u.%u %u ",
            p_parser->http_major, p_parser->http_minor, p_parser->status_code);
        p_ws->status_prefix = true;
    }

    if (0 < length)
    {
        p_ws->header = sdscatlen(p_ws->header, at, length);
    }

    return 0;
}

static int on_header_field_klb_wsclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (NULL == p_ws->header)
    {
        p_ws->header = sdsnew("");
    }

    if (!p_ws->status_crlf)
    {
        p_ws->header = sdscat(p_ws->header, "\r\n");
        p_ws->status_crlf = true;
    }

    p_ws->header = sdscatlen(p_ws->header, at, length);
    p_ws->header = sdscat(p_ws->header, ": ");

    return 0;
}

static int on_header_value_klb_wsclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    p_ws->header = sdscatlen(p_ws->header, at, length);
    p_ws->header = sdscat(p_ws->header, "\r\n");

    return 0;
}

static int on_headers_complete_klb_wsclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (NULL == p_ws->header)
    {
        p_ws->header = sdsnew("");
    }

    if (!p_ws->status_crlf)
    {
        p_ws->header = sdscat(p_ws->header, "\r\n");
        p_ws->status_crlf = true;
    }

    p_ws->header = sdscat(p_ws->header, "\r\n");

    if (101 == (int)p_parser->status_code || 0 != p_parser->upgrade)
    {
        return 1;
    }

    return 0;
}

static int on_body_klb_wsclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

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

static int on_message_complete_klb_wsclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    p_ws->parser_status = KLB_WSCLIENT_PARSER_OVER;

    return 0;
}

static void compact_read_klb_wsclient_conn(klb_wsclient_conn_t* p_ws)
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

static void reset_frame_klb_wsclient_conn(klb_wsclient_conn_t* p_ws)
{
    if (NULL != p_ws->p_ws_payload)
    {
        klb_buffer_reset(p_ws->p_ws_payload);
    }

    p_ws->ws_status = KLB_WSCLIENT_WS_HEAD;
    p_ws->ws_remaining = 0;
}

static bool buffer_utf8_ok_klb_wsclient_conn(klb_buffer_t* p_buffer)
{
    if (NULL == p_buffer || 0 >= klb_buffer_datalen(p_buffer))
    {
        return true;
    }

    klb_buf_t* p_part = klb_buffer_join(p_buffer, NULL, NULL);
    if (NULL == p_part)
    {
        return false;
    }

    bool ok = klb_websocket_utf8_ok((const uint8_t*)(p_part->p_buf + p_part->start), klb_buf_data_len(p_part));
    KLB_FREE_BY(p_part, klb_buf_unref);
    return ok;
}

static int append_msg_klb_wsclient_conn(klb_wsclient_conn_t* p_ws)
{
    int add = 0;
    if (NULL != p_ws->p_ws_payload)
    {
        add = klb_buffer_datalen(p_ws->p_ws_payload);
    }

    int have = 0;
    if (NULL != p_ws->p_msg)
    {
        have = klb_buffer_datalen(p_ws->p_msg);
    }

    if ((int64_t)have + (int64_t)add > (int64_t)p_ws->max_payload)
    {
        return -1;
    }

    if (0 < add)
    {
        if (NULL == p_ws->p_msg)
        {
            p_ws->p_msg = klb_buffer_create(4096);
        }

        klb_buf_t* p_part = klb_buffer_join(p_ws->p_ws_payload, NULL, NULL);
        if (NULL != p_part)
        {
            klb_buffer_write(p_ws->p_msg, p_part->p_buf + p_part->start, klb_buf_data_len(p_part));
            KLB_FREE_BY(p_part, klb_buf_unref);
        }

        klb_buffer_reset(p_ws->p_ws_payload);
    }

    return 0;
}

static void queue_raw_klb_wsclient_conn(klb_netconn_t* p_conn, uint8_t opcode, const char* p_data, int data_len)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    uint8_t mask_key[4] = { 0 };
    klb_websocket_mask_key(mask_key);
    klb_buf_t* p_frame = klb_websocket_pack_fin(opcode, mask_key, p_data, data_len);
    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_conn->p_socket, true);
}

static void push_close_klb_wsclient_conn(klb_netconn_t* p_conn, int code, const char* p_reason, int reason_len)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (0 > reason_len)
    {
        reason_len = 0;
    }

    klb_buf_t* p_data = klb_buf_malloc(2 + reason_len, false);
    p_data->p_buf[0] = (char)((code >> 8) & 0xFF);
    p_data->p_buf[1] = (char)(code & 0xFF);
    if (0 < reason_len && NULL != p_reason)
    {
        memcpy(p_data->p_buf + 2, p_reason, (size_t)reason_len);
    }

    p_data->start = 0;
    p_data->end = 2 + reason_len;

    push_data_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_PACK_CLOSE, p_data);
    klb_socket_set_reading(p_conn->p_socket, false);
    p_ws->ws_dead = true;
}

static void fail_close_klb_wsclient_conn(klb_netconn_t* p_conn, int code)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    if (p_ws->ws_dead)
    {
        return;
    }

    char payload[2];
    payload[0] = (char)((code >> 8) & 0xFF);
    payload[1] = (char)(code & 0xFF);
    queue_raw_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_OPCODE_CLOSE, payload, 2);
    push_close_klb_wsclient_conn(p_conn, code, NULL, 0);
}

static void finish_ws_frame_klb_wsclient_conn(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    uint8_t opcode = p_ws->ws.opcode;
    bool fin = (0 != p_ws->ws.fin);
    bool in_msg = (0xFF != p_ws->msg_opcode);

    if (p_ws->ws_dead)
    {
        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_PING == opcode)
    {
        // last_recv_tc 由 klbiopoll 更新
        queue_ctrl_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_OPCODE_PONG);
        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_PONG == opcode)
    {
        // last_recv_tc 由 klbiopoll 更新
        klb_buf_t* p_part = NULL;
        if (NULL != p_ws->p_ws_payload && 0 < klb_buffer_datalen(p_ws->p_ws_payload))
        {
            p_part = klb_buffer_join(p_ws->p_ws_payload, NULL, NULL);
            klb_buffer_reset(p_ws->p_ws_payload);
        }

        push_data_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_PACK_PONG, p_part);
        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_CLOSE == opcode)
    {
        int code = KLB_WEBSOCKET_CLOSE_NOSTATUS;
        const char* p_reason = NULL;
        int reason_len = 0;
        klb_buf_t* p_part = NULL;
        if (NULL != p_ws->p_ws_payload && 0 < klb_buffer_datalen(p_ws->p_ws_payload))
        {
            p_part = klb_buffer_join(p_ws->p_ws_payload, NULL, NULL);
        }

        int n = (NULL != p_part) ? klb_buf_data_len(p_part) : 0;
        if (0 < n && !klb_websocket_close_ok((const uint8_t*)(p_part->p_buf + p_part->start), n, &code))
        {
            KLB_FREE_BY(p_part, klb_buf_unref);
            reset_frame_klb_wsclient_conn(p_ws);
            fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (2 < n)
        {
            p_reason = p_part->p_buf + p_part->start + 2;
            reason_len = n - 2;
        }

        queue_ctrl_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_OPCODE_CLOSE);
        push_close_klb_wsclient_conn(p_conn, code, p_reason, reason_len);
        KLB_FREE_BY(p_part, klb_buf_unref);
        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_TEXT == opcode || KLB_WEBSOCKET_OPCODE_BINARY == opcode)
    {
        if (in_msg)
        {
            reset_frame_klb_wsclient_conn(p_ws);
            fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (!fin)
        {
            if (0 != append_msg_klb_wsclient_conn(p_ws))
            {
                reset_frame_klb_wsclient_conn(p_ws);
                fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_TOOBIG);
                return;
            }

            p_ws->msg_opcode = opcode;
            reset_frame_klb_wsclient_conn(p_ws);
            return;
        }

        if (KLB_WEBSOCKET_OPCODE_TEXT == opcode && !buffer_utf8_ok_klb_wsclient_conn(p_ws->p_ws_payload))
        {
            reset_frame_klb_wsclient_conn(p_ws);
            fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_UTF8);
            return;
        }

        int packtype = (KLB_WEBSOCKET_OPCODE_BINARY == opcode) ? KLB_MNP_BINARY : KLB_MNP_TEXT;
        push_buffer_klb_wsclient_conn(p_conn, packtype, p_ws->p_ws_payload);
        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_CONTINUATION == opcode)
    {
        if (!in_msg)
        {
            reset_frame_klb_wsclient_conn(p_ws);
            fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (0 != append_msg_klb_wsclient_conn(p_ws))
        {
            reset_frame_klb_wsclient_conn(p_ws);
            fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_TOOBIG);
            return;
        }

        if (fin)
        {
            if (KLB_WEBSOCKET_OPCODE_TEXT == p_ws->msg_opcode &&
                !buffer_utf8_ok_klb_wsclient_conn(p_ws->p_msg))
            {
                reset_frame_klb_wsclient_conn(p_ws);
                fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_UTF8);
                return;
            }

            int packtype = (KLB_WEBSOCKET_OPCODE_BINARY == p_ws->msg_opcode) ? KLB_MNP_BINARY : KLB_MNP_TEXT;
            push_buffer_klb_wsclient_conn(p_conn, packtype, p_ws->p_msg);
            p_ws->msg_opcode = 0xFF;
        }

        reset_frame_klb_wsclient_conn(p_ws);
        return;
    }

    reset_frame_klb_wsclient_conn(p_ws);
    fail_close_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
}

static int parser_ws_klb_wsclient_conn(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        if (p_ws->ws_dead)
        {
            break;
        }

        uint8_t* p_data = (uint8_t*)(p_buf->p_buf + p_buf->start);
        int data_len = p_buf->end - p_buf->start;

        if (KLB_WSCLIENT_WS_HEAD == p_ws->ws_status)
        {
            klb_websocket_t ws = { 0 };
            int ret = klb_websocket_parse(&ws, p_data, data_len);
            if (0 != ret)
            {
                break;
            }

            if (!klb_websocket_frame_ok(&ws, false))
            {
                p_ws->parser_status = KLB_WSCLIENT_PARSER_ERR;
                return -1;
            }

            p_buf->start += ws.head_len;
            p_ws->ws = ws;
            p_ws->ws_remaining = ws.payload_len;
            p_ws->ws_status = KLB_WSCLIENT_WS_BODY;

            if (0 == p_ws->ws_remaining)
            {
                finish_ws_frame_klb_wsclient_conn(p_conn);
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
                finish_ws_frame_klb_wsclient_conn(p_conn);
            }
        }
    }

    compact_read_klb_wsclient_conn(p_ws);
    return 0;
}

static int parser_http_klb_wsclient_conn(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        size_t have = (size_t)(p_buf->end - p_buf->start);
        size_t nparsed = http_parser_execute(&p_ws->parser, &p_ws->settings,
            p_buf->p_buf + p_buf->start, have);

        if (0 != p_ws->parser.http_errno)
        {
            p_ws->parser_status = KLB_WSCLIENT_PARSER_ERR;
            compact_read_klb_wsclient_conn(p_ws);
            return -1;
        }

        if (0 < nparsed)
        {
            p_buf->start += (int)nparsed;
        }

        if (KLB_WSCLIENT_PARSER_OVER == p_ws->parser_status || 0 != p_ws->parser.upgrade)
        {
            bool ok = false;
            if (101 == (int)p_ws->parser.status_code && '\0' != p_ws->accept_expect[0])
            {
                char got[40] = { 0 };
                if (klb_websocket_header_get(p_ws->header, "Sec-WebSocket-Accept", got, (int)sizeof(got)))
                {
                    if (0 == strcmp(got, p_ws->accept_expect))
                    {
                        bool proto_ok = true;
                        if ('\0' != p_ws->protocol[0])
                        {
                            char proto[128] = { 0 };
                            if (klb_websocket_header_get(p_ws->header, "Sec-WebSocket-Protocol", proto, (int)sizeof(proto)) &&
                                0 != strcmp(proto, p_ws->protocol))
                            {
                                proto_ok = false;
                            }
                        }

                        if (proto_ok)
                        {
                            ok = true;
                        }
                    }
                }
            }

            if (!ok)
            {
                clear_pending_klb_wsclient_conn(p_ws);
                drop_http_klb_wsclient_conn(p_ws);
                p_ws->parser_status = KLB_WSCLIENT_PARSER_ERR;
                compact_read_klb_wsclient_conn(p_ws);
                return -1;
            }

            p_ws->handshake_ok = true;
            p_ws->http_over = true;
            p_ws->ws_status = KLB_WSCLIENT_WS_HEAD;
            drop_http_klb_wsclient_conn(p_ws);
            flush_pending_klb_wsclient_conn(p_conn);
            push_data_klb_wsclient_conn(p_conn, KLB_WEBSOCKET_PACK_OPEN, NULL);
            compact_read_klb_wsclient_conn(p_ws);
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

    compact_read_klb_wsclient_conn(p_ws);
    return 0;
}

static int parser_klb_wsclient_conn(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (!p_ws->http_over)
    {
        int ret = parser_http_klb_wsclient_conn(p_conn);
        if (0 != ret)
        {
            return ret;
        }
    }

    if (p_ws->http_over)
    {
        return parser_ws_klb_wsclient_conn(p_conn);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klb_wsclient_conn_destroy(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    assert(NULL == p_ws->p_netmulti);

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    klb_wsclient_conn_quit(p_conn);
    KLB_FREE(p_conn);
}

static int klb_wsclient_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

static int klb_wsclient_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_wsclient_conn_send(p_conn, p_head, head_len, p_body, body_len);
}

static int klb_wsclient_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

static int klb_wsclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
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
        push_netcode_klb_wsclient_conn(p_conn, close_status);
        return write_num;
    }

    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
        push_netcode_klb_wsclient_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

static int do_recv_klb_wsclient_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    if (KLB_WSCLIENT_PARSER_ERR == p_ws->parser_status)
    {
        return -1;
    }

    int idle_len = klb_buf_idle_len(p_buf);
    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_wsclient_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_wsclient_conn(p_conn, KLB_NETCODE_RBUF_FULL);
            return -1;
        }
    }

    int recv_len = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), idle_len);
    int judge = 0;
    int close_status = KLB_SOCKET_DISCONNECT;

    if (0 < recv_len)
    {
        p_buf->end += recv_len;
        *p_read_num += recv_len;
    }
    else
    {
        judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
        if (KLB_SOCKET_RECV_AGAIN == judge)
        {
            return 1;
        }
        /* EOF/ERROR: parse residual then close after parser below */
    }

    int ret_parse = parser_klb_wsclient_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_wsclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 >= recv_len && KLB_SOCKET_RECV_AGAIN != judge)
    {
        klb_socket_recv_closed(p_socket, close_status);
        push_netcode_klb_wsclient_conn(p_conn, close_status);
        return -1;
    }

    return 0;
}

static int klb_wsclient_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_wsclient_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

static int klb_wsclient_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_wsclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_wsclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_wsclient_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_wsclient_conn_free(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_ws->p_netmulti, p_conn);
    p_ws->p_netmulti = NULL;
}

int klb_wsclient_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
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

    if (!p_ws->handshake_ok && !p_ws->upgrade_sent)
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

        p_ws->upgrade_sent = true;
        klb_nlist_push_tail(p_ws->p_write_nlist, p_data);
        klb_socket_set_writing(p_socket, true);
        return 0;
    }

    if (!p_ws->handshake_ok && KLB_WSCLIENT_PARSER_ERR == p_ws->parser_status)
    {
        return 1;
    }

    uint8_t mask_key[4] = { 0 };
    klb_websocket_mask_key(mask_key);
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

    klb_buf_t* p_frame = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_TEXT, mask_key, p_send, send_len);
    KLB_FREE_BY(p_payload, klb_buf_unref);

    if (!p_ws->handshake_ok)
    {
        klb_nlist_push_tail(p_ws->p_pending_nlist, p_frame);
        return 0;
    }

    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

int klb_wsclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (!p_ws->handshake_ok)
    {
        if (KLB_WSCLIENT_PARSER_ERR == p_ws->parser_status || !p_ws->upgrade_sent)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            return 1;
        }

        klb_nlist_push_tail(p_ws->p_pending_nlist, p_data);
        return 0;
    }

    klb_nlist_push_tail(p_ws->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

bool klb_wsclient_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_ws->p_write_nlist))
    {
        return false;
    }

    return true;
}

int64_t klb_wsclient_wbuf_size(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    int64_t total = 0;

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_ws->p_write_nlist);
    while (NULL != p_iter)
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_data(p_iter);
        if (NULL != p_buf)
        {
            total += p_buf->end - p_buf->start;
        }

        p_iter = klb_nlist_next(p_iter);
    }

    return total;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_wsclient_conn_init(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    {
        p_conn->vtable.destroy = NULL;
        p_conn->vtable.recv_data = NULL;
        p_conn->vtable.ioctrl = klb_wsclient_conn_ioctrl;
        p_conn->vtable.send_normal = klb_wsclient_conn_send_normal;
        p_conn->vtable.send_media = klb_wsclient_conn_send_media;
        p_conn->vtable.on_send = klb_wsclient_conn_on_send;
        p_conn->vtable.on_recv = klb_wsclient_conn_on_recv;
        p_conn->vtable.on_msg = klb_wsclient_conn_on_msg;
    }

    p_conn->protocol = KLB_PROTOCOL_WS;

    p_ws->p_write_nlist = klb_nlist_create();
    p_ws->p_pending_nlist = klb_nlist_create();
    p_ws->p_read_buf = klb_buf_malloc(1024 * 16, false);

    http_parser_init(&p_ws->parser, HTTP_RESPONSE);
    http_parser_settings_init(&p_ws->settings);
    p_ws->parser.data = p_conn;

    p_ws->settings.on_message_begin = on_message_begin_klb_wsclient_conn;
    p_ws->settings.on_url = on_url_klb_wsclient_conn;
    p_ws->settings.on_status = on_status_klb_wsclient_conn;
    p_ws->settings.on_header_field = on_header_field_klb_wsclient_conn;
    p_ws->settings.on_header_value = on_header_value_klb_wsclient_conn;
    p_ws->settings.on_headers_complete = on_headers_complete_klb_wsclient_conn;
    p_ws->settings.on_body = on_body_klb_wsclient_conn;
    p_ws->settings.on_message_complete = on_message_complete_klb_wsclient_conn;

    p_ws->http_over = false;
    p_ws->parser_status = KLB_WSCLIENT_PARSER;
    p_ws->status_prefix = false;
    p_ws->status_crlf = false;
    p_ws->header = NULL;
    p_ws->p_body = NULL;
    p_ws->ws_status = KLB_WSCLIENT_WS_HEAD;
    p_ws->ws_remaining = 0;
    p_ws->p_ws_payload = NULL;
    p_ws->p_msg = NULL;
    p_ws->msg_opcode = 0xFF;
    p_ws->ws_dead = false;
    p_ws->protocol[0] = '\0';

    return 0;
}

static void klb_wsclient_conn_quit(klb_netconn_t* p_conn)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_ws->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_ws->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    clear_pending_klb_wsclient_conn(p_ws);

    KLB_FREE_BY(p_ws->header, sdsfree);
    KLB_FREE_BY(p_ws->p_body, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_ws_payload, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_msg, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_ws->p_write_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_ws->p_pending_nlist, klb_nlist_destroy);
}

klb_netconn_t* klb_wsclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code)
{
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;
    klb_netconn_t* p_netconn = NULL;
    int code = 0;

    if (NULL == p_netmulti || NULL == p_host || 0 == p_host[0])
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        code = klb_netcode_last_syserr();
        if (KLB_NETCODE_IO == code || KLB_NETCODE_ERR == code)
        {
            code = KLB_NETCODE_CONNECT_FAIL;
        }

        if (NULL != p_code)
        {
            *p_code = code;
        }

        return NULL;
    }

    // 创建 klb_socket_t*; tls 走 mbedtls 封装, 失败不关 fd
    if (tls)
    {
        p_socket = klb_socket_async_create_tls(fd);
        if (NULL == p_socket)
        {
            KLB_SOCKET_CLOSE(fd);
            if (NULL != p_code)
            {
                *p_code = KLB_NETCODE_TLS_HANDSHAKE;
            }

            return NULL;
        }
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    p_netconn = klb_wsclient_conn_create(p_netmulti, p_socket);
    klb_netmulti_push(p_netmulti, p_netconn);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_netconn;
}

klb_netconn_t* klb_wsclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_wsclient_conn_t));
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;

    klb_wsclient_conn_init(p_conn);
    p_conn->vtable.destroy = klb_wsclient_conn_destroy;

    {
        p_ws->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        klb_socket_set_reading(p_socket, true);
    }

    return p_conn;
}

void klb_wsclient_conn_set_key(klb_netconn_t* p_conn, const char* p_key)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    p_ws->accept_expect[0] = '\0';

    if (NULL == p_key || '\0' == p_key[0])
    {
        return;
    }

    klb_websocket_accept(p_key, p_ws->accept_expect, (int)sizeof(p_ws->accept_expect));
}

void klb_wsclient_conn_set_protocol(klb_netconn_t* p_conn, const char* p_protocol)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    p_ws->protocol[0] = '\0';

    if (NULL == p_protocol || '\0' == p_protocol[0])
    {
        return;
    }

    int n = (int)strlen(p_protocol);
    if (0 >= n || n >= (int)sizeof(p_ws->protocol))
    {
        return;
    }

    memcpy(p_ws->protocol, p_protocol, (size_t)n + 1);
}


void klb_wsclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    p_ws->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_ws->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_ws->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}

void klb_wsclient_conn_set_max_payload(klb_netconn_t* p_conn, int64_t max_payload)
{
    klb_wsclient_conn_t* p_ws = (klb_wsclient_conn_t*)p_conn->extra;
    p_ws->max_payload = (0 < max_payload) ? max_payload : KLB_WEBSOCKET_MAX_PAYLOAD;
}

// end