// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbrtsp/core/klb_rtspclient_conn.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klb_socket_udp.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_rbuf.h"
#include "klbmem/klb_buffer.h"
#include "klbmem/klb_buf_atom.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_h26x.h"
#include "klbplatform/klb_time.h"
#include "klbbase/klb_mnp.h"
#include "klbrtsp/core/klb_rtspparser.h"
#include <string.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <errno.h>
#endif



/// @def   KLB_RTSPCLIENT_rbuf_min
/// @brief 临时接收数据最小长度
#define KLB_RTSPCLIENT_rbuf_min    (4096)

/// @def   KLB_RTSPCLIENT_rbuf_max
/// @brief 临时接收数据最大长度
#define KLB_RTSPCLIENT_rbuf_max    (65536)

/// @def   KLB_RTSPCLIENT_rtcp_interval_ms
/// @brief RTCP RR 周期(毫秒); UDP 媒体建立后按此周期向对端反馈
#define KLB_RTSPCLIENT_rtcp_interval_ms    (5000)

/// @def   KLB_RTSPCLIENT_rtp_payload
/// @brief 出方向单个 RTP 负载最大字节, 超出则按 FU-A 分片
#define KLB_RTSPCLIENT_rtp_payload    (1400)


/// @struct klb_rtspclient_conn_t
/// @brief  RTSP客户端连接
typedef struct klb_rtspclient_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用

    // write
    struct
    {
        klb_nlist_t*                p_write_nlist;      ///< 待发送数据列表
    };
 
    // temp read
    struct
    {
        klb_rbuf_t*                 p_read_rbuf;        ///< 临时读取缓存
    };

    // media read
    struct
    {
        klb_rtspparser_t            parser;             ///< 上一个视频帧, 解析值

        klb_buffer_t*               p_video_buf;        ///< 视频缓存
        klb_buffer_t*               p_audio_buf;        ///< 音频缓存
    };

    uint8_t                             video_pt;           ///< H264 RTP payload type
    uint8_t                             audio_pt;           ///< AAC RTP payload type; 0=未启用
    uint32_t                            audio_samples;      ///< 音频采样率
    uint8_t                             audio_tracks;       ///< 声道数
    uint8_t                             audio_bits;         ///< bits_per_sample 字段(1=8bit,2=16bit)
    uint16_t                            rtp_seq;            ///< 视频出方向 RTP 序号
    uint16_t                            audio_seq;          ///< 音频出方向 RTP 序号
    uint32_t                            rtp_ssrc;           ///< 视频出方向 RTP SSRC
    uint32_t                            audio_ssrc;         ///< 音频出方向 RTP SSRC
    uint8_t                             rtp_ch;             ///< 视频出方向 interleaved 通道; 默认 0
    uint8_t                             audio_ch;           ///< 音频出方向 interleaved 通道; 默认 2
    bool                                stopped;            ///< 已停止读写
    bool                                udp_last_audio;     ///< 最近一次 open_udp 打开的是音频对
    int                                 read_timeout_ms;    ///< 读超时(毫秒); 0=不超时
    int                                 write_timeout_ms;   ///< 写超时(毫秒); 0=不超时
    int                                 idle_timeout_ms;    ///< 空闲超时(毫秒); 0=不超时

    klb_netconn_t*                      p_udp_rtp;          ///< 视频 UDP RTP 子连接
    klb_netconn_t*                      p_udp_rtcp;         ///< 视频 UDP RTCP 子连接
    int                                 udp_port;           ///< 视频 UDP RTP 端口, 0 表示未打开

    klb_netconn_t*                      p_udp_audio_rtp;    ///< 音频 UDP RTP 子连接
    klb_netconn_t*                      p_udp_audio_rtcp;   ///< 音频 UDP RTCP 子连接
    int                                 udp_audio_port;     ///< 音频 UDP RTP 端口

    struct sockaddr_in                  udp_peer;           ///< 对端 RTP 地址(udp_punch 记录)
    int                                 udp_peer_rtp_port;  ///< 对端 RTP 端口; 0=未记录
    bool                                udp_peer_audio;     ///< udp_peer 属音频对
    int                                 rtcp_interval_ms;   ///< RTCP RR 周期(毫秒); 0=关
    int64_t                             last_rtcp_send_tc;  ///< 上次发 RR 时刻
    uint32_t                            rtcp_sr_count;      ///< 收到对端 SR 计数
    bool                                rtcp_bye;           ///< 收到对端 RTCP BYE
}klb_rtspclient_conn_t;



//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klb_rtspclient_conn_quit(klb_netconn_t* p_conn);
static void consume_h264_rtp_klb_rtspclient_conn(klb_netconn_t* p_conn, char* p_base, klb_rtspparser_t* p_parser);
static void consume_aac_rtp_klb_rtspclient_conn(klb_netconn_t* p_conn, char* p_base, klb_rtspparser_t* p_parser);
static void handle_rtcp_klb_rtspclient_conn(klb_rtspclient_conn_t* p_rtsp, const uint8_t* p_data, int data_len);
static void send_periodic_rr_klb_rtspclient_conn(klb_netconn_t* p_conn, int64_t now);


//////////////////////////////////////////////////////////////////////////

static void push_data_klb_rtspclient_conn(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

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


static void push_video_data_klb_rtspclient_conn(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_buffer_t* p_video_buf = p_rtsp->p_video_buf;

    int data_len = klb_buffer_datalen(p_video_buf);
    if (data_len <= 0)
    {
        return;
    }

    klb_buf_t* p_data = klb_buffer_join(p_video_buf, klb_buf_atom_malloc, NULL);
    klb_buffer_reset(p_video_buf);

    klb_mnp_media_t* p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);
    p_media->size = klb_buf_data_len(p_data);

    push_data_klb_rtspclient_conn(p_conn, 0, KLB_MNP_MEDIA, p_data);
}


