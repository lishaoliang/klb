// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbrtsp/core/klb_rtspserve_conn.h"
#include "klbnet/klb_socket_udp.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_rbuf.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_h26x.h"
#include "klbplatform/klb_time.h"
#include "klbbase/klb_mnp.h"
#include "klbrtsp/core/klb_rtspparser.h"
#include "klbrtsp/core/klb_rtspclient_conn.h"
#include <string.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <errno.h>
#endif


/// @def   KLB_RTSPSERVE_rbuf_min
/// @brief 临时接收数据最小长度
#define KLB_RTSPSERVE_rbuf_min    (4096)

/// @def   KLB_RTSPSERVE_rbuf_max
/// @brief 临时接收数据最大长度
#define KLB_RTSPSERVE_rbuf_max    (65536)


/// @def   KLB_RTSPSERVE_rtp_payload
/// @brief 单个 RTP 负载最大字节, 超出则按 FU-A 分片
#define KLB_RTSPSERVE_rtp_payload    (1400)


/// @def   KLB_RTSPSERVE_rtcp_interval_ms
/// @brief RTCP SR 周期(毫秒); 发送过媒体后按此周期向对端反馈
#define KLB_RTSPSERVE_rtcp_interval_ms    (5000)


/// @def   KLB_RTSPSERVE_MAX_TRACKS
/// @brief 单连接最大 RTP 轨数
#define KLB_RTSPSERVE_MAX_TRACKS    (8)


/// @struct klb_rtspserve_track_t
/// @brief  RTP 轨(单路 RTP/RTCP 媒体流)
typedef struct klb_rtspserve_track_t_
{
    bool                                used;               ///< 是否使用
    bool                                is_audio;           ///< 音频轨
    uint8_t                             pt;                 ///< payload type
    uint8_t                             ch;                 ///< TCP interleaved 通道
    uint16_t                            seq;                ///< RTP 序号
    uint32_t                            ssrc;               ///< RTP SSRC
    uint32_t                            pkt_count;          ///< 已发 RTP 包数
    uint32_t                            octet_count;        ///< 已发 RTP 负载字节数
    uint32_t                            last_ts;            ///< 最近 RTP 时间戳
    bool                                use_udp;            ///< 走 UDP
    struct sockaddr_in                  udp_peer;           ///< 客户端 RTP 地址
    klb_netconn_t*                      p_udp_rtp;          ///< UDP RTP
    klb_netconn_t*                      p_udp_rtcp;         ///< UDP RTCP
}klb_rtspserve_track_t;


/// @struct klb_rtspserve_conn_t
/// @brief  RTSP客户端连接
typedef struct klb_rtspserve_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // write
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 待发送数据列表
    };
 
    // read
    struct
    {
        klb_rbuf_t*                 p_read_rbuf;        ///< 临时读取缓存
    };

    klb_rtspserve_track_t               tracks[KLB_RTSPSERVE_MAX_TRACKS]; ///< RTP 轨数组
    int                                 track_count;        ///< 轨数量
    bool                                stopped;            ///< 已停止读写
    int                                 read_timeout_ms;    ///< 读超时(毫秒); 0=不超时
    int                                 write_timeout_ms;   ///< 写超时(毫秒); 0=不超时
    int                                 idle_timeout_ms;    ///< 空闲超时(毫秒); 0=不超时
    int                                 rtcp_interval_ms;   ///< RTCP SR 周期(毫秒); 0=关
    int64_t                             last_rtcp_send_tc;  ///< 上次发 SR 时刻
    uint32_t                            rtcp_rr_count;      ///< 收到对端 RR 计数
}klb_rtspserve_conn_t;



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_rtspserve_conn_quit(klb_netconn_t* p_conn);
static void handle_rtcp_klb_rtspserve_conn(klb_rtspserve_conn_t* p_rtsp, const uint8_t* p_data, int data_len);
static void on_udp_rtcp_klb_rtspserve_conn(void* p_udata, char* p_data, int data_len);
static void send_rtcp_sr_klb_rtspserve_conn(klb_netconn_t* p_conn, int track, int64_t now);


//////////////////////////////////////////////////////////////////////////

static void push_data_klb_rtspserve_conn(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (KLB_SOCKET_OK != code && KLB_SOCKET_CONNECT != code)
    {
        // 关闭 socket 读写
        p_rtsp->stopped = true;
        klb_socket_set_reading(p_conn->p_socket, false);
        klb_socket_set_writing(p_conn->p_socket, false);
    }

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, packtype, p_data);
    }
    else
    {
        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }
}

//////////////////////////////////////////////////////////////////////////
// 

