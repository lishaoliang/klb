// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_http.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_list.h"
#include "klbmem/klb_rbuf.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbthird/sds.h"
#include "klbutil/klb_log.h"
#include "klbbase/klb_mnp.h"
#include <limits.h>
#include <assert.h>

typedef struct klb_http_t_
{
    // send发送相关
    struct
    {
        klb_list_t* p_w_list;           ///< klb_buf_t*
        klb_buf_t*  p_w_cur;            ///< 当前正在发送的缓存
        int         w_len;              ///< 当前等待发送的缓存量
    };

    // recv接收相关
    struct
    {
        klb_buf_t*  p_r_buf;            ///< 临时读取缓存
        klb_list_t* p_recv_list;        ///< 读取到的数据包列表
    };

    // http_parser
    struct
    {
        enum http_parser_type   parser_type;

        int                     parser_status;  ///< klua_khttp_parser_status_e
#define KLB_HTTP_PARSER_ERR                   (-1)
#define KLB_HTTP_PARSER                       0
#define KLB_HTTP_PARSER_CHUNK                 1
#define KLB_HTTP_PARSER_OVER                  2

        http_parser             parser;         ///< HTTP解析器
        http_parser_settings    settings;       ///< 解析器设置

        sds                     header;
        klb_buffer_t*           p_body;

        uint64_t                content_length; ///< 
    };
}klb_http_t;


//////////////////////////////////////////////////////////////////////////

static int on_message_begin_klb_http(http_parser* p_parser)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_message_begin\n");

    p_http->header = sdsnew(http_method_str(p_parser->method));
    p_http->header = sdscat(p_http->header, " ");

    return 0;
}

static int on_url_klb_http(http_parser* p_parser, const char* at, size_t length)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_url\n");

    // eg. GET /wsflv.html?a=1 HTTP/1.1
    p_http->header = sdscatlen(p_http->header, at, length);
    //p_http->header = sdscatfmt(p_http->header, " HTTP/%i.%i\r\n", 1, 1);
    p_http->header = sdscat(p_http->header, "\r\n");

    return 0;
}

static int on_status_klb_http(http_parser* p_parser, const char* at, size_t length)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_status\n");

    //sds code_str = NULL;
    //if (0 < length)
    //{
    //    code_str = sdsnewlen(at, length);
    //}
    //else
    //{
    //    code_str = sdsnew("");
    //}

    //KLB_FREE_BY(p_khttp->header_field, sdsfree);

    //// eg. "HTTP/1.1 200 OK"
    //p_khttp->header_field = sdsnew("");
    //p_khttp->header_field = sdscatfmt(p_khttp->header_field, "HTTP/%u.%u %u %S", p_parser->http_major, p_parser->http_minor, p_parser->status_code, code_str);

    //call_lua_reg_on_recv_klua_khttp(p_khttp, "http",
    //    p_khttp->header_field, sdslen(p_khttp->header_field),
    //    NULL, 0);

    //KLB_FREE_BY(code_str, sdsfree);
    return 0;
}

static int on_header_field_klb_http(http_parser* p_parser, const char* at, size_t length)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_header_field\n");

    p_http->header = sdscatlen(p_http->header, at, length);
    p_http->header = sdscat(p_http->header, ": ");

    return 0;
}

static int on_header_value_klb_http(http_parser* p_parser, const char* at, size_t length)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;

    p_http->header = sdscatlen(p_http->header, at, length);
    p_http->header = sdscat(p_http->header, "\r\n");

    return 0;
}

static int on_headers_complete_klb_http(http_parser* p_parser)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_headers_complete\n");

    if (ULLONG_MAX == p_parser->content_length)
    {
        p_http->content_length = 0;
    }
    else
    {
        p_http->content_length = p_parser->content_length;
    }

    p_http->header = sdscat(p_http->header, "\r\n");

    //call_lua_reg_on_recv_klua_khttp(p_khttp, "header_complete", NULL, 0, NULL, 0);

    return 0;
}

