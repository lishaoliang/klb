// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsmp/core/klb_smpserve_conn.h"
#include "klbsmp/core/klb_smpparser.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_smp.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_codec.h"
#include "klbbase/klb_mnp_crypto.h"
#include <string.h>
#include "klbplatform/klb_time.h"
#include <assert.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <errno.h>
#endif


/// @brief 非阻塞套接字无数据时 recv 返回负值, 不是断线
static bool recv_would_block_klb_smpserve_conn(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();

    if (WSAEWOULDBLOCK == err || WSAEINTR == err)
    {
        return true;
    }
#else
    if (EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
    {
        return true;
    }
#endif

    return false;
}


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

        int                         ping_interval_ms;   ///< 服务端 Ping 间隔; 0=关
        int                         ping_timeout_ms;    ///< 等 Pong 超时
        int64_t                     last_ping_tc;       ///< 上次 Ping 时刻; 0=未武装
        int64_t                     ping_deadline_tc;   ///< 等待 Pong 截止
        bool                        ping_waiting;       ///< 已发 Ping 待 Pong
    };

    klb_mnp_codec_text_policy_t     text_pol;           ///< TEXT 压缩策略(跟随客户端)
    klb_mnp_crypto_policy_t         crypto_pol;         ///< 帧内加码(分方向)
}klb_smpserve_conn_t;

#define KLB_SMPSERVE_PING_INTERVAL_MS_DEFAULT   15000
#define KLB_SMPSERVE_PING_TIMEOUT_MS_DEFAULT    30000


//////////////////////////////////////////////////////////////////////////
static void klb_smpserve_conn_quit(klb_netconn_t* p_conn);
static void push_netcode_klb_smpserve_conn(klb_netconn_t* p_conn, int code);


static void handle_options_klb_smpserve_conn(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    int rc = KLB_MNP_CRYPTO_OK;

    if (NULL != p_data)
    {
        rc = klb_mnp_crypto_on_options_recv(&p_smpserve->crypto_pol,
            (const uint8_t*)(p_data->p_buf + p_data->start), klb_buf_data_len(p_data));
        (void)rc;
    }

    KLB_FREE_BY(p_data, klb_buf_unref);
}


static void push_data_klb_smpserve_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    if (KLB_MNP_OPTIONS == packtype)
    {
        handle_options_klb_smpserve_conn(p_conn, p_data);
        return;
    }

    if (NULL != p_data && klb_mnp_crypto_packtype_controlled(packtype))
    {
        int crc = klb_mnp_crypto_xform_payload(&p_smpserve->crypto_pol.rx, packtype, (uint8_t*)(p_data->p_buf + p_data->start), klb_buf_data_len(p_data), false);

        if (KLB_MNP_CRYPTO_OK != crc)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            push_netcode_klb_smpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return;
        }
    }

    if (KLB_MNP_TEXT == packtype && NULL != p_data)
    {
        int rc = klb_mnp_codec_text_on_recv(&p_smpserve->text_pol, &p_data);

        if (KLB_MNP_CODEC_OK != rc)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            push_netcode_klb_smpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return;
        }
    }

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

static bool is_payload_pack_klb_smpserve_conn(int packtype)
{
    if (KLB_MNP_TEXT == packtype ||
        KLB_MNP_BINARY == packtype ||
        KLB_MNP_MEDIA == packtype ||
        KLB_MNP_OPTIONS == packtype)
    {
        return true;
    }

    return false;
}

static void skip_body_klb_smpserve_conn(klb_netconn_t* p_conn, klb_smpparser_t* p_parser, klb_rbuf_t* p_head)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    int body_need = p_parser->pack_len - p_parser->head_len;

    if (0 > body_need)
    {
        body_need = 0;
    }

    int body_len = 0;
    klb_rbuf_data(p_head, &body_len);

    int cp_len = MIN(body_need, body_len);
    if (0 < cp_len)
    {
        klb_rbuf_use(p_head, cp_len);
        body_need -= cp_len;
    }

    if (0 < body_need)
    {
        p_smpserve->p_read_body = klb_buf_malloc(body_need, false);
        p_smpserve->reading_len = body_need;
        p_smpserve->parser = *p_parser;
        p_smpserve->parser_status = KLB_SMPPARSER_body;
    }
    else
    {
        p_smpserve->parser_status = KLB_SMPPARSER_head;
    }
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