/// @brief 处理收到的 RTCP 包. 逐条解析复合包, RR(201) 计数
/// @param [in]  p_data     RTCP 数据(不含 interleaved 4 字节头, 或 UDP 整包)
/// @param [in]  data_len   数据长度
static void handle_rtcp_klb_rtspserve_conn(klb_rtspserve_conn_t* p_rtsp, const uint8_t* p_data, int data_len)
{
    int off = 0;

    if (NULL == p_rtsp || NULL == p_data || 0 >= data_len)
    {
        return;
    }

    while (off + 4 <= data_len)
    {
        int pt = p_data[off + 1];
        int pack_len = (int)((KLB_RB16(p_data + off + 2) + 1) * 4);

        if (pack_len < 4 || off + pack_len > data_len)
        {
            break;
        }

        if (201 == pt)
        {
            p_rtsp->rtcp_rr_count += 1;
        }

        off += pack_len;
    }
}


static void on_udp_rtcp_klb_rtspserve_conn(void* p_udata, char* p_data, int data_len)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_udata;
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (p_rtsp->stopped)
    {
        return;
    }

    handle_rtcp_klb_rtspserve_conn(p_rtsp, (const uint8_t*)p_data, data_len);
}


/// @brief 向对端 RTCP 端口发送 SR(200) 反馈. 仅在 UDP 媒体已建立时生效
/// @param [in]  track   轨索引
/// @param [in]  now     当前时刻(毫秒); NTP 字段为近似值
static void send_rtcp_sr_klb_rtspserve_conn(klb_netconn_t* p_conn, int track, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_netconn_t* p_udp = NULL;
    struct sockaddr_in peer;
    uint8_t sr[28];
    uint32_t ssrc = 0;
    uint32_t last_ts = 0;
    uint32_t pkt_count = 0;
    uint32_t octet_count = 0;

    klb_rtspserve_track_t* p_track = NULL;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return;
    }

    p_track = &p_rtsp->tracks[track];

    if (!p_track->use_udp || NULL == p_track->p_udp_rtcp || NULL == p_track->p_udp_rtcp->p_socket)
    {
        return;
    }

    p_udp = p_track->p_udp_rtcp;
    peer = p_track->udp_peer;
    ssrc = p_track->ssrc;
    last_ts = p_track->last_ts;
    pkt_count = p_track->pkt_count;
    octet_count = p_track->octet_count;

    // NTP 时间戳为近似值(相对 tick); 接收端主要用于统计与唇音同步参考
    uint32_t ntp_msw = (uint32_t)(now / 1000);
    uint32_t ntp_lsw = (uint32_t)((now % 1000) * 4294967LL);

    sr[0] = 0x80;
    sr[1] = 200;
    KLB_WB16(sr + 2, 6);
    KLB_WB32(sr + 4, ssrc);
    KLB_WB32(sr + 8, ntp_msw);
    KLB_WB32(sr + 12, ntp_lsw);
    KLB_WB32(sr + 16, last_ts);
    KLB_WB32(sr + 20, pkt_count);
    KLB_WB32(sr + 24, octet_count);

    peer.sin_port = htons((uint16_t)(ntohs(peer.sin_port) + 1));
    (void)klb_socket_udp_sendto(p_udp->p_socket, sr, 28, (const struct sockaddr*)&peer, (int)sizeof(peer));
}


// 解析从网络上接收到的数据
static int parse_recv_data_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_rbuf_t* p_read_rbuf = p_rtsp->p_read_rbuf;

    while (true)
    {
        // 直接读取的数据
        int data_len = 0;
        char* p_data = klb_rbuf_data(p_read_rbuf, &data_len);

        if (data_len <= 0)
        {
            break; // 数据长度不足
        }

        // 解析数据
        klb_rtspparser_t parser = { 0 };
        int ret = klb_rtspparser_parse(&parser, p_data, data_len);

        if (0 == ret)
        {
            int pack_len = parser.pack_len;
            char* ptr = p_data;

            if (KLB_RTSPTYPE_rtsp == parser.rtsptype)
            {
                // rtsp 完整数据包
                klb_buf_t* p_buf = klb_buf_malloc(pack_len, false);
                klb_buf_write(p_buf, ptr, pack_len);

                push_data_klb_rtspserve_conn(p_conn, 0, KLB_MNP_TEXT, p_buf);
            }
            else if(KLB_RTSPTYPE_rtp == parser.rtsptype)
            {
                // rtp 包
            }
            else if(KLB_RTSPTYPE_rtcp == parser.rtsptype)
            {
                // rtcp 包 (interleaved, 跳过 4 字节 "$ 通道 长度" 头)
                handle_rtcp_klb_rtspserve_conn(p_rtsp, (const uint8_t*)(ptr + 4), pack_len - 4);
            }

            klb_rbuf_use(p_read_rbuf, parser.pack_len);
        }
        else if(0 < ret)
        {
            // 数据长度不足
            break;
        }
        else
        {
            // 协议解析错误, 停止该连接
            push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL, 0, NULL);
            return -1;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// msg

// 握手 连接超时
static void on_msg_connect_timeout_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
}

