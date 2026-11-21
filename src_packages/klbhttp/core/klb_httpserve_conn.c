// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbhttp/core/klb_httpserve_conn.h"
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


/// @brief klb_mnp_text_t.size 为 27 位位域的最大值 (128MiB - 1)
#define KLB_HTTPSERVE_TEXT_SIZE_MAX     (0x7FFFFFF)


/// @struct klb_httpserve_conn_t
/// @brief  HTTP服务连接(HTTP serve connect)
typedef struct klb_httpserve_conn_t_
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
#define KLB_HTTPSERVE_PARSER_ERR                (-1)
#define KLB_HTTPSERVE_PARSER                    0
#define KLB_HTTPSERVE_PARSER_OVER               2

        http_parser                 parser;             ///< HTTP解析器
        http_parser_settings        settings;           ///< 解析器设置
        bool                        req_line_crlf;      ///< 请求行是否已补 HTTP/x.x CRLF
        bool                        header_value_open;  ///< 头字段值是否处于未闭合状态 (分片回调)
        bool                        upgrade_hold;       ///< Upgrade 后停在 HTTP 解析, 尾字节留给下一协议

        klb_buf_t*                  p_read_buf;         ///< 读取缓存

        sds                         header;             ///< 已解析的请求头
        klb_buffer_t*               p_body;             ///< 已解析的请求体
    };

    // 超时 / 体上限 (0=关闭)
    struct
    {
        int                         read_timeout_ms;    ///< 读超时
        int                         write_timeout_ms;   ///< 写超时
        int                         idle_timeout_ms;    ///< 空闲超时
        int64_t                     max_body;           ///< 请求体上限; 0=不限
        bool                        body_too_large;     ///< 体超限标记
    };
}klb_httpserve_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_httpserve_conn_quit(klb_netconn_t* p_conn);
static void reset_msg_klb_httpserve_conn(klb_netconn_t* p_conn);
static void close_file_klb_httpserve_conn(klb_httpserve_conn_t* p_httpserve);
static void push_netcode_klb_httpserve_conn(klb_netconn_t* p_conn, int code);


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
static bool header_name_eq_klb_httpserve_conn(const char* p, int line_len, const char* name, int name_len)
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
static void rewrite_decoded_header_klb_httpserve_conn(klb_httpserve_conn_t* p_httpserve, int body_len)
{
    sds old = NULL;
    sds neu = NULL;
    const char* p = NULL;
    const char* end = NULL;

    if (NULL == p_httpserve || NULL == p_httpserve->header)
    {
        return;
    }

    if (0 == (p_httpserve->parser.flags & F_CHUNKED))
    {
        return;
    }

    old = p_httpserve->header;
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

        if (header_name_eq_klb_httpserve_conn(p, line_len, "transfer-encoding", 17)
            || header_name_eq_klb_httpserve_conn(p, line_len, "content-length", 14))
        {
            p = line_end + 2;
            continue;
        }

        neu = sdscatlen(neu, p, (size_t)(line_len + 2));
        p = line_end + 2;
    }

    neu = sdscatfmt(neu, "Content-Length: %i\r\n\r\n", body_len);
    sdsfree(old);
    p_httpserve->header = neu;
}

/// @brief 组装完整HTTP请求, 按 TEXT 包上送
static void push_pack_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    int head_len = 0;
    int body_len = 0;
    klb_buf_t* p_body_part = NULL;
    int total = 0;

    if (NULL != p_httpserve->p_body && 0 < klb_buffer_datalen(p_httpserve->p_body))
    {
        p_body_part = klb_buffer_join(p_httpserve->p_body, NULL, NULL);
        body_len = klb_buf_data_len(p_body_part);
        klb_buffer_reset(p_httpserve->p_body);
    }

    // chunked 体已由 parser 解码; 头须与解码后长度一致
    rewrite_decoded_header_klb_httpserve_conn(p_httpserve, body_len);

    if (NULL != p_httpserve->header)
    {
        head_len = (int)sdslen(p_httpserve->header);
    }

    total = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_data = klb_buf_malloc(total, false);

    klb_mnp_text_t com = { 0 };
    // size 为 27 位位域(上限 128MiB); 本机 TEXT 包按缓冲真实长度取体,
    // 超限时钳到上限, 避免回绕成小值误导其他读取方
    com.size = (uint32_t)((KLB_HTTPSERVE_TEXT_SIZE_MAX < total) ? KLB_HTTPSERVE_TEXT_SIZE_MAX : total);
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

