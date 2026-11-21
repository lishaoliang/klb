// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbhttp/core/klb_httpclient_conn.h"
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
#include <stdio.h>
#ifndef _WIN32
#include <sys/types.h>
#endif
#include <assert.h>


/// @struct klb_httpclient_conn_t
/// @brief  HTTP客户端连接(HTTP client connect)
typedef struct klb_httpclient_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // 发送 数据体
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送 HTTP数据 列表
        FILE*                       p_file;             ///< 待发送文件; 无则为 NULL
        int64_t                     file_remain;        ///< 文件剩余未发送字节
    };

    // 读取 数据
    struct
    {
        int                         parser_status;      ///< 读取解析状态
#define KLB_HTTPCLIENT_PARSER_ERR               (-1)
#define KLB_HTTPCLIENT_PARSER                   0
#define KLB_HTTPCLIENT_PARSER_OVER              2

        http_parser                 parser;             ///< HTTP解析器
        http_parser_settings        settings;           ///< 解析器设置
        bool                        status_prefix;      ///< 是否已写 HTTP/x.x code 前缀
        bool                        status_crlf;        ///< 状态行是否已补 CRLF
        bool                        header_value_open;  ///< 头字段值是否处于未闭合状态 (分片回调)

        klb_buf_t*                  p_read_buf;         ///< 读取缓存

        sds                         header;             ///< 已解析的响应头
        klb_buffer_t*               p_body;             ///< 已解析的响应体
    };

    // 超时 / 体上限 (0=关闭)
    struct
    {
        int                         read_timeout_ms;    ///< 读超时
        int                         write_timeout_ms;   ///< 写超时
        int                         idle_timeout_ms;    ///< 空闲超时
        int64_t                     max_body;           ///< 响应体上限; 0=不限
        bool                        body_too_large;     ///< 体超限标记
    };
}klb_httpclient_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_httpclient_conn_quit(klb_netconn_t* p_conn);
static void reset_msg_klb_httpclient_conn(klb_netconn_t* p_conn);
static void close_file_klb_httpclient_conn(klb_httpclient_conn_t* p_httpclient);
static void push_netcode_klb_httpclient_conn(klb_netconn_t* p_conn, int code);


//////////////////////////////////////////////////////////////////////////
// 内部函数


/// @brief 接收到数据 之后 放入数据
/// @param [in] packtype    数包类型: klb_mnp_packtype_e
static void push_data_klb_httpclient_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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
static void push_netcode_klb_httpclient_conn(klb_netconn_t* p_conn, int code)
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

/// @brief 头字段名是否匹配(大小写不敏感; 后接 ':')
static bool header_name_eq_klb_httpclient_conn(const char* p, int line_len, const char* name, int name_len)
{
    int i = 0;

    if (line_len <= name_len || ':' != p[name_len])
    {
        return false;
    }

    for (i = 0; i < name_len; i++)
    {
        char a = p[i];
        char b = name[i];

        if ('A' <= a && 'Z' >= a)
        {
            a = (char)(a - 'A' + 'a');
        }

        if (a != b)
        {
            return false;
        }
    }

    return true;
}

/// @brief chunked 已解码时改写头: 去掉 Transfer-Encoding, 写 Content-Length=body_len
static void rewrite_decoded_header_klb_httpclient_conn(klb_httpclient_conn_t* p_httpclient, int body_len)
{
    sds old = NULL;
    sds neu = NULL;
    const char* p = NULL;
    const char* end = NULL;

    if (NULL == p_httpclient || NULL == p_httpclient->header)
    {
        return;
    }

    if (0 == (p_httpclient->parser.flags & F_CHUNKED))
    {
        return;
    }

    old = p_httpclient->header;
    neu = sdsnew("");
    p = old;
    end = old + (int)sdslen(old);

    while (p < end)
    {
        const char* line_end = p;
        int line_len = 0;

        while (line_end + 1 < end && !('\r' == line_end[0] && '\n' == line_end[1]))
        {
            line_end++;
        }

        if (line_end + 1 >= end)
        {
            break;
        }

        line_len = (int)(line_end - p);
        if (0 == line_len)
        {
            break;
        }

        if (header_name_eq_klb_httpclient_conn(p, line_len, "transfer-encoding", 17)
            || header_name_eq_klb_httpclient_conn(p, line_len, "content-length", 14))
        {
            p = line_end + 2;
            continue;
        }

        neu = sdscatlen(neu, p, (size_t)(line_len + 2));
        p = line_end + 2;
    }

    neu = sdscatfmt(neu, "Content-Length: %i\r\n\r\n", body_len);
    sdsfree(old);
    p_httpclient->header = neu;
}