// 握手完成
static void on_msg_connected_klb_rtspserve_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_CONNECT, 0, NULL);
}

// 定时器消息. 参考 mnp/smp: 读写/空闲超时判定与回收
static void on_msg_ticker_klb_rtspserve_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return;
    }

    bool writing = (0 < klb_nlist_size(p_rtsp->p_write_nlist));
    bool reading = (0 < klb_rbuf_datalen(p_rtsp->p_read_rbuf));

    // step1. 写超时: 有数据待发但长时间未能写空
    if (writing && 0 < p_rtsp->write_timeout_ms &&
        (int64_t)p_rtsp->write_timeout_ms <= ABS_SUB(now, p_socket->last_send_tc))
    {
        push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step2. 读超时: 有半帧待续但长时间无新数据到达
    if (reading && 0 < p_rtsp->read_timeout_ms &&
        (int64_t)p_rtsp->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step3. 空闲超时: 无读无写, 长时间无任何接收, 回收会话
    if (!writing && !reading && 0 < p_rtsp->idle_timeout_ms &&
        (int64_t)p_rtsp->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step4. RTCP: 发送过媒体后按周期向对端反馈 SR
    if (0 < p_rtsp->rtcp_interval_ms &&
        (0 == p_rtsp->last_rtcp_send_tc ||
         (int64_t)p_rtsp->rtcp_interval_ms <= ABS_SUB(now, p_rtsp->last_rtcp_send_tc)))
    {
        bool sent_media = false;

        for (int index = 0; index < p_rtsp->track_count; index++)
        {
            if (0 < p_rtsp->tracks[index].pkt_count)
            {
                send_rtcp_sr_klb_rtspserve_conn(p_conn, index, now);
                sent_media = true;
            }
        }

        if (sent_media)
        {
            p_rtsp->last_rtcp_send_tc = now;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_rtspserve_conn_destroy(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    for (int index = 0; index < KLB_RTSPSERVE_MAX_TRACKS; index++)
    {
        klb_rtspserve_track_t* p_track = &p_rtsp->tracks[index];

        klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_track->p_udp_rtp, p_track->p_udp_rtcp);
        p_track->p_udp_rtp = NULL;
        p_track->p_udp_rtcp = NULL;
        p_track->use_udp = false;
    }

    if (NULL != p_rtsp->p_netmulti)
    {
        klb_netmulti_remove(p_rtsp->p_netmulti, p_conn);
        p_rtsp->p_netmulti = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    klb_rtspserve_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_rtspserve_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    return 0;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_rtspserve_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (KLB_MNP_TEXT == packtype)
    {
        klb_buf_t* p_buf = klb_buf_malloc(head_len + body_len, false);

        if (0 < head_len) { klb_buf_write(p_buf, p_head, head_len); }
        if (0 < body_len) { klb_buf_write(p_buf, p_body, body_len); }


        klb_nlist_push_tail(p_rtsp->p_write_nlist, p_buf);
        klb_socket_set_writing(p_conn->p_socket, true);
    }

    return 0;
}

static void send_udp_rtp_klb_rtspserve_conn(klb_netconn_t* p_conn, int track, uint32_t timestamp, bool marker, const uint8_t* p_pay0, int pay0_len, const uint8_t* p_pay1, int pay1_len)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_rtspserve_track_t* p_track = NULL;
    klb_netconn_t* p_udp = NULL;
    int payload_len = 0;
    int rtp_len = 0;
    uint8_t pkt[1600];
    uint8_t* p_dst = NULL;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return;
    }

    p_track = &p_rtsp->tracks[track];

    if (!p_track->use_udp || NULL == p_track->p_udp_rtp || NULL == p_track->p_udp_rtp->p_socket)
    {
        return;
    }

    payload_len = pay0_len + pay1_len;
    rtp_len = 12 + payload_len;
    p_udp = p_track->p_udp_rtp;

    if (payload_len <= 0 || rtp_len > (int)sizeof(pkt))
    {
        return;
    }

    pkt[0] = 0x80;
    pkt[1] = (uint8_t)((marker ? 0x80 : 0x00) | (p_track->pt & 0x7F));
    KLB_WB16(pkt + 2, p_track->seq);
    p_track->seq = (uint16_t)(p_track->seq + 1);
    KLB_WB32(pkt + 4, timestamp);
    KLB_WB32(pkt + 8, p_track->ssrc);

    p_dst = pkt + 12;

    if (0 < pay0_len && NULL != p_pay0)
    {
        memcpy(p_dst, p_pay0, (size_t)pay0_len);
        p_dst += pay0_len;
    }

    if (0 < pay1_len && NULL != p_pay1)
    {
        memcpy(p_dst, p_pay1, (size_t)pay1_len);
    }

    (void)klb_socket_udp_sendto(p_udp->p_socket, pkt, rtp_len,
        (const struct sockaddr*)&p_track->udp_peer, (int)sizeof(p_track->udp_peer));
}


static void enqueue_rtp_klb_rtspserve_conn(klb_netconn_t* p_conn, int track, uint32_t timestamp, bool marker, const uint8_t* p_pay0, int pay0_len, const uint8_t* p_pay1, int pay1_len)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_rtspserve_track_t* p_track = NULL;
    int payload_len = pay0_len + pay1_len;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return;
    }

    if (payload_len <= 0)
    {
        return;
    }

    p_track = &p_rtsp->tracks[track];

    // RTCP SR 统计
    p_track->pkt_count += 1;
    p_track->octet_count += (uint32_t)payload_len;
    p_track->last_ts = timestamp;

    if (p_track->use_udp)
    {
        send_udp_rtp_klb_rtspserve_conn(p_conn, track, timestamp, marker, p_pay0, pay0_len, p_pay1, pay1_len);
        return;
    }

    int rtp_len = 12 + payload_len;
    int pack_len = 4 + rtp_len;
    klb_buf_t* p_buf = klb_buf_malloc(pack_len, false);
    uint8_t hdr[16];

    hdr[0] = 0x24;
    hdr[1] = p_track->ch;
    KLB_WB16(hdr + 2, (uint16_t)rtp_len);
    hdr[4] = 0x80;
    hdr[5] = (uint8_t)((marker ? 0x80 : 0x00) | (p_track->pt & 0x7F));
    KLB_WB16(hdr + 6, p_track->seq);
    p_track->seq = (uint16_t)(p_track->seq + 1);
    KLB_WB32(hdr + 8, timestamp);
    KLB_WB32(hdr + 12, p_track->ssrc);

    klb_buf_write(p_buf, (const char*)hdr, 16);

    if (0 < pay0_len && NULL != p_pay0)
    {
        klb_buf_write(p_buf, (const char*)p_pay0, pay0_len);
    }

    if (0 < pay1_len && NULL != p_pay1)
    {
        klb_buf_write(p_buf, (const char*)p_pay1, pay1_len);
    }

    klb_nlist_push_tail(p_rtsp->p_write_nlist, p_buf);
    klb_socket_set_writing(p_conn->p_socket, true);
}


static void enqueue_h264_nalu_klb_rtspserve_conn(klb_netconn_t* p_conn, int track, uint32_t timestamp, const uint8_t* p_nalu, int nalu_len, bool is_last)
{
    if (NULL == p_nalu || nalu_len <= 0)
    {
        return;
    }

    uint8_t nalu_type = (uint8_t)(p_nalu[0] & 0x1F);
    bool is_vcl = (1 <= nalu_type && 5 >= nalu_type);

    if (nalu_len <= KLB_RTSPSERVE_rtp_payload)
    {
        bool marker = (is_vcl && is_last);
        enqueue_rtp_klb_rtspserve_conn(p_conn, track, timestamp, marker, p_nalu, nalu_len, NULL, 0);
        return;
    }

    uint8_t fu_ind = (uint8_t)((p_nalu[0] & 0xE0) | KLB_RTP_NALU_FU_A);
    uint8_t fu_type = (uint8_t)(p_nalu[0] & 0x1F);
    const uint8_t* p_body = p_nalu + 1;
    int body_len = nalu_len - 1;
    int off = 0;
    bool first = true;

    if (body_len <= 0)
    {
        return;
    }

    while (off < body_len)
    {
        int chunk = body_len - off;
        bool last_frag = false;
        uint8_t fu_hdr[2];
        bool marker = false;

        if (chunk > KLB_RTSPSERVE_rtp_payload)
        {
            chunk = KLB_RTSPSERVE_rtp_payload;
        }

        last_frag = (off + chunk >= body_len);
        fu_hdr[0] = fu_ind;
        fu_hdr[1] = fu_type;

        if (first)
        {
            fu_hdr[1] = (uint8_t)(fu_hdr[1] | 0x80);
        }

        if (last_frag)
        {
            fu_hdr[1] = (uint8_t)(fu_hdr[1] | 0x40);
        }

        marker = (is_vcl && is_last && last_frag);
        enqueue_rtp_klb_rtspserve_conn(p_conn, track, timestamp, marker, fu_hdr, 2, p_body + off, chunk);

        off += chunk;
        first = false;
    }
}


static int send_h264_klb_rtspserve_conn(klb_netconn_t* p_conn, klb_buf_t* p_data, int track)
{
    int data_len = klb_buf_data_len(p_data);

    if (data_len < (int)sizeof(klb_mnp_media_t))
    {
        return -1;
    }

    klb_mnp_media_t* p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);

    if (KLB_MNP_DTYPE_H264 != p_media->dtype)
    {
        return -1;
    }

    int pack_len = (int)p_media->size;
    int head_len = (int)sizeof(klb_mnp_media_t);

    if (pack_len <= 0 || pack_len > data_len)
    {
        pack_len = data_len;
    }

    if (pack_len <= head_len)
    {
        return -1;
    }

    char* p_h26x = (char*)(p_data->p_buf + p_data->start + head_len);
    int h26x_len = pack_len - head_len;
    uint32_t rtp_ts = (uint32_t)((uint64_t)p_media->time * 90ull);
    int nalu_count = 0;
    int scan_off = 0;

    while (scan_off < h26x_len)
    {
        int nal_start = 0;
        int nal_len = 0;
        int nal_h = 0;
        bool is_end = false;
        int ret = klb_h26x_scan_nalu(p_h26x + scan_off, h26x_len - scan_off, &nal_start, &nal_len, &nal_h, &is_end);

        if (0 != ret || nal_len <= nal_h)
        {
            break;
        }

        nalu_count += 1;

        if (is_end)
        {
            break;
        }

        scan_off += nal_start + nal_len;
    }

    if (0 == nalu_count)
    {
        enqueue_h264_nalu_klb_rtspserve_conn(p_conn, track, rtp_ts, (const uint8_t*)p_h26x, h26x_len, true);
        return 0;
    }

    scan_off = 0;

    for (int index = 0; index < nalu_count; index++)
    {
        int nal_start = 0;
        int nal_len = 0;
        int nal_h = 0;
        bool is_end = false;
        int ret = klb_h26x_scan_nalu(p_h26x + scan_off, h26x_len - scan_off, &nal_start, &nal_len, &nal_h, &is_end);

        if (0 != ret || nal_len <= nal_h)
        {
            break;
        }

        const uint8_t* p_nalu = (const uint8_t*)(p_h26x + scan_off + nal_start + nal_h);
        int nalu_len = nal_len - nal_h;
        bool is_last = (index + 1 >= nalu_count);

        enqueue_h264_nalu_klb_rtspserve_conn(p_conn, track, rtp_ts, p_nalu, nalu_len, is_last);

        if (is_end)
        {
            break;
        }

        scan_off += nal_start + nal_len;
    }

    return 0;
}


