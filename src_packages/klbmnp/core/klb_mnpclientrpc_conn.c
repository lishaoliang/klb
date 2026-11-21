// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbmnp/core/klb_mnpclientrpc_conn.h"
#include "klbmnp/core/klb_mnpserverpc_conn.h"
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_netcode.h"
#include <string.h>
#include <assert.h>


/// @struct klb_mnprpc_conn_t
/// @brief  MNP RPC 连接, 客户端与服务端共用
typedef struct klb_mnprpc_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用
    bool                            server_heartbeat;   ///< true.服务端主动 Ping

    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 发送列表
    };

    struct
    {
        klb_buf_t*                  p_read_buf;         ///< 读取缓存
        klb_mnp_t                   mnp;                ///< 当前帧头
        int                         left_len;           ///< 当前帧剩余载荷
        klb_buffer_t*               p_rpc;              ///< RPC 重组
        bool                        asm_open;           ///< true.已收 BEGIN, 等待 END
        int                         asm_packtype;       ///< 正在重组的 packtype; 空闲为 -1

        int                         ping_interval_ms;   ///< 服务端 Ping 间隔; 0=关
        int                         ping_timeout_ms;    ///< 等 Pong 超时
        int64_t                     last_ping_tc;       ///< 上次 Ping 时刻; 0=未武装
        int64_t                     ping_deadline_tc;   ///< 等待 Pong 截止
        bool                        ping_waiting;       ///< 已发 Ping 待 Pong
        int64_t                     op_deadline_tc;     ///< co_call deadline; 0=none
        int64_t                     max_msg;            ///< RPC reassembly max; 0=unlimited
    };
}klb_mnprpc_conn_t;

#define KLB_MNPRPC_MAX_MSG_DEFAULT           (16 * 1024 * 1024)
#define KLB_MNPRPC_PING_INTERVAL_MS_DEFAULT 15000
#define KLB_MNPRPC_PING_TIMEOUT_MS_DEFAULT  30000


static void klb_mnprpc_conn_quit(klb_netconn_t* p_conn);


static bool is_rpc_pack_klb_mnprpc_conn(int packtype)
{
    if (KLB_MNP_RPC_LUA == packtype || KLB_MNP_RPC_JSON == packtype)
    {
        return true;
    }

    return false;
}

static void push_data_klb_mnprpc_conn(klb_netconn_t* p_conn, int packtype, klb_buf_t* p_data)
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

static void push_netcode_klb_mnprpc_conn(klb_netconn_t* p_conn, int code)
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

static int enqueue_payload_klb_mnprpc_conn(klb_netconn_t* p_conn, int packtype, const uint8_t* p_payload, int payload_len)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    int chunk_max = (int)KLB_MNP_BLOCK_SIZE_MAX - (int)sizeof(klb_mnp_t);
    int off = 0;
    bool first = true;

    if (payload_len < 0)
    {
        payload_len = 0;
    }

    if (chunk_max <= 0)
    {
        return 1;
    }

    if (0 == payload_len)
    {
        klb_mnp_t mnp = { 0 };
        klb_buf_t* p_buf = NULL;

        mnp.magic = KLB_MNP_MAGIC;
        mnp.size = (uint16_t)sizeof(klb_mnp_t);
        mnp.opt = KLB_MNP_FULL;
        mnp.packtype = (uint8_t)packtype;

        p_buf = klb_buf_malloc((int)sizeof(klb_mnp_t), false);
        klb_buf_write(p_buf, (const char*)&mnp, (int)sizeof(mnp));
        klb_nlist_push_tail(p_rpc->p_write_nlist, p_buf);
        klb_socket_set_writing(p_socket, true);
        return 0;
    }

    while (off < payload_len)
    {
        int remain = payload_len - off;
        int chunk = remain;
        uint8_t opt = KLB_MNP_FULL;
        klb_mnp_t mnp = { 0 };
        klb_buf_t* p_buf = NULL;

        if (chunk_max < chunk)
        {
            chunk = chunk_max;
        }

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

        mnp.magic = KLB_MNP_MAGIC;
        mnp.size = (uint16_t)(sizeof(klb_mnp_t) + chunk);
        mnp.opt = opt;
        mnp.packtype = (uint8_t)packtype;

        p_buf = klb_buf_malloc((int)sizeof(klb_mnp_t) + chunk, false);
        klb_buf_write(p_buf, (const char*)&mnp, (int)sizeof(mnp));
        klb_buf_write(p_buf, (const char*)(p_payload + off), chunk);
        klb_nlist_push_tail(p_rpc->p_write_nlist, p_buf);

        off += chunk;
        first = false;
    }

    klb_socket_set_writing(p_socket, true);
    return 0;
}

