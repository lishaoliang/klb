// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmnp/core/klb_mnpclient_conn.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include <assert.h>


/// @struct klb_mnpclient_conn_t
/// @brief  MNP客户端连接
typedef struct klb_mnpclient_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送列表
    };

    struct
    {
        klb_buf_t*                  p_read_buf;         ///< 读取缓存
        klb_mnp_t                   mnp;                ///< 当前帧头
        int                         left_len;           ///< 当前帧剩余载荷
        klb_buffer_t*               p_txt;              ///< TEXT/BINARY 重组
        klb_buffer_t*               p_media;            ///< MEDIA 重组
    };
}klb_mnpclient_conn_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_mnpclient_conn_quit(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数

static void push_data_klb_mnpclient_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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

static void push_netcode_klb_mnpclient_conn(klb_netconn_t* p_conn, int code)
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

static void on_connect_timeout_klb_mnpclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
}

static void on_connected_klb_mnpclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_CONNECT);
}

static void on_ticker_klb_mnpclient_conn(klb_netconn_t* p_conn, int64_t now)
{

}

static int enqueue_mnp_payload_klb_mnpclient_conn(klb_netconn_t* p_conn, int packtype, const uint8_t* p_payload, int payload_len)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (payload_len < 0)
    {
        payload_len = 0;
    }

    int chunk_max = (int)KLB_MNP_BLOCK_SIZE_MAX - (int)sizeof(klb_mnp_t);
    if (chunk_max <= 0)
    {
        return 1;
    }

    int off = 0;
    bool first = true;

    if (0 == payload_len)
    {
        klb_mnp_t mnp = { 0 };
        mnp.magic = KLB_MNP_MAGIC;
        mnp.size = (uint16_t)sizeof(klb_mnp_t);
        mnp.opt = KLB_MNP_FULL;
        mnp.packtype = (uint8_t)packtype;

        klb_buf_t* p_buf = klb_buf_malloc((int)sizeof(klb_mnp_t), false);
        klb_buf_write(p_buf, (const char*)&mnp, (int)sizeof(mnp));
        klb_nlist_push_tail(p_mnpclient->p_write_nlist, p_buf);
        klb_socket_set_writing(p_socket, true);
        return 0;
    }

    while (off < payload_len)
    {
        int remain = payload_len - off;
        int chunk = remain;
        if (chunk_max < chunk)
        {
            chunk = chunk_max;
        }

        uint8_t opt = KLB_MNP_FULL;
        if (chunk_max < payload_len)
        {
            if (first)
            {
                opt = KLB_MNP_BEGIN;
            }
            else if (remain <= chunk_max)
            {
                opt = KLB_MNP_END;
            }
            else
            {
                opt = KLB_MNP_CONTINUE;
            }
        }

        klb_mnp_t mnp = { 0 };
        mnp.magic = KLB_MNP_MAGIC;
        mnp.size = (uint16_t)(sizeof(klb_mnp_t) + chunk);
        mnp.opt = opt;
        mnp.packtype = (uint8_t)packtype;

        klb_buf_t* p_buf = klb_buf_malloc((int)sizeof(klb_mnp_t) + chunk, false);
        klb_buf_write(p_buf, (const char*)&mnp, (int)sizeof(mnp));
        klb_buf_write(p_buf, (const char*)(p_payload + off), chunk);
        klb_nlist_push_tail(p_mnpclient->p_write_nlist, p_buf);

        off += chunk;
        first = false;
    }

    klb_socket_set_writing(p_socket, true);
    return 0;
}

static void send_pong_klb_mnpclient_conn(klb_netconn_t* p_conn)
{
    enqueue_mnp_payload_klb_mnpclient_conn(p_conn, KLB_MNP_PONG, NULL, 0);
}

static void finish_pack_klb_mnpclient_conn(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    int packtype = p_mnpclient->mnp.packtype;

    if (KLB_MNP_TEXT == packtype || KLB_MNP_BINARY == packtype)
    {
        if (NULL == p_mnpclient->p_txt || 0 >= klb_buffer_datalen(p_mnpclient->p_txt))
        {
            return;
        }

        klb_buf_t* p_data = klb_buffer_join(p_mnpclient->p_txt, NULL, NULL);
        klb_buffer_reset(p_mnpclient->p_txt);
        push_data_klb_mnpclient_conn(p_conn, packtype, p_data);
    }
    else if (KLB_MNP_MEDIA == packtype)
    {
        if (NULL == p_mnpclient->p_media || 0 >= klb_buffer_datalen(p_mnpclient->p_media))
        {
            return;
        }

        klb_buf_t* p_data = klb_buffer_join(p_mnpclient->p_media, NULL, NULL);
        klb_buffer_reset(p_mnpclient->p_media);
        push_data_klb_mnpclient_conn(p_conn, packtype, p_data);
    }
}

