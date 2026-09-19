// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsmp/core/klb_smpserve_conn.h"
#include "klbsmp/core/klb_smpparser.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_smp.h"
#include "klbbase/klb_mnp.h"
#include <string.h>
#include <assert.h>


/// @struct klb_smpserve_conn_t
/// @brief  SMP服务连接
typedef struct klb_smpserve_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送列表
    };

    struct
    {
        int                         parser_status;      ///< klb_smpparser_status_e
        klb_smpparser_t             parser;             ///< 解析器
        klb_rbuf_t*                 p_read_head;        ///< 头部缓存
        klb_buf_t*                  p_read_body;        ///< 正在读取的载荷
        int                         reading_len;        ///< 剩余载荷
    };
}klb_smpserve_conn_t;


//////////////////////////////////////////////////////////////////////////
static void klb_smpserve_conn_quit(klb_netconn_t* p_conn);


static void push_data_klb_smpserve_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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

static void push_netcode_klb_smpserve_conn(klb_netconn_t* p_conn, int code)
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

static klb_buf_t* make_payload_klb_smpserve_conn(int packtype, klb_buf_t* p_raw)
{
    if (KLB_MNP_TEXT == packtype || KLB_MNP_BINARY == packtype)
    {
        int data_len = klb_buf_data_len(p_raw);
        if (data_len < (int)sizeof(klb_mnp_text_t))
        {
            return p_raw;
        }

        klb_mnp_text_t* p_txt = (klb_mnp_text_t*)(p_raw->p_buf + p_raw->start);
        int head_len = (int)p_txt->head_size;
        int body_len = (int)p_txt->size - head_len - (int)sizeof(klb_mnp_text_t);
        if (body_len < 0)
        {
            body_len = 0;
        }

        int out_len = head_len + body_len;
        klb_buf_t* p_out = klb_buf_malloc(out_len, false);
        const char* p_head = (const char*)(p_txt + 1);
        if (0 < out_len)
        {
            klb_buf_write(p_out, p_head, out_len);
        }

        KLB_FREE_BY(p_raw, klb_buf_unref);
        return p_out;
    }

    return p_raw;
}

static int enqueue_smp_payload_klb_smpserve_conn(klb_netconn_t* p_conn, int packtype, const uint8_t* p_payload, int payload_len)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (payload_len < 0)
    {
        payload_len = 0;
    }

    klb_smp_t smp = { 0 };
    smp.magic = KLB_SMP_MAGIC;
    smp.size = (uint32_t)(sizeof(klb_smp_t) + payload_len);
    smp.packtype = (uint32_t)packtype;

    klb_buf_t* p_buf = klb_buf_malloc((int)smp.size, false);
    klb_buf_write(p_buf, (const char*)&smp, (int)sizeof(smp));
    if (0 < payload_len && NULL != p_payload)
    {
        klb_buf_write(p_buf, (const char*)p_payload, payload_len);
    }

    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_buf);
    klb_socket_set_writing(p_socket, true);
    return 0;
}

static void send_pong_klb_smpserve_conn(klb_netconn_t* p_conn)
{
    enqueue_smp_payload_klb_smpserve_conn(p_conn, KLB_MNP_PONG, NULL, 0);
}


//////////////////////////////////////////////////////////////////////////

static void klb_smpserve_conn_destroy(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    if (NULL != p_smpserve->p_netmulti)
    {
        klb_netmulti_remove(p_smpserve->p_netmulti, p_conn);
        p_smpserve->p_netmulti = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    klb_smpserve_conn_quit(p_conn);
    KLB_FREE(p_conn);
}

static int klb_smpserve_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

static int klb_smpserve_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return klb_smpserve_conn_send(p_conn, packtype, sequence, p_head, head_len, p_body, body_len);
}

static int klb_smpserve_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    int data_len = klb_buf_data_len(p_data);

    enqueue_smp_payload_klb_smpserve_conn(p_conn, KLB_MNP_MEDIA,
        (const uint8_t*)(p_data->p_buf + p_data->start), data_len);
    KLB_FREE_BY(p_data, klb_buf_unref);
    return 0;
}

static int klb_smpserve_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_smpserve->p_write_nlist;
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
        push_netcode_klb_smpserve_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