static void consume_h264_rtp_klb_rtspclient_conn(klb_netconn_t* p_conn, char* p_base, klb_rtspparser_t* p_parser)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    char* p_nalu = p_base + p_parser->nalu_pos;
    int nalu_len = p_parser->nalu_len;
    int nalu_pos = 0;

    while (0 == klb_rtspparser_parse_rtp_h264_step(p_parser, p_nalu, nalu_len, &nalu_pos))
    {
        if (KLB_MNP_BEGIN == p_parser->opt || KLB_MNP_FULL == p_parser->opt)
        {
            push_video_data_klb_rtspclient_conn(p_conn);
        }

        if (NULL != p_parser->p_data && 0 < p_parser->data_len)
        {
            klb_buffer_t* p_video_buf = p_rtsp->p_video_buf;

            if (KLB_MNP_BEGIN == p_parser->opt || KLB_MNP_FULL == p_parser->opt)
            {
                int nalu_type = klb_h264_nalu_type(p_parser->nalu_value);
                klb_mnp_media_t media = { 0 };
                uint8_t nalu_h26x[4] = { 0x0, 0x0, 0x0, 0x1 };
                uint8_t nalu_value = 0;

                media.dtype = KLB_MNP_DTYPE_H264;
                media.time = p_parser->rtp_head.timestamp;

                if (KLB_H264_PSLICE == nalu_type)
                {
                    media.vtype = KLB_MNP_VTYPE_P;
                }
                else if (KLB_H264_BSLICE == nalu_type)
                {
                    media.vtype = KLB_MNP_VTYPE_B;
                }
                else if (KLB_H264_IDRSLICE == nalu_type)
                {
                    media.vtype = KLB_MNP_VTYPE_I;
                }
                else if (KLB_H264_SPS == nalu_type || KLB_H264_PPS == nalu_type)
                {
                    media.vtype = KLB_MNP_VTYPE_CFG;
                }

                klb_buffer_write(p_video_buf, (const char*)&media, sizeof(klb_mnp_media_t));
                klb_buffer_write(p_video_buf, nalu_h26x, 4);

                nalu_value = p_parser->nalu_value;
                klb_buffer_write(p_video_buf, &nalu_value, 1);
            }

            klb_buffer_write(p_video_buf, p_parser->p_data, p_parser->data_len);
        }

        if (KLB_MNP_END == p_parser->opt || KLB_MNP_FULL == p_parser->opt)
        {
            push_video_data_klb_rtspclient_conn(p_conn);
        }

        p_rtsp->parser = *p_parser;
    }
}


static void push_audio_data_klb_rtspclient_conn(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_buffer_t* p_audio_buf = p_rtsp->p_audio_buf;
    int data_len = 0;
    klb_buf_t* p_data = NULL;
    klb_mnp_media_t* p_media = NULL;

    data_len = klb_buffer_datalen(p_audio_buf);
    if (data_len <= 0)
    {
        return;
    }

    p_data = klb_buffer_join(p_audio_buf, klb_buf_atom_malloc, NULL);
    klb_buffer_reset(p_audio_buf);

    p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);
    p_media->size = klb_buf_data_len(p_data);

    push_data_klb_rtspclient_conn(p_conn, 0, KLB_MNP_MEDIA, p_data);
}


static void consume_aac_rtp_klb_rtspclient_conn(klb_netconn_t* p_conn, char* p_base, klb_rtspparser_t* p_parser)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    char* p_payload = p_base + p_parser->nalu_pos;
    int payload_len = p_parser->nalu_len;
    klb_mnp_media_t media = { 0 };
    klb_mnp_audio_t audio = { 0 };

    if (0 != klb_rtspparser_parse_rtp_aac(p_parser, p_payload, payload_len))
    {
        return;
    }

    if (NULL == p_parser->p_data || p_parser->data_len <= 0)
    {
        return;
    }

    media.dtype = KLB_MNP_DTYPE_AAC;
    media.sidx = KLB_MNP_SIDX_A1;
    media.time = p_parser->rtp_head.timestamp;
    media.size = (uint32_t)(sizeof(klb_mnp_media_t) + sizeof(klb_mnp_audio_t) + p_parser->data_len);

    audio.samples = p_rtsp->audio_samples;
    audio.tracks = p_rtsp->audio_tracks;
    audio.bits_per_sample = p_rtsp->audio_bits;

    klb_buffer_reset(p_rtsp->p_audio_buf);
    klb_buffer_write(p_rtsp->p_audio_buf, (const char*)&media, sizeof(klb_mnp_media_t));
    klb_buffer_write(p_rtsp->p_audio_buf, (const char*)&audio, sizeof(klb_mnp_audio_t));
    klb_buffer_write(p_rtsp->p_audio_buf, (const char*)p_parser->p_data, p_parser->data_len);
    push_audio_data_klb_rtspclient_conn(p_conn);
}