/// @brief ticker 定时器消息: 读/写/idle 超时
static void on_ticker_klb_httpserve_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    bool writing = false;
    bool reading = false;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return;
    }

    writing = (0 < klb_nlist_size(p_httpserve->p_write_nlist)) || (NULL != p_httpserve->p_file);
    reading = (NULL != p_httpserve->header)
        || (NULL != p_httpserve->p_body && 0 < klb_buffer_datalen(p_httpserve->p_body))
        || (p_httpserve->p_read_buf->start < p_httpserve->p_read_buf->end);

    // step1. 写超时: 队列未空且长时间无发包
    if (writing && 0 < p_httpserve->write_timeout_ms
        && (int64_t)p_httpserve->write_timeout_ms <= ABS_SUB(now, p_socket->last_send_tc))
    {
        push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    // step2. 读超时: 请求进行中且长时间无收包
    if (reading && 0 < p_httpserve->read_timeout_ms
        && (int64_t)p_httpserve->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    // step3. idle: 无读写进行且长时间无收包
    if (!writing && !reading && 0 < p_httpserve->idle_timeout_ms
        && (int64_t)p_httpserve->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }
}


//////////////////////////////////////////////////////////////////////////
// http_parser 回调

static int on_message_begin_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    // 注意. message_begin 时 method 仅为首字母猜测 (P->POST), PROPFIND 等尚未解析完
    // 请求行 METHOD 在 finish_req_line 用最终 p_parser->method 回填
    KLB_FREE_BY(p_httpserve->header, sdsfree);
    p_httpserve->header = sdsnew("");
    p_httpserve->req_line_crlf = false;
    p_httpserve->header_value_open = false;

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
    const char* p_meth = NULL;
    sds url = NULL;

    if (p_httpserve->req_line_crlf)
    {
        return;
    }

    if (NULL == p_httpserve->header)
    {
        p_httpserve->header = sdsnew("");
    }

    // header 当前仅为 URL (on_url 追加); 拼最终 METHOD + URL + 版本
    p_meth = http_method_str((enum http_method)p_parser->method);
    if (NULL == p_meth || 0 == p_meth[0])
    {
        p_meth = "GET";
    }

    url = p_httpserve->header;
    if (0 == sdslen(url))
    {
        p_httpserve->header = sdscatfmt(sdsempty(), "%s / HTTP/%u.%u\r\n",
            p_meth, p_parser->http_major, p_parser->http_minor);
    }
    else
    {
        p_httpserve->header = sdscatfmt(sdsempty(), "%s %s HTTP/%u.%u\r\n",
            p_meth, url, p_parser->http_major, p_parser->http_minor);
    }
    sdsfree(url);
    p_httpserve->req_line_crlf = true;
}

static int on_header_field_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    finish_req_line_klb_httpserve_conn(p_parser, p_httpserve);

    if (p_httpserve->header_value_open)
    {
        p_httpserve->header = sdscat(p_httpserve->header, "\r\n");
        p_httpserve->header_value_open = false;
    }

    p_httpserve->header = sdscatlen(p_httpserve->header, at, length);

    return 0;
}

static int on_header_value_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    if (NULL == p_httpserve->header)
    {
        p_httpserve->header = sdsnew("");
    }

    if (!p_httpserve->header_value_open)
    {
        p_httpserve->header = sdscat(p_httpserve->header, ": ");
        p_httpserve->header_value_open = true;
    }

    p_httpserve->header = sdscatlen(p_httpserve->header, at, length);

    return 0;
}

static int on_headers_complete_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    finish_req_line_klb_httpserve_conn(p_parser, p_httpserve);

    if (p_httpserve->header_value_open)
    {
        p_httpserve->header = sdscat(p_httpserve->header, "\r\n");
        p_httpserve->header_value_open = false;
    }

    p_httpserve->header = sdscat(p_httpserve->header, "\r\n");

    // Content-Length 已知且超上限: 尽早拒收
    if (0 < p_httpserve->max_body
        && 0 < p_parser->content_length
        && (uint64_t)p_httpserve->max_body < p_parser->content_length)
    {
        p_httpserve->body_too_large = true;
        return -1;
    }

    return 0;
}

static int on_body_klb_httpserve_conn(http_parser* p_parser, const char* at, size_t length)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    int cur = 0;

    if (0 == length)
    {
        return 0;
    }

    if (NULL == p_httpserve->p_body)
    {
        p_httpserve->p_body = klb_buffer_create(4096);
    }

    cur = klb_buffer_datalen(p_httpserve->p_body);
    if (0 < p_httpserve->max_body
        && p_httpserve->max_body < (int64_t)cur + (int64_t)length)
    {
        p_httpserve->body_too_large = true;
        return -1;
    }

    klb_buffer_write(p_httpserve->p_body, at, (int)length);

    return 0;
}