static void send_pong_klb_mnprpc_conn(klb_netconn_t* p_conn)
{
    enqueue_payload_klb_mnprpc_conn(p_conn, KLB_MNP_PONG, NULL, 0);
}

static void send_ping_klb_mnprpc_conn(klb_netconn_t* p_conn)
{
    enqueue_payload_klb_mnprpc_conn(p_conn, KLB_MNP_PING, NULL, 0);
}

static void normalize_ping_klb_mnprpc_conn(klb_mnprpc_conn_t* p_rpc, int interval_ms, int timeout_ms)
{
    if (0 > interval_ms)
    {
        interval_ms = KLB_MNPRPC_PING_INTERVAL_MS_DEFAULT;
    }

    if (0 == interval_ms)
    {
        p_rpc->ping_interval_ms = 0;
        p_rpc->ping_timeout_ms = 0;
        p_rpc->ping_waiting = false;
    p_rpc->op_deadline_tc = 0;
    p_rpc->max_msg = KLB_MNPRPC_MAX_MSG_DEFAULT;
        p_rpc->ping_deadline_tc = 0;
        return;
    }

    if (0 >= timeout_ms)
    {
        timeout_ms = KLB_MNPRPC_PING_TIMEOUT_MS_DEFAULT;
    }


    p_rpc->ping_interval_ms = interval_ms;
    p_rpc->ping_timeout_ms = timeout_ms;
}

static void on_ticker_klb_mnprpc_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    if (!p_rpc->server_heartbeat)
    {
        return;
    }

    if (0 >= p_rpc->ping_interval_ms)
    {
        return;
    }

    if (0 == p_rpc->last_ping_tc)
    {
        p_rpc->last_ping_tc = now;
        return;
    }

    if (p_rpc->ping_waiting)
    {
        if (now >= p_rpc->ping_deadline_tc)
        {
            klb_socket_recv_closed(p_conn->p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnprpc_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
        }

        return;
    }

    if ((int64_t)p_rpc->ping_interval_ms <= ABS_SUB(now, p_rpc->last_ping_tc))
    {
        send_ping_klb_mnprpc_conn(p_conn);
        p_rpc->last_ping_tc = now;
        p_rpc->ping_waiting = true;
        p_rpc->ping_deadline_tc = now + (int64_t)p_rpc->ping_timeout_ms;
    }

    if (0 < p_rpc->op_deadline_tc && now >= p_rpc->op_deadline_tc)
    {
        p_rpc->op_deadline_tc = 0;
        push_netcode_klb_mnprpc_conn(p_conn, KLB_SOCKET_TIMEOUT);
        return;
    }
}