// 解析从网络上接收到的数据
static int parse_recv_data_klb_rtspclient_conn(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
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

                push_data_klb_rtspclient_conn(p_conn, 0, KLB_MNP_TEXT, p_buf);
            }
            else if(KLB_RTSPTYPE_rtp == parser.rtsptype)
            {
                // rtp 包
                if (p_rtsp->video_pt == parser.rtp_head.payload_type)
                {
                    consume_h264_rtp_klb_rtspclient_conn(p_conn, ptr, &parser);
                }
                else if (0 != p_rtsp->audio_pt && p_rtsp->audio_pt == parser.rtp_head.payload_type)
                {
                    consume_aac_rtp_klb_rtspclient_conn(p_conn, ptr, &parser);
                }
            }
            else if(KLB_RTSPTYPE_rtcp == parser.rtsptype)
            {
                // rtcp 包 (interleaved, 跳过 4 字节 "$ 通道 长度" 头)
                handle_rtcp_klb_rtspclient_conn(p_rtsp, (const uint8_t*)(ptr + 4), pack_len - 4);
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
            push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL, 0, NULL);
            return -1;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// msg

// 握手 连接超时
static void on_msg_connect_timeout_klb_rtspclient_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
}

// 握手完成
static void on_msg_connected_klb_rtspclient_conn(klb_netconn_t* p_conn)
{
    push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_CONNECT, 0, NULL);
}

// 定时器消息. 参考 mnp/smp: 读写/空闲超时判定
static void on_msg_ticker_klb_rtspclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
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
        push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step2. 读超时: 有半帧待续但长时间无新数据到达
    if (reading && 0 < p_rtsp->read_timeout_ms &&
        (int64_t)p_rtsp->read_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step3. 空闲超时: 无读无写, 长时间无任何接收
    if (!writing && !reading && 0 < p_rtsp->idle_timeout_ms &&
        (int64_t)p_rtsp->idle_timeout_ms <= ABS_SUB(now, p_socket->last_recv_tc))
    {
        push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_TIMEOUT, 0, NULL);
        return;
    }

    // step4. RTCP: UDP 媒体建立后按周期向对端反馈 RR
    send_periodic_rr_klb_rtspclient_conn(p_conn, now);
}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

/// @brief 销毁
static void klb_rtspclient_conn_destroy(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_rtsp->p_udp_rtp, p_rtsp->p_udp_rtcp);
    p_rtsp->p_udp_rtp = NULL;
    p_rtsp->p_udp_rtcp = NULL;
    p_rtsp->udp_port = 0;

    klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_rtsp->p_udp_audio_rtp, p_rtsp->p_udp_audio_rtcp);
    p_rtsp->p_udp_audio_rtp = NULL;
    p_rtsp->p_udp_audio_rtcp = NULL;
    p_rtsp->udp_audio_port = 0;

    if (NULL != p_rtsp->p_netmulti)
    {
        klb_netmulti_remove(p_rtsp->p_netmulti, p_conn);
        p_rtsp->p_netmulti = NULL;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);

    klb_rtspclient_conn_quit(p_conn);

    KLB_FREE(p_conn);
}

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
static int klb_rtspclient_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    return 0;
}

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
static int klb_rtspclient_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

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

/// @brief 将一包 RTP 数据按 interleaved 封装后入写出队列
/// @param [in]  is_audio   true.音频; false.视频
/// @param [in]  timestamp  RTP 时间戳
/// @param [in]  marker     M 位
/// @param [in]  p_pay0/pay0_len  负载 0
/// @param [in]  p_pay1/pay1_len  负载 1(可为 NULL/0)
static void enqueue_rtp_klb_rtspclient_conn(klb_netconn_t* p_conn, bool is_audio, uint32_t timestamp, bool marker, const uint8_t* p_pay0, int pay0_len, const uint8_t* p_pay1, int pay1_len)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    int payload_len = pay0_len + pay1_len;
    uint8_t pt = 0;
    uint8_t ch = 0;
    uint16_t* p_seq = NULL;
    uint32_t ssrc = 0;

    if (payload_len <= 0)
    {
        return;
    }

    if (is_audio)
    {
        pt = p_rtsp->audio_pt;
        ch = p_rtsp->audio_ch;
        p_seq = &p_rtsp->audio_seq;
        ssrc = p_rtsp->audio_ssrc;
    }
    else
    {
        pt = p_rtsp->video_pt;
        ch = p_rtsp->rtp_ch;
        p_seq = &p_rtsp->rtp_seq;
        ssrc = p_rtsp->rtp_ssrc;
    }

    int rtp_len = 12 + payload_len;
    int pack_len = 4 + rtp_len;
    klb_buf_t* p_buf = klb_buf_malloc(pack_len, false);
    uint8_t hdr[16];

    hdr[0] = 0x24;
    hdr[1] = ch;
    KLB_WB16(hdr + 2, (uint16_t)rtp_len);
    hdr[4] = 0x80;
    hdr[5] = (uint8_t)((marker ? 0x80 : 0x00) | (pt & 0x7F));
    KLB_WB16(hdr + 6, *p_seq);
    *p_seq = (uint16_t)(*p_seq + 1);
    KLB_WB32(hdr + 8, timestamp);
    KLB_WB32(hdr + 12, ssrc);

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