static void compact_read_klb_mnpclient_conn(klb_mnpclient_conn_t* p_mnpclient)
{
    klb_buf_t* p_buf = p_mnpclient->p_read_buf;

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

static int parser_klb_mnpclient_conn(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_mnpclient->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        int data_len = p_buf->end - p_buf->start;

        if (p_mnpclient->left_len <= 0)
        {
            if (data_len < (int)sizeof(klb_mnp_t))
            {
                break;
            }

            klb_mnp_t mnp = { 0 };
            memcpy(&mnp, p_buf->p_buf + p_buf->start, sizeof(klb_mnp_t));
            p_buf->start += (int)sizeof(klb_mnp_t);

            if (KLB_MNP_MAGIC != mnp.magic)
            {
                return -1;
            }

            if (mnp.size < sizeof(klb_mnp_t) || KLB_MNP_BLOCK_SIZE_MAX < mnp.size)
            {
                return -1;
            }

            p_mnpclient->mnp = mnp;
            p_mnpclient->left_len = (int)mnp.size - (int)sizeof(klb_mnp_t);

            if (KLB_MNP_PING == mnp.packtype)
            {
                send_pong_klb_mnpclient_conn(p_conn);
            }

            if (p_mnpclient->left_len <= 0)
            {
                if (KLB_MNP_FULL == mnp.opt || KLB_MNP_END == mnp.opt)
                {
                    if (KLB_MNP_TEXT == mnp.packtype ||
                        KLB_MNP_BINARY == mnp.packtype ||
                        KLB_MNP_MEDIA == mnp.packtype)
                    {
                        finish_pack_klb_mnpclient_conn(p_conn);
                    }
                }

                p_mnpclient->left_len = 0;
            }
            else if (KLB_MNP_BEGIN == mnp.opt || KLB_MNP_FULL == mnp.opt)
            {
                if (KLB_MNP_TEXT == mnp.packtype || KLB_MNP_BINARY == mnp.packtype)
                {
                    if (NULL != p_mnpclient->p_txt)
                    {
                        klb_buffer_reset(p_mnpclient->p_txt);
                    }
                }
                else if (KLB_MNP_MEDIA == mnp.packtype)
                {
                    if (NULL != p_mnpclient->p_media)
                    {
                        klb_buffer_reset(p_mnpclient->p_media);
                    }
                }
            }

            continue;
        }

        int r_len = MIN(p_mnpclient->left_len, data_len);
        int packtype = p_mnpclient->mnp.packtype;

        if (KLB_MNP_TEXT == packtype || KLB_MNP_BINARY == packtype)
        {
            if (NULL == p_mnpclient->p_txt)
            {
                p_mnpclient->p_txt = klb_buffer_create(4096);
            }

            klb_buffer_write(p_mnpclient->p_txt, p_buf->p_buf + p_buf->start, r_len);
        }
        else if (KLB_MNP_MEDIA == packtype)
        {
            if (NULL == p_mnpclient->p_media)
            {
                p_mnpclient->p_media = klb_buffer_create(4096);
            }

            klb_buffer_write(p_mnpclient->p_media, p_buf->p_buf + p_buf->start, r_len);
        }

        p_buf->start += r_len;
        p_mnpclient->left_len -= r_len;

        if (p_mnpclient->left_len <= 0)
        {
            if (KLB_MNP_FULL == p_mnpclient->mnp.opt || KLB_MNP_END == p_mnpclient->mnp.opt)
            {
                if (KLB_MNP_TEXT == packtype ||
                    KLB_MNP_BINARY == packtype ||
                    KLB_MNP_MEDIA == packtype)
                {
                    finish_pack_klb_mnpclient_conn(p_conn);
                }
            }

            p_mnpclient->left_len = 0;
        }
    }

    compact_read_klb_mnpclient_conn(p_mnpclient);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klb_mnpclient_conn_destroy(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    assert(NULL == p_mnpclient->p_netmulti);

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    klb_mnpclient_conn_quit(p_conn);
    KLB_FREE(p_conn);
}

static int klb_mnpclient_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

static int klb_mnpclient_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_mnpclient_conn_send(p_conn, packtype, sequence, p_head, head_len, p_body, body_len);
}

static int klb_mnpclient_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    int data_len = klb_buf_data_len(p_data);

    enqueue_mnp_payload_klb_mnpclient_conn(p_conn, KLB_MNP_MEDIA,
        (const uint8_t*)(p_data->p_buf + p_data->start), data_len);
    KLB_FREE_BY(p_data, klb_buf_unref);

    return 0;
}

static int klb_mnpclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_mnpclient->p_write_nlist;
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
        push_netcode_klb_mnpclient_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