static int send_aac_klb_rtspserve_conn(klb_netconn_t* p_conn, klb_buf_t* p_data, int track)
{
    int data_len = klb_buf_data_len(p_data);
    klb_mnp_media_t* p_media = NULL;
    klb_mnp_audio_t* p_audio = NULL;
    char* p_aac = NULL;
    int aac_len = 0;
    int head_len = (int)(sizeof(klb_mnp_media_t) + sizeof(klb_mnp_audio_t));
    int pack_len = 0;
    uint32_t rtp_ts = 0;
    uint8_t au_hdr[4];
    uint32_t samples = 44100;

    if (data_len < head_len)
    {
        return -1;
    }

    p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);

    if (KLB_MNP_DTYPE_AAC != p_media->dtype)
    {
        return -1;
    }

    pack_len = (int)p_media->size;

    if (pack_len <= 0 || pack_len > data_len)
    {
        pack_len = data_len;
    }

    if (pack_len <= head_len)
    {
        return -1;
    }

    p_audio = (klb_mnp_audio_t*)(p_data->p_buf + p_data->start + sizeof(klb_mnp_media_t));
    p_aac = (char*)(p_data->p_buf + p_data->start + head_len);
    aac_len = pack_len - head_len;

    if (0 < p_audio->samples)
    {
        samples = p_audio->samples;
    }

    // RTP clock = sample rate; time field is ms
    rtp_ts = (uint32_t)((uint64_t)p_media->time * (uint64_t)samples / 1000ull);

    // RFC3640 AAC-hbr: AU-headers-length(16bit)=16, AU-size(13)+index(3)
    if (aac_len <= 0 || aac_len > 0x1FFF)
    {
        return -1;
    }

    KLB_WB16(au_hdr, 16);
    KLB_WB16(au_hdr + 2, (uint16_t)((aac_len << 3) & 0xFFF8));

    enqueue_rtp_klb_rtspserve_conn(p_conn, track, rtp_ts, true, au_hdr, 4, (const uint8_t*)p_aac, aac_len);
    return 0;
}