/// @brief 将单个 NALU 打包为 RTP; 超长按 FU-A 分片
/// @param [in]  is_last  是否为帧内最后一个 NALU(VCL 时用于置 marker)
static void enqueue_h264_nalu_klb_rtspclient_conn(klb_netconn_t* p_conn, uint32_t timestamp, const uint8_t* p_nalu, int nalu_len, bool is_last)
{
    if (NULL == p_nalu || nalu_len <= 0)
    {
        return;
    }

    uint8_t nalu_type = (uint8_t)(p_nalu[0] & 0x1F);
    bool is_vcl = (1 <= nalu_type && 5 >= nalu_type);

    if (nalu_len <= KLB_RTSPCLIENT_rtp_payload)
    {
        bool marker = (is_vcl && is_last);
        enqueue_rtp_klb_rtspclient_conn(p_conn, false, timestamp, marker, p_nalu, nalu_len, NULL, 0);
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

        if (chunk > KLB_RTSPCLIENT_rtp_payload)
        {
            chunk = KLB_RTSPCLIENT_rtp_payload;
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
        enqueue_rtp_klb_rtspclient_conn(p_conn, false, timestamp, marker, fu_hdr, 2, p_body + off, chunk);

        off += chunk;
        first = false;
    }
}


/// @brief 发送 H264 媒体包(klb_mnp_media_t + Annex-B H264), 按 NALU 扫描后打包
/// @return int 0.成功; 非0.失败
static int send_h264_klb_rtspclient_conn(klb_netconn_t* p_conn, klb_buf_t* p_data)
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
        enqueue_h264_nalu_klb_rtspclient_conn(p_conn, rtp_ts, (const uint8_t*)p_h26x, h26x_len, true);
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

        enqueue_h264_nalu_klb_rtspclient_conn(p_conn, rtp_ts, p_nalu, nalu_len, is_last);

        if (is_end)
        {
            break;
        }

        scan_off += nal_start + nal_len;
    }

    return 0;
}


/// @brief 发送 AAC 媒体包(klb_mnp_media_t + klb_mnp_audio_t + AAC), 按 RFC3640 AAC-hbr 打包
/// @return int 0.成功; 非0.失败
static int send_aac_klb_rtspclient_conn(klb_netconn_t* p_conn, klb_buf_t* p_data)
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

    enqueue_rtp_klb_rtspclient_conn(p_conn, true, rtp_ts, true, au_hdr, 4, (const uint8_t*)p_aac, aac_len);
    return 0;
}


/// @brief 发送媒体数据包; 按 dtype 打包后走 TCP interleaved
/// @return int 0.成功; 非0.失败
static int klb_rtspclient_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    int ret = -1;

    if (NULL != p_data)
    {
        int data_len = klb_buf_data_len(p_data);

        if ((int)sizeof(klb_mnp_media_t) <= data_len)
        {
            klb_mnp_media_t* p_media = (klb_mnp_media_t*)(p_data->p_buf + p_data->start);

            if (KLB_MNP_DTYPE_H264 == p_media->dtype)
            {
                ret = send_h264_klb_rtspclient_conn(p_conn, p_data);
            }
            else if (KLB_MNP_DTYPE_AAC == p_media->dtype)
            {
                ret = send_aac_klb_rtspclient_conn(p_conn, p_data);
            }
        }

        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }

    return ret;
}

/// @brief 当网络上可以发送数据时
/// @return int
static int klb_rtspclient_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
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
        push_data_klb_rtspclient_conn(p_conn, close_status, 0, NULL);
        return write_num;
    }

    // 数据写完了, 无需读取
    if (klb_nlist_size(p_write_nlist) <= 0)
    {
        klb_socket_set_writing(p_socket, false);
    }

    return write_num;
}

