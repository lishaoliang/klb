// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmnp/core/klb_mnpclient_conn.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_codec.h"
#include "klbbase/klb_mnp_crypto.h"
#include <string.h>
#include "klbplatform/klb_time.h"
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
        klb_buffer_t*               p_txt;              ///< TEXT 重组
        klb_buffer_t*               p_bin;              ///< BINARY 重组
        klb_buffer_t*               p_media;            ///< MEDIA 重组
        klb_buffer_t*               p_opt;              ///< OPTIONS 重组(须 FULL)
        bool                        asm_open;           ///< true.已收 BEGIN, 等待 END
        int                         asm_packtype;       ///< 正在重组的 packtype; 空闲为 -1
        int64_t                     max_msg;            ///< 单条重组上限; 0=不限
        int                         read_timeout_ms;    ///< 读超时; 0=关
        int                         write_timeout_ms;   ///< 写超时; 0=关
        int                         idle_timeout_ms;    ///< 空闲超时; 0=关
        int                         max_wbuf_n;         ///< 写队列条数上限; 0=不限
    };

    klb_mnp_codec_text_policy_t     text_pol;           ///< TEXT 压缩策略(Lua 无感)
    klb_mnp_crypto_policy_t         crypto_pol;         ///< 帧内加码(分方向)
    uint32_t                        crypto_rekey_after; ///< >0 自动换钥阈值; 0=关
    uint32_t                        crypto_sess_count;  ///< 已发送受控业务帧计数
    int                             crypto_opt_seq;     ///< CRYPTO_PSK OPTIONS 序号
}klb_mnpclient_conn_t;
#define KLB_MNPCLIENT_MAX_MSG_DEFAULT    (16 * 1024 * 1024)
#define KLB_MNPCLIENT_MAX_WBUF_N_DEFAULT 256



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_mnpclient_conn_quit(klb_netconn_t* p_conn);
static void push_netcode_klb_mnpclient_conn(klb_netconn_t* p_conn, int code);
static void maybe_rekey_klb_mnpclient_conn(klb_netconn_t* p_conn);


//////////////////////////////////////////////////////////////////////////
// 内部函数

static void handle_options_klb_mnpclient_conn(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    int rc = KLB_MNP_CRYPTO_OK;

    if (NULL != p_data)
    {
        rc = klb_mnp_crypto_on_options_recv(&p_mnpclient->crypto_pol,
            (const uint8_t*)(p_data->p_buf + p_data->start), klb_buf_data_len(p_data));
        (void)rc;
    }

    KLB_FREE_BY(p_data, klb_buf_unref);
}


/// @brief 受控业务帧发送成功后计次; 达 rekey_after 则换钥并再发 CRYPTO_PSK
static void maybe_rekey_klb_mnpclient_conn(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    uint8_t* p_key = NULL;
    uint32_t key_size = 0;
    uint32_t format = 0;
    int rc = 0;

    if (0 == p_mnpclient->crypto_rekey_after)
    {
        return;
    }

    if (KLB_MNP_CRYPTO_NULL == p_mnpclient->crypto_pol.tx.format)
    {
        return;
    }

    p_mnpclient->crypto_sess_count++;
    if (p_mnpclient->crypto_sess_count < p_mnpclient->crypto_rekey_after)
    {
        return;
    }

    format = p_mnpclient->crypto_pol.tx.format;
    key_size = p_mnpclient->crypto_pol.tx.key_size;
    if (0 == key_size || KLB_MNP_CRYPTO_KEY_SIZE_MAX < key_size)
    {
        return;
    }

    p_key = KLB_MALLOC(uint8_t, (int)key_size, 0);
    klb_mnp_crypto_fill_key(p_key, key_size);

    p_mnpclient->crypto_opt_seq++;
    rc = klb_mnpclient_conn_send_crypto_psk(p_conn, p_mnpclient->crypto_opt_seq,
        format, p_key, key_size);
    KLB_FREE(p_key);

    if (0 == rc)
    {
        p_mnpclient->crypto_sess_count = 0;
    }
}