/// @brief 按媒体头选择目标轨. 优先用 sidx 显式路由(V1..V3/A1..A3),
///        否则按 dtype 取首个同类轨(H264->首个非音频轨; AAC->首个音频轨)
/// @return int 轨索引; <0 表示无匹配
static int find_track_klb_rtspserve_conn(klb_rtspserve_conn_t* p_rtsp, const klb_mnp_media_t* p_media)
{
    bool is_audio = (KLB_MNP_DTYPE_AAC == p_media->dtype);
    int want = 0;
    int pos = 0;
    int track = -1;
    int index = 0;
    int sidx = (int)p_media->sidx;

    // 显式流序号: V1..V3 视频 / A1..A3 音频
    if (KLB_MNP_SIDX_V1 <= sidx && KLB_MNP_SIDX_V3 >= sidx)
    {
        is_audio = false;
        want = sidx - (int)KLB_MNP_SIDX_V1;
    }
    else if (KLB_MNP_SIDX_A1 <= sidx && KLB_MNP_SIDX_A3 >= sidx)
    {
        is_audio = true;
        want = sidx - (int)KLB_MNP_SIDX_A1;
    }

    for (index = 0; index < p_rtsp->track_count; index++)
    {
        if (!p_rtsp->tracks[index].used || p_rtsp->tracks[index].is_audio != is_audio)
        {
            continue;
        }

        if (pos >= want)
        {
            track = index;
            break;
        }

        pos += 1;
    }

    return track;
}


