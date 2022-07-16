// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbnet/klb_ncm.h"
#include "klbthird/http_parser.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_list.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbthird/sds.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef enum klb_ncm_ops_http_flv_parser_status_e_
{
    KLB_NCM_HTTP_FLV_PARSER_ERR = -1,
    KLB_NCM_HTTP_FLV_PARSER = 0,
    KLB_NCM_HTTP_FLV_PARSER_CHUNK = 1,
    KLB_NCM_HTTP_FLV_PARSER_OVER = 2,
}klb_ncm_ops_http_flv_parser_status_e;


typedef struct klb_ncm_ops_http_flv_t_
{
    klb_ncm_ops_lparam_t    lparam;     ///< ncm 关键参数1
    klb_ncm_ops_wparam_t    wparam;     ///< ncm 辅助参数2

    // send发送相关
    struct
    {
        klb_list_t*         p_w_list;   ///< 待发送列表: klb_buf_t*
        klb_buf_t*          p_w_cur;    ///< 当前正在发送的缓存
        int                 w_start;    ///< 当前发送的起始点
    };

    // recv接收相关
    struct
    {
        klb_buf_t*          p_r_buf;    ///< 临时读取缓存
    };

    // http 解析
    struct
    {
        int                     parser_status;  ///< klb_ncm_ops_http_flv_parser_status_e

        http_parser             parser;         ///< HTTP解析器
        http_parser_settings    settings;       ///< 解析器设置

        sds                     url;
    };
}klb_ncm_ops_http_flv_t;


//////////////////////////////////////////////////////////////////////////

static int on_message_klb_ncm_ops_http_flv(http_parser* p_parser)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;


    return 0;
}

static int on_url_klb_ncm_ops_http_flv(http_parser* p_parser, const char* at, size_t length)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;

    KLB_FREE(p_ops->url);
    p_ops->url = sdsnewlen(at, length);

    return 0;
}

static int on_status_klb_ncm_ops_http_flv(http_parser* p_parser, const char* at, size_t length)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;

    return 0;
}

static int on_header_field_klb_ncm_ops_http_flv(http_parser* p_parser, const char* at, size_t length)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;

    return 0;
}

static int on_header_value_klb_ncm_ops_http_flv(http_parser* p_parser, const char* at, size_t length)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;
    
    return 0;
}

static int on_headers_complete_klb_ncm_ops_http_flv(http_parser* p_parser)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;
    
    return 0;
}

static int on_body_klb_ncm_ops_http_flv(http_parser* p_parser, const char* at, size_t length)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;
   
    return 0;
}

static int on_message_complete_klb_ncm_ops_http_flv(http_parser* p_parser)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;
   
    p_ops->parser_status = KLB_NCM_HTTP_FLV_PARSER_OVER;

    return 0;
}

static int on_chunk_header_klb_ncm_ops_http_flv(http_parser* p_parser)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;

    p_ops->parser_status = KLB_NCM_HTTP_FLV_PARSER_CHUNK;

    return 0;
}

static int on_chunk_complete_klb_ncm_ops_http_flv(http_parser* p_parser)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)p_parser->data;

    if (0 == p_parser->content_length)
    {
        p_ops->parser_status = KLB_NCM_HTTP_FLV_PARSER_OVER;
    }

    return 0;
}