static void push_data_klb_mnpclient_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    if (KLB_MNP_OPTIONS == packtype)
    {
        handle_options_klb_mnpclient_conn(p_conn, p_data);
        return;
    }

    if (NULL != p_data && klb_mnp_crypto_packtype_controlled(packtype))
    {
        int crc = klb_mnp_crypto_xform_payload(&p_mnpclient->crypto_pol.rx, packtype, (uint8_t*)(p_data->p_buf + p_data->start), klb_buf_data_len(p_data), false);

        if (KLB_MNP_CRYPTO_OK != crc)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            klb_socket_recv_closed(p_conn->p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return;
        }
    }

    if (KLB_MNP_TEXT == packtype && NULL != p_data)
    {
        int rc = klb_mnp_codec_text_on_recv(&p_mnpclient->text_pol, &p_data);

        if (KLB_MNP_CODEC_OK != rc)
        {
            KLB_FREE_BY(p_data, klb_buf_unref);
            klb_socket_recv_closed(p_conn->p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
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

static void push_netcode_klb_mnpclient_conn(klb_netconn_t* p_conn, int code)
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
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    bool writing = false;
    bool reading = false;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return;
    }

    writing = (0 < klb_nlist_size(p_mnpclient->p_write_nlist));
    reading = p_mnpclient->asm_open
        || (p_mnpclient->p_read_buf->start < p_mnpclient->p_read_buf->end)
        || (0 < p_mnpclient->left_len);

    if (writing && 0 < p_mnpclient->write_timeout_ms
        && (int64_t)p_mnpclient->write_timeout_ms <= ABS_SUB(now, p_socket->last_send_tc))
    {
        push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    if (reading && 0 < p_mnpclient->read_timeout_ms
        && (int64_t)p_mnpclient->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }

    if (!writing && !reading && 0 < p_mnpclient->idle_timeout_ms
        && (int64_t)p_mnpclient->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }
}

static int enqueue_mnp_payload_klb_mnpclient_conn(klb_netconn_t* p_conn, int packtype, const uint8_t* p_payload, int payload_len)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (0 < p_mnpclient->max_wbuf_n
        && (int)klb_nlist_size(p_mnpclient->p_write_nlist) >= p_mnpclient->max_wbuf_n)
    {
        return KLB_NETCODE_WBUF_FULL;
    }

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

static void reset_asm_buf_klb_mnpclient_conn(klb_mnpclient_conn_t* p_mnpclient, int packtype)
{
    klb_buffer_t* p_buf = NULL;

    if (KLB_MNP_TEXT == packtype)
    {
        p_buf = p_mnpclient->p_txt;
    }
    else if (KLB_MNP_BINARY == packtype)
    {
        p_buf = p_mnpclient->p_bin;
    }
    else if (KLB_MNP_MEDIA == packtype)
    {
        p_buf = p_mnpclient->p_media;
    }
    else if (KLB_MNP_OPTIONS == packtype)
    {
        p_buf = p_mnpclient->p_opt;
    }

    if (NULL != p_buf)
    {
        klb_buffer_reset(p_buf);
    }
}

// BEGIN/FULL 清对应缓冲; CONTINUE/END 必须接在同类型 BEGIN 之后
static int check_frag_klb_mnpclient_conn(klb_mnpclient_conn_t* p_mnpclient, int packtype, uint8_t opt)
{
    if (KLB_MNP_OPTIONS == packtype)
    {
        // CRYPTO_PSK 等须单帧 FULL
        if (KLB_MNP_FULL != opt)
        {
            return -1;
        }

        return 0;
    }

    if (KLB_MNP_TEXT != packtype &&
        KLB_MNP_BINARY != packtype &&
        KLB_MNP_MEDIA != packtype)
    {
        return 0;
    }

    if (KLB_MNP_BEGIN == opt || KLB_MNP_FULL == opt)
    {
        if (p_mnpclient->asm_open)
        {
            return -1;
        }

        reset_asm_buf_klb_mnpclient_conn(p_mnpclient, packtype);

        if (KLB_MNP_BEGIN == opt)
        {
            p_mnpclient->asm_open = true;
            p_mnpclient->asm_packtype = packtype;
        }
    }
    else if (KLB_MNP_CONTINUE == opt || KLB_MNP_END == opt)
    {
        if (!p_mnpclient->asm_open || p_mnpclient->asm_packtype != packtype)
        {
            return -1;
        }

        if (KLB_MNP_END == opt)
        {
            p_mnpclient->asm_open = false;
            p_mnpclient->asm_packtype = -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

static int append_asm_klb_mnpclient_conn(klb_mnpclient_conn_t* p_mnpclient, int packtype, const char* p_data, int len)
{
    klb_buffer_t** p_slot = NULL;
    int64_t have = 0;

    if (KLB_MNP_TEXT == packtype)
    {
        p_slot = &p_mnpclient->p_txt;
    }
    else if (KLB_MNP_BINARY == packtype)
    {
        p_slot = &p_mnpclient->p_bin;
    }
    else if (KLB_MNP_MEDIA == packtype)
    {
        p_slot = &p_mnpclient->p_media;
    }
    else if (KLB_MNP_OPTIONS == packtype)
    {
        p_slot = &p_mnpclient->p_opt;
    }
    else
    {
        return 0;
    }

    if (NULL == *p_slot)
    {
        *p_slot = klb_buffer_create(4096);
    }

    have = (int64_t)klb_buffer_datalen(*p_slot);
    if (0 < p_mnpclient->max_msg && (have + (int64_t)len) > p_mnpclient->max_msg)
    {
        return -1;
    }

    klb_buffer_write(*p_slot, p_data, len);
    return 0;
}

static void finish_pack_klb_mnpclient_conn(klb_netconn_t* p_conn)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    int packtype = p_mnpclient->mnp.packtype;
    klb_buffer_t* p_asm = NULL;

    if (KLB_MNP_TEXT == packtype)
    {
        p_asm = p_mnpclient->p_txt;
    }
    else if (KLB_MNP_BINARY == packtype)
    {
        p_asm = p_mnpclient->p_bin;
    }
    else if (KLB_MNP_MEDIA == packtype)
    {
        p_asm = p_mnpclient->p_media;
    }
    else if (KLB_MNP_OPTIONS == packtype)
    {
        p_asm = p_mnpclient->p_opt;
    }
    else
    {
        return;
    }

    if (NULL == p_asm || 0 >= klb_buffer_datalen(p_asm))
    {
        return;
    }

    klb_buf_t* p_data = klb_buffer_join(p_asm, NULL, NULL);
    klb_buffer_reset(p_asm);
    push_data_klb_mnpclient_conn(p_conn, packtype, p_data);
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

            if (KLB_MNP_PING == mnp.packtype || KLB_MNP_PONG == mnp.packtype)
            {
                if (0 < p_mnpclient->left_len)
                {
                    return -1;
                }

                // last_recv_tc 由 klbiopoll 在收包路径更新; 本层仅协议处理

                if (KLB_MNP_PING == mnp.packtype)
                {
                    send_pong_klb_mnpclient_conn(p_conn);
                }

                p_mnpclient->left_len = 0;
                continue;
            }

            if (0 != check_frag_klb_mnpclient_conn(p_mnpclient, mnp.packtype, mnp.opt))
            {
                return -1;
            }

            if (p_mnpclient->left_len <= 0)
            {
                if (KLB_MNP_FULL == mnp.opt || KLB_MNP_END == mnp.opt)
                {
                    if (KLB_MNP_TEXT == mnp.packtype ||
                        KLB_MNP_BINARY == mnp.packtype ||
                        KLB_MNP_MEDIA == mnp.packtype ||
                        KLB_MNP_OPTIONS == mnp.packtype)
                    {
                        finish_pack_klb_mnpclient_conn(p_conn);
                    }
                }

                p_mnpclient->left_len = 0;
            }

            continue;
        }

        int r_len = MIN(p_mnpclient->left_len, data_len);
        int packtype = p_mnpclient->mnp.packtype;

        if (KLB_MNP_TEXT == packtype ||
            KLB_MNP_BINARY == packtype ||
            KLB_MNP_MEDIA == packtype ||
            KLB_MNP_OPTIONS == packtype)
        {
            if (0 != append_asm_klb_mnpclient_conn(p_mnpclient, packtype, p_buf->p_buf + p_buf->start, r_len))
            {
                return -1;
            }
        }

        p_buf->start += r_len;
        p_mnpclient->left_len -= r_len;

        if (p_mnpclient->left_len <= 0)
        {
            if (KLB_MNP_FULL == p_mnpclient->mnp.opt || KLB_MNP_END == p_mnpclient->mnp.opt)
            {
                if (KLB_MNP_TEXT == packtype ||
                    KLB_MNP_BINARY == packtype ||
                    KLB_MNP_MEDIA == packtype ||
                    KLB_MNP_OPTIONS == packtype)
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
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    int data_len = klb_buf_data_len(p_data);
    int crc = 0;

    crc = klb_mnp_crypto_xform_payload(&p_mnpclient->crypto_pol.tx, KLB_MNP_MEDIA,
        (uint8_t*)(p_data->p_buf + p_data->start), data_len, true);
    if (KLB_MNP_CRYPTO_OK != crc)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
        return 1;
    }

    enqueue_mnp_payload_klb_mnpclient_conn(p_conn, KLB_MNP_MEDIA,
        (const uint8_t*)(p_data->p_buf + p_data->start), data_len);
    KLB_FREE_BY(p_data, klb_buf_unref);
    maybe_rekey_klb_mnpclient_conn(p_conn);
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
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        // step1. 部分/全部写出: 推进偏移, 继续 while 直到不能写
        if (0 < send_num)
        {
            p_buf->start += send_num;
            write_num += send_num;
            continue;
        }

        // step2. 判定 TLS/非 TLS (AGAIN=停等; ERROR=关连接)
        judge = klb_socket_send_judge(p_socket, send_num, &close_status);

        if (KLB_SOCKET_SEND_AGAIN == judge)
        {
            // 发送缓存满或 TLS WANT: 保持 writing, 等下次可写
            break;
        }

        // step3. 写侧致命错: 清 reading/writing 并通知上层
        klb_socket_recv_closed(p_socket, close_status);
        push_netcode_klb_mnpclient_conn(p_conn, close_status);
        return write_num;
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
            klb_socket_recv_closed(p_socket, KLB_SOCKET_ERR_PROTOCOL);
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
    int judge = 0;
    int close_status = KLB_SOCKET_DISCONNECT;

    // step1. 有数据: 入缓冲并解析, 继续 while 排空
    if (0 < recv_len)
    {
        p_buf->end += recv_len;
        *p_read_num += recv_len;

        if (0 != parser_klb_mnpclient_conn(p_conn))
        {
            klb_socket_recv_closed(p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }

        return 0;
    }

    // step2. 判定 TLS/非 TLS 关断 (0 与 EAGAIN/WANT 区分)
    judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);

    if (KLB_SOCKET_RECV_AGAIN == judge)
    {
        // 缓存读完仍存活 (或 TLS WANT): 停本轮, 保持 reading
        return 1;
    }

    // step3. EOF/ERROR: 先尽量解析残留, 再通知上层关闭
    if (p_buf->start < p_buf->end)
    {
        if (0 != parser_klb_mnpclient_conn(p_conn))
        {
            klb_socket_recv_closed(p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnpclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            return -1;
        }
    }

    klb_socket_recv_closed(p_socket, close_status);
    push_netcode_klb_mnpclient_conn(p_conn, close_status);
    return -1;
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

        // 0=有数据继续; 1=AGAIN 停; <0=已关或协议错 (do_recv 内已 push)
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

    {
        klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
        int encode = KLB_MNP_ENCODE_NULL;
        klb_buf_t* p_payload = NULL;
        int ret = 0;

        if (KLB_MNP_TEXT == packtype)
        {
            encode = klb_mnp_codec_text_send_encode(&p_mnpclient->text_pol);
        }

        p_payload = klb_mnp_codec_pack_tb(sequence, encode, p_head, head_len, p_body, body_len);
        if (NULL == p_payload)
        {
            return 1;
        }

        {
            int crc = klb_mnp_crypto_xform_payload(&p_mnpclient->crypto_pol.tx, packtype, (uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload), true);

            if (KLB_MNP_CRYPTO_OK != crc)
            {
                KLB_FREE_BY(p_payload, klb_buf_unref);
                return 1;
            }
        }

        ret = enqueue_mnp_payload_klb_mnpclient_conn(p_conn, packtype,
            (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
        KLB_FREE_BY(p_payload, klb_buf_unref);

        if (0 == ret)
        {
            maybe_rekey_klb_mnpclient_conn(p_conn);
        }

        return ret;
    }
}

int klb_mnpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (0 < p_mnpclient->max_wbuf_n
        && (int)klb_nlist_size(p_mnpclient->p_write_nlist) >= p_mnpclient->max_wbuf_n)
    {
        return KLB_NETCODE_WBUF_FULL;
    }

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
    p_mnpclient->p_bin = NULL;
    p_mnpclient->p_media = NULL;
    p_mnpclient->p_opt = NULL;
    p_mnpclient->asm_open = false;
    p_mnpclient->asm_packtype = -1;
    p_mnpclient->max_msg = KLB_MNPCLIENT_MAX_MSG_DEFAULT;
    p_mnpclient->read_timeout_ms = 0;
    p_mnpclient->write_timeout_ms = 0;
    p_mnpclient->idle_timeout_ms = 0;
    p_mnpclient->max_wbuf_n = KLB_MNPCLIENT_MAX_WBUF_N_DEFAULT;

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
    KLB_FREE_BY(p_mnpclient->p_bin, klb_buffer_destroy);
    KLB_FREE_BY(p_mnpclient->p_media, klb_buffer_destroy);
    KLB_FREE_BY(p_mnpclient->p_opt, klb_buffer_destroy);
    KLB_FREE_BY(p_mnpclient->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_mnpclient->p_write_nlist, klb_nlist_destroy);
    klb_mnp_crypto_policy_clear(&p_mnpclient->crypto_pol);
}


//////////////////////////////////////////////////////////////////////////

klb_netconn_t* klb_mnpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code)
{
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;
    klb_netconn_t* p_netconn = NULL;
    int code = 0;

    if (NULL == p_netmulti || NULL == p_host || 0 == p_host[0])
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_EINVAL;
        }

        return NULL;
    }

    fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        code = klb_netcode_last_syserr();
        if (KLB_NETCODE_EIO == code || KLB_NETCODE_EPERM == code)
        {
            code = KLB_NETCODE_CONNECT_FAIL;
        }

        if (NULL != p_code)
        {
            *p_code = code;
        }

        return NULL;
    }

    // step1. 明文或 TLS 封装
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

    p_netconn = klb_mnpclient_conn_create(p_netmulti, p_socket);
    klb_netmulti_push(p_netmulti, p_netconn);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_netconn;
}

klb_netconn_t* klb_mnpclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_mnpclient_conn_t));
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    klb_mnpclient_conn_init(p_conn);
    p_conn->vtable.destroy = klb_mnpclient_conn_destroy;

    p_mnpclient->p_netmulti = p_netmulti;
    p_mnpclient->text_pol.is_server = false;
    p_mnpclient->text_pol.use_compress = false;
    p_mnpclient->text_pol.peer_locked = false;
    p_mnpclient->text_pol.peer_compress = false;
    p_mnpclient->text_pol.peer_format = 0;
    klb_mnp_crypto_policy_init(&p_mnpclient->crypto_pol);
    p_mnpclient->crypto_rekey_after = 0;
    p_mnpclient->crypto_sess_count = 0;
    p_mnpclient->crypto_opt_seq = 0;
    p_conn->p_socket = p_socket;
    klb_socket_set_reading(p_socket, true);

    return p_conn;
}