/// @brief 当网络上可以接收数据时
/// @return int
static char* idle_recv_klb_rtspclient_conn(klb_rbuf_t* p_read_rbuf, int* p_idle_len)
{
    int idle_len = 0;
    char* p_idle = klb_rbuf_idle(p_read_rbuf, 0, &idle_len);

    if (NULL == p_idle)
    {
        int buf_len = klb_rbuf_buflen(p_read_rbuf);

        if (buf_len < KLB_RTSPCLIENT_rbuf_max)
        {
            int add_len = KLB_RTSPCLIENT_rbuf_min;

            if (buf_len + add_len > KLB_RTSPCLIENT_rbuf_max)
            {
                add_len = KLB_RTSPCLIENT_rbuf_max - buf_len;
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



static int klb_rtspclient_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
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
        char* p_idle = idle_recv_klb_rtspclient_conn(p_read_rbuf, &idle_len);

        if (NULL == p_idle)
        {
            push_data_klb_rtspclient_conn(p_conn, KLB_SOCKET_ERR_PROTOCOL, 0, NULL);
            break;
        }

        int recv_len = klb_socket_recv(p_socket, (uint8_t*)p_idle, idle_len);
        int judge = 0;
        int close_status = KLB_SOCKET_DISCONNECT;

        if (0 < recv_len)
        {
            klb_rbuf_cat_end(p_read_rbuf, recv_len);
            read_num += recv_len;

            if (0 != parse_recv_data_klb_rtspclient_conn(p_conn))
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
                (void)parse_recv_data_klb_rtspclient_conn(p_conn);
            }
        }

        klb_socket_recv_closed(p_socket, close_status);
        push_data_klb_rtspclient_conn(p_conn, close_status, 0, NULL);
        break;
    }

    return read_num;
}

/// @brief 当网络上有消息传来时
/// @return int
static int klb_rtspclient_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    switch (msg)
    {
    case KLB_NETCONN_MSG_connect_timeout:
        on_msg_connect_timeout_klb_rtspclient_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_connected:
        on_msg_connected_klb_rtspclient_conn(p_conn);
        break;

    case KLB_NETCONN_MSG_onticker:
        on_msg_ticker_klb_rtspclient_conn(p_conn, now);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// init / quit

static int klb_rtspclient_conn_init(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    // 初始化 vtable
    p_conn->vtable.destroy = NULL; // 销毁
    p_conn->vtable.recv_data = NULL; // 当前连接接收到数据/错误等信息,后调用此函数

    p_conn->vtable.ioctrl = klb_rtspclient_conn_ioctrl; // 对连接进行控制操作: get/set,etc.
    p_conn->vtable.send_normal = klb_rtspclient_conn_send_normal; // 调用者 发送常规数据包
    p_conn->vtable.send_media = klb_rtspclient_conn_send_media; // 调用者 发送媒体数据包
    p_conn->vtable.on_send = klb_rtspclient_conn_on_send; // 当网络上可以发送数据时
    p_conn->vtable.on_recv = klb_rtspclient_conn_on_recv; // 当网络上可以接收数据时
    p_conn->vtable.on_msg = klb_rtspclient_conn_on_msg; // 当网络上有消息传来时

    // 初始化其他

    p_rtsp->p_write_nlist = klb_nlist_create();

    // 读
    p_rtsp->p_read_rbuf = klb_rbuf_malloc(KLB_RTSPCLIENT_rbuf_min);

    // 视频缓存
    p_rtsp->p_video_buf = klb_buffer_create(KLB_RTSPCLIENT_rbuf_min);
    p_rtsp->p_audio_buf = klb_buffer_create(KLB_RTSPCLIENT_rbuf_min);
    p_rtsp->video_pt = 96;
    p_rtsp->audio_pt = 0;
    p_rtsp->audio_samples = 44100;
    p_rtsp->audio_tracks = 2;
    p_rtsp->audio_bits = 2;
    p_rtsp->rtp_ch = 0;
    p_rtsp->audio_ch = 2;
    p_rtsp->rtp_ssrc = klb_tick_count();
    p_rtsp->audio_ssrc = p_rtsp->rtp_ssrc ^ 0xA5A5A5A5u;

    if (0 == p_rtsp->rtp_ssrc)
    {
        p_rtsp->rtp_ssrc = 1;
    }

    if (0 == p_rtsp->audio_ssrc)
    {
        p_rtsp->audio_ssrc = 2;
    }

    p_rtsp->rtcp_interval_ms = KLB_RTSPCLIENT_rtcp_interval_ms;

    return 0;
}

static void klb_rtspclient_conn_quit(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    // 释放内存
    while (0 < klb_nlist_size(p_rtsp->p_write_nlist))
    {
        klb_buf_t* p_buf = klb_nlist_pop_head(p_rtsp->p_write_nlist);
        KLB_FREE_BY(p_buf, klb_buf_unref_next);
    }

    // 销毁
    KLB_FREE_BY(p_rtsp->p_write_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_rtsp->p_read_rbuf, klb_rbuf_free);
    KLB_FREE_BY(p_rtsp->p_video_buf, klb_buffer_destroy);
    KLB_FREE_BY(p_rtsp->p_audio_buf, klb_buffer_destroy);
}

//////////////////////////////////////////////////////////////////////////

klb_netconn_t* klb_rtspclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, int* p_code)
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

    p_socket = klb_socket_async_create(fd);
    p_netconn = klb_rtspclient_conn_create(p_netmulti, p_socket);
    klb_netmulti_push(p_netmulti, p_netconn);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_netconn;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建rtsp连接
/// @return klb_netconn_t*
klb_netconn_t* klb_rtspclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket)
{
    klb_netconn_t* p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_rtspclient_conn_t));
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    klb_rtspclient_conn_init(p_conn);

    // 补写 销毁函数
    p_conn->vtable.destroy = klb_rtspclient_conn_destroy;

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

    return p_conn;
}


//////////////////////////////////////////////////////////////////////////
// UDP RTP / RTCP

#define KLB_RTSPUDP_port_base    20000
#define KLB_RTSPUDP_port_try     128
#define KLB_RTSPUDP_recv_max     65536


typedef struct klb_rtspudp_conn_t_
{
    klb_netmulti_t*                 p_netmulti;         ///< 复用
    void*                           p_udata;            ///< 回调用户数据
    klb_rtspudp_rtp_cb              cb_rtp;             ///< RTP 回调; RTCP 连接为 NULL
    klb_rtspudp_rtp_cb              cb_rtcp;            ///< RTCP 回调; RTP 连接为 NULL
    bool                            is_rtp;             ///< true.RTP; false.RTCP
    uint8_t*                        p_recv;             ///< 接收缓冲
}klb_rtspudp_conn_t;


static uint16_t s_rtspudp_next = KLB_RTSPUDP_port_base;


/// @brief 处理收到的 RTCP 包. 逐条解析复合包, SR(200) 计数, BYE(203) 置位
/// @param [in]  p_data     RTCP 数据(不含 interleaved 4 字节头, 或 UDP 整包)
/// @param [in]  data_len   数据长度
static void handle_rtcp_klb_rtspclient_conn(klb_rtspclient_conn_t* p_rtsp, const uint8_t* p_data, int data_len)
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

        if (200 == pt)
        {
            p_rtsp->rtcp_sr_count += 1;
        }
        else if (203 == pt)
        {
            p_rtsp->rtcp_bye = true;
        }

        off += pack_len;
    }
}


static void on_udp_rtcp_klb_rtspclient_conn(void* p_udata, char* p_data, int data_len)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_udata;
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    if (p_rtsp->stopped)
    {
        return;
    }

    handle_rtcp_klb_rtspclient_conn(p_rtsp, (const uint8_t*)p_data, data_len);
}


static void on_udp_rtp_klb_rtspclient_conn(void* p_udata, char* p_data, int data_len)
{
    klb_netconn_t* p_conn = (klb_netconn_t*)p_udata;
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_rtspparser_t parser = { 0 };

    if (p_rtsp->stopped)
    {
        return;
    }

    if (0 != klb_rtspparser_parse_rtp(&parser, p_data, data_len))
    {
        return;
    }

    if (p_rtsp->video_pt == parser.rtp_head.payload_type)
    {
        consume_h264_rtp_klb_rtspclient_conn(p_conn, p_data, &parser);
    }
    else if (0 != p_rtsp->audio_pt && p_rtsp->audio_pt == parser.rtp_head.payload_type)
    {
        consume_aac_rtp_klb_rtspclient_conn(p_conn, p_data, &parser);
    }
}