static int klb_ncm_ops_http_flv_parse(klb_ncm_ops_http_flv_t* p_ops, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_ops->p_r_buf;

    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    int parser = http_parser_execute(&p_ops->parser, &p_ops->settings, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

    if (0 == p_ops->parser.http_errno)
    {
        if (0 < parser)
        {
            p_buf->start += parser;

            if (KLB_NCM_HTTP_FLV_PARSER_OVER == p_ops->parser_status)
            {

            }
        }
    }
    else
    {
        p_ops->parser_status = KLB_NCM_HTTP_FLV_PARSER_ERR;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建连接
/// @param [in] *p_ncm          ncm模块
/// @return void* 连接的指针
static void* create_klb_ncm_ops_http_flv(klb_ncm_ops_lparam_t* p_lparam, klb_ncm_ops_wparam_t* p_wparam)
{
    klb_ncm_ops_http_flv_t* p_ops = KLB_MALLOCZ(klb_ncm_ops_http_flv_t, 1, 0);

    // ncm 参数
    memcpy(&p_ops->lparam, p_lparam, sizeof(klb_ncm_ops_lparam_t));
    memcpy(&p_ops->wparam, p_wparam, sizeof(klb_ncm_ops_wparam_t));

    // send发送相关
    p_ops->p_w_list = klb_list_create();

    // recv接收相关
    p_ops->p_r_buf = klb_buf_malloc(p_ops->wparam.read_buffer_size, false);

    p_ops->url = sdsnew("");

    // http parser
    http_parser_init(&p_ops->parser, HTTP_REQUEST);
    http_parser_settings_init(&p_ops->settings);

    p_ops->parser.data = p_ops; // 回调函数绑定的用户字段

    p_ops->settings.on_message_begin = on_message_klb_ncm_ops_http_flv;
    p_ops->settings.on_url = on_url_klb_ncm_ops_http_flv;
    p_ops->settings.on_status = on_status_klb_ncm_ops_http_flv;
    p_ops->settings.on_header_field = on_header_field_klb_ncm_ops_http_flv;
    p_ops->settings.on_header_value = on_header_value_klb_ncm_ops_http_flv;
    p_ops->settings.on_headers_complete = on_headers_complete_klb_ncm_ops_http_flv;
    p_ops->settings.on_body = on_body_klb_ncm_ops_http_flv;
    p_ops->settings.on_message_complete = on_message_complete_klb_ncm_ops_http_flv;
    p_ops->settings.on_chunk_header = on_chunk_header_klb_ncm_ops_http_flv;
    p_ops->settings.on_chunk_complete = on_chunk_complete_klb_ncm_ops_http_flv;

    return p_ops;
}

/// @brief 销毁连接
/// @param [in] *ptr            连接的指针
/// @return 无
static void destroy_klb_ncm_ops_http_flv(void* ptr)
{
    klb_ncm_ops_http_flv_t* p_opt = (klb_ncm_ops_http_flv_t*)ptr;

    // w
    while (0 < klb_list_size(p_opt->p_w_list))
    {
        klb_buf_t* p_tmp = klb_list_pop_head(p_opt->p_w_list);
        klb_buf_unref_next(p_tmp);
    }

    if (NULL != p_opt->p_w_cur)
    {
        klb_buf_unref_next(p_opt->p_w_cur);
        p_opt->p_w_cur = NULL;
    }

    // 销毁
    KLB_FREE_BY(p_opt->p_w_list, klb_list_destroy);
    KLB_FREE(p_opt->p_r_buf);
    KLB_FREE(p_opt->url);
    KLB_FREE(p_opt);
}


static int init_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    klb_buf_t* p_buf = p_ops->p_r_buf;
    assert(data_len <= p_buf->buf_len);

    memcpy(p_buf->p_buf, p_data, data_len);
    p_buf->end = data_len;


    while (true)
    {
        if (0 != klb_ncm_ops_http_flv_parse(p_ops, p_socket))
        {
            break;
        }
    }

    return 0;
}

/// @brief 主动发送文本数据
/// @param [in] *ptr            连接的指针
/// @return int
static int send_text_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    int totol_len =  extra_len + data_len;

    klb_buf_t* p_buf = p_ops->lparam.cb_malloc(p_ops->lparam.p_pool, KLB_ALIGNED_4(totol_len));

    int offset = 0;
    if (0 < extra_len)
    {
        memcpy(p_buf->p_buf + offset, p_extra, extra_len);
        offset += extra_len;
    }

    if (0 < data_len)
    {
        memcpy(p_buf->p_buf + offset + extra_len, p_data, data_len);
        offset += data_len;
    }

    p_buf->end = totol_len;

    klb_list_push_tail(p_ops->p_w_list, p_buf);

    klb_socket_set_writing(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 主动发送二进制数据
/// @param [in] *ptr            连接的指针
/// @return int
static int send_binary_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    return 0;
}

/// @brief 主动发送媒体数据
/// @param [in] *ptr            连接的指针
/// @return int
static int send_media_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    //klb_buf_ref_next(p_data);

    //klb_list_push_tail(p_ops->p_w_list, p_data);
    //klb_socket_set_writing(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_send_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;
    bool first = true;

    while (true)
    {
        if (NULL == p_ops->p_w_cur && 0 < klb_list_size(p_ops->p_w_list))
        {
            p_ops->p_w_cur = (klb_buf_t*)klb_list_pop_head(p_ops->p_w_list);
            if (NULL != p_ops->p_w_cur)
            {
                p_ops->w_start = p_ops->p_w_cur->start;
            }
        }

        klb_buf_t* p_buf = p_ops->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, p_buf->p_buf + p_ops->w_start, p_buf->end - p_ops->w_start);
        if (0 < w)
        {
            p_ops->w_start += w;
            send += w;

            if (p_buf->end <= p_ops->w_start)
            {
                p_ops->p_w_cur = p_buf->p_next;
                if (NULL != p_ops->p_w_cur)
                {
                    p_ops->w_start = p_ops->p_w_cur->start;
                }

                klb_buf_unref(p_buf);
            }
        }
        else
        {
            if (first)
            {
                // select成功, 却第一次无法写, 表明对方已经断开
                err = KLB_SOCKET_DISCONNECT;
            }

            break; // 无法再发送
        }

        first = false;
    }

    if (NULL == p_ops->p_w_cur && klb_list_size(p_ops->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        //p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return send;
}

/// @brief 当网络上可以接收数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_recv_klb_ncm_ops_http_flv(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_http_flv_t* p_ops = (klb_ncm_ops_http_flv_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int recv = 0;
    bool first = true;

    while (true)
    {
        klb_buf_t* p_buf = p_ops->p_r_buf;

        int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            while (true)
            {
                if (0 != klb_ncm_ops_http_flv_parse(p_ops, p_socket))
                {
                    break;
                }
            }

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
        else
        {
            if (first)
            {
                // select成功, 却第一次读不到数据, 表明对方已断开 
                err = KLB_SOCKET_DISCONNECT;
            }
            break; // 无法再接收
        }

        first = false;
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        //p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return recv;
}

//////////////////////////////////////////////////////////////////////////

int klb_ncm_register_ops_http_flv(klb_ncm_t* p_ncm, int protocol)
{
    klb_ncm_ops_t ops = { 0 };

    // 创建/销毁
    ops.cb_create = create_klb_ncm_ops_http_flv;
    ops.cb_destroy = destroy_klb_ncm_ops_http_flv;

    ops.cb_init = init_klb_ncm_ops_http_flv;
    ops.cb_ctrl = NULL;

    //ops.cb_send_text = send_text_klb_ncm_ops_http_flv;
    //ops.cb_send_binary = send_binary_klb_ncm_ops_http_flv;
    ops.cb_send_media = send_media_klb_ncm_ops_http_flv;

    ops.on_send = on_send_klb_ncm_ops_http_flv;
    ops.on_recv = on_recv_klb_ncm_ops_http_flv;

    int ret = klb_ncm_register(p_ncm, protocol, &ops);
    assert(0 == ret);

    return ret;
}