/// @brief 发送媒体数据包
/// @return int
static int klb_rtspserve_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    int ret = -1;

    if (NULL != p_data)
    {
        int data_len = klb_buf_data_len(p_data);

        if ((int)sizeof(klb_mnp_media_t) <= data_len)
        {
            klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
            klb_mnp_media_t* p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);
            int track = find_track_klb_rtspserve_conn(p_rtsp, p_media);

            if (0 > track)
            {
                ret = -1;
            }
            else if (KLB_MNP_DTYPE_H264 == p_media->dtype)
            {
                ret = send_h264_klb_rtspserve_conn(p_conn, p_data, track);
            }
            else if (KLB_MNP_DTYPE_AAC == p_media->dtype)
            {
                ret = send_aac_klb_rtspserve_conn(p_conn, p_data, track);
            }
        }

        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }

    return ret;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_rtspserve_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_nlist_t* p_write_nlist = p_rtsp->p_write_nlist;
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
            KLB_FREE_BY(p_tmp, klb_buf_unref_next);

            continue;
        }

        uint8_t* p_data = p_buf->p_buf + p_buf->start;
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
        push_data_klb_rtspserve_conn(p_conn, close_status, 0, NULL);
        return write_num;
    }

    // 数据写完了, 无需读取
    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
    }

    return write_num;
}

static char* idle_recv_klb_rtspserve_conn(klb_rbuf_t* p_read_rbuf, int* p_idle_len)
{
    int idle_len = 0;
    char* p_idle = klb_rbuf_idle(p_read_rbuf, 0, &idle_len);

    if (NULL == p_idle)
    {
        int buf_len = klb_rbuf_buflen(p_read_rbuf);

        if (buf_len < KLB_RTSPSERVE_rbuf_max)
        {
            int add_len = KLB_RTSPSERVE_rbuf_min;

            if (buf_len + add_len > KLB_RTSPSERVE_rbuf_max)
            {
                add_len = KLB_RTSPSERVE_rbuf_max - buf_len;
            }

            if (0 < add_len)
            {
                p_idle = klb_rbuf_idle(p_read_rbuf, add_len, &idle_len);
            }
        }
    }

    if (NULL != p_idle_len)
    {
        *p_idle_len = idle_len;
    }

    return p_idle;
}



