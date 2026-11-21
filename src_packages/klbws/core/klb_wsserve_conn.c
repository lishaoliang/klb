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
#include "klbplatform/klb_time.h"
#include <assert.h>


/// @struct klb_wsserve_conn_t
/// @brief  WS服务连接
typedef struct klb_wsserve_conn_t_
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
        klb_buffer_t*               p_msg;              ///< 分片重组
        uint8_t                     msg_opcode;         ///< 0xFF.空闲; 否则 text/binary
        bool                        ws_dead;            ///< 已结束, 不再解析

        bool                        allow_raw;          ///< 允许发送裸 HTTP(101)
        bool                        handshake_ok;       ///< 握手已通过
        char                        expect_path[256];   ///< 路径过滤; 空为任意
        char                        expect_protocol[128]; ///< 选定的子协议; 空为不协商

        int                         ping_interval_ms;   ///< 服务端 Ping 间隔; 0=关
        int                         ping_timeout_ms;    ///< 等 Pong 超时
        int64_t                     last_ping_tc;       ///< 上次 Ping 时刻; 0=未武装
        int64_t                     ping_deadline_tc;   ///< 等待 Pong 截止
        bool                        ping_waiting;       ///< 已发 Ping 待 Pong
    };
}klb_wsserve_conn_t;

#define KLB_WSSERVE_PING_INTERVAL_MS_DEFAULT    15000
#define KLB_WSSERVE_PING_TIMEOUT_MS_DEFAULT     30000


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_wsserve_conn_quit(klb_netconn_t* p_conn);
static void queue_raw_klb_wsserve_conn(klb_netconn_t* p_conn, uint8_t opcode, const char* p_data, int data_len);
static void fail_close_klb_wsserve_conn(klb_netconn_t* p_conn, int code);


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

static void drop_http_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
{
    KLB_FREE_BY(p_ws->header, sdsfree);
    if (NULL != p_ws->p_body)
    {
        klb_buffer_reset(p_ws->p_body);
    }
}

static void clear_pending_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
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

static void flush_pending_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
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

static void queue_ctrl_klb_wsserve_conn(klb_netconn_t* p_conn, uint8_t opcode)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

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

    klb_buf_t* p_frame = klb_websocket_pack_fin(opcode, NULL, p_data, data_len);
    KLB_FREE_BY(p_part, klb_buf_unref);

    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_conn->p_socket, true);
}

static void push_buffer_klb_wsserve_conn(klb_netconn_t* p_conn, int packtype, klb_buffer_t* p_src)
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
    push_data_klb_wsserve_conn(p_conn, packtype, p_data);
}

static void on_connect_timeout_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

static void on_connected_timeout_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{

}

static void normalize_ping_klb_wsserve_conn(klb_wsserve_conn_t* p_ws, int interval_ms, int timeout_ms)
{
    if (0 > interval_ms)
    {
        interval_ms = KLB_WSSERVE_PING_INTERVAL_MS_DEFAULT;
    }

    if (0 == interval_ms)
    {
        p_ws->ping_interval_ms = 0;
        p_ws->ping_timeout_ms = 0;
        p_ws->ping_waiting = false;
        p_ws->ping_deadline_tc = 0;
        return;
    }

    if (0 >= timeout_ms)
    {
        timeout_ms = KLB_WSSERVE_PING_TIMEOUT_MS_DEFAULT;
    }


    p_ws->ping_interval_ms = interval_ms;
    p_ws->ping_timeout_ms = timeout_ms;
}

static void on_ticker_klb_wsserve_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (!p_ws->handshake_ok || p_ws->ws_dead)
    {
        return;
    }

    if (0 >= p_ws->ping_interval_ms)
    {
        return;
    }

    // step1. 握手后首 tick 武装计时
    if (0 == p_ws->last_ping_tc)
    {
        p_ws->last_ping_tc = now;
        return;
    }

    // step2. 等待 Pong 超时则拆连
    if (p_ws->ping_waiting)
    {
        if (now >= p_ws->ping_deadline_tc)
        {
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
        }

        return;
    }

    // step3. 到间隔则发 Ping
    if ((int64_t)p_ws->ping_interval_ms <= ABS_SUB(now, p_ws->last_ping_tc))
    {
        queue_raw_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_OPCODE_PING, NULL, 0);
        p_ws->last_ping_tc = now;
        p_ws->ping_waiting = true;
        p_ws->ping_deadline_tc = now + (int64_t)p_ws->ping_timeout_ms;
    }
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

    if (0 != p_parser->upgrade)
    {
        return 1;
    }

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