static int check_frag_klb_mnprpc_conn(klb_mnprpc_conn_t* p_rpc, int packtype, uint8_t opt)
{
    if (!is_rpc_pack_klb_mnprpc_conn(packtype))
    {
        return 0;
    }

    if (KLB_MNP_BEGIN == opt || KLB_MNP_FULL == opt)
    {
        if (p_rpc->asm_open)
        {
            return -1;
        }

        if (NULL != p_rpc->p_rpc)
        {
            klb_buffer_reset(p_rpc->p_rpc);
        }

        if (KLB_MNP_BEGIN == opt)
        {
            p_rpc->asm_open = true;
            p_rpc->asm_packtype = packtype;
        }
    }
    else if (KLB_MNP_CONTINUE == opt || KLB_MNP_END == opt)
    {
        if (!p_rpc->asm_open || p_rpc->asm_packtype != packtype)
        {
            return -1;
        }

        if (KLB_MNP_END == opt)
        {
            p_rpc->asm_open = false;
            p_rpc->asm_packtype = -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

static int append_rpc_klb_mnprpc_conn(klb_mnprpc_conn_t* p_rpc, const char* p_data, int len)
{
    if (0 >= len)
    {
        return 0;
    }

    if (NULL == p_rpc->p_rpc)
    {
        p_rpc->p_rpc = klb_buffer_create(4096);
    }

        if (0 < p_rpc->max_msg
        && ((int64_t)klb_buffer_datalen(p_rpc->p_rpc) + (int64_t)len) > p_rpc->max_msg)
    {
        return -1;
    }

    klb_buffer_write(p_rpc->p_rpc, p_data, len);
    return 0;
}

static int finish_rpc_klb_mnprpc_conn(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    int packtype = p_rpc->mnp.packtype;
    klb_buf_t* p_data = NULL;

    if (!is_rpc_pack_klb_mnprpc_conn(packtype))
    {
        return 0;
    }

    if (NULL == p_rpc->p_rpc || (int)sizeof(klb_mnp_rpc_t) > klb_buffer_datalen(p_rpc->p_rpc))
    {
        return -1;
    }

    p_data = klb_buffer_join(p_rpc->p_rpc, NULL, NULL);
    klb_buffer_reset(p_rpc->p_rpc);
    push_data_klb_mnprpc_conn(p_conn, packtype, p_data);
    return 0;
}

static void compact_read_klb_mnprpc_conn(klb_mnprpc_conn_t* p_rpc)
{
    klb_buf_t* p_buf = p_rpc->p_read_buf;

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

static int parser_klb_mnprpc_conn(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    klb_buf_t* p_buf = p_rpc->p_read_buf;

    while (p_buf->start < p_buf->end)
    {
        int data_len = p_buf->end - p_buf->start;

        if (p_rpc->left_len <= 0)
        {
            klb_mnp_t mnp = { 0 };

            if (data_len < (int)sizeof(klb_mnp_t))
            {
                break;
            }

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

            p_rpc->mnp = mnp;
            p_rpc->left_len = (int)mnp.size - (int)sizeof(klb_mnp_t);

            if (KLB_MNP_PING == mnp.packtype || KLB_MNP_PONG == mnp.packtype)
            {
                if (0 < p_rpc->left_len)
                {
                    return -1;
                }

                if (KLB_MNP_PING == mnp.packtype)
                {
                    send_pong_klb_mnprpc_conn(p_conn);
                }
                else if (KLB_MNP_PONG == mnp.packtype)
                {
                    // C 层吞掉 Pong; 不推 Lua
                    p_rpc->ping_waiting = false;
    p_rpc->op_deadline_tc = 0;
    p_rpc->max_msg = KLB_MNPRPC_MAX_MSG_DEFAULT;
                    p_rpc->ping_deadline_tc = 0;
                }

                p_rpc->left_len = 0;
                continue;
            }

            if (0 != check_frag_klb_mnprpc_conn(p_rpc, mnp.packtype, mnp.opt))
            {
                return -1;
            }

            if (p_rpc->left_len <= 0)
            {
                if (KLB_MNP_FULL == mnp.opt || KLB_MNP_END == mnp.opt)
                {
                    if (0 != finish_rpc_klb_mnprpc_conn(p_conn))
                    {
                        return -1;
                    }
                }

                p_rpc->left_len = 0;
            }

            continue;
        }

        {
            int r_len = MIN(p_rpc->left_len, data_len);
            int packtype = p_rpc->mnp.packtype;

            if (is_rpc_pack_klb_mnprpc_conn(packtype))
            {
                if (0 != append_rpc_klb_mnprpc_conn(p_rpc, p_buf->p_buf + p_buf->start, r_len))
            {
                return -1;
            }
            }

            p_buf->start += r_len;
            p_rpc->left_len -= r_len;

            if (p_rpc->left_len <= 0)
            {
                if (KLB_MNP_FULL == p_rpc->mnp.opt || KLB_MNP_END == p_rpc->mnp.opt)
                {
                    if (0 != finish_rpc_klb_mnprpc_conn(p_conn))
                    {
                        return -1;
                    }
                }

                p_rpc->left_len = 0;
            }
        }
    }

    compact_read_klb_mnprpc_conn(p_rpc);
    return 0;
}

static void klb_mnprpc_conn_destroy(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    assert(NULL == p_rpc->p_netmulti);

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    klb_mnprpc_conn_quit(p_conn);
    KLB_FREE(p_conn);
}

static int klb_mnprpc_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    return 1;
}

static int klb_mnprpc_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    return 1;
}

static int klb_mnprpc_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    KLB_FREE_BY(p_data, klb_buf_unref);
    return 1;
}

static int klb_mnprpc_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_nlist_t* p_write_nlist = p_rpc->p_write_nlist;
    int write_num = 0;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    while (true)
    {
        klb_buf_t* p_buf = klb_nlist_head(p_write_nlist);

        if (NULL == p_buf)
        {
            break;
        }

        {
            int data_len = klb_buf_data_len(p_buf);

            if (data_len <= 0)
            {
                klb_buf_t* p_tmp = klb_nlist_pop_head(p_write_nlist);
                KLB_FREE_BY(p_tmp, klb_buf_unref);
                continue;
            }

            {
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
                    break;
                }

                // step3. 写侧致命错
                klb_socket_recv_closed(p_socket, close_status);
                push_netcode_klb_mnprpc_conn(p_conn, close_status);
                return write_num;
            }
        }
    }

    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
        push_netcode_klb_mnprpc_conn(p_conn, KLB_NETCODE_WBUF_EMPTY);
    }

    return write_num;
}