static void klb_rtspudp_conn_destroy(klb_netconn_t* p_conn)
{
    klb_rtspudp_conn_t* p_udp = (klb_rtspudp_conn_t*)p_conn->extra;

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    KLB_FREE(p_udp->p_recv);
    KLB_FREE(p_conn);
}


static int klb_rtspudp_conn_on_recv(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspudp_conn_t* p_udp = (klb_rtspudp_conn_t*)p_conn->extra;
    int read_num = 0;

    (void)now;

    while (true)
    {
        struct sockaddr_storage addr;
        int addr_len = (int)sizeof(addr);
        int n = 0;
        int judge = 0;

        memset(&addr, 0, sizeof(addr));
        n = klb_socket_udp_recvfrom(p_conn->p_socket, p_udp->p_recv, KLB_RTSPUDP_recv_max,
            (struct sockaddr*)&addr, &addr_len);
        judge = klb_socket_udp_recv_judge(p_conn->p_socket, n, KLB_RTSPUDP_recv_max, NULL);

        if (KLB_SOCKET_UDP_RECV_DATA == judge)
        {
            if (0 < n)
            {
                read_num += n;

                if (p_udp->is_rtp)
                {
                    if (NULL != p_udp->cb_rtp)
                    {
                        p_udp->cb_rtp(p_udp->p_udata, (char*)p_udp->p_recv, n);
                    }
                }
                else if (NULL != p_udp->cb_rtcp)
                {
                    p_udp->cb_rtcp(p_udp->p_udata, (char*)p_udp->p_recv, n);
                }
            }
        }
        else
        {
            break;
        }
    }

    return read_num;
}


static int klb_rtspudp_conn_on_send(klb_netconn_t* p_conn, int64_t now)
{
    (void)p_conn;
    (void)now;
    return 0;
}


static int klb_rtspudp_conn_on_msg(klb_netconn_t* p_conn, int msg, int64_t now)
{
    (void)p_conn;
    (void)msg;
    (void)now;
    return 0;
}


static int klb_rtspudp_conn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_conn;
    (void)p_in;
    (void)p_out;
    return 0;
}


static int klb_rtspudp_conn_send_normal(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len)
{
    (void)p_conn;
    (void)packtype;
    (void)sequence;
    (void)p_head;
    (void)head_len;
    (void)p_body;
    (void)body_len;
    return 0;
}


static int klb_rtspudp_conn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data)
{
    (void)p_conn;
    KLB_FREE_BY(p_data, klb_buf_unref_next);
    return 0;
}


static klb_netconn_t* create_klb_rtspudp_conn(klb_netmulti_t* p_multi, klb_socket_t* p_socket,
    void* p_udata, klb_rtspudp_rtp_cb cb_rtp, klb_rtspudp_rtp_cb cb_rtcp, bool is_rtp)
{
    klb_netconn_t* p_conn = NULL;
    klb_rtspudp_conn_t* p_udp = NULL;

    if (NULL == p_multi || NULL == p_socket)
    {
        return NULL;
    }

    p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_rtspudp_conn_t));
    p_udp = (klb_rtspudp_conn_t*)p_conn->extra;

    p_udp->p_netmulti = p_multi;
    p_udp->p_udata = p_udata;
    p_udp->cb_rtp = cb_rtp;
    p_udp->cb_rtcp = cb_rtcp;
    p_udp->is_rtp = is_rtp;
    p_udp->p_recv = KLB_MALLOC(uint8_t, KLB_RTSPUDP_recv_max, 0);

    p_conn->p_socket = p_socket;
    p_conn->protocol = KLB_PROTOCOL_RTSP;
    p_conn->vtable.destroy = klb_rtspudp_conn_destroy;
    p_conn->vtable.on_recv = klb_rtspudp_conn_on_recv;
    p_conn->vtable.on_send = klb_rtspudp_conn_on_send;
    p_conn->vtable.on_msg = klb_rtspudp_conn_on_msg;
    p_conn->vtable.ioctrl = klb_rtspudp_conn_ioctrl;
    p_conn->vtable.send_normal = klb_rtspudp_conn_send_normal;
    p_conn->vtable.send_media = klb_rtspudp_conn_send_media;

    klb_socket_set_reading(p_socket, true);
    klb_socket_set_connected(p_socket, true);
    klb_netmulti_push(p_multi, p_conn);

    return p_conn;
}


void klb_rtspudp_close_pair(klb_netmulti_t* p_multi, klb_netconn_t* p_rtp, klb_netconn_t* p_rtcp)
{
    if (NULL != p_rtp)
    {
        if (NULL != p_multi)
        {
            klb_netmulti_remove(p_multi, p_rtp);
        }

        klb_netconn_destroy(p_rtp);
    }

    if (NULL != p_rtcp)
    {
        if (NULL != p_multi)
        {
            klb_netmulti_remove(p_multi, p_rtcp);
        }

        klb_netconn_destroy(p_rtcp);
    }
}