static void reset_frame_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
{
    if (NULL != p_ws->p_ws_payload)
    {
        klb_buffer_reset(p_ws->p_ws_payload);
    }

    p_ws->ws_status = KLB_WSSERVE_WS_HEAD;
    p_ws->ws_remaining = 0;
}

static bool buffer_utf8_ok_klb_wsserve_conn(klb_buffer_t* p_buffer)
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

static int append_msg_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
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

    if ((int64_t)have + (int64_t)add > (int64_t)KLB_WEBSOCKET_MAX_PAYLOAD)
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

static void queue_raw_klb_wsserve_conn(klb_netconn_t* p_conn, uint8_t opcode, const char* p_data, int data_len)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_buf_t* p_frame = klb_websocket_pack_fin(opcode, NULL, p_data, data_len);
    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_conn->p_socket, true);
}

static void push_close_klb_wsserve_conn(klb_netconn_t* p_conn, int code, const char* p_reason, int reason_len)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

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

    push_data_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_PACK_CLOSE, p_data);
    klb_socket_set_reading(p_conn->p_socket, false);
    p_ws->ws_dead = true;
}

static void fail_close_klb_wsserve_conn(klb_netconn_t* p_conn, int code)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    if (p_ws->ws_dead)
    {
        return;
    }

    char payload[2];
    payload[0] = (char)((code >> 8) & 0xFF);
    payload[1] = (char)(code & 0xFF);
    queue_raw_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_OPCODE_CLOSE, payload, 2);
    push_close_klb_wsserve_conn(p_conn, code, NULL, 0);
}

static void finish_ws_frame_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    uint8_t opcode = p_ws->ws.opcode;
    bool fin = (0 != p_ws->ws.fin);
    bool in_msg = (0xFF != p_ws->msg_opcode);

    if (p_ws->ws_dead)
    {
        reset_frame_klb_wsserve_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_PING == opcode)
    {
        // last_recv_tc 由 klbiopoll 更新
        queue_ctrl_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_OPCODE_PONG);
        reset_frame_klb_wsserve_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_PONG == opcode)
    {
        // last_recv_tc 由 klbiopoll 更新; C 层吞掉 Pong
        if (NULL != p_ws->p_ws_payload)
        {
            klb_buffer_reset(p_ws->p_ws_payload);
        }

        p_ws->ping_waiting = false;
        p_ws->ping_deadline_tc = 0;
        reset_frame_klb_wsserve_conn(p_ws);
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
            reset_frame_klb_wsserve_conn(p_ws);
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (2 < n)
        {
            p_reason = p_part->p_buf + p_part->start + 2;
            reason_len = n - 2;
        }

        queue_ctrl_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_OPCODE_CLOSE);
        push_close_klb_wsserve_conn(p_conn, code, p_reason, reason_len);
        KLB_FREE_BY(p_part, klb_buf_unref);
        reset_frame_klb_wsserve_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_TEXT == opcode || KLB_WEBSOCKET_OPCODE_BINARY == opcode)
    {
        if (in_msg)
        {
            reset_frame_klb_wsserve_conn(p_ws);
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (!fin)
        {
            if (0 != append_msg_klb_wsserve_conn(p_ws))
            {
                reset_frame_klb_wsserve_conn(p_ws);
                fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_TOOBIG);
                return;
            }

            p_ws->msg_opcode = opcode;
            reset_frame_klb_wsserve_conn(p_ws);
            return;
        }

        if (KLB_WEBSOCKET_OPCODE_TEXT == opcode && !buffer_utf8_ok_klb_wsserve_conn(p_ws->p_ws_payload))
        {
            reset_frame_klb_wsserve_conn(p_ws);
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_UTF8);
            return;
        }

        int packtype = (KLB_WEBSOCKET_OPCODE_BINARY == opcode) ? KLB_MNP_BINARY : KLB_MNP_TEXT;
        push_buffer_klb_wsserve_conn(p_conn, packtype, p_ws->p_ws_payload);
        reset_frame_klb_wsserve_conn(p_ws);
        return;
    }

    if (KLB_WEBSOCKET_OPCODE_CONTINUATION == opcode)
    {
        if (!in_msg)
        {
            reset_frame_klb_wsserve_conn(p_ws);
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
            return;
        }

        if (0 != append_msg_klb_wsserve_conn(p_ws))
        {
            reset_frame_klb_wsserve_conn(p_ws);
            fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_TOOBIG);
            return;
        }

        if (fin)
        {
            if (KLB_WEBSOCKET_OPCODE_TEXT == p_ws->msg_opcode &&
                !buffer_utf8_ok_klb_wsserve_conn(p_ws->p_msg))
            {
                reset_frame_klb_wsserve_conn(p_ws);
                fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_UTF8);
                return;
            }

            int packtype = (KLB_WEBSOCKET_OPCODE_BINARY == p_ws->msg_opcode) ? KLB_MNP_BINARY : KLB_MNP_TEXT;
            push_buffer_klb_wsserve_conn(p_conn, packtype, p_ws->p_msg);
            p_ws->msg_opcode = 0xFF;
        }

        reset_frame_klb_wsserve_conn(p_ws);
        return;
    }

    reset_frame_klb_wsserve_conn(p_ws);
    fail_close_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_CLOSE_PROTO);
}