void klb_mnpclient_conn_set_compress(klb_netconn_t* p_conn, bool enable)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    if (!klb_mnp_codec_compress_supported())
    {
        p_mnpclient->text_pol.use_compress = false;
        return;
    }

    p_mnpclient->text_pol.use_compress = enable ? true : false;
}


int klb_mnpclient_conn_send_crypto_psk(klb_netconn_t* p_conn, int sequence,
    uint32_t format, const uint8_t* p_key, uint32_t key_size)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    klb_buf_t* p_payload = NULL;
    int rc = 0;
    int ret = 0;

    rc = klb_mnp_crypto_set_tx(&p_mnpclient->crypto_pol, format, p_key, key_size);
    if (KLB_MNP_CRYPTO_OK != rc)
    {
        return 1;
    }

    p_payload = klb_mnp_crypto_pack_psk(sequence, format, p_key, key_size);
    if (NULL == p_payload)
    {
        return 1;
    }

    ret = enqueue_mnp_payload_klb_mnpclient_conn(p_conn, KLB_MNP_OPTIONS,
        (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
    KLB_FREE_BY(p_payload, klb_buf_unref);
    return ret;
}

int klb_mnpclient_conn_set_crypto(klb_netconn_t* p_conn,
    uint32_t format, const uint8_t* p_key, uint32_t key_size, uint32_t rekey_after)
{
    klb_mnpclient_conn_t* p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;
    uint8_t* p_gen = NULL;
    const uint8_t* p_use = p_key;
    int rc = 0;

    if (NULL == p_conn || KLB_MNP_CRYPTO_NULL == format)
    {
        return 1;
    }

    if (0 == key_size || KLB_MNP_CRYPTO_KEY_SIZE_MAX < key_size)
    {
        return 1;
    }

    if (NULL == p_key)
    {
        p_gen = KLB_MALLOC(uint8_t, (int)key_size, 0);
        klb_mnp_crypto_fill_key(p_gen, key_size);
        p_use = p_gen;
    }

    p_mnpclient->crypto_rekey_after = rekey_after;
    p_mnpclient->crypto_sess_count = 0;
    p_mnpclient->crypto_opt_seq++;

    rc = klb_mnpclient_conn_send_crypto_psk(p_conn, p_mnpclient->crypto_opt_seq,
        format, p_use, key_size);
    KLB_FREE(p_gen);
    return rc;
}


void klb_mnpclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    klb_mnpclient_conn_t* p_mnpclient = NULL;

    assert(NULL != p_conn);
    p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    p_mnpclient->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_mnpclient->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_mnpclient->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}


void klb_mnpclient_conn_set_max_msg(klb_netconn_t* p_conn, int64_t max_msg)
{
    klb_mnpclient_conn_t* p_mnpclient = NULL;

    assert(NULL != p_conn);
    p_mnpclient = (klb_mnpclient_conn_t*)p_conn->extra;

    p_mnpclient->max_msg = (0 < max_msg) ? max_msg : 0;
}

// end