/// @brief 组装完整HTTP响应, 按 TEXT 包上送
static void push_pack_klb_httpclient_conn(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    int head_len = 0;
    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    int total = 0;

    if (NULL != p_httpclient->p_body && 0 < klb_buffer_datalen(p_httpclient->p_body))
    {
        p_body_part = klb_buffer_join(p_httpclient->p_body, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_httpclient->p_body);
    }

    // chunked 体已由 parser 解码; 头须与解码后长度一致
    rewrite_decoded_header_klb_httpclient_conn(p_httpclient, body_len);

    if (NULL != p_httpclient->header)
    {
        head_len = (int)sdslen(p_httpclient->header);
    }

    total = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_data = klb_buf_malloc(total, false);

    klb_mnp_text_t com = { 0 };
    com.size = (uint32_t)total;
    com.head_size = (uint32_t)head_len;

    memcpy(p_data->p_buf, &com, sizeof(com));
    if (0 < head_len)
    {
        memcpy(p_data->p_buf + sizeof(com), p_httpclient->header, head_len);
    }

    if (0 < body_len)
    {
        memcpy(p_data->p_buf + sizeof(com) + head_len, p_body_part->p_buf + p_body_part->start, body_len);
    }

    p_data->start = 0;
    p_data->end = total;

    KLB_FREE_BY(p_body_part, klb_buf_unref);
    KLB_FREE_BY(p_httpclient->header, sdsfree);

    push_data_klb_httpclient_conn(p_conn, KLB_MNP_TEXT, p_data);
}

/// @brief connect 超时
static void on_connect_timeout_klb_httpclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
}

/// @brief connect 握手完成
static void on_connected_timeout_klb_httpclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_CONNECT);
}

/// @brief ticker 定时器消息: 读/写/idle 超时
static void on_ticker_klb_httpclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    bool writing = false;
    bool reading = false;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return;
    }

    writing = (0 < klb_nlist_size(p_httpclient->p_write_nlist)) || (NULL != p_httpclient->p_file);
    reading = (NULL != p_httpclient->header)
        || (NULL != p_httpclient->p_body && 0 < klb_buffer_datalen(p_httpclient->p_body))
        || (p_httpclient->p_read_buf->start < p_httpclient->p_read_buf->end);

    if (writing && 0 < p_httpclient->write_timeout_ms
        && (int64_t)p_httpclient->write_timeout_ms <= ABS_SUB(now, p_socket->last_send_tc))
    {
        push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    if (reading && 0 < p_httpclient->read_timeout_ms
        && (int64_t)p_httpclient->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    if (!writing && !reading && 0 < p_httpclient->idle_timeout_ms
        && (int64_t)p_httpclient->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }
}


//////////////////////////////////////////////////////////////////////////
// http_parser 回调

static int on_message_begin_klb_httpclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    KLB_FREE_BY(p_httpclient->header, sdsfree);
    p_httpclient->header = sdsnew("");
    p_httpclient->status_prefix = false;
    p_httpclient->status_crlf = false;
    p_httpclient->header_value_open = false;

    return 0;
}

static int on_url_klb_httpclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    return 0;
}

static int on_status_klb_httpclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (NULL == p_httpclient->header)
    {
        p_httpclient->header = sdsnew("");
    }

    if (!p_httpclient->status_prefix)
    {
        p_httpclient->header = sdscatfmt(p_httpclient->header, "HTTP/%u.%u %u ",
            p_parser->http_major, p_parser->http_minor, p_parser->status_code);
        p_httpclient->status_prefix = true;
    }

    if (0 < length)
    {
        p_httpclient->header = sdscatlen(p_httpclient->header, at, length);
    }

    return 0;
}