int klb_rtspudp_open_pair(klb_netmulti_t* p_multi, void* p_udata, klb_rtspudp_rtp_cb cb_rtp, klb_rtspudp_rtp_cb cb_rtcp, klb_netconn_t** p_out_rtp, klb_netconn_t** p_out_rtcp, int* p_rtp_port)
{
    int i = 0;

    if (NULL == p_multi || NULL == p_out_rtp || NULL == p_out_rtcp || NULL == p_rtp_port)
    {
        return -1;
    }

    *p_out_rtp = NULL;
    *p_out_rtcp = NULL;
    *p_rtp_port = 0;

    for (i = 0; i < KLB_RTSPUDP_port_try; i++)
    {
        uint16_t port = s_rtspudp_next;
        klb_socket_t* p_sock_rtp = NULL;
        klb_socket_t* p_sock_rtcp = NULL;
        uint32_t flags = KLB_SOCKET_F_REUSEADDR;

        if (KLB_RTSPUDP_port_base > port || 0 != (port % 2) || 64000 < port)
        {
            port = KLB_RTSPUDP_port_base;
        }

        s_rtspudp_next = (uint16_t)(port + 2);

        // step1. 先开未 bind 的 UDP, 再按偶/奇端口对重试 bind
        p_sock_rtp = klb_socket_udp_open(KLB_SOCKET_FAMILY_V4, NULL, 0, flags);
        p_sock_rtcp = klb_socket_udp_open(KLB_SOCKET_FAMILY_V4, NULL, 0, flags);

        if (NULL == p_sock_rtp || NULL == p_sock_rtcp)
        {
            KLB_FREE_BY(p_sock_rtp, klb_socket_destroy);
            KLB_FREE_BY(p_sock_rtcp, klb_socket_destroy);
            return -1;
        }

        if (0 == klb_socket_udp_bind(p_sock_rtp, "*", (int)port, flags)
            && 0 == klb_socket_udp_bind(p_sock_rtcp, "*", (int)(port + 1), flags))
        {
            *p_out_rtp = create_klb_rtspudp_conn(p_multi, p_sock_rtp, p_udata, cb_rtp, NULL, true);
            *p_out_rtcp = create_klb_rtspudp_conn(p_multi, p_sock_rtcp, p_udata, NULL, cb_rtcp, false);

            if (NULL == *p_out_rtp || NULL == *p_out_rtcp)
            {
                klb_rtspudp_close_pair(p_multi, *p_out_rtp, *p_out_rtcp);
                *p_out_rtp = NULL;
                *p_out_rtcp = NULL;
                return -1;
            }

            *p_rtp_port = (int)port;
            return 0;
        }

        KLB_FREE_BY(p_sock_rtp, klb_socket_destroy);
        KLB_FREE_BY(p_sock_rtcp, klb_socket_destroy);
    }

    return -1;
}


void klb_rtspclient_conn_close_udp(klb_netconn_t* p_conn)
{
    klb_rtspclient_conn_t* p_rtsp = NULL;

    if (NULL == p_conn)
    {
        return;
    }

    p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_rtsp->p_udp_rtp, p_rtsp->p_udp_rtcp);
    p_rtsp->p_udp_rtp = NULL;
    p_rtsp->p_udp_rtcp = NULL;
    p_rtsp->udp_port = 0;

    klb_rtspudp_close_pair(p_rtsp->p_netmulti, p_rtsp->p_udp_audio_rtp, p_rtsp->p_udp_audio_rtcp);
    p_rtsp->p_udp_audio_rtp = NULL;
    p_rtsp->p_udp_audio_rtcp = NULL;
    p_rtsp->udp_audio_port = 0;
    p_rtsp->udp_last_audio = false;
}


int klb_rtspclient_conn_open_udp(klb_netconn_t* p_conn, int* p_rtp_port)
{
    klb_rtspclient_conn_t* p_rtsp = NULL;
    int port = 0;
    int ret = 0;

    if (NULL == p_conn || NULL == p_rtp_port)
    {
        return -1;
    }

    p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    // 视频对已开则开音频对; 否则开视频对
    if (0 < p_rtsp->udp_port && NULL != p_rtsp->p_udp_rtp)
    {
        if (0 < p_rtsp->udp_audio_port && NULL != p_rtsp->p_udp_audio_rtp)
        {
            *p_rtp_port = p_rtsp->udp_audio_port;
            p_rtsp->udp_last_audio = true;
            return 0;
        }

        ret = klb_rtspudp_open_pair(p_rtsp->p_netmulti, p_conn, on_udp_rtp_klb_rtspclient_conn, on_udp_rtcp_klb_rtspclient_conn,
            &p_rtsp->p_udp_audio_rtp, &p_rtsp->p_udp_audio_rtcp, &port);

        if (0 != ret)
        {
            return -1;
        }

        p_rtsp->udp_audio_port = port;
        p_rtsp->udp_last_audio = true;
        *p_rtp_port = port;
        return 0;
    }

    ret = klb_rtspudp_open_pair(p_rtsp->p_netmulti, p_conn, on_udp_rtp_klb_rtspclient_conn, on_udp_rtcp_klb_rtspclient_conn,
        &p_rtsp->p_udp_rtp, &p_rtsp->p_udp_rtcp, &port);

    if (0 != ret)
    {
        return -1;
    }

    p_rtsp->udp_port = port;
    p_rtsp->udp_last_audio = false;
    *p_rtp_port = port;
    return 0;
}