static int on_message_complete_klb_httpserve_conn(http_parser* p_parser)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_parser->data;
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->parser_status = KLB_HTTPSERVE_PARSER_OVER;

    // 流水线: 同一读缓存可能含多条请求; http_parser_execute 默认会一次解析完,
    // 后续消息的 message_begin 会重置 header/body, 只留下最后一条.
    // 暂停在消息边界, 交由解析循环逐条上送 (回调内不得重入 Lua)
    http_parser_pause(p_parser, 1);

    return 0;
}

/// @brief 复位单条消息收集状态(keep-alive 下 parser 本身不 init)
static void reset_msg_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->parser_status = KLB_HTTPSERVE_PARSER;
    p_httpserve->req_line_crlf = false;
    p_httpserve->body_too_large = false;

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

        // 暂停表示停在单条消息边界 (流水线), 不是错误: 解除暂停后按 OVER 处理本条
        if (HPE_PAUSED == p_httpserve->parser.http_errno)
        {
            http_parser_pause(&p_httpserve->parser, 0);
        }
        else if (0 != p_httpserve->parser.http_errno)
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
            bool upgrading = (0 != p_httpserve->parser.upgrade);

            push_pack_klb_httpserve_conn(p_conn);
            if (upgrading)
            {
                p_httpserve->upgrade_hold = true;
                compact_read_klb_httpserve_conn(p_httpserve);
                return 0;
            }

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

#define KLB_HTTPSERVE_FILE_CHUNK            (32 * 1024)


/// @brief 关闭待发送文件
static void close_file_klb_httpserve_conn(klb_httpserve_conn_t* p_httpserve)
{
    if (NULL != p_httpserve->p_file)
    {
        fclose(p_httpserve->p_file);
        p_httpserve->p_file = NULL;
    }

    p_httpserve->file_remain = 0;
}


/// @brief 文件 seek 到 offset
static int seek_file_klb_httpserve_conn(FILE* p_file, int64_t offset)
{
#ifdef _WIN32
    return _fseeki64(p_file, offset, SEEK_SET);
#else
    return fseeko(p_file, (off_t)offset, SEEK_SET);
#endif
}