static int on_header_field_klb_httpclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (NULL == p_httpclient->header)
    {
        p_httpclient->header = sdsnew("");
    }

    if (p_httpclient->header_value_open)
    {
        p_httpclient->header = sdscat(p_httpclient->header, "\r\n");
        p_httpclient->header_value_open = false;
    }

    if (!p_httpclient->status_crlf)
    {
        p_httpclient->header = sdscat(p_httpclient->header, "\r\n");
        p_httpclient->status_crlf = true;
    }

    p_httpclient->header = sdscatlen(p_httpclient->header, at, length);

    return 0;
}

static int on_header_value_klb_httpclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (NULL == p_httpclient->header)
    {
        p_httpclient->header = sdsnew("");
    }

    if (!p_httpclient->header_value_open)
    {
        p_httpclient->header = sdscat(p_httpclient->header, ": ");
        p_httpclient->header_value_open = true;
    }

    p_httpclient->header = sdscatlen(p_httpclient->header, at, length);

    return 0;
}

static int on_headers_complete_klb_httpclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (NULL == p_httpclient->header)
    {
        p_httpclient->header = sdsnew("");
    }

    if (p_httpclient->header_value_open)
    {
        p_httpclient->header = sdscat(p_httpclient->header, "\r\n");
        p_httpclient->header_value_open = false;
    }

    if (!p_httpclient->status_crlf)
    {
        p_httpclient->header = sdscat(p_httpclient->header, "\r\n");
        p_httpclient->status_crlf = true;
    }

    p_httpclient->header = sdscat(p_httpclient->header, "\r\n");

    if (0 < p_httpclient->max_body
        && 0 < p_parser->content_length
        && (uint64_t)p_httpclient->max_body < p_parser->content_length)
    {
        p_httpclient->body_too_large = true;
        return -1;
    }

    return 0;
}

static int on_body_klb_httpclient_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    int cur = 0;

    if (0 >= (int)length)
    {
        return 0;
    }

    if (NULL == p_httpclient->p_body)
    {
        p_httpclient->p_body = klb_buffer_create(4096);
    }

    cur = klb_buffer_datalen(p_httpclient->p_body);
    if (0 < p_httpclient->max_body
        && p_httpclient->max_body < (int64_t)cur + (int64_t)length)
    {
        p_httpclient->body_too_large = true;
        return -1;
    }

    klb_buffer_write(p_httpclient->p_body, at, (int)length);

    return 0;
}

static int on_message_complete_klb_httpclient_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    p_httpclient->parser_status = KLB_HTTPCLIENT_PARSER_OVER;

    return 0;
}

/// @brief 复位单条消息收集状态(keep-alive 下 parser 本身不 init)
static void reset_msg_klb_httpclient_conn(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    p_httpclient->parser_status = KLB_HTTPCLIENT_PARSER;
    p_httpclient->status_prefix = false;
    p_httpclient->status_crlf = false;
    p_httpclient->body_too_large = false;

    KLB_FREE_BY(p_httpclient->header, sdsfree);
    if (NULL != p_httpclient->p_body)
    {
        klb_buffer_reset(p_httpclient->p_body);
    }
}