/// @brief 当网络上可以接收数据时
/// @return int
static int klb_rtspserve_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;

    if (p_rtsp->stopped)
    {
        return 0;
    }

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_rbuf_t* p_read_rbuf = p_rtsp->p_read_rbuf;
    int read_num = 0;

    while (true)
    {
        int idle_len = 0;
        char* p_idle = idle_recv_klb_rtspserve_conn(p_read_rbuf, &idle_len);

        if (NULL == p_idle)
        {
            push_data_klb_rtspserve_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL, 0, NULL);
            break;
        }

        int recv_len = klb_socket_recv(p_socket, (uint8_t*)p_idle, idle_len);
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_read_rbuf, recv_len);
            read_num += recv_len;

            if (0 != parse_recv_data_klb_rtspserve_conn(p_conn))
            {
                break;
            }

            klb_rbuf_memmove(p_read_rbuf);
            continue;
        }

        judge = klb_socket_recv_judge(p_socket, recv_len, &close_status);
        if (KLB_SOCKET_RECV_AGAIN == judge)
        {
            break;
        }

        // EOF/ERROR: 先尽量解析残留, 再关闭并通知
        {
            int left = 0;

            (void)klb_rbuf_data(p_read_rbuf, &left);
            if (0 < left)
            {
                (void)parse_recv_data_klb_rtspserve_conn(p_conn);
            }
        }

        klb_socket_recv_closed(p_socket, close_status);
        push_data_klb_rtspserve_conn(p_conn, close_status, 0, NULL);
        break;
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_rtspserve_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_msg_connect_timeout_klb_rtspserve_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_connected:
        on_msg_connected_klb_rtspserve_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_msg_ticker_klb_rtspserve_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_rtspserve_conn_init(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 初始化 vtable
    p_conn->vtable.destroy = NULL; // 销毁
    p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

    p_conn->vtable.ioctrl = klb_rtspserve_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
    p_conn->vtable.send_normal = klb_rtspserve_conn_send_normal; // 调用者 发送常规数据包
    p_conn->vtable.send_media = klb_rtspserve_conn_send_media; // 调用者 发送媒体数据包
    p_conn->vtable.on_send = klb_rtspserve_conn_on_send; // 当网络上可以发送数据时
    p_conn->vtable.on_recv = klb_rtspserve_conn_on_recv; // 当网络上可以接收数据时
    p_conn->vtable.on_msg = klb_rtspserve_conn_on_msg; // 当网络上有消息传来时

    // 初始化其他

    p_rtsp->p_write_nlist = klb_nlist_create();

    // 读
    p_rtsp->p_read_rbuf = klb_rbuf_malloc(KLB_RTSPSERVE_rbuf_min);

    // 默认两轨: [0]视频, [1]音频
    {
        klb_rtspserve_track_t* p_video = &p_rtsp->tracks[0];
        klb_rtspserve_track_t* p_audio = &p_rtsp->tracks[1];

        p_video->used = true;
        p_video->is_audio = false;
        p_video->pt = 96;
        p_video->ch = 0;
        p_video->ssrc = klb_tick_count();

        p_audio->used = true;
        p_audio->is_audio = true;
        p_audio->pt = 97;
        p_audio->ch = 2;
        p_audio->ssrc = p_video->ssrc ^ 0xA5A5A5A5u;

        if (0 == p_video->ssrc)
        {
            p_video->ssrc = 1;
        }

        if (0 == p_audio->ssrc)
        {
            p_audio->ssrc = 2;
        }

        p_rtsp->track_count = 2;
    }

    p_rtsp->rtcp_interval_ms = KLB_RTSPSERVE_rtcp_interval_ms;

    return 0;
}

static void klb_rtspserve_conn_quit(klb_netconn_t* p_conn)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 释放内存
    if (NULL != p_rtsp->p_write_nlist)
    {
        while (0 < klb_nlist_size(p_rtsp->p_write_nlist))
        {
            klb_buf_t* p_buf = klb_nlist_pop_head(p_rtsp->p_write_nlist);
            KLB_FREE_BY(p_buf, klb_buf_unref_next);
        }
    }

    // 销毁
    KLB_FREE_BY(p_rtsp->p_write_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_rtsp->p_read_rbuf, klb_rbuf_free);
}


//////////////////////////////////////////////////////////////////////////

/// @brief 创建rtsp连接
/// @return klb_netconn_t*
klb_netconn_t* klb_rtspserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_rtspserve_conn_t));
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    klb_rtspserve_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_rtspserve_conn_destroy;

    // socket
    {
        p_rtsp->p_netmulti = p_netmulti;
        p_conn->p_socket = p_socket;
    }

    {
        // 开启读写
        //klb_socket_set_writing(p_socket, true);
        klb_socket_set_reading(p_socket, true);
    }

    // 
    {
        // 放入复用模块
        klb_netmulti_push(p_netmulti, p_conn);
    }


    return p_conn;
}


static void close_udp_track_klb_rtspserve_conn(klb_netconn_t* p_conn, int track)
{
    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    klb_rtspserve_track_t* p_track = NULL;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return;
    }

    p_track = &p_rtsp->tracks[track];

    klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_track->p_udp_rtp, p_track->p_udp_rtcp);
    p_track->p_udp_rtp = NULL;
    p_track->p_udp_rtcp = NULL;
    p_track->use_udp = false;
}


int klb_rtspserve_conn_setup_tcp_track(klb_netconn_t* p_conn, int track, int rtp_ch)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;

    if (NULL == p_conn)
    {
        return -1;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return -1;
    }

    if (rtp_ch < 0 || rtp_ch > 254)
    {
        rtp_ch = 0;
    }

    close_udp_track_klb_rtspserve_conn(p_conn, track);
    p_rtsp->tracks[track].ch = (uint8_t)rtp_ch;

    return 0;
}


void klb_rtspserve_conn_setup_tcp(klb_netconn_t* p_conn, int rtp_ch)
{
    // 旧语义: ch < 2 视频轨(轨0); 否则音频轨(轨1)
    int track = (2 > rtp_ch) ? 0 : 1;

    klb_rtspserve_conn_setup_tcp_track(p_conn, track, rtp_ch);
}