static void send_ping_klb_smpserve_conn(klb_netconn_t* p_conn)
{
    enqueue_smp_payload_klb_smpserve_conn(p_conn, KLB_MNP_PING, NULL, 0);
}

static void normalize_ping_klb_smpserve_conn(klb_smpserve_conn_t* p_smpserve, int interval_ms, int timeout_ms)
{
    if (0 > interval_ms)
    {
        interval_ms = KLB_SMPSERVE_PING_INTERVAL_MS_DEFAULT;
    }

    if (0 == interval_ms)
    {
        p_smpserve->ping_interval_ms = 0;
        p_smpserve->ping_timeout_ms = 0;
        p_smpserve->ping_waiting = false;
        p_smpserve->ping_deadline_tc = 0;
        return;
    }

    if (0 >= timeout_ms)
    {
        timeout_ms = KLB_SMPSERVE_PING_TIMEOUT_MS_DEFAULT;
    }


    p_smpserve->ping_interval_ms = interval_ms;
    p_smpserve->ping_timeout_ms = timeout_ms;
}

static void on_ticker_klb_smpserve_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    if (0 >= p_smpserve->ping_interval_ms)
    {
        return;
    }

    if (0 == p_smpserve->last_ping_tc)
    {
        p_smpserve->last_ping_tc = now;
        return;
    }

    if (p_smpserve->ping_waiting)
    {
        if (now >= p_smpserve->ping_deadline_tc)
        {
            push_netcode_klb_smpserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        }

        return;
    }

    if ((int64_t)p_smpserve->ping_interval_ms <= ABS_SUB(now, p_smpserve->last_ping_tc))
    {
        send_ping_klb_smpserve_conn(p_conn);
        p_smpserve->last_ping_tc = now;
        p_smpserve->ping_waiting = true;
        p_smpserve->ping_deadline_tc = now + (int64_t)p_smpserve->ping_timeout_ms;
    }
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
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    int data_len = klb_buf_data_len(p_data);
    int crc = 0;

    crc = klb_mnp_crypto_xform_payload(&p_smpserve->crypto_pol.tx, KLB_MNP_MEDIA,
        (uint8_t*)(p_data->p_buf + p_data->start), data_len, true);
    if (KLB_MNP_CRYPTO_OK != crc)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        return 1;
    }

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
        push_netcode_klb_smpserve_conn(p_conn, close_status);
        return write_num;
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
        else
        {
            int close_status = KLB_SOCKET_DISCONNECT;
            int judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                return 1;
            }
            klb_socket_recv_closed(p_socket, close_status);
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
                // last_recv_tc 由 klbiopoll 更新
                send_pong_klb_smpserve_conn(p_conn);
                p_smpserve->parser_status = KLB_SMPPARSER_head;
                klb_rbuf_memmove(p_head);
            }
            else if (KLB_MNP_PONG == parser.packtype)
            {
                // last_recv_tc 由 klbiopoll 更新; C 层吞掉 Pong
                p_smpserve->ping_waiting = false;
                p_smpserve->ping_deadline_tc = 0;
                p_smpserve->parser_status = KLB_SMPPARSER_head;
                klb_rbuf_memmove(p_head);
            }
            else if (KLB_MNP_TEXT == parser.packtype ||
                     KLB_MNP_BINARY == parser.packtype ||
                     KLB_MNP_MEDIA == parser.packtype ||
                     KLB_MNP_OPTIONS == parser.packtype)
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
                    klb_rbuf_memmove(p_head);
                }
                else
                {
                    p_smpserve->parser_status = KLB_SMPPARSER_head;
                    // 先整理 rbuf, 再回调; 避免 Lua disconnect 后 memmove 悬空
                    klb_rbuf_memmove(p_head);
                    push_data_klb_smpserve_conn(p_conn, parser.packtype, p_buf);
                }
            }
            else
            {
                skip_body_klb_smpserve_conn(p_conn, &parser, p_head);
                klb_rbuf_memmove(p_head);
            }
        }
        else if (0 < ret_parse)
        {
            return 0;
        }
        else
        {
            return -2;
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

                if (is_payload_pack_klb_smpserve_conn(p_smpserve->parser.packtype))
                {
                    push_data_klb_smpserve_conn(p_conn, p_smpserve->parser.packtype, p_buf);
                }
                else
                {
                    KLB_FREE_BY(p_buf, klb_buf_unref);
                }
            }
        }
        else
        {
            int close_status = KLB_SOCKET_DISCONNECT;
            int judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                return 1;
            }
            klb_socket_recv_closed(p_socket, close_status);
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

        if (0 > ret)
        {
            int code = (int)klb_socket_get_status(p_socket);

            if (-2 == ret)
            {
                code = KLB_SOCKET_ERR_PROTOCOL;
                klb_socket_recv_closed(p_socket, code);
            }
            else if (KLB_SOCKET_OK == code)
            {
                code = KLB_SOCKET_DISCONNECT;
                klb_socket_recv_closed(p_socket, code);
            }

            push_netcode_klb_smpserve_conn(p_conn, code);
            break;
        }

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
        on_ticker_klb_smpserve_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////