static int do_recv_klb_smpserve_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SMPPARSER_head == p_smpserve->parser_status)
    {
        klb_rbuf_t* p_head = p_smpserve->p_read_head;

        int idle_len = 0;
        char* p_idle = klb_rbuf_idle(p_head, 0, &idle_len);
        int recv_len = klb_socket_recv(p_socket, (uint8_t*)p_idle, idle_len);

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_head, recv_len);
            *p_read_num += recv_len;
        }
        else if (0 == recv_len)
        {
            return 1;
        }
        else
        {
            return -1;
        }

        int data_len = 0;
        char* p_data = klb_rbuf_data(p_head, &data_len);

        klb_smpparser_t parser = { 0 };
        int ret_parse = klb_smpparser_parse(&parser, p_data, data_len);

        if (0 == ret_parse)
        {
            klb_rbuf_use(p_head, parser.head_len);

            if (KLB_MNP_PING == parser.packtype)
            {
                send_pong_klb_smpserve_conn(p_conn);
                p_smpserve->parser_status = KLB_SMPPARSER_head;
            }
            else if (KLB_MNP_PONG == parser.packtype)
            {
                p_smpserve->parser_status = KLB_SMPPARSER_head;
            }
            else if (KLB_MNP_TEXT == parser.packtype ||
                     KLB_MNP_BINARY == parser.packtype ||
                     KLB_MNP_MEDIA == parser.packtype)
            {
                int body_need = parser.pack_len - parser.head_len;
                if (body_need < 0)
                {
                    body_need = 0;
                }

                klb_buf_t* p_buf = klb_buf_malloc(body_need, false);

                int body_len = 0;
                char* p_body = klb_rbuf_data(p_head, &body_len);
                int cp_len = MIN(body_need, body_len);
                if (0 < cp_len)
                {
                    klb_buf_write(p_buf, p_body, cp_len);
                    klb_rbuf_use(p_head, cp_len);
                    body_need -= cp_len;
                }

                if (0 < body_need)
                {
                    p_smpserve->p_read_body = p_buf;
                    p_smpserve->reading_len = body_need;
                    p_smpserve->parser = parser;
                    p_smpserve->parser_status = KLB_SMPPARSER_body;
                }
                else
                {
                    p_smpserve->parser_status = KLB_SMPPARSER_head;
                    p_buf = make_payload_klb_smpserve_conn(parser.packtype, p_buf);
                    push_data_klb_smpserve_conn(p_conn, parser.packtype, p_buf);
                }
            }
            else
            {
                p_smpserve->parser_status = KLB_SMPPARSER_head;
            }

            klb_rbuf_memmove(p_head);
        }
        else if (0 < ret_parse)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else if (KLB_SMPPARSER_body == p_smpserve->parser_status)
    {
        klb_buf_t* p_buf = p_smpserve->p_read_body;
        int idle_len = MIN(p_smpserve->reading_len, klb_buf_idle_len(p_buf));
        char* p_idle = p_buf->p_buf + p_buf->end;

        int recv_len = klb_socket_recv(p_socket, (uint8_t*)p_idle, idle_len);
        if (0 < recv_len)
        {
            p_buf->end += recv_len;
            p_smpserve->reading_len -= recv_len;
            *p_read_num += recv_len;

            if (p_smpserve->reading_len <= 0)
            {
                p_smpserve->p_read_body = NULL;
                p_smpserve->parser_status = KLB_SMPPARSER_head;

                p_buf = make_payload_klb_smpserve_conn(p_smpserve->parser.packtype, p_buf);
                push_data_klb_smpserve_conn(p_conn, p_smpserve->parser.packtype, p_buf);
            }
        }
        else if (0 == recv_len)
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

static int klb_smpserve_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    int read_num = 0;

    while (true)
    {
        int ret = do_recv_klb_smpserve_conn(p_conn, &read_num);

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

static int klb_smpserve_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        break;

    case KLB_NETCONN_MSG_connected:
        break;

    case KLB_NETCONN_MSG_onticker:
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////

void klb_smpserve_conn_free(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_smpserve->p_netmulti, p_conn);
    p_smpserve->p_netmulti = NULL;
}

int klb_smpserve_conn_send(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
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
        return enqueue_smp_payload_klb_smpserve_conn(p_conn, packtype, NULL, 0);
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

    int ret = enqueue_smp_payload_klb_smpserve_conn(p_conn, packtype,
        (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
    KLB_FREE_BY(p_payload, klb_buf_unref);

    return ret;
}

int klb_smpserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    klb_nlist_push_tail(p_smpserve->p_write_nlist, p_data);
    klb_socket_set_writing(p_socket, true);
    return 0;
}

bool klb_smpserve_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_smpserve->p_write_nlist))
    {
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////

static int klb_smpserve_conn_init(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    p_conn->vtable.destroy = NULL;
    p_conn->vtable.recv_data = NULL;
    p_conn->vtable.ioctrl = klb_smpserve_conn_ioctrl;
    p_conn->vtable.send_normal = klb_smpserve_conn_send_normal;
    p_conn->vtable.send_media = klb_smpserve_conn_send_media;
    p_conn->vtable.on_send = klb_smpserve_conn_on_send;
    p_conn->vtable.on_recv = klb_smpserve_conn_on_recv;
    p_conn->vtable.on_msg = klb_smpserve_conn_on_msg;

    p_smpserve->p_write_nlist = klb_nlist_create();
    p_smpserve->p_read_head = klb_rbuf_malloc(KLB_SMP_HEAD_MAX);
    p_smpserve->parser_status = KLB_SMPPARSER_head;
    p_smpserve->p_read_body = NULL;
    p_smpserve->reading_len = 0;

    return 0;
}

static void klb_smpserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_smpserve->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_smpserve->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_smpserve->p_read_head, klb_rbuf_free);
    KLB_FREE_BY(p_smpserve->p_read_body, klb_buf_unref);
    KLB_FREE_BY(p_smpserve->p_write_nlist, klb_nlist_destroy);
}

klb_netconn_t* klb_smpserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_smpserve_conn_t));
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    klb_smpserve_conn_init(p_conn);
    p_conn->vtable.destroy = klb_smpserve_conn_destroy;

    p_smpserve->p_netmulti = p_netmulti;
    p_conn->p_socket = p_socket;
    klb_socket_set_reading(p_socket, true);
    klb_netmulti_push(p_netmulti, p_conn);

    return p_conn;
}

// end