static int do_recv_klb_mnpclient_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_mnpclient->p_read_buf;

    int idle_len = klb_buf_idle_len(p_buf);
    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_mnpclient_conn(p_conn);
        if (0 != ret_parse)
        {
            push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            push_netcode_klb_mnpclient_conn(p_conn, KLB_NETCODE_RBUF_FULL);
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

    int ret_parse = parser_klb_mnpclient_conn(p_conn);
    if (0 != ret_parse)
    {
        push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        return -1;
    }

    if (0 == recv_len)
    {
        return 1;
    }

    return 0;
}

static int klb_mnpclient_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_mnpclient_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

static int klb_mnpclient_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_connect_timeout_klb_mnpclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_connected:
        on_connected_klb_mnpclient_conn(p_conn, now);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_mnpclient_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 导出函数

void klb_mnpclient_conn_free(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_mnpclient->p_netmulti, p_conn);
    p_mnpclient->p_netmulti = NULL;
}

int klb_mnpclient_conn_send(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    if (head_len < 0)
    {
        head_len = 0;
    }

    if (body_len < 0)
    {
        body_len = 0;
    }

    if (KLB_MNP_PING == packtype || KLB_MNP_PONG == packtype)
    {
        return enqueue_mnp_payload_klb_mnpclient_conn(p_conn, packtype, NULL, 0);
    }

    if (KLB_MNP_TEXT != packtype && KLB_MNP_BINARY != packtype)
    {
        return 1;
    }

    int payload_len = (int)sizeof(klb_mnp_text_t) + head_len + body_len;
    klb_buf_t* p_payload = klb_buf_malloc(payload_len, false);

    klb_mnp_text_t txt = { 0 };
    txt.size = (uint32_t)payload_len;
    txt.encode = KLB_MNP_ENCODE_NULL;
    txt.sequence = (uint32_t)sequence;
    txt.head_size = (uint32_t)head_len;

    klb_buf_write(p_payload, (const char*)&txt, (int)sizeof(txt));
    if (0 < head_len && NULL != p_head)
    {
        klb_buf_write(p_payload, (const char*)p_head, head_len);
    }

    if (0 < body_len && NULL != p_body)
    {
        klb_buf_write(p_payload, (const char*)p_body, body_len);
    }

    int ret = enqueue_mnp_payload_klb_mnpclient_conn(p_conn, packtype,
        (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
    KLB_FREE_BY(p_payload, klb_buf_unref);

    return ret;
}

int klb_mnpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    klb_nlist_push_tail(p_mnpclient->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);

    return 0;
}

bool klb_mnpclient_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_mnpclient->p_write_nlist))
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_mnpclient_conn_init(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    p_conn->vtable.destroy = NULL;
    p_conn->vtable.recv_data = NULL;
    p_conn->vtable.ioctrl = klb_mnpclient_conn_ioctrl;
    p_conn->vtable.send_normal = klb_mnpclient_conn_send_normal;
    p_conn->vtable.send_media = klb_mnpclient_conn_send_media;
    p_conn->vtable.on_send = klb_mnpclient_conn_on_send;
    p_conn->vtable.on_recv = klb_mnpclient_conn_on_recv;
    p_conn->vtable.on_msg = klb_mnpclient_conn_on_msg;

    p_conn->protocol = KLB_PROTOCOL_MNP;

    p_mnpclient->p_write_nlist = klb_nlist_create();
    p_mnpclient->p_read_buf = klb_buf_malloc((int)KLB_MNP_BLOCK_SIZE_MAX, false);
    p_mnpclient->left_len = 0;
    p_mnpclient->p_txt = NULL;
    p_mnpclient->p_media = NULL;

    return 0;
}

static void klb_mnpclient_conn_quit(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_mnpclient->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_mnpclient->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_mnpclient->p_txt, klb_buffer_destroy);
    KLB_FREE_BY(p_mnpclient->p_media, klb_buffer_destroy);
    KLB_FREE_BY(p_mnpclient->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_mnpclient->p_write_nlist, klb_nlist_destroy);
}


//////////////////////////////////////////////////////////////////////////

klb_netconn_t* klb_mnpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port)
{
    klb_socket_fd fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        return NULL;
    }

    klb_socket_t* p_socket = klb_socket_async_create(fd);
    klb_netconn_t* p_netconn = klb_mnpclient_conn_create(p_netmulti, p_socket);

    klb_netmulti_push(p_netmulti, p_netconn);

    return p_netconn;
}

klb_netconn_t* klb_mnpclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_mnpclient_conn_t));
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    klb_mnpclient_conn_init(p_conn);
    p_conn->vtable.destroy = klb_mnpclient_conn_destroy;

    p_mnpclient->p_netmulti = p_netmulti;
    p_conn->p_socket = p_socket;
    klb_socket_set_reading(p_socket, true);

    return p_conn;
}

// end