static int parser_ws_klb_wsserve_conn(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_ws->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        if (p_ws->ws_dead)
        {
            break;
        }

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

            if (!klb_websocket_frame_ok(&ws, true))
            {
                p_ws->parser_status = KLB_WSSERVE_PARSER_ERR;
                return -1;
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

static bool path_ok_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
{
    if ('\0' == p_ws->expect_path[0])
    {
        return true;
    }

    if (NULL == p_ws->header || 0 != strncmp(p_ws->header, "GET ", 4))
    {
        return false;
    }

    const char* p = p_ws->header + 4;
    const char* start = p;
    while ('\0' != *p && ' ' != *p && '?' != *p)
    {
        p += 1;
    }

    int n = (int)(p - start);
    int expect_n = (int)strlen(p_ws->expect_path);
    if (n != expect_n)
    {
        return false;
    }

    return (0 == memcmp(start, p_ws->expect_path, (size_t)n));
}

static bool protocol_ok_klb_wsserve_conn(klb_wsserve_conn_t* p_ws)
{
    if ('\0' == p_ws->expect_protocol[0])
    {
        return true;
    }

    char offered[256] = { 0 };
    if (!klb_websocket_header_get(p_ws->header, "Sec-WebSocket-Protocol", offered, (int)sizeof(offered)))
    {
        return false;
    }

    return klb_websocket_protocol_has(offered, p_ws->expect_protocol);
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
            bool ok = false;
            if (NULL != p_ws->header && 0 == strncmp(p_ws->header, "GET ", 4) &&
                klb_websocket_header_has(p_ws->header, "Upgrade", "websocket") &&
                klb_websocket_header_has(p_ws->header, "Connection", "Upgrade"))
            {
                char version[16] = { 0 };
                char key[128] = { 0 };
                char accept[40] = { 0 };
                if (klb_websocket_header_get(p_ws->header, "Sec-WebSocket-Version", version, (int)sizeof(version)) &&
                    0 == strcmp(version, "13") &&
                    klb_websocket_header_get(p_ws->header, "Sec-WebSocket-Key", key, (int)sizeof(key)) &&
                    0 == klb_websocket_accept(key, accept, (int)sizeof(accept)) &&
                    path_ok_klb_wsserve_conn(p_ws) &&
                    protocol_ok_klb_wsserve_conn(p_ws))
                {
                    sds str = sdsnew("");
                    str = sdscat(str, "HTTP/1.1 101 Switching Protocols\r\n");
                    str = sdscat(str, "Upgrade: websocket\r\n");
                    str = sdscat(str, "Connection: Upgrade\r\n");
                    str = sdscatfmt(str, "Sec-WebSocket-Accept: %s\r\n", accept);
                    if ('\0' != p_ws->expect_protocol[0])
                    {
                        str = sdscatfmt(str, "Sec-WebSocket-Protocol: %s\r\n", p_ws->expect_protocol);
                    }
                    str = sdscat(str, "\r\n");

                    p_ws->allow_raw = true;
                    int ret = klb_wsserve_conn_send(p_conn, (const uint8_t*)str, (int)sdslen(str), NULL, 0);
                    p_ws->allow_raw = false;
                    KLB_FREE_BY(str, sdsfree);
                    if (0 == ret)
                    {
                        ok = true;
                    }
                }
            }

            if (!ok)
            {
                clear_pending_klb_wsserve_conn(p_ws);
                drop_http_klb_wsserve_conn(p_ws);
                p_ws->parser_status = KLB_WSSERVE_PARSER_ERR;
                compact_read_klb_wsserve_conn(p_ws);
                return -1;
            }

            p_ws->handshake_ok = true;
            p_ws->http_over = true;
            p_ws->ws_status = KLB_WSSERVE_WS_HEAD;
            drop_http_klb_wsserve_conn(p_ws);
            flush_pending_klb_wsserve_conn(p_conn);
            push_data_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_PACK_OPEN, NULL);
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
        push_netcode_klb_wsserve_conn(p_conn, close_status);
        return write_num;
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

    int ret_parse = parser_klb_wsserve_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_wsserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 >= recv_len && KLB_SOCKET_RECV_AGAIN != judge)
    {
        klb_socket_recv_closed(p_socket, close_status);
        push_netcode_klb_wsserve_conn(p_conn, close_status);
        return -1;
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

    if (!p_ws->handshake_ok && p_ws->allow_raw)
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

    if (!p_ws->handshake_ok && KLB_WSSERVE_PARSER_ERR == p_ws->parser_status)
    {
        return 1;
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

    if (!p_ws->handshake_ok)
    {
        klb_nlist_push_tail(p_ws->p_pending_nlist, p_frame);
        return 0;
    }

    klb_nlist_push_tail(p_ws->p_write_nlist, p_frame);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

int klb_wsserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (!p_ws->handshake_ok)
    {
        if (KLB_WSSERVE_PARSER_ERR == p_ws->parser_status)
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

bool klb_wsserve_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_ws->p_write_nlist))
    {
        return false;
    }

    return true;
}

int64_t klb_wsserve_wbuf_size(klb_netconn_t* p_conn)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
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
    p_ws->p_pending_nlist = klb_nlist_create();
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
    p_ws->p_msg = NULL;
    p_ws->msg_opcode = 0xFF;
    p_ws->ws_dead = false;
    p_ws->expect_path[0] = '\0';
    p_ws->expect_protocol[0] = '\0';
    normalize_ping_klb_wsserve_conn(p_ws, KLB_WSSERVE_PING_INTERVAL_MS_DEFAULT, KLB_WSSERVE_PING_TIMEOUT_MS_DEFAULT);
    p_ws->last_ping_tc = 0;
    p_ws->ping_deadline_tc = 0;
    p_ws->ping_waiting = false;

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

    clear_pending_klb_wsserve_conn(p_ws);

    KLB_FREE_BY(p_ws->header, sdsfree);
    KLB_FREE_BY(p_ws->p_body, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_ws_payload, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_msg, klb_buffer_destroy);
    KLB_FREE_BY(p_ws->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_ws->p_write_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_ws->p_pending_nlist, klb_nlist_destroy);
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

static bool copy_filter_klb_wsserve_conn(char* p_dst, int cap, const char* p_src)
{
    p_dst[0] = '\0';
    if (NULL == p_src || '\0' == p_src[0])
    {
        return true;
    }

    int n = (int)strlen(p_src);
    if (0 >= n || n >= cap)
    {
        return false;
    }

    memcpy(p_dst, p_src, (size_t)n + 1);
    return true;
}

void klb_wsserve_conn_set_filter(klb_netconn_t* p_conn, const char* p_path, const char* p_protocol)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    if (!copy_filter_klb_wsserve_conn(p_ws->expect_path, (int)sizeof(p_ws->expect_path), p_path))
    {
        p_ws->expect_path[0] = '\0';
    }

    if (!copy_filter_klb_wsserve_conn(p_ws->expect_protocol, (int)sizeof(p_ws->expect_protocol), p_protocol))
    {
        p_ws->expect_protocol[0] = '\0';
    }
}