static int do_recv_klb_mnprpc_conn(klb_netconn_t* p_conn, int* p_read_num)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    klb_buf_t* p_buf = p_rpc->p_read_buf;
    int idle_len = klb_buf_idle_len(p_buf);

    if (idle_len <= 0)
    {
        int ret_parse = parser_klb_mnprpc_conn(p_conn);

        if (0 != ret_parse)
        {
            return -2;
        }

        idle_len = klb_buf_idle_len(p_buf);
        if (idle_len <= 0)
        {
            return -2;
        }
    }

    {
        int recv_len = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), idle_len);
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        // step1. 有数据: 入缓冲, 下方统一 parse 后继续排空
        if (0 < recv_len)
        {
            p_buf->end += recv_len;
            *p_read_num += recv_len;
        }
        else
        {
            // step2. 判定 TLS/非 TLS 关断
            judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);

            if (KLB_SOCKET_RECV_AGAIN == judge)
            {
                return 1;
            }

            // step3. EOF/ERROR: 若有残留先 parse, 再关
            if (p_buf->start < p_buf->end)
            {
                if (0 != parser_klb_mnprpc_conn(p_conn))
                {
                    return -2;
                }
            }

            klb_socket_recv_closed(p_socket, close_status);
            push_netcode_klb_mnprpc_conn(p_conn, close_status);
            return -1;
        }
    }

    if (0 != parser_klb_mnprpc_conn(p_conn))
    {
        return -2;
    }

    return 0;
}

static int klb_mnprpc_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_socket_t* p_socket = p_conn->p_socket;
    int read_num = 0;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    while (true)
    {
        int ret = do_recv_klb_mnprpc_conn(p_conn, &read_num);

        if (-1 == ret)
        {
            // do_recv 内已 recv_closed + push (EOF/ERROR)
            break;
        }

        if (-2 == ret)
        {
            klb_socket_recv_closed(p_socket, KLB_SOCKET_ERR_PROTOCOL);
            push_netcode_klb_mnprpc_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL);
            break;
        }

        if (0 != ret)
        {
            break;
        }
    }

    return read_num;
}

static int klb_mnprpc_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        push_netcode_klb_mnprpc_conn(p_conn, KLB_SOCKET_TIMEOUT);
        break;

    case KLB_NETCONN_MSG_connected:
        push_netcode_klb_mnprpc_conn(p_conn, KLB_SOCKET_CONNECT);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_ticker_klb_mnprpc_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}

static int klb_mnprpc_conn_init(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    p_conn->vtable.destroy = NULL;
    p_conn->vtable.recv_data = NULL;
    p_conn->vtable.ioctrl = klb_mnprpc_conn_ioctrl;
    p_conn->vtable.send_normal = klb_mnprpc_conn_send_normal;
    p_conn->vtable.send_media = klb_mnprpc_conn_send_media;
    p_conn->vtable.on_send = klb_mnprpc_conn_on_send;
    p_conn->vtable.on_recv = klb_mnprpc_conn_on_recv;
    p_conn->vtable.on_msg = klb_mnprpc_conn_on_msg;
    p_conn->protocol = KLB_PROTOCOL_MNP;

    p_rpc->p_write_nlist = klb_nlist_create();
    p_rpc->p_read_buf = klb_buf_malloc((int)KLB_MNP_BLOCK_SIZE_MAX, false);
    p_rpc->left_len = 0;
    p_rpc->p_rpc = NULL;
    p_rpc->asm_open = false;
    p_rpc->asm_packtype = -1;
    p_rpc->server_heartbeat = false;
    normalize_ping_klb_mnprpc_conn(p_rpc, KLB_MNPRPC_PING_INTERVAL_MS_DEFAULT, KLB_MNPRPC_PING_TIMEOUT_MS_DEFAULT);
    p_rpc->last_ping_tc = 0;
    p_rpc->ping_deadline_tc = 0;
    p_rpc->ping_waiting = false;
    p_rpc->op_deadline_tc = 0;
    p_rpc->max_msg = KLB_MNPRPC_MAX_MSG_DEFAULT;

    return 0;
}

static void klb_mnprpc_conn_quit(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    while (0 < klb_nlist_size(p_rpc->p_write_nlist))
    {
        klb_buf_t* p_tmp = klb_nlist_pop_head(p_rpc->p_write_nlist);
        KLB_FREE_BY(p_tmp, klb_buf_unref);
    }

    KLB_FREE_BY(p_rpc->p_rpc, klb_buffer_destroy);
    KLB_FREE_BY(p_rpc->p_read_buf, klb_buf_unref);
    KLB_FREE_BY(p_rpc->p_write_nlist, klb_nlist_destroy);
}