static int resolve_ipv4_klb_rtspclient(const char* p_host, struct in_addr* p_addr)
{
    if (NULL == p_host || '\0' == p_host[0] || NULL == p_addr)
    {
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo* p_res = NULL;
    struct sockaddr_in* p_in = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (0 != getaddrinfo(p_host, NULL, &hints, &p_res))
    {
        return -1;
    }

    if (NULL == p_res || NULL == p_res->ai_addr || AF_INET != p_res->ai_family)
    {
        if (NULL != p_res)
        {
            freeaddrinfo(p_res);
        }

        return -1;
    }

    p_in = (struct sockaddr_in*)p_res->ai_addr;
    *p_addr = p_in->sin_addr;
    freeaddrinfo(p_res);
    return 0;
}


static void send_rr_klb_rtspclient(klb_socket_t* p_socket, struct in_addr addr, int port)
{
    if (NULL == p_socket || 0 >= port || 65535 < port)
    {
        return;
    }

    uint8_t rr[8];
    struct sockaddr_in peer;

    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr = addr;

    rr[0] = 0x80;
    rr[1] = 201;
    KLB_WB16(rr + 2, 1);
    KLB_WB32(rr + 4, 1);

    (void)klb_socket_udp_sendto(p_socket, rr, 8, (const struct sockaddr*)&peer, (int)sizeof(peer));
}


/// @brief 周期向对端 RTCP 端口发送 RR 反馈. UDP 媒体建立(udp_punch)后生效
static void send_periodic_rr_klb_rtspclient_conn(klb_netconn_t* p_conn, int64_t now)
{
    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_netconn_t* p_rtcp = NULL;

    if (0 >= p_rtsp->rtcp_interval_ms || 0 >= p_rtsp->udp_peer_rtp_port)
    {
        return;
    }

    if (0 != p_rtsp->last_rtcp_send_tc &&
        ABS_SUB(now, p_rtsp->last_rtcp_send_tc) < (int64_t)p_rtsp->rtcp_interval_ms)
    {
        return;
    }

    p_rtcp = p_rtsp->udp_peer_audio ? p_rtsp->p_udp_audio_rtcp : p_rtsp->p_udp_rtcp;

    if (NULL == p_rtcp || NULL == p_rtcp->p_socket)
    {
        return;
    }

    send_rr_klb_rtspclient(p_rtcp->p_socket, p_rtsp->udp_peer.sin_addr, p_rtsp->udp_peer_rtp_port + 1);

    p_rtsp->last_rtcp_send_tc = now;
}


int klb_rtspclient_conn_udp_punch(klb_netconn_t* p_conn, const char* p_host, int server_rtp_port)
{
    if (NULL == p_conn || 0 >= server_rtp_port || 65535 <= server_rtp_port)
    {
        return -1;
    }

    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    klb_netconn_t* p_rtp = NULL;
    klb_netconn_t* p_rtcp = NULL;
    struct in_addr addr;

    if (p_rtsp->udp_last_audio)
    {
        p_rtp = p_rtsp->p_udp_audio_rtp;
        p_rtcp = p_rtsp->p_udp_audio_rtcp;
    }
    else
    {
        p_rtp = p_rtsp->p_udp_rtp;
        p_rtcp = p_rtsp->p_udp_rtcp;
    }

    if (NULL == p_rtp || NULL == p_rtcp)
    {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));

    if (0 != resolve_ipv4_klb_rtspclient(p_host, &addr))
    {
        return -1;
    }

    // 记录对端地址与 RTCP 端口, 供后续周期 RR 使用
    memset(&p_rtsp->udp_peer, 0, sizeof(p_rtsp->udp_peer));
    p_rtsp->udp_peer.sin_family = AF_INET;
    p_rtsp->udp_peer.sin_addr = addr;
    p_rtsp->udp_peer.sin_port = htons((uint16_t)server_rtp_port);
    p_rtsp->udp_peer_rtp_port = server_rtp_port;
    p_rtsp->udp_peer_audio = p_rtsp->udp_last_audio;
    p_rtsp->last_rtcp_send_tc = klb_tick_counti64();

    if (NULL != p_rtp->p_socket)
    {
        send_rr_klb_rtspclient(p_rtp->p_socket, addr, server_rtp_port);
    }

    if (NULL != p_rtcp->p_socket)
    {
        send_rr_klb_rtspclient(p_rtcp->p_socket, addr, server_rtp_port + 1);
    }

    return 0;
}


void klb_rtspclient_conn_set_video_pt(klb_netconn_t* p_conn, int pt)
{
    if (NULL == p_conn)
    {
        return;
    }

    if (pt <= 0 || pt >= 128)
    {
        return;
    }

    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    p_rtsp->video_pt = (uint8_t)pt;
}


void klb_rtspclient_conn_set_audio_pt(klb_netconn_t* p_conn, int pt)
{
    if (NULL == p_conn)
    {
        return;
    }

    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    if (pt <= 0 || pt >= 128)
    {
        p_rtsp->audio_pt = 0;
        return;
    }

    p_rtsp->audio_pt = (uint8_t)pt;
}


void klb_rtspclient_conn_set_audio_info(klb_netconn_t* p_conn, int samples, int tracks, int bits_per_sample)
{
    if (NULL == p_conn)
    {
        return;
    }

    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;

    if (0 < samples)
    {
        p_rtsp->audio_samples = (uint32_t)samples;
    }

    if (0 < tracks && 255 >= tracks)
    {
        p_rtsp->audio_tracks = (uint8_t)tracks;
    }

    if (0 < bits_per_sample && 255 >= bits_per_sample)
    {
        p_rtsp->audio_bits = (uint8_t)bits_per_sample;
    }
}

void klb_rtspclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms)
{
    if (NULL == p_conn)
    {
        return;
    }

    klb_rtspclient_conn_t* p_rtsp = (klb_rtspclient_conn_t*)p_conn->extra;
    p_rtsp->read_timeout_ms = (0 < read_ms) ? read_ms : 0;
    p_rtsp->write_timeout_ms = (0 < write_ms) ? write_ms : 0;
    p_rtsp->idle_timeout_ms = (0 < idle_ms) ? idle_ms : 0;
}

// end