void klb_wsserve_conn_set_ping(klb_netconn_t* p_conn, int interval_ms, int timeout_ms)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;

    normalize_ping_klb_wsserve_conn(p_ws, interval_ms, timeout_ms);
    p_ws->last_ping_tc = 0;
    p_ws->ping_deadline_tc = 0;
    p_ws->ping_waiting = false;
}


void klb_wsserve_conn_adopt_tail(klb_netconn_t* p_conn, klb_buf_t* p_tail)
{
    klb_wsserve_conn_t* p_ws = (klb_wsserve_conn_t*)p_conn->extra;
    int n = 0;

    p_ws->handshake_ok = true;
    p_ws->http_over = true;
    p_ws->allow_raw = false;
    p_ws->ws_status = KLB_WSSERVE_WS_HEAD;

    if (NULL != p_tail)
    {
        n = klb_buf_data_len(p_tail);
    }

    if (0 < n)
    {
        if (klb_buf_idle_len(p_ws->p_read_buf) < n)
        {
            klb_buf_t* p_big = klb_buf_malloc(n, false);
            KLB_FREE_BY(p_ws->p_read_buf, klb_buf_unref);
            p_ws->p_read_buf = p_big;
        }

        klb_buf_write(p_ws->p_read_buf, p_tail->p_buf + p_tail->start, n);
    }

    KLB_FREE_BY(p_tail, klb_buf_unref);
    push_data_klb_wsserve_conn(p_conn, KLB_WEBSOCKET_PACK_OPEN, NULL);

    if (0 < n)
    {
        parser_ws_klb_wsserve_conn(p_conn);
    }
}

// end