/// @brief 压缩读缓存
static void compact_read_klb_httpclient_conn(klb_httpclient_conn_t* p_httpclient)
{
    klb_buf_t* p_buf = p_httpclient->p_read_buf;

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

/// @brief 解析读缓存中的HTTP响应
static int parser_klb_httpclient_conn(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_httpclient->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        size_t have = (size_t)(p_buf->end - p_buf->start);
        size_t nparsed = http_parser_execute(&p_httpclient->parser, &p_httpclient->settings,
            p_buf->p_buf + p_buf->start, have);

        if (0 != p_httpclient->parser.http_errno)
        {
            p_httpclient->parser_status = KLB_HTTPCLIENT_PARSER_ERR;
            compact_read_klb_httpclient_conn(p_httpclient);
            return -1;
        }

        if (0 < nparsed)
        {
            p_buf->start += (int)nparsed;
        }

        if (KLB_HTTPCLIENT_PARSER_OVER == p_httpclient->parser_status)
        {
            push_pack_klb_httpclient_conn(p_conn);
            reset_msg_klb_httpclient_conn(p_conn);
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

    compact_read_klb_httpclient_conn(p_httpclient);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_httpclient_conn_destroy(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    // 之前 需要 调用 klb_httpclient_conn_free 函数
    assert(NULL == p_httpclient->p_netmulti);

    // 关闭socket
    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    // 退出
    klb_httpclient_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_httpclient_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_httpclient_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_httpclient_conn_send(p_conn, p_head, head_len, p_body, body_len);
}

/// @brief 发送媒体数据包
/// @return int
static int klb_httpclient_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    return 1;
}

#define KLB_HTTPCLIENT_FILE_CHUNK           (32 * 1024)


/// @brief 关闭待发送文件
static void close_file_klb_httpclient_conn(klb_httpclient_conn_t* p_httpclient)
{
    if (NULL != p_httpclient->p_file)
    {
        fclose(p_httpclient->p_file);
        p_httpclient->p_file = NULL;
    }

    p_httpclient->file_remain = 0;
}


/// @brief 文件 seek 到 offset
static int seek_file_klb_httpclient_conn(FILE* p_file, int64_t offset)
{
#ifdef _WIN32
    return _fseeki64(p_file, offset, SEEK_SET);
#else
    return fseeko(p_file, (off_t)offset, SEEK_SET);
#endif
}


/// @brief 从文件读一块入写队列
/// @return int 0.成功或无文件; 非0.读失败
static int fill_file_klb_httpclient_conn(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (NULL == p_httpclient->p_file)
    {
        return 0;
    }

    if (p_httpclient->file_remain <= 0)
    {
        close_file_klb_httpclient_conn(p_httpclient);
        return 0;
    }

    int chunk = KLB_HTTPCLIENT_FILE_CHUNK;
    if (p_httpclient->file_remain < (int64_t)chunk)
    {
        chunk = (int)p_httpclient->file_remain;
    }

    klb_buf_t* p_buf = klb_buf_malloc(chunk, false);
    size_t n = fread(p_buf->p_buf, 1, (size_t)chunk, p_httpclient->p_file);
    if (0 == n)
    {
        KLB_FREE_BY(p_buf, klb_buf_unref);
        close_file_klb_httpclient_conn(p_httpclient);
        push_netcode_klb_httpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    p_buf->end = (int)n;
    p_httpclient->file_remain -= (int64_t)n;
    if (p_httpclient->file_remain <= 0)
    {
        close_file_klb_httpclient_conn(p_httpclient);
    }

    klb_nlist_push_tail(p_httpclient->p_write_nlist, p_buf);
    return 0;
}


/// @brief 当网络上可以发送数据时
/// @return int
static int klb_httpclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_httpclient->p_write_nlist;
    int write_num = 0;

    while (true)
    {
        klb_buf_t* p_buf = klb_nlist_head(p_write_nlist);
        if (NULL == p_buf)
        {
            if (0 != fill_file_klb_httpclient_conn(p_conn))
            {
                break;
            }

            p_buf = klb_nlist_head(p_write_nlist);
            if (NULL == p_buf)
            {
                break;
            }
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
        push_netcode_klb_httpclient_conn(p_conn, close_status);
        return write_num;
    }

    // 缓存的数据写完了
    if (klb_nlist_size(p_write_nlist) <= 0 && NULL == p_httpclient->p_file)
    {
        klb_socket_set_writing(p_socket, false);

        // 缓存写完了 消息
        push_netcode_klb_httpclient_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

/// @brief 接收网络数据
/// @return int
static int do_recv_klb_httpclient_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_httpclient->p_read_buf;

    if (KLB_HTTPCLIENT_PARSER_ERR == p_httpclient->parser_status)
    {
        return -1;
    }

    int idle_len = klb_buf_idle_len(p_buf);
    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_httpclient_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_httpclient_conn(p_conn,
                p_httpclient->body_too_large ? KLB_NETCODE_PROTO_TOO_LARGE : KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_httpclient_conn(p_conn, KLB_NETCODE_RBUF_FULL);
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

    int ret_parse = parser_klb_httpclient_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_httpclient_conn(p_conn,
            p_httpclient->body_too_large ? KLB_NETCODE_PROTO_TOO_LARGE : KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 >= recv_len && KLB_SOCKET_RECV_AGAIN != judge)
    {
        klb_socket_recv_closed(p_socket, close_status);
        push_netcode_klb_httpclient_conn(p_conn, close_status);
        return -1;
    }

    return 0;
}

/// @brief 当网络上可以接收数据时
/// @return int
static int klb_httpclient_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_httpclient_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @param [in] msg             消息类型: klb_netconn_msg_e
static int klb_httpclient_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_httpclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_timeout_klb_httpclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_httpclient_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

/// @brief 关闭连接
void klb_httpclient_conn_free(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    // 关闭连接
    klb_netmulti_closing(p_httpclient->p_netmulti, p_conn);
    p_httpclient->p_netmulti = NULL;
}

/// @brief 发送HTTP请求
int klb_httpclient_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
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

    klb_nlist_push_tail(p_httpclient->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

/// @brief 按 buf 发送HTTP数据
int klb_httpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    klb_nlist_push_tail(p_httpclient->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

/// @brief 写缓存 是否为空
bool klb_httpclient_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_httpclient->p_write_nlist))
    {
        return false; // 还有数据
    }

    if (NULL != p_httpclient->p_file)
    {
        return false;
    }

    return true; // 空了
}


/// @brief 设置读/写/idle 超时(毫秒; 负值钳为 0)
void klb_httpclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    klb_httpclient_conn_t* p_httpclient = NULL;

    assert(NULL != p_conn);
    p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    p_httpclient->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_httpclient->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_httpclient->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}


/// @brief 设置响应体上限(字节; 负值钳为 0=不限)
void klb_httpclient_conn_set_max_body(klb_netconn_t* p_conn, int64_t max_body)
{
    klb_httpclient_conn_t* p_httpclient = NULL;

    assert(NULL != p_conn);
    p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    p_httpclient->max_body = (0 < max_body) ? max_body : 0;
}


/// @brief 发送HTTP请求头 + 本地文件体
int klb_httpclient_conn_send_file(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len,
    const char* p_path, int64_t offset, int64_t length)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (NULL == p_path || 0 == p_path[0] || offset < 0 || length < 0)
    {
        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_httpclient->p_file)
    {
        return KLB_NETCODE_BUSY;
    }

    FILE* p_file = NULL;
    if (0 < length)
    {
        p_file = fopen(p_path, "rb");
        if (NULL == p_file)
        {
            return KLB_NETCODE_NOENT;
        }

        if (0 < offset)
        {
            if (0 != seek_file_klb_httpclient_conn(p_file, offset))
            {
                fclose(p_file);
                return KLB_NETCODE_IO;
            }
        }
    }

    if (0 < head_len && NULL != p_head)
    {
        klb_httpclient_conn_send(p_conn, p_head, head_len, NULL, 0);
    }

    p_httpclient->p_file = p_file;
    p_httpclient->file_remain = length;
    klb_socket_set_writing(p_socket, true);

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

/// @brief 初始化
static int klb_httpclient_conn_init(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    // 初始化 vtable
    {
        p_conn->vtable.destroy = NULL; // 销毁
        p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

        p_conn->vtable.ioctrl = klb_httpclient_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
        p_conn->vtable.send_normal = klb_httpclient_conn_send_normal; // 调用者 发送常规数据包
        p_conn->vtable.send_media = klb_httpclient_conn_send_media; // 调用者 发送媒体数据包
        p_conn->vtable.on_send = klb_httpclient_conn_on_send; // 当网络上可以发送数据时
        p_conn->vtable.on_recv = klb_httpclient_conn_on_recv; // 当网络上可以接收数据时
        p_conn->vtable.on_msg = klb_httpclient_conn_on_msg; // 当网络上有消息传来时
    }

    p_conn->protocol = KLB_PROTOCOL_HTTP;

    // 初始化
    p_httpclient->p_write_nlist = klb_nlist_create();
    p_httpclient->p_read_buf = klb_buf_malloc(1024 * 16, false);

    http_parser_init(&p_httpclient->parser, HTTP_RESPONSE);
    http_parser_settings_init(&p_httpclient->settings);
    p_httpclient->parser.data = p_conn;

    p_httpclient->settings.on_message_begin = on_message_begin_klb_httpclient_conn;
    p_httpclient->settings.on_url = on_url_klb_httpclient_conn;
    p_httpclient->settings.on_status = on_status_klb_httpclient_conn;
    p_httpclient->settings.on_header_field = on_header_field_klb_httpclient_conn;
    p_httpclient->settings.on_header_value = on_header_value_klb_httpclient_conn;
    p_httpclient->settings.on_headers_complete = on_headers_complete_klb_httpclient_conn;
    p_httpclient->settings.on_body = on_body_klb_httpclient_conn;
    p_httpclient->settings.on_message_complete = on_message_complete_klb_httpclient_conn;

    p_httpclient->parser_status = KLB_HTTPCLIENT_PARSER;
    p_httpclient->status_prefix = false;
    p_httpclient->status_crlf = false;
    p_httpclient->header = NULL;
    p_httpclient->p_body = NULL;
    p_httpclient->p_file = NULL;
    p_httpclient->file_remain = 0;
    p_httpclient->read_timeout_ms = 0;
    p_httpclient->write_timeout_ms = 0;
    p_httpclient->idle_timeout_ms = 0;
    p_httpclient->max_body = 0;
    p_httpclient->body_too_large = false;

    return 0;
}

/// @brief 退出
static void klb_httpclient_conn_quit(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    close_file_klb_httpclient_conn(p_httpclient);

    while (0 < klb_nlist_size(p_httpclient->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_httpclient->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_httpclient->header, sdsfree);
    KLB_FREE_BY(p_httpclient->p_body, klb_buffer_destroy);
    KLB_FREE_BY(p_httpclient->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_httpclient->p_write_nlist, klb_nlist_destroy);
}


//////////////////////////////////////////////////////////////////////////
/// 连接目标

/// @brief HTTP连接目标
klb_netconn_t* klb_httpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code)
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

    // 连接 socket
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

    // 创建 klb_netconn_t*
    p_netconn = klb_httpclient_conn_create(p_netmulti, p_socket);

    {
        // 放入复用模块
        klb_netmulti_push(p_netmulti, p_netconn);
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_netconn;
}


//////////////////////////////////////////////////////////////////////////
/// 正向代理隧道

/// @brief 置/清 CONNECT 应答模式
/// @note 响应解析器无请求行, method 恒为初值; 置 HTTP_CONNECT 令 2xx 应答按无体收尾
int klb_httpclient_conn_set_connect(klb_netconn_t* p_conn, bool connect)
{
    klb_httpclient_conn_t* p_httpclient = NULL;

    if (NULL == p_conn)
    {
        return KLB_NETCODE_INVAL;
    }

    p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    p_httpclient->parser.method = connect ? HTTP_CONNECT : 0;

    return 0;
}


/// @brief 明文连接就地升级为 TLS
int klb_httpclient_conn_tls_upgrade(klb_netconn_t* p_conn)
{
    klb_httpclient_conn_t* p_httpclient = NULL;
    klb_socket_t* p_old = NULL;
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;

    if (NULL == p_conn)
    {
        return KLB_NETCODE_INVAL;
    }

    p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;
    p_old = p_conn->p_socket;

    if (NULL == p_old || klb_socket_is_tls(p_old))
    {
        return KLB_NETCODE_INVAL;
    }

    // 读缓存须已空: 隧道后余量为下一协议数据, 无法在此接管
    if (p_httpclient->p_read_buf->start < p_httpclient->p_read_buf->end)
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    // step1. 分离 fd 并销毁明文 socket; fd 已分离故不关闭
    fd = klb_socket_detach_fd(p_old);
    klb_socket_destroy(p_old);

    // step2. 同一 fd 换 TLS socket
    p_socket = klb_socket_async_create_tls(fd);
    if (NULL == p_socket)
    {
        // 失败: 以明文 socket 收回同 fd, 连接仍可正常关闭
        p_conn->p_socket = klb_socket_async_create(fd);
        return KLB_NETCODE_TLS_HANDSHAKE;
    }

    klb_socket_set_connected(p_socket, true);
    p_conn->p_socket = p_socket;

    // step3. 隧道后为全新协议流: 复位解析器与 method
    http_parser_init(&p_httpclient->parser, HTTP_RESPONSE);
    p_httpclient->parser.data = p_conn;
    p_httpclient->parser_status = KLB_HTTPCLIENT_PARSER;

    return 0;
}


//////////////////////////////////////////////////////////////////////////

/// @brief 创建HTTP连接
/// @return klb_netconn_t*
klb_netconn_t* klb_httpclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_httpclient_conn_t));
    klb_httpclient_conn_t* p_httpclient = (klb_httpclient_conn_t*)p_conn->extra;

    klb_httpclient_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_httpclient_conn_destroy;

    // socket
    {
        p_httpclient->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        // 开启读写
        klb_socket_set_reading(p_socket, true);
    }

    return p_conn;
}

// end