void klb_smpserve_conn_set_ping(klb_netconn_t* p_conn, int interval_ms, int timeout_ms)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;

    normalize_ping_klb_smpserve_conn(p_smpserve, interval_ms, timeout_ms);
    p_smpserve->last_ping_tc = 0;
    p_smpserve->ping_deadline_tc = 0;
    p_smpserve->ping_waiting = false;
}

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

    {
        klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
        int encode = KLB_MNP_ENCODE_NULL;
        klb_buf_t* p_payload = NULL;
        int ret = 0;

        if (KLB_MNP_TEXT == packtype)
        {
            encode = klb_mnp_codec_text_send_encode(&p_smpserve->text_pol);
        }

        p_payload = klb_mnp_codec_pack_tb(sequence, encode, p_head, head_len, p_body, body_len);
        if (NULL == p_payload)
        {
            return 1;
        }

        {
            int crc = klb_mnp_crypto_xform_payload(&p_smpserve->crypto_pol.tx, packtype, (uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload), true);

            if (KLB_MNP_CRYPTO_OK != crc)
            {
                KLB_FREE_BY(p_payload, klb_buf_unref);
                return 1;
            }
        }

        ret = enqueue_smp_payload_klb_smpserve_conn(p_conn, packtype,
            (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
        KLB_FREE_BY(p_payload, klb_buf_unref);
        return ret;
    }
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

int64_t klb_smpserve_wbuf_size(klb_netconn_t* p_conn)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    int64_t total = 0;

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_smpserve->p_write_nlist);
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
    normalize_ping_klb_smpserve_conn(p_smpserve, KLB_SMPSERVE_PING_INTERVAL_MS_DEFAULT, KLB_SMPSERVE_PING_TIMEOUT_MS_DEFAULT);
    p_smpserve->last_ping_tc = 0;
    p_smpserve->ping_deadline_tc = 0;
    p_smpserve->ping_waiting = false;
    p_smpserve->text_pol.is_server = true;
    p_smpserve->text_pol.use_compress = false;
    p_smpserve->text_pol.peer_locked = false;
    p_smpserve->text_pol.peer_compress = false;
    p_smpserve->text_pol.peer_format = 0;
    klb_mnp_crypto_policy_init(&p_smpserve->crypto_pol);

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
    klb_mnp_crypto_policy_clear(&p_smpserve->crypto_pol);
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


int klb_smpserve_conn_send_crypto_psk(klb_netconn_t* p_conn, int sequence,
    uint32_t format, const uint8_t* p_key, uint32_t key_size)
{
    klb_smpserve_conn_t* p_smpserve = (klb_smpserve_conn_t*)p_conn->extra;
    klb_buf_t* p_payload = NULL;
    int rc = 0;
    int ret = 0;

    rc = klb_mnp_crypto_set_tx(&p_smpserve->crypto_pol, format, p_key, key_size);
    if (KLB_MNP_CRYPTO_OK != rc)
    {
        return 1;
    }

    p_payload = klb_mnp_crypto_pack_psk(sequence, format, p_key, key_size);
    if (NULL == p_payload)
    {
        return 1;
    }

    ret = enqueue_smp_payload_klb_smpserve_conn(p_conn, KLB_MNP_OPTIONS,
        (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
    KLB_FREE_BY(p_payload, klb_buf_unref);
    return ret;
}

// end