static int on_body_klb_http(http_parser* p_parser, const char* at, size_t length)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_body\n");

    //klb_buffer_write(p_khttp->p_body, at, length);

    return 0;
}

static int on_message_complete_klb_http(http_parser* p_parser)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_message_complete\n");

    p_http->parser_status = KLB_HTTP_PARSER_OVER;

    return 0;
}

static int on_chunk_header_klb_http(http_parser* p_parser)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_chunk_header\n");

    p_http->parser_status = KLB_HTTP_PARSER_CHUNK;

    return 0;
}

static int on_chunk_complete_klb_http(http_parser* p_parser)
{
    klb_http_t* p_http = (klb_http_t*)p_parser->data;
    //KLB_LOG("on_chunk_complete\n");

    if (0 == p_parser->content_length)
    {
        //KLB_LOG("on_chunk_header end\n");
        p_http->parser_status = KLB_HTTP_PARSER_OVER;
    }

    return 0;
}

static void klb_http_reset(klb_http_t* p_http)
{
    // 复位 http parser
    http_parser_init(&p_http->parser, p_http->parser_type/*HTTP_REQUEST*//*HTTP_RESPONSE*/);
    http_parser_settings_init(&p_http->settings);
    p_http->parser.data = p_http; // 回调函数绑定的用户字段

    p_http->settings.on_message_begin = on_message_begin_klb_http;
    p_http->settings.on_url = on_url_klb_http;
    p_http->settings.on_status = on_status_klb_http;
    p_http->settings.on_header_field = on_header_field_klb_http;
    p_http->settings.on_header_value = on_header_value_klb_http;
    p_http->settings.on_headers_complete = on_headers_complete_klb_http;
    p_http->settings.on_body = on_body_klb_http;
    p_http->settings.on_message_complete = on_message_complete_klb_http;
    p_http->settings.on_chunk_header = on_chunk_header_klb_http;
    p_http->settings.on_chunk_complete = on_chunk_complete_klb_http;

    // 
    p_http->parser_status = KLB_HTTP_PARSER;
    p_http->content_length = 0;

    KLB_FREE_BY(p_http->header, sdsfree);
    KLB_FREE_BY(p_http->p_body, klb_buffer_destroy);
}

klb_http_t* klb_http_create(enum http_parser_type parser_type)
{
    klb_http_t* p_http = KLB_MALLOCZ(klb_http_t, 1, 0);

    p_http->parser_type = parser_type;

    p_http->p_w_list = klb_list_create();
    p_http->p_recv_list = klb_list_create();

    p_http->p_r_buf = klb_buf_malloc(1024 * 16, false);

    klb_http_reset(p_http);

    return p_http;
}

void klb_http_destroy(klb_http_t* p_http)
{
    assert(NULL != p_http);

    KLB_FREE(p_http);
}

static void klb_http_push_pack(klb_http_t* p_http)
{
    klb_buf_t* p_data = NULL;
    int head_len = sdslen(p_http->header);
    int body_len = 0;

    if (NULL != p_http->p_body)
    {
        p_data = klb_buffer_join_offset(p_http->p_body, head_len + sizeof(klb_mnp_common_t), 0, NULL, NULL);
        klb_buffer_reset(p_http->p_body);

        body_len = p_data->end - p_data->start;
    }
    else
    {
        p_data = klb_buf_malloc(head_len + sizeof(klb_mnp_common_t), false);
    }

    klb_mnp_common_t com = { 0 };
    com.size = head_len + body_len + sizeof(klb_mnp_common_t);
    com.extra = head_len;

    memcpy(p_data->p_buf, &com, sizeof(klb_mnp_common_t));

    if (0 < head_len)
    {
        memcpy(p_data->p_buf + sizeof(klb_mnp_common_t), p_http->header, head_len);
    }

    p_data->start = 0;
    p_data->end = com.size;

    KLB_FREE_BY(p_http->header, sdsfree);

    klb_list_push_tail(p_http->p_recv_list, p_data);
}