/// @brief 从文件读一块入写队列
/// @return int 0.成功或无文件; 非0.读失败
static int fill_file_klb_httpserve_conn(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    if (NULL == p_httpserve->p_file)
    {
        return 0;
    }

    if (p_httpserve->file_remain <= 0)
    {
        close_file_klb_httpserve_conn(p_httpserve);
        return 0;
    }

    int chunk = KLB_HTTPSERVE_FILE_CHUNK;
    if (p_httpserve->file_remain < (int64_t)chunk)
    {
        chunk = (int)p_httpserve->file_remain;
    }

    klb_buf_t* p_buf = klb_buf_malloc(chunk, false);
    size_t n = fread(p_buf->p_buf, 1, (size_t)chunk, p_httpserve->p_file);
    if (0 == n)
    {
        KLB_FREE_BY(p_buf, klb_buf_unref);
        close_file_klb_httpserve_conn(p_httpserve);
        push_netcode_klb_httpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    p_buf->end = (int)n;
    p_httpserve->file_remain -= (int64_t)n;
    if (p_httpserve->file_remain <= 0)
    {
        close_file_klb_httpserve_conn(p_httpserve);
    }

    klb_nlist_push_tail(p_httpserve->p_write_nlist, p_buf);
    return 0;
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
            if (0 != fill_file_klb_httpserve_conn(p_conn))
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
        push_netcode_klb_httpserve_conn(p_conn, close_status);
        return write_num;
    }

    if (klb_nlist_size(p_write_nlist) <= 0 && NULL == p_httpserve->p_file)
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
        if (!p_httpserve->upgrade_hold)
        {
            int ret_parse = parser_klb_httpserve_conn(p_conn);
            if (0 != ret_parse)
            {
                push_netcode_klb_httpserve_conn(p_conn,
                    p_httpserve->body_too_large ? KLB_NETCODE_PROTO_TOO_LARGE : KLB_SOCKET_ERR_PROTOCOL);
                return -1;
            }
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_httpserve_conn(p_conn, KLB_NETCODE_RBUF_FULL);
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

    if (!p_httpserve->upgrade_hold)
    {
        int ret_parse = parser_klb_httpserve_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_httpserve_conn(p_conn,
                p_httpserve->body_too_large ? KLB_NETCODE_PROTO_TOO_LARGE : KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }
    }

    if (0 >= recv_len && KLB_SOCKET_RECV_AGAIN != judge)
    {
        klb_socket_recv_closed(p_socket, close_status);
        push_netcode_klb_httpserve_conn(p_conn, close_status);
        return -1;
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

    if (NULL != p_httpserve->p_file)
    {
        return false;
    }

    return true;
}


/// @brief 设置读/写/idle 超时(毫秒; 负值钳为 0)
void klb_httpserve_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    klb_httpserve_conn_t* p_httpserve = NULL;

    assert(NULL != p_conn);
    p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_httpserve->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_httpserve->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}


/// @brief 设置请求体上限(字节; 负值钳为 0=不限)
void klb_httpserve_conn_set_max_body(klb_netconn_t* p_conn, int64_t max_body)
{
    klb_httpserve_conn_t* p_httpserve = NULL;

    assert(NULL != p_conn);
    p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    p_httpserve->max_body = (0 < max_body) ? max_body : 0;
}


/// @brief 复制读缓冲中尚未解析的尾部
static klb_buf_t* copy_tail_klb_httpserve_conn(klb_httpserve_conn_t* p_httpserve)
{
    klb_buf_t* p_buf = p_httpserve->p_read_buf;

    if (NULL == p_buf || p_buf->end <= p_buf->start)
    {
        return NULL;
    }

    int n = p_buf->end - p_buf->start;
    klb_buf_t* p_tail = klb_buf_malloc(n, false);
    memcpy(p_tail->p_buf, p_buf->p_buf + p_buf->start, (size_t)n);
    p_tail->start = 0;
    p_tail->end = n;
    return p_tail;
}


/// @brief 摘出 socket 与握手后剩余字节, 并销毁 HTTP 连接
int klb_httpserve_conn_take(klb_netconn_t* p_conn, klb_socket_t** pp_socket, klb_buf_t** pp_tail)
{
    klb_httpserve_conn_t* p_httpserve = NULL;

    if (NULL == p_conn || NULL == pp_socket || NULL == pp_tail)
    {
        return KLB_NETCODE_INVAL;
    }

    p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    *pp_socket = NULL;
    *pp_tail = NULL;

    if (!p_httpserve->upgrade_hold)
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    if (!klb_httpserve_wbuf_is_empty(p_conn))
    {
        return KLB_NETCODE_BUSY;
    }

    if (0 != klb_netmulti_remove(p_httpserve->p_netmulti, p_conn))
    {
        return KLB_NETCODE_ERR;
    }

    klb_buf_t* p_tail = copy_tail_klb_httpserve_conn(p_httpserve);
    klb_socket_t* p_socket = p_conn->p_socket;
    p_conn->p_socket = NULL;
    p_httpserve->p_netmulti = NULL;

    klb_netconn_bind_recv_data(p_conn, NULL);
    klb_netconn_set_udata(p_conn, NULL);
    klb_httpserve_conn_destroy(p_conn);

    *pp_socket = p_socket;
    *pp_tail = p_tail;
    return 0;
}


/// @brief 发送HTTP响应头 + 本地文件体
int klb_httpserve_conn_send_file(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len,
    const char* p_path, int64_t offset, int64_t length)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (NULL == p_path || 0 == p_path[0] || offset < 0 || length < 0)
    {
        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_httpserve->p_file)
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
            if (0 != seek_file_klb_httpserve_conn(p_file, offset))
            {
                fclose(p_file);
                return KLB_NETCODE_IO;
            }
        }
    }

    if (0 < head_len && NULL != p_head)
    {
        klb_httpserve_conn_send(p_conn, p_head, head_len, NULL, 0);
    }

    p_httpserve->p_file = p_file;
    p_httpserve->file_remain = length;
    klb_socket_set_writing(p_socket, true);

    return 0;
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
    p_httpserve->upgrade_hold = false;
    p_httpserve->header = NULL;
    p_httpserve->p_body = NULL;
    p_httpserve->p_file = NULL;
    p_httpserve->file_remain = 0;
    p_httpserve->read_timeout_ms = 0;
    p_httpserve->write_timeout_ms = 0;
    p_httpserve->idle_timeout_ms = 0;
    p_httpserve->max_body = 0;
    p_httpserve->body_too_large = false;

    return 0;
}

/// @brief 退出
static void klb_httpserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_httpserve_conn_t* p_httpserve = (klb_httpserve_conn_t*)p_conn->extra;

    close_file_klb_httpserve_conn(p_httpserve);

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