int klb_rtspserve_conn_setup_udp_track(klb_netconn_t* p_conn, int track, int client_rtp_port, int* p_server_rtp_port)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;
    klb_rtspserve_track_t* p_track = NULL;
    struct sockaddr_in peer;
    int port = 0;
    int ret = 0;

#ifdef _WIN32
    int peer_len = (int)sizeof(peer);
#else
    socklen_t peer_len = (socklen_t)sizeof(peer);
#endif

    if (NULL == p_conn || NULL == p_server_rtp_port)
    {
        return -1;
    }

    if (client_rtp_port <= 0 || client_rtp_port > 65535)
    {
        return -1;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    *p_server_rtp_port = 0;

    if (0 > track || track >= p_rtsp->track_count)
    {
        return -1;
    }

    p_track = &p_rtsp->tracks[track];

    memset(&peer, 0, sizeof(peer));

    if (0 != getpeername(klb_socket_get_fd(p_conn->p_socket), (struct sockaddr*)&peer, &peer_len))
    {
        return -1;
    }

    if (AF_INET != peer.sin_family)
    {
        return -1;
    }

    peer.sin_port = htons((uint16_t)client_rtp_port);

    close_udp_track_klb_rtspserve_conn(p_conn, track);
    ret = klb_rtspudp_open_pair(p_rtsp->p_netmulti, p_conn, NULL, on_udp_rtcp_klb_rtspserve_conn,
        &p_track->p_udp_rtp, &p_track->p_udp_rtcp, &port);

    if (0 != ret)
    {
        close_udp_track_klb_rtspserve_conn(p_conn, track);
        return -1;
    }

    p_track->udp_peer = peer;
    p_track->use_udp = true;
    *p_server_rtp_port = port;

    return 0;
}


int klb_rtspserve_conn_setup_udp(klb_netconn_t* p_conn, int client_rtp_port, int* p_server_rtp_port)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;
    int track = -1;

    if (NULL == p_conn)
    {
        return -1;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 旧语义: 取首个尚未 UDP 的轨; 默认两轨时即 先视频(0) 后音频(1)
    for (int index = 0; index < p_rtsp->track_count; index++)
    {
        if (p_rtsp->tracks[index].used && !p_rtsp->tracks[index].use_udp)
        {
            track = index;
            break;
        }
    }

    if (0 > track)
    {
        return -1;
    }

    return klb_rtspserve_conn_setup_udp_track(p_conn, track, client_rtp_port, p_server_rtp_port);
}

void klb_rtspserve_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    if (NULL == p_conn)
    {
        return;
    }

    klb_rtspserve_conn_t* p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    p_rtsp->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_rtsp->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_rtsp->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}


void klb_rtspserve_conn_set_track_pt(klb_netconn_t* p_conn, int track, int pt, bool is_audio)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;
    klb_rtspserve_track_t* p_track = NULL;

    if (NULL == p_conn)
    {
        return;
    }

    if (0 > track || KLB_RTSPSERVE_MAX_TRACKS <= track)
    {
        return;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    // 索引超出已用轨: 扩展并初始化新轨
    if (track >= p_rtsp->track_count)
    {
        for (int index = p_rtsp->track_count; index <= track; index++)
        {
            klb_rtspserve_track_t* p_new = &p_rtsp->tracks[index];

            p_new->used = true;
            p_new->is_audio = false;
            p_new->pt = 0;
            p_new->ch = (uint8_t)(index * 2);
            p_new->ssrc = klb_tick_count() + (uint32_t)index * 2654435761u;

            if (0 == p_new->ssrc)
            {
                p_new->ssrc = (uint32_t)(index + 1);
            }
        }

        p_rtsp->track_count = track + 1;
    }

    p_track = &p_rtsp->tracks[track];

    p_track->used = true;
    p_track->is_audio = is_audio;

    if (0 < pt && 128 > pt)
    {
        p_track->pt = (uint8_t)pt;
    }
    else
    {
        p_track->pt = 0;
    }
}


void klb_rtspserve_conn_set_video_pt(klb_netconn_t* p_conn, int pt)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;

    if (NULL == p_conn)
    {
        return;
    }

    if (pt <= 0 || pt >= 128)
    {
        return;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;
    p_rtsp->tracks[0].pt = (uint8_t)pt;
}


void klb_rtspserve_conn_set_audio_pt(klb_netconn_t* p_conn, int pt)
{
    klb_rtspserve_conn_t* p_rtsp = NULL;

    if (NULL == p_conn)
    {
        return;
    }

    p_rtsp = (klb_rtspserve_conn_t*)p_conn->extra;

    if (pt <= 0 || pt >= 128)
    {
        p_rtsp->tracks[1].pt = 0;
        return;
    }

    p_rtsp->tracks[1].pt = (uint8_t)pt;
}

// end