static int klb_http_push_data(klb_http_t* p_http, const uint8_t* p_data, int data_len)
{
    if (NULL != p_data && 0 < data_len)
    {
        klb_buf_t* p_buf = klb_buf_malloc(data_len, false);

        memcpy(p_buf->p_buf, p_data, data_len);
        p_buf->end = data_len;

        klb_list_push_tail(p_http->p_w_list, p_buf);
    }

    return 0;
}

static int parser_klb_http(klb_http_t* p_http)
{
    klb_buf_t* p_buf = p_http->p_r_buf;

    size_t parser = http_parser_execute(&p_http->parser, &p_http->settings, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

    if (0 == p_http->parser.http_errno)
    {
        if (0 < parser)
        {
            p_buf->start += parser;

            if (KLB_HTTP_PARSER_OVER == p_http->parser_status)
            {
                //
                klb_http_push_pack(p_http);

                // 复位
                klb_http_reset(p_http);
            }

            if (p_buf->end <= p_buf->start)
            {
                p_buf->start = 0;
                p_buf->end = 0;
            }
            else
            {
                memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
                p_buf->end = p_buf->end - p_buf->start;
                p_buf->start = 0;
            }
        }
    }
    else
    {
        p_http->parser_status = KLB_HTTP_PARSER_ERR;
    }

    return 0;
}

int klb_http_init(klb_http_t* p_http, const uint8_t* p_data, int data_len)
{
    assert(data_len < p_http->p_r_buf->buf_len);

    klb_buf_t* p_buf = p_http->p_r_buf;

    memcpy(p_buf->p_buf + p_buf->end, p_data, data_len);
    p_buf->end += data_len;
    assert(p_buf->end <= p_buf->buf_len);

    parser_klb_http(p_http);

    return 0;
}

/// @brief 放入发送数据
int klb_http_push(klb_http_t* p_http, klb_socket_t* p_socket, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    int len = head_len + body_len;

    klb_http_push_data(p_http, p_head, head_len);
    klb_http_push_data(p_http, p_body, body_len);

    if (0 < len)
    {
        klb_socket_set_writing(p_socket, true);
    }

    return len;
}

/// @brief 发送数据
int klb_http_send(klb_http_t* p_http, klb_socket_t* p_socket)
{
    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    if (NULL == p_http->p_w_cur && 0 < klb_list_size(p_http->p_w_list))
    {
        p_http->p_w_cur = (klb_buf_t*)klb_list_pop_head(p_http->p_w_list);
    }

    int ret = 0;

    klb_buf_t* p_buf = p_http->p_w_cur;
    if (NULL != p_buf)
    {
        int len = p_buf->end - p_buf->start;
        if (0 < len)
        {
            int s = klb_socket_send(p_socket, p_buf->p_buf + p_buf->start, len);
            if (0 < s)
            {
                p_buf->start += s;
                ret += s;

                if (p_buf->end <= p_buf->start)
                {
                    p_http->p_w_cur = NULL;
                    KLB_FREE(p_buf);
                }
            }
        }
        else
        {
            assert(false);
            p_http->p_w_cur = NULL;
            KLB_FREE(p_buf);
        }
    }
    else
    {
        klb_socket_set_writing(p_socket, false);
    }

    return ret;
}

int klb_http_recv(klb_http_t* p_http, klb_socket_t* p_socket)
{
    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_buf_t* p_buf = p_http->p_r_buf;

    int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

    if (0 < r)
    {
        p_buf->end += r;

        parser_klb_http(p_http);
    }

    return r;
}

int klb_http_get(klb_http_t* p_http, klb_buf_t** p_data)
{
    if (0 < klb_list_size(p_http->p_recv_list))
    {
        *p_data = (klb_buf_t*)klb_list_pop_head(p_http->p_recv_list);
        return 0;
    }

    return 1;
}