static int send_rpc_klb_mnprpc_conn(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len)
{
    klb_mnp_rpc_t rpc = { 0 };
    klb_buf_t* p_payload = NULL;
    int payload_len = 0;
    int ret = 1;

    if (body_len < 0)
    {
        body_len = 0;
    }

    payload_len = (int)sizeof(klb_mnp_rpc_t) + body_len;
    rpc.sequence = (uint32_t)sequence;
    rpc.rpctype = (uint32_t)rpctype;
    rpc.method = (uint32_t)method;
    rpc.encode = KLB_MNP_ENCODE_NULL;
    rpc.size = (uint32_t)payload_len;

    p_payload = klb_buf_malloc(payload_len, false);
    klb_buf_write(p_payload, (const char*)&rpc, (int)sizeof(rpc));

    if (0 < body_len && NULL != p_body)
    {
        klb_buf_write(p_payload, (const char*)p_body, body_len);
    }

    ret = enqueue_payload_klb_mnprpc_conn(p_conn, rpctype,
        (const uint8_t*)(p_payload->p_buf + p_payload->start), klb_buf_data_len(p_payload));
    KLB_FREE_BY(p_payload, klb_buf_unref);
    return ret;
}


void klb_mnpclientrpc_conn_free(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    klb_netmulti_closing(p_rpc->p_netmulti, p_conn);
    p_rpc->p_netmulti = NULL;
}

int klb_mnpclientrpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len)
{
    return send_rpc_klb_mnprpc_conn(p_conn, rpctype, method, sequence, p_body, body_len);
}

int klb_mnpclientrpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data)
{
    int body_len = 0;
    const uint8_t* p_body = NULL;
    int ret = 1;

    if (NULL != p_data)
    {
        body_len = klb_buf_data_len(p_data);
        p_body = (const uint8_t*)(p_data->p_buf + p_data->start);
    }

    ret = send_rpc_klb_mnprpc_conn(p_conn, rpctype, method, sequence, p_body, body_len);
    if (0 == ret)
    {
        KLB_FREE_BY(p_data, klb_buf_unref);
    }

    return ret;
}

bool klb_mnpclientrpc_wbuf_is_empty(klb_netconn_t* p_conn)
{
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    if (0 < klb_nlist_size(p_rpc->p_write_nlist))
    {
        return false;
    }

    return true;
}

klb_netconn_t* klb_mnpclientrpc_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code)
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

    p_netconn = klb_mnpclientrpc_conn_create(p_netmulti, p_socket);
    klb_netmulti_push(p_netmulti, p_netconn);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_netconn;
}

klb_netconn_t* klb_mnpclientrpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_mnprpc_conn_t));
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    klb_mnprpc_conn_init(p_conn);
    p_conn->vtable.destroy = klb_mnprpc_conn_destroy;
    p_rpc->p_netmulti = p_netmulti;
    p_conn->p_socket = p_socket;
    klb_socket_set_reading(p_socket, true);
    return p_conn;
}

klb_netconn_t* klb_mnpserverpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = klb_mnpclientrpc_conn_create(p_netmulti, p_socket);
    klb_mnprpc_conn_t* p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;

    p_rpc->server_heartbeat = true;
    return p_conn;
}

void klb_mnpserverpc_conn_free(klb_netconn_t* p_conn)
{
    klb_mnpclientrpc_conn_free(p_conn);
}

int klb_mnpserverpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len)
{
    return klb_mnpclientrpc_conn_send(p_conn, rpctype, method, sequence, p_body, body_len);
}

int klb_mnpserverpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data)
{
    return klb_mnpclientrpc_conn_send_buf(p_conn, rpctype, method, sequence, p_data);
}


void klb_mnpclientrpc_conn_set_op_deadline(klb_netconn_t* p_conn, int64_t deadline_tc)
{
    klb_mnprpc_conn_t* p_rpc = NULL;
    assert(NULL != p_conn);
    p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    p_rpc->op_deadline_tc = (0 < deadline_tc) ? deadline_tc : 0;
}

void klb_mnpclientrpc_conn_set_max_msg(klb_netconn_t* p_conn, int64_t max_msg)
{
    klb_mnprpc_conn_t* p_rpc = NULL;
    assert(NULL != p_conn);
    p_rpc = (klb_mnprpc_conn_t*)p_conn->extra;
    p_rpc->max_msg = (0 < max_msg) ? max_msg : 0;
}

// end