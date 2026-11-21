// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WebRTC PeerConnection 会话编排 (SDP + ICE + DTLS + SRTP + DataChannel)
/// @version 1.1
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2-1.0 会话编排落地: ICE 收集 / 凭据交换 / SDP 生成与解析 / 收发; DTLS-SRTP 握手与 SRTP 收发, 分流与统计; DataChannel (SCTP 最小子集 + DCEP); 媒体面 RTP/RTCP 分流 + H.264 打包/解包 + 抖动缓冲 + NACK 重传; 会话事件队列; RTCP SR/RR·TWCC 反馈与 AIMD 码率自适应
/// @warning DTLS 未配置证书时 send/recv 走裸 ICE 应用数据通道
///  \n 报文分流: 首字节 20..23 为 DTLS 记录; 版本位 10 为 SRTP; 其余按应用数据处理
///  \n SRTP 解包后按 PT 192..223 分 RTCP, 其余为 RTP (入抖动缓冲)
///  \n DataChannel 为单流单通道, 不支持分片/SACK/重传
///////////////////////////////////////////////////////////////////////////
#include "klbwebrtc/core/klb_webrtc.h"
#include "klbwebrtc/core/klb_webrtc_sctp.h"
#include "klbwebrtc/core/klb_webrtc_jitter.h"
#include "klbice/core/klb_ice.h"
#include "klbsrtp/core/klb_srtp.h"
#include "klbnet/klbrtp/klb_rtp.h"
#include "klbrtp/core/klb_rtp_pkt.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_dtls.h"
#include "klbplatform/klb_time.h"
#include "klbmem/klb_mem.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


/// @def   KLB_WEBRTC_SDP_MAX
/// @brief 任务内部 SDP 文本缓冲长度
#define KLB_WEBRTC_SDP_MAX              4096

/// @def   KLB_WEBRTC_CRED_MAX
/// @brief 本地 ufrag/pwd 文本缓冲长度
#define KLB_WEBRTC_CRED_MAX             64

/// @def   KLB_WEBRTC_QUEUE_SLOTS
/// @brief 会话内单队列槽位
#define KLB_WEBRTC_QUEUE_SLOTS          8

/// @def   KLB_WEBRTC_PKT_MAX
/// @brief 单个报文缓冲上限
#define KLB_WEBRTC_PKT_MAX              1600

/// @def   KLB_WEBRTC_EVT_SLOTS
/// @brief 会话事件队列槽位
#define KLB_WEBRTC_EVT_SLOTS            16

/// @def   KLB_WEBRTC_RTP_MTU
/// @brief 媒体面单包上限 (含 RTP 头)
#define KLB_WEBRTC_RTP_MTU              1200

/// @def   KLB_WEBRTC_SENT_SLOTS
/// @brief 发送重传缓存槽位 (按 NACK 序号回查)
#define KLB_WEBRTC_SENT_SLOTS           32

/// @def   KLB_WEBRTC_NACK_REPLY
/// @brief 同一缺口最多补发 NACK 次数
#define KLB_WEBRTC_NACK_REPLY           3

/// @def   KLB_WEBRTC_JB_GAP_MAX
/// @brief 缺口持续轮询次数上限, 达到后跳过缺口
#define KLB_WEBRTC_JB_GAP_MAX           16

/// @def   KLB_WEBRTC_NALU_MAX
/// @brief H.264 分片重组缓冲上限
#define KLB_WEBRTC_NALU_MAX             (256 * 1024)


/// @struct klb_webrtc_pkt_t
/// @brief  队列中的单个报文
typedef struct klb_webrtc_pkt_t_
{
    int     len;                                    ///< 报文长度
    uint8_t data[KLB_WEBRTC_PKT_MAX];               ///< 报文内容
}klb_webrtc_pkt_t;


/// @struct klb_webrtc_queue_t
/// @brief  简单 FIFO 报文队列
typedef struct klb_webrtc_queue_t_
{
    int                 head;                       ///< 出队下标
    int                 count;                      ///< 当前元素数
    klb_webrtc_pkt_t    slots[KLB_WEBRTC_QUEUE_SLOTS];  ///< 槽位
}klb_webrtc_queue_t;


/// @struct klb_webrtc_sent_t
/// @brief  已发送 RTP 包 (SRTP 保护后的字节) 的重传缓存项
typedef struct klb_webrtc_sent_t_
{
    uint16_t    seq;                    ///< RTP 序号
    int         len;                    ///< 报文长度
    uint8_t*    p_data;                 ///< 报文副本; 空槽为 NULL
}klb_webrtc_sent_t;


/// @struct klb_webrtc_media_cfg_t
/// @brief  本地配置的一条媒体 m-line (offer 生成用)
typedef struct klb_webrtc_media_cfg_t_
{
    int                     kind;                               ///< klb_webrtc_kind_e
    int                     codec_num;                          ///< 编码个数
    klb_webrtc_codec_t      codecs[KLB_WEBRTC_CODEC_MAX];       ///< 编码
}klb_webrtc_media_cfg_t;


/// @struct klb_webrtc_peer_t_
/// @brief  PeerConnection 会话 (对外不透明)
struct klb_webrtc_peer_t_
{
    klb_netmulti_t*     p_netmulti;     ///< 复用模块
    int                 role;           ///< 会话角色: klb_webrtc_role_e
    klb_ice_agent_t*    p_ice;          ///< ICE 代理
    bool                gathered;       ///< 本地候选已收集
    bool                started;        ///< 已进入连通性检查
    klb_webrtc_queue_t  q_dtls;         ///< 待喂给 mbedTLS 的 DTLS 记录
    klb_webrtc_queue_t  q_app;          ///< 已解保护/裸应用报文
    char*               p_cert_pem;     ///< 本端证书 PEM
    char*               p_key_pem;      ///< 本端私钥 PEM
    char*               p_ca_pem;       ///< 对端校验 CA PEM
    int                 verify_peer;    ///< 0(false).不校验; 1(true).校验
    char                remote_fp[KLB_WEBRTC_FP_MAX];   ///< 远端指纹
    char                remote_setup[16];               ///< 远端 a=setup 值
    klb_socket_t*       p_dtls;         ///< DTLS 会话; NULL 表未启用
    bool                dtls_done;      ///< DTLS 握手完成
    klb_srtp_ctx_t*     p_srtp_tx;      ///< SRTP 发送上下文
    klb_srtp_ctx_t*     p_srtp_rx;      ///< SRTP 接收上下文
    klb_webrtc_dc_t*    p_dc;           ///< DataChannel; NULL 表未启用
    uint64_t            srtp_tx_pkts;   ///< SRTP/SRTCP 发送包数
    uint64_t            srtp_rx_pkts;   ///< SRTP/SRTCP 接收包数
    uint64_t            dtls_tx_pkts;   ///< DTLS 应用数据发送包数
    uint64_t            dtls_rx_pkts;   ///< DTLS 应用数据接收包数
    uint32_t            rtp_ssrc_tx;    ///< 本端媒体 SSRC
    uint32_t            rtp_ssrc_rx;    ///< 对端媒体 SSRC; 0 表未收到
    uint16_t            rtp_seq_tx;     ///< 本端下一 RTP 序号
    int                 rtp_gap;        ///< 缺口持续轮询计数
    uint16_t            nack_first;     ///< 上次 NACK 的首个缺口序号
    int                 nack_try;       ///< 当前缺口已补发 NACK 次数
    klb_webrtc_jb_t*    p_jb;           ///< 抖动缓冲; 收到首个 RTP 时创建
    klb_webrtc_sent_t   sent[KLB_WEBRTC_SENT_SLOTS];    ///< 重传缓存 (环形)
    int                 sent_head;      ///< 重传缓存写入下标
    uint8_t*            p_nalu;         ///< H.264 分片重组缓冲
    int                 nalu_len;       ///< 已拼长度
    int                 nalu_cap;       ///< 重组缓冲容量
    bool                nalu_broken;    ///< 分片中断, 丢弃至下一 FU 起始
    uint8_t*            p_stap;         ///< STAP-A 子 NALU 队列 [2字节长度 + NALU]
    int                 stap_len;       ///< 队列已用长度
    int                 stap_off;       ///< 已取偏移
    int                 stap_cap;       ///< 队列容量
    uint64_t            rtp_tx_pkts;    ///< RTP 发送包数
    uint64_t            rtp_rx_pkts;    ///< RTP 接收包数
    uint64_t            rtcp_tx_pkts;   ///< RTCP 发送包数
    uint64_t            rtcp_rx_pkts;   ///< RTCP 接收包数
    uint64_t            nack_tx_pkts;   ///< NACK 发送包数
    uint64_t            nack_rx_pkts;   ///< NACK 接收包数
    uint64_t            retrans_pkts;   ///< 按 NACK 重传的 RTP 包数
    uint64_t            rtp_lost_pkts;  ///< 抖动缓冲跳过缺口丢弃的序号数
    int                 evt_head;       ///< 事件队列出队下标
    int                 evt_count;      ///< 事件队列当前元素数
    int                 evt_ice_state;  ///< 已知 ICE 状态 (去重)
    bool                evt_dtls;       ///< DTLS 就绪事件已产生
    bool                evt_srtp;       ///< SRTP 就绪事件已产生
    bool                evt_dc;         ///< DataChannel 打开事件已产生
    klb_webrtc_event_t  evt[KLB_WEBRTC_EVT_SLOTS];  ///< 事件队列
    klb_webrtc_media_cfg_t media[KLB_WEBRTC_MLINE_MAX]; ///< 本地媒体 m-line 配置
    int                 media_num;      ///< 已配置媒体 m-line 数
    uint32_t            rtp_ts_tx;      ///< 最近发送 RTP 时间戳
    uint64_t            rtp_tx_bytes;   ///< RTP 负载发送字节数
    bool                rr_base_set;    ///< 接收统计基线已建立
    int64_t             rr_transit;     ///< 上次传输时延 (到达 ticks - RTP ts)
    uint32_t            rr_jitter;      ///< 接收抖动 (RTP 时钟单位)
    uint32_t            rr_ext_max;     ///< 最高扩展序号 (低 16 位)
    uint64_t            rr_expected;    ///< 期望收到的 RTP 包数
    uint32_t            rr_lsr;         ///< 最近 SR 的 NTP 中间 32 位
    uint64_t            rr_last_sr_ms;  ///< 最近 SR 到达 tick
    uint16_t            twcc_base;      ///< TWCC 窗口起始序号
    int                 twcc_count;     ///< TWCC 窗口包数
    uint8_t             twcc_status[KLB_WEBRTC_TWCC_MAX];   ///< 0.未收到; 非 0.已收到
    uint32_t            twcc_arrive[KLB_WEBRTC_TWCC_MAX];   ///< 各槽到达 tick (ms); 0.未收到
    uint8_t             twcc_fb_count;  ///< TWCC 反馈序号 (自增)
    uint64_t            twcc_tx_pkts;   ///< TWCC 反馈发送包数
    uint64_t            twcc_rx_pkts;   ///< TWCC 反馈接收包数
    uint64_t            report_tx_pkts; ///< SR/RR 发送包数
    uint64_t            report_rx_pkts; ///< SR/RR 接收包数
    uint32_t            target_bps;     ///< TWCC 自适应目标码率 (bps)
    uint32_t            twcc_loss_permille; ///< 最近一轮 TWCC 丢包率 (千分比)
    uint64_t            twcc_fb_rounds; ///< 已处理的 TWCC 反馈轮数
    uint64_t            rtcp_last_report_ms; ///< 上次 SR/RR 周期上报时刻
};


/// @brief 周期性发送 SR/RR (由 recv 泵驱动)
/// @param [in] *p_peer      会话
/// @return 无
static void rtcp_periodic_klb_webrtc(klb_webrtc_peer_t* p_peer);


/// @brief 追加格式化文本到 SDP 缓冲
/// @param [in,out] *p_buf  输出缓冲
/// @param [in] cap          缓冲容量
/// @param [in] pos          当前写入位置
/// @param [in] *p_fmt       格式串
/// @return int 大于等于 0.新的写入位置; 小于 0.klb_netcode_e
static int sdp_append_klb_webrtc(char* p_buf, int cap, int pos, const char* p_fmt, ...)
{
    if ((0 > pos) || (pos >= cap))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    va_list ap;
    va_start(ap, p_fmt);
    int n = vsnprintf(p_buf + pos, (size_t)(cap - pos), p_fmt, ap);
    va_end(ap);

    if (0 > n)
    {
        return KLB_NETCODE_ERR;
    }

    if (n >= (cap - pos))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    return pos + n;
}


/// @brief 复制定长文本并补 '\0'
/// @param [out] *p_dst     目标缓冲
/// @param [in] cap         目标容量
/// @param [in] *p_src      源文本
/// @param [in] len         源长度
/// @return 无
static void copy_text_klb_webrtc(char* p_dst, int cap, const char* p_src, int len)
{
    if (len >= cap)
    {
        len = cap - 1;
    }

    if (0 > len)
    {
        len = 0;
    }

    memcpy(p_dst, p_src, (size_t)len);
    p_dst[len] = '\0';
}


/// @brief 解析 m= 行 (媒体类型 / 端口 / 载荷类型列表)
/// @param [in] *p_line     行内容 (不含行首 "m=")
/// @param [in] len         行长度
/// @param [out] *p_ml      解析结果
/// @return 无
static void parse_mline_klb_webrtc(const char* p_line, int len, klb_webrtc_mline_t* p_ml)
{
    KLB_MEMSET(p_ml, 0, sizeof(*p_ml));

    int i = 0;
    while ((i < len) && (' ' != p_line[i]) && ('\t' != p_line[i]))
    {
        i++;
    }

    copy_text_klb_webrtc(p_ml->kind, (int)sizeof(p_ml->kind), p_line, i);

    while ((i < len) && ((' ' == p_line[i]) || ('\t' == p_line[i])))
    {
        i++;
    }

    int port = 0;
    while ((i < len) && ('0' <= p_line[i]) && ('9' >= p_line[i]))
    {
        port = (port * 10) + (int)(p_line[i] - '0');
        i++;
    }

    p_ml->port = port;

    // 跳过空白后再跳过协议串
    while ((i < len) && ((' ' == p_line[i]) || ('\t' == p_line[i])))
    {
        i++;
    }

    while ((i < len) && (' ' != p_line[i]) && ('\t' != p_line[i]))
    {
        i++;
    }

    while (i < len)
    {
        while ((i < len) && ((' ' == p_line[i]) || ('\t' == p_line[i])))
        {
            i++;
        }

        if (i >= len)
        {
            break;
        }

        int pt = 0;
        while ((i < len) && ('0' <= p_line[i]) && ('9' >= p_line[i]))
        {
            pt = (pt * 10) + (int)(p_line[i] - '0');
            i++;
        }

        if (KLB_WEBRTC_PT_MAX > p_ml->pt_num)
        {
            p_ml->pts[p_ml->pt_num] = pt;
            p_ml->pt_num++;
        }

        while ((i < len) && (' ' != p_line[i]) && ('\t' != p_line[i]))
        {
            i++;
        }
    }
}


/// @brief 解析 a=rtpmap 行 (载荷类型 -> 编码名)
/// @param [in] *p_attr     属性内容 (不含 "a=rtpmap:")
/// @param [in] len         长度
/// @param [in,out] *p_ml   所属 m-line
/// @return 无
static void parse_rtpmap_klb_webrtc(const char* p_attr, int len, klb_webrtc_mline_t* p_ml)
{
    int i = 0;
    int pt = 0;

    while ((i < len) && ('0' <= p_attr[i]) && ('9' >= p_attr[i]))
    {
        pt = (pt * 10) + (int)(p_attr[i] - '0');
        i++;
    }

    while ((i < len) && ((' ' == p_attr[i]) || ('\t' == p_attr[i])))
    {
        i++;
    }

    int start = i;
    while ((i < len) && ('/' != p_attr[i]))
    {
        i++;
    }

    for (int k = 0; k < p_ml->pt_num; k++)
    {
        if (pt == p_ml->pts[k])
        {
            copy_text_klb_webrtc(p_ml->codec[k], KLB_WEBRTC_CODEC_NAME, p_attr + start, i - start);

            // 可选 "/时钟频率[/声道数]"
            int j = i;
            int rate = 0;
            int channels = 0;
            if ((j < len) && ('/' == p_attr[j]))
            {
                j++;
                while ((j < len) && ('0' <= p_attr[j]) && ('9' >= p_attr[j]))
                {
                    rate = (rate * 10) + (int)(p_attr[j] - '0');
                    j++;
                }
            }

            if ((j < len) && ('/' == p_attr[j]))
            {
                j++;
                while ((j < len) && ('0' <= p_attr[j]) && ('9' >= p_attr[j]))
                {
                    channels = (channels * 10) + (int)(p_attr[j] - '0');
                    j++;
                }
            }

            p_ml->rate[k] = rate;
            p_ml->channels[k] = channels;
            break;
        }
    }
}


/// @brief 解析 a=fmtp 行 (载荷类型 -> 参数串)
/// @param [in] *p_attr     属性内容 (不含 "a=fmtp:")
/// @param [in] len         长度
/// @param [in,out] *p_ml   所属 m-line
/// @return 无
static void parse_fmtp_klb_webrtc(const char* p_attr, int len, klb_webrtc_mline_t* p_ml)
{
    int i = 0;
    int pt = 0;

    while ((i < len) && ('0' <= p_attr[i]) && ('9' >= p_attr[i]))
    {
        pt = (pt * 10) + (int)(p_attr[i] - '0');
        i++;
    }

    while ((i < len) && ((' ' == p_attr[i]) || ('\t' == p_attr[i])))
    {
        i++;
    }

    for (int k = 0; k < p_ml->pt_num; k++)
    {
        if (pt == p_ml->pts[k])
        {
            copy_text_klb_webrtc(p_ml->fmtp[k], KLB_WEBRTC_FMTP_MAX, p_attr + i, len - i);
            break;
        }
    }
}


/// @brief 解析 a=rtcp-fb 行 (载荷类型 -> 反馈能力位)
/// @param [in] *p_attr     属性内容 (不含 "a=rtcp-fb:")
/// @param [in] len         长度
/// @param [in,out] *p_ml   所属 m-line
/// @return 无
static void parse_rtcp_fb_klb_webrtc(const char* p_attr, int len, klb_webrtc_mline_t* p_ml)
{
    int i = 0;
    int pt = 0;

    while ((i < len) && ('0' <= p_attr[i]) && ('9' >= p_attr[i]))
    {
        pt = (pt * 10) + (int)(p_attr[i] - '0');
        i++;
    }

    while ((i < len) && ((' ' == p_attr[i]) || ('\t' == p_attr[i])))
    {
        i++;
    }

    int start = i;
    while ((i < len) && (' ' != p_attr[i]) && ('\t' != p_attr[i]))
    {
        i++;
    }

    int token_len = i - start;
    int fb = 0;

    if ((4 <= token_len) && (0 == strncmp(p_attr + start, "nack", 4)))
    {
        fb = KLB_WEBRTC_FB_NACK;

        // "nack pli" 同时声明 PLI
        if (((i + 4) <= len) && (0 == strncmp(p_attr + i + 1, "pli", 3)))
        {
            fb |= KLB_WEBRTC_FB_PLI;
        }
    }
    else if ((9 <= token_len) && (0 == strncmp(p_attr + start, "goog-remb", 9)))
    {
        fb = KLB_WEBRTC_FB_REMB;
    }
    else if ((12 <= token_len) && (0 == strncmp(p_attr + start, "transport-cc", 12)))
    {
        fb = KLB_WEBRTC_FB_TWCC;
    }
    else if ((3 <= token_len) && (0 == strncmp(p_attr + start, "ccm", 3)))
    {
        // "ccm fir"
        if (((i + 4) <= len) && (0 == strncmp(p_attr + i + 1, "fir", 3)))
        {
            fb = KLB_WEBRTC_FB_FIR;
        }
    }

    for (int k = 0; k < p_ml->pt_num; k++)
    {
        if (pt == p_ml->pts[k])
        {
            p_ml->fb_mask[k] |= fb;
            break;
        }
    }
}


/// @brief 判断编码名是否为本地支持的编码 (大小写不敏感)
/// @param [in] *p_name     编码名
/// @return bool true.支持
static bool supported_codec_klb_webrtc(const char* p_name)
{
    static const char* const s_names[] =
    {
        "h264", "h265", "vp8", "vp9", "av1", "opus", "pcmu", "pcma", "red", "ulpfec",
    };

    for (size_t i = 0; i < (sizeof(s_names) / sizeof(s_names[0])); i++)
    {
        const char* p_a = p_name;
        const char* p_b = s_names[i];

        while (('\0' != *p_a) && ('\0' != *p_b))
        {
            char ca = *p_a;
            char cb = *p_b;

            if (('A' <= ca) && ('Z' >= ca))
            {
                ca = (char)(ca + ('a' - 'A'));
            }

            if (ca != cb)
            {
                break;
            }

            p_a++;
            p_b++;
        }

        if (('\0' == *p_a) && ('\0' == *p_b))
        {
            return true;
        }
    }

    return false;
}


/// @brief 接收侧 RR 统计: 期望包数与抖动
/// @param [in] *p_peer     会话
/// @param [in] seq         RTP 序号
/// @param [in] rtp_ts      RTP 时间戳
/// @return 无
static void rr_track_klb_webrtc(klb_webrtc_peer_t* p_peer, uint16_t seq, uint32_t rtp_ts)
{
    uint64_t now = klb_tick_count64();

    if (!p_peer->rr_base_set)
    {
        p_peer->rr_base_set = true;
        p_peer->rr_ext_max = seq;
        p_peer->rr_expected = 1;
        p_peer->rr_transit = (int64_t)(now * 90u) - (int64_t)rtp_ts;

        return;
    }

    // 16 位序号按有符号差值展开
    int16_t diff = (int16_t)(seq - (uint16_t)p_peer->rr_ext_max);
    if (0 < diff)
    {
        p_peer->rr_ext_max = (uint32_t)(uint16_t)((uint16_t)p_peer->rr_ext_max + (uint16_t)diff);
        p_peer->rr_expected += (uint64_t)diff;
    }

    int64_t transit = (int64_t)(now * 90u) - (int64_t)rtp_ts;
    int64_t d = transit - p_peer->rr_transit;
    if (0 > d)
    {
        d = -d;
    }

    p_peer->rr_jitter = (uint32_t)((int64_t)p_peer->rr_jitter
        + ((d - (int64_t)p_peer->rr_jitter) / 16));
    p_peer->rr_transit = transit;
}


/// @brief TWCC 到达状态记录 (滑动窗口)
/// @param [in] *p_peer     会话
/// @param [in] seq         RTP 序号
/// @return 无
static void twcc_track_klb_webrtc(klb_webrtc_peer_t* p_peer, uint16_t seq)
{
    uint32_t now = (uint32_t)klb_tick_count64();

    if (0 == p_peer->twcc_count)
    {
        p_peer->twcc_base = seq;
        p_peer->twcc_count = 1;
        KLB_MEMSET(p_peer->twcc_status, 0, sizeof(p_peer->twcc_status));
        KLB_MEMSET(p_peer->twcc_arrive, 0, sizeof(p_peer->twcc_arrive));
        p_peer->twcc_status[0] = 1;
        p_peer->twcc_arrive[0] = now;

        return;
    }

    if (0 > (int16_t)(seq - p_peer->twcc_base))
    {
        return;                         // 迟到包忽略
    }

    int off = (int)(uint16_t)(seq - p_peer->twcc_base);
    if (KLB_WEBRTC_TWCC_MAX <= off)
    {
        // 超前过多: 窗口左移, 使当前包落在末位
        int shift = off - (KLB_WEBRTC_TWCC_MAX - 1);
        if (shift >= p_peer->twcc_count)
        {
            KLB_MEMSET(p_peer->twcc_status, 0, sizeof(p_peer->twcc_status));
            KLB_MEMSET(p_peer->twcc_arrive, 0, sizeof(p_peer->twcc_arrive));
            p_peer->twcc_count = 1;
        }
        else
        {
            memmove(p_peer->twcc_status, p_peer->twcc_status + shift,
                (size_t)(p_peer->twcc_count - shift));
            KLB_MEMSET(p_peer->twcc_status + (p_peer->twcc_count - shift), 0, (size_t)shift);
            memmove(p_peer->twcc_arrive, p_peer->twcc_arrive + shift,
                (size_t)(p_peer->twcc_count - shift) * sizeof(uint32_t));
            KLB_MEMSET(p_peer->twcc_arrive + (p_peer->twcc_count - shift), 0,
                (size_t)shift * sizeof(uint32_t));
            p_peer->twcc_count -= shift;
        }

        p_peer->twcc_base = (uint16_t)(seq - (p_peer->twcc_count - 1));
        off = p_peer->twcc_count - 1;
    }

    if ((off + 1) > p_peer->twcc_count)
    {
        p_peer->twcc_count = off + 1;
    }

    p_peer->twcc_status[off] = 1;
    p_peer->twcc_arrive[off] = now;
}


/// @brief 按 TWCC 反馈丢包率做码率自适应 (AIMD)
/// @param [in] *p_peer      会话
/// @param [in] *p_twcc      反馈包信息
/// @param [in] *p_status    包状态数组
/// @return 无
static void twcc_apply_klb_webrtc(klb_webrtc_peer_t* p_peer, const klb_rtcp_twcc_t* p_twcc,
    const uint8_t* p_status)
{
    int count = (int)p_twcc->count;
    if (0 >= count)
    {
        return;
    }

    int rx = 0;
    for (int i = 0; i < count; i++)
    {
        if (0 != p_status[i])
        {
            rx++;
        }
    }

    p_peer->twcc_loss_permille = (uint32_t)(((count - rx) * 1000) / count);
    p_peer->twcc_fb_rounds++;

    uint32_t target = p_peer->target_bps;
    if (0 == target)
    {
        target = KLB_WEBRTC_BPS_INIT;
    }

    if (100 <= p_peer->twcc_loss_permille)
    {
        target = (uint32_t)((target * 85u) / 100u);         // 重丢包: 乘性下降
    }
    else if (20 <= p_peer->twcc_loss_permille)
    {
        target = (uint32_t)((target * 95u) / 100u);         // 轻丢包: 小幅下降
    }
    else
    {
        target += (target / 16u);                           // 无丢包: 按比例提升
    }

    if (KLB_WEBRTC_BPS_MIN > target)
    {
        target = KLB_WEBRTC_BPS_MIN;
    }
    else if (KLB_WEBRTC_BPS_MAX < target)
    {
        target = KLB_WEBRTC_BPS_MAX;
    }

    p_peer->target_bps = target;
}


/// @brief 组装一个 RR 报告块 (取本端接收统计)
/// @param [in] *p_peer     会话
/// @param [out] *p_block   报告块
/// @return int 1.有数据; 0.尚未收到 RTP
static int build_block_klb_webrtc(klb_webrtc_peer_t* p_peer, klb_rtcp_report_block_t* p_block)
{
    if (!p_peer->rr_base_set)
    {
        return 0;
    }

    KLB_MEMSET(p_block, 0, sizeof(*p_block));

    uint64_t lost = (p_peer->rtp_rx_pkts < p_peer->rr_expected)
        ? (p_peer->rr_expected - p_peer->rtp_rx_pkts) : 0;

    p_block->ssrc = p_peer->rtp_ssrc_rx;
    p_block->ext_highest_seq = p_peer->rr_ext_max;
    p_block->jitter = p_peer->rr_jitter;
    p_block->cumulative_lost = (int32_t)lost;

    if (0 < p_peer->rr_expected)
    {
        uint64_t frac = (lost * 256u) / p_peer->rr_expected;
        p_block->fraction_lost = (uint8_t)((255 < frac) ? 255 : frac);
    }

    p_block->lsr = p_peer->rr_lsr;
    if (0 != p_peer->rr_lsr)
    {
        uint64_t d = klb_tick_count64() - p_peer->rr_last_sr_ms;
        p_block->dlsr = (uint32_t)((d * 65536u) / 1000u);
    }

    return 1;
}


/// @brief answer 侧镜像远端媒体 m-line (本地未配置时)
/// @param [in] *p_peer     会话
/// @param [in] *p_info     远端 SDP 解析结果
/// @return 无
static void adopt_media_klb_webrtc(klb_webrtc_peer_t* p_peer, const klb_webrtc_sdp_t* p_info)
{
    if (0 < p_peer->media_num)
    {
        return;
    }

    for (int i = 0; i < p_info->mline_num; i++)
    {
        const klb_webrtc_mline_t* p_ml = &p_info->mlines[i];
        if ((KLB_WEBRTC_MLINE_MAX <= p_peer->media_num) || (0 == p_ml->pt_num))
        {
            break;
        }

        int kind = KLB_WEBRTC_KIND_AUDIO;
        if (0 == strcmp(p_ml->kind, "video"))
        {
            kind = KLB_WEBRTC_KIND_VIDEO;
        }
        else if (0 != strcmp(p_ml->kind, "audio"))
        {
            continue;                   // application / 其他跳过
        }

        klb_webrtc_media_cfg_t* p_cfg = &p_peer->media[p_peer->media_num];
        KLB_MEMSET(p_cfg, 0, sizeof(*p_cfg));
        p_cfg->kind = kind;

        for (int k = 0; ((k < p_ml->pt_num) && (KLB_WEBRTC_CODEC_MAX > p_cfg->codec_num)); k++)
        {
            if (!supported_codec_klb_webrtc(p_ml->codec[k]))
            {
                continue;               // 能力裁剪: 跳过本地不支持的编码
            }

            klb_webrtc_codec_t* p_codec = &p_cfg->codecs[p_cfg->codec_num];
            p_codec->pt = p_ml->pts[k];
            copy_text_klb_webrtc(p_codec->name, KLB_WEBRTC_CODEC_NAME,
                p_ml->codec[k], (int)strlen(p_ml->codec[k]));
            p_codec->clock_rate = (0 < p_ml->rate[k]) ? p_ml->rate[k]
                : ((KLB_WEBRTC_KIND_VIDEO == kind) ? 90000 : 48000);
            p_codec->channels = p_ml->channels[k];
            copy_text_klb_webrtc(p_codec->fmtp, KLB_WEBRTC_FMTP_MAX,
                p_ml->fmtp[k], (int)strlen(p_ml->fmtp[k]));
            p_codec->fb_mask = p_ml->fb_mask[k];
            p_cfg->codec_num++;
        }

        if (0 < p_cfg->codec_num)
        {
            p_peer->media_num++;
        }
    }
}


int klb_webrtc_sdp_parse(const char* p_sdp, klb_webrtc_sdp_t* p_info)
{
    if ((NULL == p_sdp) || (NULL == p_info))
    {
        return KLB_NETCODE_INVAL;
    }

    KLB_MEMSET(p_info, 0, sizeof(klb_webrtc_sdp_t));

    const char* p_line = p_sdp;
    while (0 != *p_line)
    {
        const char* p_eol = strchr(p_line, '\n');
        int len = (NULL != p_eol) ? (int)(p_eol - p_line) : (int)strlen(p_line);

        // 去掉行尾 '\r' 与空白
        while ((0 < len) && (('\r' == p_line[len - 1]) || (' ' == p_line[len - 1])))
        {
            len--;
        }

        if ((2 <= len) && (0 == strncmp(p_line, "m=", 2)))
        {
            if ((KLB_WEBRTC_MLINE_MAX + 1) > p_info->mline_num)
            {
                parse_mline_klb_webrtc(p_line + 2, len - 2,
                    &p_info->mlines[p_info->mline_num]);
                p_info->mline_num++;
            }
        }
        else if ((2 < len) && (0 == strncmp(p_line, "a=", 2)))
        {
            const char* p_attr = p_line + 2;
            int alen = len - 2;

            if ((10 < alen) && (0 == strncmp(p_attr, "ice-ufrag:", 10)))
            {
                copy_text_klb_webrtc(p_info->ufrag, (int)sizeof(p_info->ufrag), p_attr + 10, alen - 10);
            }
            else if ((8 < alen) && (0 == strncmp(p_attr, "ice-pwd:", 8)))
            {
                copy_text_klb_webrtc(p_info->pwd, (int)sizeof(p_info->pwd), p_attr + 8, alen - 8);
            }
            else if ((19 < alen) && (0 == strncmp(p_attr, "fingerprint:", 12)))
            {
                copy_text_klb_webrtc(p_info->fingerprint, (int)sizeof(p_info->fingerprint),
                    p_attr + 12, alen - 12);
            }
            else if ((6 < alen) && (0 == strncmp(p_attr, "setup:", 6)))
            {
                copy_text_klb_webrtc(p_info->setup, (int)sizeof(p_info->setup),
                    p_attr + 6, alen - 6);
            }
            else if ((10 <= alen) && (0 == strncmp(p_attr, "candidate:", 10))
                     && (KLB_WEBRTC_MAX_CAND > p_info->candidate_num))
            {
                copy_text_klb_webrtc(p_info->candidates[p_info->candidate_num],
                    KLB_WEBRTC_CAND_MAX, p_attr, alen);
                p_info->candidate_num++;
            }
            else if ((7 < alen) && (0 == strncmp(p_attr, "rtpmap:", 7))
                     && (0 < p_info->mline_num))
            {
                parse_rtpmap_klb_webrtc(p_attr + 7, alen - 7,
                    &p_info->mlines[p_info->mline_num - 1]);
            }
            else if ((5 < alen) && (0 == strncmp(p_attr, "fmtp:", 5))
                     && (0 < p_info->mline_num))
            {
                parse_fmtp_klb_webrtc(p_attr + 5, alen - 5,
                    &p_info->mlines[p_info->mline_num - 1]);
            }
            else if ((8 < alen) && (0 == strncmp(p_attr, "rtcp-fb:", 8))
                     && (0 < p_info->mline_num))
            {
                parse_rtcp_fb_klb_webrtc(p_attr + 8, alen - 8,
                    &p_info->mlines[p_info->mline_num - 1]);
            }
        }

        if (NULL == p_eol)
        {
            break;
        }

        p_line = p_eol + 1;
    }

    return 0;
}


/// @brief 生成 SDP 文本 (offer / answer 共用)
/// @param [in]  *p_peer    会话 (本地候选须已收集)
/// @param [out] *p_sdp     输出缓冲
/// @param [in]  cap        缓冲容量
/// @return int >0.SDP 长度; 小于 0.klb_netcode_e
static int build_sdp_klb_webrtc(klb_webrtc_peer_t* p_peer, char* p_sdp, int cap)
{
    char ufrag[KLB_WEBRTC_CRED_MAX];
    char pwd[KLB_WEBRTC_CRED_MAX];

    int ret = klb_ice_agent_local_ufrag(p_peer->p_ice, ufrag, (int)sizeof(ufrag));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    ret = klb_ice_agent_local_pwd(p_peer->p_ice, pwd, (int)sizeof(pwd));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    int cand_num = klb_ice_agent_local_count(p_peer->p_ice);
    if (0 > cand_num)
    {
        return cand_num;
    }

    // step1. 角色: offer 用 actpass; answer 取与对端相反的角色
    const char* p_setup = "actpass";
    if (KLB_WEBRTC_ROLE_OFFER != p_peer->role)
    {
        p_setup = (0 == strcmp(p_peer->remote_setup, "active")) ? "passive" : "active";
    }

    int pos = 0;

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "v=0\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "o=- 0 0 IN IP4 0.0.0.0\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "s=-\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "t=0 0\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=group:BUNDLE 0");
    if (0 > pos)
    {
        return pos;
    }

    for (int i = 0; i < p_peer->media_num; i++)
    {
        pos = sdp_append_klb_webrtc(p_sdp, cap, pos, " %d", i + 1);
        if (0 > pos)
        {
            return pos;
        }
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=ice-options:trickle\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "m=application 9 UDP/DTLS/SCTP webrtc-datachannel\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "c=IN IP4 0.0.0.0\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=ice-ufrag:%s\r\n", ufrag);
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=ice-pwd:%s\r\n", pwd);
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=setup:%s\r\n", p_setup);
    if (0 > pos)
    {
        return pos;
    }

    // 已配置证书时附带 DTLS 指纹 (SDP 标准属性名)
    if (NULL != p_peer->p_cert_pem)
    {
        char fp[KLB_WEBRTC_FP_MAX];
        int fn = klb_socket_dtls_cert_fingerprint(p_peer->p_cert_pem, fp, (int)sizeof(fp));
        if (0 < fn)
        {
            pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=fingerprint:%s\r\n", fp);
            if (0 > pos)
            {
                return pos;
            }
        }
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=mid:0\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=sctp-port:5000\r\n");
    if (0 > pos)
    {
        return pos;
    }

    pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=max-message-size:262144\r\n");
    if (0 > pos)
    {
        return pos;
    }

    for (int i = 0; i < cand_num; i++)
    {
        char line[KLB_WEBRTC_CAND_MAX];

        int n = klb_ice_agent_local_candidate(p_peer->p_ice, i, line, (int)sizeof(line));
        if (0 > n)
        {
            continue;
        }

        pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=%s\r\n", line);
        if (0 > pos)
        {
            return pos;
        }
    }

    // 媒体 m-line: 走 BUNDLE, ICE/DTLS 参数集中在 mid 0
    for (int i = 0; i < p_peer->media_num; i++)
    {
        const klb_webrtc_media_cfg_t* p_cfg = &p_peer->media[i];
        const char* p_kind = (KLB_WEBRTC_KIND_AUDIO == p_cfg->kind) ? "audio" : "video";

        pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "m=%s 9 UDP/TLS/RTP/SAVPF", p_kind);
        if (0 > pos)
        {
            return pos;
        }

        for (int k = 0; k < p_cfg->codec_num; k++)
        {
            pos = sdp_append_klb_webrtc(p_sdp, cap, pos, " %d", p_cfg->codecs[k].pt);
            if (0 > pos)
            {
                return pos;
            }
        }

        pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
            "\r\nc=IN IP4 0.0.0.0\r\na=rtcp-mux\r\na=mid:%d\r\na=bundle-only\r\na=sendrecv\r\n",
            i + 1);
        if (0 > pos)
        {
            return pos;
        }

        for (int k = 0; k < p_cfg->codec_num; k++)
        {
            const klb_webrtc_codec_t* p_codec = &p_cfg->codecs[k];
            int rate = (0 < p_codec->clock_rate) ? p_codec->clock_rate
                : ((KLB_WEBRTC_KIND_AUDIO == p_cfg->kind) ? 48000 : 90000);

            pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=rtpmap:%d %s/%d",
                p_codec->pt, p_codec->name, rate);
            if (0 > pos)
            {
                return pos;
            }

            if ((KLB_WEBRTC_KIND_AUDIO == p_cfg->kind) && (1 < p_codec->channels))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "/%d", p_codec->channels);
                if (0 > pos)
                {
                    return pos;
                }
            }

            pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "\r\n");
            if (0 > pos)
            {
                return pos;
            }

            // step2. 编码参数与反馈能力
            if ('\0' != p_codec->fmtp[0])
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos, "a=fmtp:%d %s\r\n",
                    p_codec->pt, p_codec->fmtp);
                if (0 > pos)
                {
                    return pos;
                }
            }

            int fb = p_codec->fb_mask;

            if (0 != (fb & KLB_WEBRTC_FB_PLI))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
                    "a=rtcp-fb:%d nack pli\r\n", p_codec->pt);
            }
            else if (0 != (fb & KLB_WEBRTC_FB_NACK))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
                    "a=rtcp-fb:%d nack\r\n", p_codec->pt);
            }

            if (0 > pos)
            {
                return pos;
            }

            if (0 != (fb & KLB_WEBRTC_FB_FIR))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
                    "a=rtcp-fb:%d ccm fir\r\n", p_codec->pt);
                if (0 > pos)
                {
                    return pos;
                }
            }

            if (0 != (fb & KLB_WEBRTC_FB_REMB))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
                    "a=rtcp-fb:%d goog-remb\r\n", p_codec->pt);
                if (0 > pos)
                {
                    return pos;
                }
            }

            if (0 != (fb & KLB_WEBRTC_FB_TWCC))
            {
                pos = sdp_append_klb_webrtc(p_sdp, cap, pos,
                    "a=rtcp-fb:%d transport-cc\r\n", p_codec->pt);
                if (0 > pos)
                {
                    return pos;
                }
            }
        }
    }

    return pos;
}


/// @brief 确保本地候选已收集
/// @param [in] *p_peer     会话
/// @return int 0.成功; 小于 0.klb_netcode_e
static int prepare_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if (p_peer->gathered)
    {
        return 0;
    }

    int ret = klb_ice_agent_gather(p_peer->p_ice);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->gathered = true;
    return 0;
}


/// @brief 复制一份文本到堆
/// @param [in] *p_src      源文本; 可为 NULL
/// @return char* NULL.失败或源为 NULL; 非 NULL.副本
static char* dup_text_klb_webrtc(const char* p_src)
{
    if (NULL == p_src)
    {
        return NULL;
    }

    int len = (int)strlen(p_src);
    char* p_dst = KLB_MALLOC(char, len + 1, 0);
    if (NULL == p_dst)
    {
        return NULL;
    }

    memcpy(p_dst, p_src, (size_t)len);
    p_dst[len] = '\0';

    return p_dst;
}


/// @brief 队列写入一个报文
/// @return int 0.成功; 小于 0.klb_netcode_e
static int queue_push_klb_webrtc(klb_webrtc_queue_t* p_queue, const uint8_t* p_data, int len)
{
    if ((0 > len) || (KLB_WEBRTC_PKT_MAX < len))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    if (KLB_WEBRTC_QUEUE_SLOTS <= p_queue->count)
    {
        return KLB_NETCODE_LIMIT;
    }

    int idx = (p_queue->head + p_queue->count) % KLB_WEBRTC_QUEUE_SLOTS;

    memcpy(p_queue->slots[idx].data, p_data, (size_t)len);
    p_queue->slots[idx].len = len;
    p_queue->count++;

    return 0;
}


/// @brief 队列取出一个报文
/// @return int 大于 0.字节数; 0.队列空
static int queue_pop_klb_webrtc(klb_webrtc_queue_t* p_queue, uint8_t* p_buf, int cap)
{
    if (0 >= p_queue->count)
    {
        return 0;
    }

    klb_webrtc_pkt_t* p_pkt = &p_queue->slots[p_queue->head];
    int n = (p_pkt->len > cap) ? cap : p_pkt->len;

    memcpy(p_buf, p_pkt->data, (size_t)n);

    p_queue->head = (p_queue->head + 1) % KLB_WEBRTC_QUEUE_SLOTS;
    p_queue->count--;

    return n;
}


/// @brief DTLS 抽象传输: 经 ICE 已选通道发送
static int dtls_io_send_klb_webrtc(void* p_ctx, const uint8_t* p_data, int len)
{
    klb_webrtc_peer_t* p_peer = (klb_webrtc_peer_t*)p_ctx;

    int ret = klb_ice_agent_send(p_peer->p_ice, p_data, len);

    return (0 == ret) ? len : -1;
}


/// @brief DTLS 抽象传输: 从 DTLS 记录队列取包
static int dtls_io_recv_klb_webrtc(void* p_ctx, uint8_t* p_buf, int cap)
{
    klb_webrtc_peer_t* p_peer = (klb_webrtc_peer_t*)p_ctx;

    return queue_pop_klb_webrtc(&p_peer->q_dtls, p_buf, cap);
}


/// @brief 初始化 SRTP 收发上下文 (双向密钥)
/// @return int 0.成功; 小于 0.klb_netcode_e
static int init_srtp_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    uint8_t key[KLB_SRTP_MASTER_KEY_MAX];
    uint8_t salt[KLB_SRTP_MASTER_SALT_MAX];
    int code = 0;

    int ret = klb_socket_dtls_srtp_export(p_peer->p_dtls, key, (int)sizeof(key),
        salt, (int)sizeof(salt));
    if (0 > ret)
    {
        return ret;
    }

    p_peer->p_srtp_tx = klb_srtp_create(KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80, &code);
    if (NULL == p_peer->p_srtp_tx)
    {
        return (0 != code) ? code : KLB_NETCODE_ERR;
    }

    ret = klb_srtp_set_key(p_peer->p_srtp_tx, key, (int)sizeof(key), salt, (int)sizeof(salt));
    if (0 > ret)
    {
        return ret;
    }

    ret = klb_socket_dtls_srtp_export_peer(p_peer->p_dtls, key, (int)sizeof(key),
        salt, (int)sizeof(salt));
    if (0 > ret)
    {
        return ret;
    }

    p_peer->p_srtp_rx = klb_srtp_create(KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80, &code);
    if (NULL == p_peer->p_srtp_rx)
    {
        return (0 != code) ? code : KLB_NETCODE_ERR;
    }

    ret = klb_srtp_set_key(p_peer->p_srtp_rx, key, (int)sizeof(key), salt, (int)sizeof(salt));
    if (0 > ret)
    {
        return ret;
    }

    // 允许 NACK 重传的同序号包通过重放窗口
    return klb_srtp_set_repeat(p_peer->p_srtp_rx, 1);
}


/// @brief 按需创建 DTLS 会话
/// @return int 0.已就绪; 1.ICE 尚未连通; 小于 0.klb_netcode_e
static int open_dtls_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if (NULL != p_peer->p_dtls)
    {
        return 0;
    }

    if ((NULL == p_peer->p_cert_pem) || (NULL == p_peer->p_key_pem))
    {
        return KLB_NETCODE_NOTSUP;
    }

    if (KLB_ICE_STATE_CONNECTED != klb_ice_agent_get_state(p_peer->p_ice))
    {
        return 1;
    }

    klb_socket_dtls_io_t io;
    KLB_MEMSET(&io, 0, sizeof(io));
    io.send = dtls_io_send_klb_webrtc;
    io.recv = dtls_io_recv_klb_webrtc;
    io.p_ctx = p_peer;

    klb_socket_dtls_cfg_t cfg;
    KLB_MEMSET(&cfg, 0, sizeof(cfg));
    cfg.p_cert_pem = p_peer->p_cert_pem;
    cfg.p_key_pem = p_peer->p_key_pem;
    cfg.p_ca_pem = p_peer->p_ca_pem;
    cfg.verify_peer = p_peer->verify_peer;

    // offer 方 actpass 视为 DTLS 服务端; answer 方 active 视为客户端
    bool is_server = (KLB_WEBRTC_ROLE_OFFER == p_peer->role) ? true : false;
    int code = 0;

    p_peer->p_dtls = klb_socket_dtls_io_open(is_server, &io, &cfg, &code);
    if (NULL == p_peer->p_dtls)
    {
        return (0 != code) ? code : KLB_NETCODE_ERR;
    }

    return 0;
}


/// @brief 校验对端证书指纹
/// @return int 0.通过或未提供; 小于 0.klb_netcode_e
static int check_fp_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if ('\0' == p_peer->remote_fp[0])
    {
        return 0;
    }

    char fp[KLB_WEBRTC_FP_MAX];
    int ret = klb_socket_dtls_fingerprint(p_peer->p_dtls, fp, (int)sizeof(fp));
    if (0 > ret)
    {
        // 不校验对端证书时, DTLS 服务端不请求客户端证书, 无从核对指纹
        if ((KLB_NETCODE_NOENT == ret) && (0 == p_peer->verify_peer))
        {
            return 0;
        }

        return ret;
    }

    return (0 == strcmp(fp, p_peer->remote_fp)) ? 0 : KLB_NETCODE_TLS_CERT;
}


/// @brief 推进 DTLS 握手; 首次完成时导出 SRTP 密钥并校验指纹
/// @return int 0.握手完成; 1.继续/尚未连通; 小于 0.klb_netcode_e
static int pump_dtls_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    int ret = open_dtls_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    if (1 == ret)
    {
        return 1;
    }

    int hs = klb_socket_dtls_handshake(p_peer->p_dtls);
    if (0 > hs)
    {
        return hs;
    }

    if ((0 == hs) && (!p_peer->dtls_done))
    {
        int code = check_fp_klb_webrtc(p_peer);
        if (0 > code)
        {
            return code;
        }

        code = init_srtp_klb_webrtc(p_peer);
        if (0 > code)
        {
            return code;
        }

        p_peer->dtls_done = true;
    }

    return (0 == hs) ? 0 : 1;
}


/// @brief 生成本端媒体 SSRC (时间与序号混合, 保证非零)
/// @return uint32_t SSRC
static uint32_t make_ssrc_klb_webrtc(void)
{
    static uint32_t seed = 0;

    seed++;
    uint32_t ssrc = (uint32_t)time(NULL) ^ (seed * 2654435761u);
    if (0 == ssrc)
    {
        ssrc = 0x4B4C4201u;
    }

    return ssrc;
}


/// @brief 写入重传缓存 (环形覆盖最旧一项)
/// @param [in] *p_peer     会话
/// @param [in] seq         RTP 序号
/// @param [in] *p_data     已保护的报文
/// @param [in] len         报文长度
/// @return 无
static void sent_put_klb_webrtc(klb_webrtc_peer_t* p_peer, uint16_t seq,
                                const uint8_t* p_data, int len)
{
    uint8_t* p_copy = KLB_MALLOC(uint8_t, len, 0);
    if (NULL == p_copy)
    {
        return;
    }

    memcpy(p_copy, p_data, (size_t)len);

    klb_webrtc_sent_t* p_slot = &p_peer->sent[p_peer->sent_head];
    KLB_FREE(p_slot->p_data);

    p_slot->p_data = p_copy;
    p_slot->len = len;
    p_slot->seq = seq;

    p_peer->sent_head = (p_peer->sent_head + 1) % KLB_WEBRTC_SENT_SLOTS;
}


/// @brief 按序号从重传缓存重发
/// @param [in] *p_peer     会话
/// @param [in] seq         待重传序号
/// @return int 0.已处理 (命中并发送或未命中); 小于 0.klb_netcode_e
static int retransmit_klb_webrtc(klb_webrtc_peer_t* p_peer, uint16_t seq)
{
    for (int i = 0; i < KLB_WEBRTC_SENT_SLOTS; i++)
    {
        klb_webrtc_sent_t* p_slot = &p_peer->sent[i];
        if ((NULL == p_slot->p_data) || (seq != p_slot->seq))
        {
            continue;
        }

        int ret = klb_ice_agent_send(p_peer->p_ice, p_slot->p_data, p_slot->len);
        if (0 > ret)
        {
            return ret;
        }

        p_peer->retrans_pkts++;
        p_peer->srtp_tx_pkts++;

        return 0;
    }

    return 0;
}


/// @brief 经 SRTP 保护后由 ICE 发送一个 RTP 包, 可选写入重传缓存
/// @param [in] *p_peer     会话
/// @param [in] *p_data     RTP 包 (明文)
/// @param [in] len         长度
/// @param [in] seq         RTP 序号; 仅缓存时使用
/// @param [in] cache       是否写入重传缓存
/// @return int 0.成功; 小于 0.klb_netcode_e
static int send_rtp_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_data,
                               int len, uint16_t seq, bool cache)
{
    if (KLB_WEBRTC_PKT_MAX < len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    uint8_t pkt[KLB_WEBRTC_PKT_MAX];
    memcpy(pkt, p_data, (size_t)len);

    int out_len = len;
    if (NULL != p_peer->p_srtp_tx)
    {
        int ret = klb_srtp_protect(p_peer->p_srtp_tx, pkt, len, (int)sizeof(pkt), &out_len);
        if (0 > ret)
        {
            return ret;
        }

        p_peer->srtp_tx_pkts++;
    }

    int ret = klb_ice_agent_send(p_peer->p_ice, pkt, out_len);
    if (0 > ret)
    {
        return ret;
    }

    if (cache)
    {
        sent_put_klb_webrtc(p_peer, seq, pkt, out_len);
    }

    return 0;
}


/// @brief 经 SRTCP 保护后由 ICE 发送一个 RTCP 包
/// @param [in] *p_peer     会话
/// @param [in] *p_data     RTCP 包 (明文)
/// @param [in] len         长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int send_rtcp_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    if ((0 >= len) || (KLB_WEBRTC_PKT_MAX < len))
    {
        return KLB_NETCODE_MSGSIZE;
    }

    uint8_t pkt[KLB_WEBRTC_PKT_MAX];
    memcpy(pkt, p_data, (size_t)len);

    int out_len = len;
    if (NULL != p_peer->p_srtp_tx)
    {
        int ret = klb_srtp_protect(p_peer->p_srtp_tx, pkt, len, (int)sizeof(pkt), &out_len);
        if (0 > ret)
        {
            return ret;
        }

        p_peer->srtp_tx_pkts++;
    }

    int ret = klb_ice_agent_send(p_peer->p_ice, pkt, out_len);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->rtcp_tx_pkts++;

    return 0;
}


/// @brief 媒体面发送前置: 推进 DTLS 并确认 SRTP 发送上下文就绪
/// @param [in] *p_peer     会话
/// @return int 0.就绪; 小于 0.klb_netcode_e
static int ensure_media_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer->p_cert_pem)
    {
        return KLB_NETCODE_NOTCONN;
    }

    int ret = pump_dtls_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    if (1 == ret)
    {
        return KLB_NETCODE_NOTCONN;
    }

    return (NULL != p_peer->p_srtp_tx) ? 0 : KLB_NETCODE_NOTCONN;
}


/// @brief 处理入站 RTCP: SR/RR 记录, TWCC 计数, NACK 展开序号重传
/// @param [in] *p_peer     会话
/// @param [in] *p_data     RTCP 包
/// @param [in] len         长度
/// @return 无
static void handle_rtcp_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    klb_rtcp_head_t head;
    if (0 > klb_rtcp_head_parse(p_data, len, &head))
    {
        return;
    }

    p_peer->rtcp_rx_pkts++;

    if (KLB_RTCP_SR == head.type)
    {
        klb_rtcp_sr_t sr;
        if (0 > klb_rtcp_sr_parse(p_data, len, &sr))
        {
            return;
        }

        p_peer->report_rx_pkts++;
        p_peer->rr_lsr = (uint32_t)(((sr.ntp_sec & 0xFFFFu) << 16) | (sr.ntp_frac >> 16));
        p_peer->rr_last_sr_ms = klb_tick_count64();

        return;
    }

    if (KLB_RTCP_RR == head.type)
    {
        p_peer->report_rx_pkts++;

        return;
    }

    if (KLB_RTCP_RTPFB != head.type)
    {
        return;
    }

    // RTPFB FMT 15: TWCC 传输层拥塞控制反馈
    if (15 == head.count)
    {
        klb_rtcp_twcc_t twcc;
        uint8_t status[KLB_WEBRTC_TWCC_MAX];

        if (0 > klb_rtcp_twcc_parse(p_data, len, &twcc, status, (int)sizeof(status)))
        {
            return;
        }

        p_peer->twcc_rx_pkts++;
        twcc_apply_klb_webrtc(p_peer, &twcc, status);

        return;
    }

    // RTPFB FMT 1: NACK 重传
    if (1 != head.count)
    {
        return;
    }

    klb_rtcp_fb_t fb;
    if (0 > klb_rtcp_fb_parse(p_data, len, &fb))
    {
        return;
    }

    p_peer->nack_rx_pkts++;

    retransmit_klb_webrtc(p_peer, fb.pid);

    for (int bit = 0; bit < 16; bit++)
    {
        if (0 != (fb.blp & (uint16_t)(1u << bit)))
        {
            retransmit_klb_webrtc(p_peer, (uint16_t)(fb.pid + 1 + bit));
        }
    }
}


/// @brief 处理入站 RTP: 记录对端 SSRC 并写入抖动缓冲
/// @param [in] *p_peer     会话
/// @param [in] *p_data     RTP 包
/// @param [in] len         长度
/// @return int 1.已入缓冲; 0.非 RTP 或创建缓冲失败
static int handle_rtp_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    klb_rtp_head_t head;
    if (0 > klb_rtp_head_parse(p_data, len, &head, NULL))
    {
        return 0;
    }

    if (0 == p_peer->rtp_ssrc_rx)
    {
        p_peer->rtp_ssrc_rx = head.ssrc;
    }

    rr_track_klb_webrtc(p_peer, head.sequence, head.timestamp);
    twcc_track_klb_webrtc(p_peer, head.sequence);

    if (NULL == p_peer->p_jb)
    {
        int code = 0;
        p_peer->p_jb = klb_webrtc_jb_create(KLB_WEBRTC_JB_SLOTS, &code);
        if (NULL == p_peer->p_jb)
        {
            return 0;
        }
    }

    p_peer->rtp_rx_pkts++;
    klb_webrtc_jb_put(p_peer->p_jb, head.sequence, p_data, len);

    return 1;
}


/// @brief 缺口处理: 补发 NACK, 缺口持续超时后跳过
/// @param [in] *p_peer     会话
/// @return int 1.已跳过且有包可再取; 0.无动作
static int fill_gap_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    uint16_t miss[KLB_WEBRTC_NACK_MAX];

    int num = klb_webrtc_jb_missing(p_peer->p_jb, miss, KLB_WEBRTC_NACK_MAX);
    if (0 >= num)
    {
        return 0;
    }

    if ((miss[0] != p_peer->nack_first) || (KLB_WEBRTC_NACK_REPLY > p_peer->nack_try))
    {
        if (0 == klb_webrtc_peer_nack_send(p_peer, miss, num))
        {
            p_peer->nack_first = miss[0];
        }

        p_peer->nack_try++;
    }

    p_peer->rtp_gap++;
    if (KLB_WEBRTC_JB_GAP_MAX > p_peer->rtp_gap)
    {
        return 0;
    }

    int skipped = klb_webrtc_jb_skip(p_peer->p_jb);
    if (0 < skipped)
    {
        p_peer->rtp_lost_pkts += (uint64_t)skipped;
    }

    p_peer->rtp_gap = 0;
    p_peer->nack_try = 0;
    p_peer->nack_first = 0;

    return (0 < skipped) ? 1 : 0;
}


/// @brief 追加一个事件到队列 (满则丢最旧)
/// @return 无
static void push_event_klb_webrtc(klb_webrtc_peer_t* p_peer, int type, int code, const char* p_name)
{
    if (KLB_WEBRTC_EVT_SLOTS <= p_peer->evt_count)
    {
        p_peer->evt_head = (p_peer->evt_head + 1) % KLB_WEBRTC_EVT_SLOTS;
        p_peer->evt_count--;
    }

    int idx = (p_peer->evt_head + p_peer->evt_count) % KLB_WEBRTC_EVT_SLOTS;
    klb_webrtc_event_t* p_evt = &p_peer->evt[idx];

    KLB_MEMSET(p_evt, 0, sizeof(klb_webrtc_event_t));
    p_evt->type = type;
    p_evt->code = code;

    if (NULL != p_name)
    {
        copy_text_klb_webrtc(p_evt->name, (int)sizeof(p_evt->name), p_name, (int)strlen(p_name));
    }

    p_peer->evt_count++;
}


/// @brief 按当前状态补产生事件 (仅状态迁移时入队)
/// @return 无
static void collect_events_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return;
    }

    int state = klb_ice_agent_get_state(p_peer->p_ice);
    if (state != p_peer->evt_ice_state)
    {
        p_peer->evt_ice_state = state;
        push_event_klb_webrtc(p_peer, KLB_WEBRTC_EVT_ICE_STATE, state, "ice_state");
    }

    if ((0 != p_peer->dtls_done) && (!p_peer->evt_dtls))
    {
        p_peer->evt_dtls = true;
        push_event_klb_webrtc(p_peer, KLB_WEBRTC_EVT_DTLS_READY, 0, "dtls_ready");
    }

    if ((NULL != p_peer->p_srtp_rx) && (!p_peer->evt_srtp))
    {
        p_peer->evt_srtp = true;
        push_event_klb_webrtc(p_peer, KLB_WEBRTC_EVT_SRTP_READY, 0, "srtp_ready");
    }

    if ((NULL != p_peer->p_dc) && (0 != klb_webrtc_dc_is_open(p_peer->p_dc)) && (!p_peer->evt_dc))
    {
        p_peer->evt_dc = true;
        push_event_klb_webrtc(p_peer, KLB_WEBRTC_EVT_DC_OPEN, 0, "dc_open");
    }
}


/// @brief 从 ICE 取一包并按类型分流 (定义在媒体面辅助之后)
/// @param [in] *p_peer     会话
/// @return int 1.已取到一包; 0.无数据; 小于 0.klb_netcode_e
static int pull_klb_webrtc(klb_webrtc_peer_t* p_peer);


/// @brief 媒体面取包: 推 DTLS/ICE, 再从抖动缓冲有序出队
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓冲
/// @param [in]  cap            缓冲容量
/// @return int 大于 0.字节数; 0.暂无; 小于 0.klb_netcode_e
static int media_pull_klb_webrtc(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap)
{
    collect_events_klb_webrtc(p_peer);

    for (int guard = 0; guard < 32; guard++)
    {
        int n = klb_webrtc_jb_get(p_peer->p_jb, p_buf, cap, NULL);
        if (0 < n)
        {
            p_peer->rtp_gap = 0;
            p_peer->nack_try = 0;
            p_peer->nack_first = 0;

            return n;
        }

        int got = pull_klb_webrtc(p_peer);
        if (0 > got)
        {
            return got;
        }

        if (0 == got)
        {
            break;
        }

        if (NULL != p_peer->p_cert_pem)
        {
            int hs = pump_dtls_klb_webrtc(p_peer);
            if (0 > hs)
            {
                return hs;
            }
        }
    }

    if (0 < fill_gap_klb_webrtc(p_peer))
    {
        int n = klb_webrtc_jb_get(p_peer->p_jb, p_buf, cap, NULL);
        if (0 < n)
        {
            return n;
        }
    }

    return 0;
}


/// @brief 清空 H.264 分片重组状态
/// @param [in] *p_peer     会话
/// @return 无
static void reset_nalu_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    p_peer->nalu_len = 0;
    p_peer->nalu_broken = false;
}


/// @brief 追加分片数据到重组缓冲 (按需扩容)
/// @param [in] *p_peer     会话
/// @param [in] *p_data     分片数据
/// @param [in] len         分片长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int append_nalu_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    if (0 >= len)
    {
        return 0;
    }

    int need = p_peer->nalu_len + len;
    if (KLB_WEBRTC_NALU_MAX < need)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    if (p_peer->nalu_cap < need)
    {
        int cap = (need + 1023) & ~1023;
        if (KLB_WEBRTC_NALU_MAX < cap)
        {
            cap = KLB_WEBRTC_NALU_MAX;
        }

        uint8_t* p_new = KLB_MALLOC(uint8_t, cap, 0);
        if (NULL == p_new)
        {
            return KLB_NETCODE_NOMEM;
        }

        if (NULL != p_peer->p_nalu)
        {
            memcpy(p_new, p_peer->p_nalu, (size_t)p_peer->nalu_len);
            KLB_FREE(p_peer->p_nalu);
        }

        p_peer->p_nalu = p_new;
        p_peer->nalu_cap = cap;
    }

    memcpy(p_peer->p_nalu + p_peer->nalu_len, p_data, (size_t)len);
    p_peer->nalu_len = need;

    return 0;
}


/// @brief 清空 STAP-A 子 NALU 队列
/// @param [in] *p_peer     会话
/// @return 无
static void reset_stap_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    p_peer->stap_len = 0;
    p_peer->stap_off = 0;
}


/// @brief 取 STAP-A 队列中的下一个 NALU
/// @param [in]  *p_peer    会话
/// @param [out] *p_buf     输出缓存
/// @param [in]  cap        缓存大小
/// @return int 大于 0.NALU 长度; 0.队列空; 小于 0.klb_netcode_e
static int pull_stap_klb_webrtc(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap)
{
    if (p_peer->stap_off >= p_peer->stap_len)
    {
        reset_stap_klb_webrtc(p_peer);

        return 0;
    }

    const uint8_t* p = p_peer->p_stap + p_peer->stap_off;
    int n = ((int)p[0] << 8) | (int)p[1];

    if (n > cap)
    {
        reset_stap_klb_webrtc(p_peer);

        return KLB_NETCODE_MSGSIZE;
    }

    memcpy(p_buf, p + 2, (size_t)n);
    p_peer->stap_off += 2 + n;

    return n;
}


/// @brief 把 STAP-A 载荷拆分为子 NALU 队列
/// @param [in] *p_peer     会话
/// @param [in] *p_payload  STAP-A 载荷 (含 1 字节 STAP 头)
/// @param [in] plen        载荷长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int store_stap_klb_webrtc(klb_webrtc_peer_t* p_peer, const uint8_t* p_payload, int plen)
{
    reset_stap_klb_webrtc(p_peer);

    if (p_peer->stap_cap < plen)
    {
        uint8_t* p_new = KLB_MALLOC(uint8_t, plen, 0);
        if (NULL == p_new)
        {
            return KLB_NETCODE_NOMEM;
        }

        KLB_FREE(p_peer->p_stap);
        p_peer->p_stap = p_new;
        p_peer->stap_cap = plen;
    }

    int off = 1;                        // 跳过 STAP-A 头
    while ((off + 2) <= plen)
    {
        int sub_len = ((int)p_payload[off] << 8) | (int)p_payload[off + 1];
        off += 2;

        if ((0 >= sub_len) || ((off + sub_len) > plen))
        {
            break;
        }

        uint8_t* p_dst = p_peer->p_stap + p_peer->stap_len;
        p_dst[0] = (uint8_t)((sub_len >> 8) & 0xFF);
        p_dst[1] = (uint8_t)(sub_len & 0xFF);
        memcpy(p_dst + 2, p_payload + off, (size_t)sub_len);

        p_peer->stap_len += 2 + sub_len;
        off += sub_len;
    }

    return 0;
}


/// @brief 从 ICE 取一包并按类型分流到 DTLS / 应用队列
/// @return int 1.已取到一包; 0.无数据; 小于 0.klb_netcode_e
static int pull_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if (KLB_WEBRTC_QUEUE_SLOTS <= (p_peer->q_dtls.count + p_peer->q_app.count))
    {
        return 0;
    }

    uint8_t buf[KLB_WEBRTC_PKT_MAX];
    int code = 0;

    int n = klb_ice_agent_recv(p_peer->p_ice, buf, (int)sizeof(buf), &code);
    if (0 > n)
    {
        return n;
    }

    if (0 == n)
    {
        return 0;
    }

    uint8_t b0 = buf[0];

    // DTLS 记录类型: 20(CCS)/21(Alert)/22(Handshake)/23(AppData)
    if ((20 <= b0) && (23 >= b0))
    {
        if (NULL == p_peer->p_dtls)
        {
            return 0;
        }

        queue_push_klb_webrtc(&p_peer->q_dtls, buf, n);
        return 1;
    }

    // SRTP/SRTCP: 版本位 10
    if ((0x80 == (b0 & 0xC0)) && (NULL != p_peer->p_srtp_rx))
    {
        int out_len = 0;

        if (0 > klb_srtp_unprotect(p_peer->p_srtp_rx, buf, n, &out_len))
        {
            return 1;           // 非法/重放包直接丢弃
        }

        p_peer->srtp_rx_pkts++;
        n = out_len;

        if (2 > n)
        {
            return 1;
        }

        // RTCP: PT 192..223; 其余按 RTP 入抖动缓冲
        int pt = (int)buf[1];
        if ((192 <= pt) && (223 >= pt))
        {
            handle_rtcp_klb_webrtc(p_peer, buf, n);
            return 1;
        }

        handle_rtp_klb_webrtc(p_peer, buf, n);

        return 1;
    }

    queue_push_klb_webrtc(&p_peer->q_app, buf, n);

    return 1;
}


/// @brief 把 DataChannel 待发控制包经 DTLS 全部送出
/// @return int 0.成功; 小于 0.klb_netcode_e
static int flush_dc_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if ((NULL == p_peer->p_dc) || (NULL == p_peer->p_dtls))
    {
        return 0;
    }

    uint8_t pkt[KLB_WEBRTC_PKT_MAX];

    for (int guard = 0; guard < 16; guard++)
    {
        int out_len = 0;
        int ret = klb_webrtc_dc_poll(p_peer->p_dc, pkt, (int)sizeof(pkt), &out_len);
        if (0 > ret)
        {
            return ret;
        }

        if (0 >= out_len)
        {
            return 0;
        }

        ret = klb_socket_send(p_peer->p_dtls, pkt, out_len);
        if (0 >= ret)
        {
            return (0 > ret) ? ret : 0;
        }

        p_peer->dtls_tx_pkts++;
    }

    return 0;
}


klb_webrtc_peer_t* klb_webrtc_peer_create(klb_netmulti_t* p_netmulti, int role, int* p_code)
{
    if ((NULL == p_netmulti)
        || ((KLB_WEBRTC_ROLE_OFFER != role) && (KLB_WEBRTC_ROLE_ANSWER != role)))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    int ice_role = (KLB_WEBRTC_ROLE_OFFER == role) ? KLB_ICE_ROLE_CONTROLLING : KLB_ICE_ROLE_CONTROLLED;
    int code = 0;

    klb_ice_agent_t* p_ice = klb_ice_agent_create(p_netmulti, ice_role, &code);
    if (NULL == p_ice)
    {
        if (NULL != p_code)
        {
            *p_code = (0 != code) ? code : KLB_NETCODE_ERR;
        }

        return NULL;
    }

    klb_webrtc_peer_t* p_peer = KLB_MALLOCZ(klb_webrtc_peer_t, 1, 0);
    if (NULL == p_peer)
    {
        klb_ice_agent_destroy(p_ice);

        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_NOMEM;
        }

        return NULL;
    }

    p_peer->p_netmulti = p_netmulti;
    p_peer->role = role;
    p_peer->p_ice = p_ice;
    p_peer->evt_ice_state = klb_ice_agent_get_state(p_ice);
    p_peer->rtp_ssrc_tx = make_ssrc_klb_webrtc();
    p_peer->rtp_seq_tx = (uint16_t)(p_peer->rtp_ssrc_tx & 0xFFFFu);
    p_peer->target_bps = KLB_WEBRTC_BPS_INIT;

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_peer;
}


int klb_webrtc_peer_set_dtls_cert(klb_webrtc_peer_t* p_peer, const char* p_cert_pem,
    const char* p_key_pem, const char* p_ca_pem, int verify_peer)
{
    if ((NULL == p_peer) || (NULL == p_cert_pem) || (NULL == p_key_pem) || (NULL != p_peer->p_dtls))
    {
        return KLB_NETCODE_INVAL;
    }

    char* p_cert = dup_text_klb_webrtc(p_cert_pem);
    char* p_key = dup_text_klb_webrtc(p_key_pem);
    char* p_ca = dup_text_klb_webrtc(p_ca_pem);
    if ((NULL == p_cert) || (NULL == p_key) || ((NULL != p_ca_pem) && (NULL == p_ca)))
    {
        KLB_FREE(p_cert);
        KLB_FREE(p_key);
        KLB_FREE(p_ca);

        return KLB_NETCODE_NOMEM;
    }

    KLB_FREE(p_peer->p_cert_pem);
    KLB_FREE(p_peer->p_key_pem);
    KLB_FREE(p_peer->p_ca_pem);

    p_peer->p_cert_pem = p_cert;
    p_peer->p_key_pem = p_key;
    p_peer->p_ca_pem = p_ca;
    p_peer->verify_peer = verify_peer;

    return 0;
}


void klb_webrtc_peer_destroy(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return;
    }

    if (NULL != p_peer->p_dtls)
    {
        klb_socket_destroy(p_peer->p_dtls);
        p_peer->p_dtls = NULL;
    }

    klb_srtp_destroy(p_peer->p_srtp_tx);
    klb_srtp_destroy(p_peer->p_srtp_rx);

    klb_webrtc_dc_destroy(p_peer->p_dc);

    KLB_FREE(p_peer->p_cert_pem);
    KLB_FREE(p_peer->p_key_pem);
    KLB_FREE(p_peer->p_ca_pem);

    if (NULL != p_peer->p_jb)
    {
        klb_webrtc_jb_destroy(p_peer->p_jb);
        p_peer->p_jb = NULL;
    }

    for (int i = 0; i < KLB_WEBRTC_SENT_SLOTS; i++)
    {
        KLB_FREE(p_peer->sent[i].p_data);
    }

    KLB_FREE(p_peer->p_nalu);
    KLB_FREE(p_peer->p_stap);

    if (NULL != p_peer->p_ice)
    {
        klb_ice_agent_destroy(p_peer->p_ice);
        p_peer->p_ice = NULL;
    }

    KLB_FREE(p_peer);
}


int klb_webrtc_peer_create_offer(klb_webrtc_peer_t* p_peer, char* p_sdp, int cap)
{
    if ((NULL == p_peer) || (NULL == p_sdp) || (0 >= cap))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = prepare_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    return build_sdp_klb_webrtc(p_peer, p_sdp, cap);
}


int klb_webrtc_peer_create_answer(klb_webrtc_peer_t* p_peer, const char* p_remote_sdp,
                                  char* p_sdp, int cap)
{
    if ((NULL == p_peer) || (NULL == p_remote_sdp) || (NULL == p_sdp) || (0 >= cap))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = klb_webrtc_peer_set_remote_sdp(p_peer, p_remote_sdp);
    if (0 > ret)
    {
        return ret;
    }

    return build_sdp_klb_webrtc(p_peer, p_sdp, cap);
}


int klb_webrtc_peer_set_remote_sdp(klb_webrtc_peer_t* p_peer, const char* p_sdp)
{
    if ((NULL == p_peer) || (NULL == p_sdp))
    {
        return KLB_NETCODE_INVAL;
    }

    klb_webrtc_sdp_t info;
    int ret = klb_webrtc_sdp_parse(p_sdp, &info);
    if (0 > ret)
    {
        return ret;
    }

    if (('\0' == info.ufrag[0]) || ('\0' == info.pwd[0]))
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    adopt_media_klb_webrtc(p_peer, &info);

    ret = klb_ice_agent_set_remote_credentials(p_peer->p_ice, info.ufrag, info.pwd);
    if (0 > ret)
    {
        return ret;
    }

    if ('\0' != info.fingerprint[0])
    {
        copy_text_klb_webrtc(p_peer->remote_fp, (int)sizeof(p_peer->remote_fp),
            info.fingerprint, (int)strlen(info.fingerprint));
    }

    if ('\0' != info.setup[0])
    {
        copy_text_klb_webrtc(p_peer->remote_setup, (int)sizeof(p_peer->remote_setup),
            info.setup, (int)strlen(info.setup));
    }

    for (int i = 0; i < info.candidate_num; i++)
    {
        ret = klb_ice_agent_add_remote_candidate(p_peer->p_ice, info.candidates[i]);
        if ((0 > ret) && (KLB_NETCODE_LIMIT != ret))
        {
            return ret;
        }
    }

    ret = prepare_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    ret = klb_ice_agent_start(p_peer->p_ice);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->started = true;
    return 0;
}


int klb_webrtc_peer_add_ice_candidate(klb_webrtc_peer_t* p_peer, const char* p_candidate)
{
    if ((NULL == p_peer) || (NULL == p_candidate))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = klb_ice_agent_add_remote_candidate(p_peer->p_ice, p_candidate);
    if (0 > ret)
    {
        return ret;
    }

    if (p_peer->started)
    {
        ret = klb_ice_agent_start(p_peer->p_ice);
        if (0 > ret)
        {
            return ret;
        }
    }

    return 0;
}


int klb_webrtc_peer_state(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return KLB_NETCODE_INVAL;
    }

    return klb_ice_agent_get_state(p_peer->p_ice);
}


const char* klb_webrtc_state_name(int state)
{
    switch (state)
    {
    case KLB_ICE_STATE_NEW:
        return "new";
    case KLB_ICE_STATE_GATHERING:
        return "gathering";
    case KLB_ICE_STATE_CHECKING:
        return "checking";
    case KLB_ICE_STATE_CONNECTED:
        return "connected";
    case KLB_ICE_STATE_FAILED:
        return "failed";
    case KLB_ICE_STATE_CLOSED:
        return "closed";
    default:
        break;
    }

    return "unknown";
}


int klb_webrtc_peer_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    if ((NULL == p_peer) || (NULL == p_data) || (0 >= len))
    {
        return KLB_NETCODE_INVAL;
    }

    if (NULL == p_peer->p_cert_pem)
    {
        return klb_ice_agent_send(p_peer->p_ice, p_data, len);
    }

    int ret = pump_dtls_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    if (1 == ret)
    {
        return 0;               // DTLS 握手进行中, 暂不发送
    }

    // 版本位 10 视为 RTP/RTCP: 经 SRTP 保护后直接走 ICE
    if ((0x80 == (p_data[0] & 0xC0)) && (NULL != p_peer->p_srtp_tx))
    {
        ret = send_rtp_klb_webrtc(p_peer, p_data, len, 0, false);
        if (0 == ret)
        {
            p_peer->rtp_tx_pkts++;
        }

        return ret;
    }

    ret = klb_socket_send(p_peer->p_dtls, p_data, len);
    if (0 < ret)
    {
        p_peer->dtls_tx_pkts++;
        return 0;
    }

    return ret;
}


int klb_webrtc_peer_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code)
{
    if ((NULL == p_peer) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    collect_events_klb_webrtc(p_peer);
    rtcp_periodic_klb_webrtc(p_peer);

    for (int guard = 0; guard < 32; guard++)
    {
        int n = queue_pop_klb_webrtc(&p_peer->q_app, p_buf, cap);
        if (0 < n)
        {
            return n;
        }

        if (NULL != p_peer->p_dtls)
        {
            int dn = klb_socket_recv(p_peer->p_dtls, p_buf, cap);
            if (0 < dn)
            {
                p_peer->dtls_rx_pkts++;
                return dn;
            }

            if (0 > dn)
            {
                if (NULL != p_code)
                {
                    *p_code = KLB_NETCODE_IO;
                }

                return dn;
            }
        }

        int got = pull_klb_webrtc(p_peer);
        if (0 > got)
        {
            if (NULL != p_code)
            {
                *p_code = got;
            }

            return got;
        }

        if (0 == got)
        {
            return 0;
        }

        if (NULL != p_peer->p_cert_pem)
        {
            int hs = pump_dtls_klb_webrtc(p_peer);
            if (0 > hs)
            {
                if (NULL != p_code)
                {
                    *p_code = hs;
                }

                return hs;
            }
        }
    }

    return 0;
}


int klb_webrtc_peer_rtp_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    if ((NULL == p_peer) || (NULL == p_data) || (0 >= len))
    {
        return KLB_NETCODE_INVAL;
    }

    if (KLB_WEBRTC_PKT_MAX < len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    klb_rtp_head_t head;
    if (0 > klb_rtp_head_parse(p_data, len, &head, NULL))
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    ret = send_rtp_klb_webrtc(p_peer, p_data, len, head.sequence, true);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->rtp_tx_pkts++;
    p_peer->rtp_ts_tx = head.timestamp;
    p_peer->rtp_tx_bytes += (uint64_t)len;

    return 0;
}


int klb_webrtc_peer_h264_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_nalu,
    int len, uint32_t timestamp, int marker)
{
    if ((NULL == p_peer) || (NULL == p_nalu) || (0 >= len))
    {
        return KLB_NETCODE_INVAL;
    }

    if (KLB_WEBRTC_NALU_MAX < len)
    {
        return KLB_NETCODE_MSGSIZE;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    // 打包后长度估算: 每片附加 12 字节 RTP 头 + 2 字节 FU 头
    int chunk = KLB_WEBRTC_RTP_MTU - 12 - 2;
    int pkts = (len > chunk) ? ((len + chunk - 1) / chunk) : 1;
    int cap = len + 14 * pkts + 64;

    uint8_t* p_out = KLB_MALLOC(uint8_t, cap, 0);
    if (NULL == p_out)
    {
        return KLB_NETCODE_NOMEM;
    }

    int total = 0;
    int count = klb_rtp_h264_packetize(p_nalu, len, KLB_WEBRTC_RTP_MTU, marker,
        p_peer->rtp_seq_tx, timestamp, p_peer->rtp_ssrc_tx,
        p_out, cap, &total);
    if (0 >= count)
    {
        KLB_FREE(p_out);

        return (0 > count) ? count : KLB_NETCODE_ERR;
    }

    int pos = 0;
    int sent = 0;

    for (int i = 0; i < count; i++)
    {
        int pkt_len = total - pos;
        if (KLB_WEBRTC_RTP_MTU < pkt_len)
        {
            pkt_len = KLB_WEBRTC_RTP_MTU;
        }

        uint16_t seq = (uint16_t)(p_peer->rtp_seq_tx + i);

        ret = send_rtp_klb_webrtc(p_peer, p_out + pos, pkt_len, seq, true);
        if (0 > ret)
        {
            break;
        }

        p_peer->rtp_tx_pkts++;
        sent++;
        pos += pkt_len;
    }

    p_peer->rtp_seq_tx = (uint16_t)(p_peer->rtp_seq_tx + sent);
    if (0 < sent)
    {
        p_peer->rtp_ts_tx = timestamp;
        p_peer->rtp_tx_bytes += (uint64_t)len;
    }

    KLB_FREE(p_out);

    return (0 > ret) ? ret : 0;
}


int klb_webrtc_peer_rtp_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code)
{
    if ((NULL == p_peer) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    if (NULL == p_peer->p_cert_pem)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_NOTCONN;
        }

        return KLB_NETCODE_NOTCONN;
    }

    int ret = pump_dtls_klb_webrtc(p_peer);
    if (0 > ret)
    {
        if (NULL != p_code)
        {
            *p_code = ret;
        }

        return ret;
    }

    return media_pull_klb_webrtc(p_peer, p_buf, cap);
}


int klb_webrtc_peer_h264_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code)
{
    if ((NULL == p_peer) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    // 优先返还 STAP-A 队列中尚未取出的 NALU
    int stap_n = pull_stap_klb_webrtc(p_peer, p_buf, cap);
    if (0 != stap_n)
    {
        if ((0 > stap_n) && (NULL != p_code))
        {
            *p_code = stap_n;
        }

        return stap_n;
    }

    for (int guard = 0; guard < 32; guard++)
    {
        uint8_t pkt[KLB_WEBRTC_PKT_MAX];

        int ret = klb_webrtc_peer_rtp_recv(p_peer, pkt, (int)sizeof(pkt), p_code);
        if (0 > ret)
        {
            return ret;
        }

        if (0 == ret)
        {
            return 0;
        }

        klb_rtp_head_t head;
        int head_len = 0;

        if (0 > klb_rtp_head_parse(pkt, ret, &head, &head_len))
        {
            continue;
        }

        if ((0 >= head_len) || (head_len >= ret))
        {
            continue;
        }

        const uint8_t* p_payload = pkt + head_len;
        int plen = ret - head_len;
        uint8_t nal_type = (uint8_t)(p_payload[0] & 0x1F);

        // 单 NALU 包
        if ((1 <= nal_type) && (23 >= nal_type))
        {
            reset_nalu_klb_webrtc(p_peer);

            if (plen > cap)
            {
                if (NULL != p_code)
                {
                    *p_code = KLB_NETCODE_MSGSIZE;
                }

                return KLB_NETCODE_MSGSIZE;
            }

            memcpy(p_buf, p_payload, (size_t)plen);

            return plen;
        }

        // FU-A 分片
        if (28 == nal_type)
        {
            if (2 > plen)
            {
                continue;
            }

            uint8_t fu = p_payload[1];
            bool start = (0 != (fu & 0x80)) ? true : false;
            bool end = (0 != (fu & 0x40)) ? true : false;

            if (start)
            {
                reset_nalu_klb_webrtc(p_peer);

                uint8_t nal_head = (uint8_t)((p_payload[0] & 0xE0) | (fu & 0x1F));
                if (0 > append_nalu_klb_webrtc(p_peer, &nal_head, 1))
                {
                    p_peer->nalu_broken = true;
                    continue;
                }
            }
            else if (p_peer->nalu_broken || (0 == p_peer->nalu_len))
            {
                // 分片链中断后的非起始片: 丢弃至下一个起始片
                p_peer->nalu_broken = true;
                continue;
            }

            if (0 > append_nalu_klb_webrtc(p_peer, p_payload + 2, plen - 2))
            {
                reset_nalu_klb_webrtc(p_peer);
                p_peer->nalu_broken = true;
                continue;
            }

            if (!end)
            {
                continue;
            }

            int nalu_len = p_peer->nalu_len;
            if (nalu_len > cap)
            {
                reset_nalu_klb_webrtc(p_peer);

                if (NULL != p_code)
                {
                    *p_code = KLB_NETCODE_MSGSIZE;
                }

                return KLB_NETCODE_MSGSIZE;
            }

            memcpy(p_buf, p_peer->p_nalu, (size_t)nalu_len);
            reset_nalu_klb_webrtc(p_peer);

            return nalu_len;
        }

        // STAP-A: 拆分全部子 NALU 入队, 逐个返还
        if (24 == nal_type)
        {
            if (0 > store_stap_klb_webrtc(p_peer, p_payload, plen))
            {
                continue;
            }

            int n = pull_stap_klb_webrtc(p_peer, p_buf, cap);
            if (0 < n)
            {
                return n;
            }

            if (0 > n)
            {
                if (NULL != p_code)
                {
                    *p_code = n;
                }

                return n;
            }
        }
    }

    return 0;
}


int klb_webrtc_peer_rtcp_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len)
{
    if ((NULL == p_peer) || (NULL == p_data) || (0 >= len))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    return send_rtcp_klb_webrtc(p_peer, p_data, len);
}


int klb_webrtc_peer_nack_send(klb_webrtc_peer_t* p_peer, const uint16_t* p_seqs, int num)
{
    if ((NULL == p_peer) || (NULL == p_seqs) || (0 >= num) || (KLB_WEBRTC_NACK_MAX < num))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    uint8_t pkt[256];

    ret = klb_rtcp_nack_build(p_peer->rtp_ssrc_tx, p_peer->rtp_ssrc_rx,
        p_seqs, num, pkt, (int)sizeof(pkt));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    int pkt_len = ret;

    ret = send_rtcp_klb_webrtc(p_peer, pkt, pkt_len);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->nack_tx_pkts++;

    return 0;
}


int klb_webrtc_peer_add_media(klb_webrtc_peer_t* p_peer, int kind,
    const klb_webrtc_codec_t* p_codecs, int num)
{
    if ((NULL == p_peer) || (NULL == p_codecs) || (0 >= num)
        || (KLB_WEBRTC_CODEC_MAX < num) || (KLB_WEBRTC_MLINE_MAX <= p_peer->media_num))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((KLB_WEBRTC_KIND_VIDEO != kind) && (KLB_WEBRTC_KIND_AUDIO != kind))
    {
        return KLB_NETCODE_INVAL;
    }

    for (int i = 0; i < num; i++)
    {
        if (('\0' == p_codecs[i].name[0]) || (0 > p_codecs[i].pt))
        {
            return KLB_NETCODE_INVAL;
        }
    }

    klb_webrtc_media_cfg_t* p_cfg = &p_peer->media[p_peer->media_num];
    KLB_MEMSET(p_cfg, 0, sizeof(*p_cfg));
    p_cfg->kind = kind;

    for (int i = 0; i < num; i++)
    {
        p_cfg->codecs[i] = p_codecs[i];
    }

    p_cfg->codec_num = num;
    p_peer->media_num++;

    return 0;
}


uint32_t klb_webrtc_rtcp_sr_send(klb_webrtc_peer_t* p_peer, int* p_code)
{
    if (NULL != p_code)
    {
        *p_code = 0;
    }

    if (NULL == p_peer)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return 0;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        if (NULL != p_code)
        {
            *p_code = ret;
        }

        return 0;
    }

    int64_t gmt = klb_gmt_time_ms();
    uint32_t ntp_sec = (uint32_t)(gmt / 1000) + 2208988800u;
    uint32_t ntp_frac = (uint32_t)(((uint64_t)(gmt % 1000) << 32) / 1000u);

    klb_rtcp_report_block_t block;
    int block_num = build_block_klb_webrtc(p_peer, &block);

    uint8_t pkt[512];

    int n = klb_rtcp_sr_build(p_peer->rtp_ssrc_tx, ntp_sec, ntp_frac, p_peer->rtp_ts_tx,
        (uint32_t)p_peer->rtp_tx_pkts, (uint32_t)p_peer->rtp_tx_bytes,
        (0 < block_num) ? &block : NULL, block_num, pkt, (int)sizeof(pkt));
    if (0 >= n)
    {
        if (NULL != p_code)
        {
            *p_code = (0 > n) ? n : KLB_NETCODE_ERR;
        }

        return 0;
    }

    ret = send_rtcp_klb_webrtc(p_peer, pkt, n);
    if (0 > ret)
    {
        if (NULL != p_code)
        {
            *p_code = ret;
        }

        return 0;
    }

    p_peer->report_tx_pkts++;

    return (uint32_t)(((ntp_sec & 0xFFFFu) << 16) | (ntp_frac >> 16));
}


int klb_webrtc_rtcp_rr_send(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    klb_rtcp_report_block_t block;
    int block_num = build_block_klb_webrtc(p_peer, &block);

    uint8_t pkt[256];

    ret = klb_rtcp_rr_build(p_peer->rtp_ssrc_tx, (0 < block_num) ? &block : NULL,
        block_num, pkt, (int)sizeof(pkt));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    int pkt_len = ret;

    ret = send_rtcp_klb_webrtc(p_peer, pkt, pkt_len);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->report_tx_pkts++;

    return 0;
}


int klb_webrtc_rtcp_twcc_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_status,
                              int count, uint16_t base_seq, uint32_t ref_time)
{
    if ((NULL == p_peer) || (NULL == p_status) || (0 >= count)
        || (KLB_WEBRTC_TWCC_MAX < count))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    uint8_t pkt[512];

    ret = klb_rtcp_twcc_build(p_peer->rtp_ssrc_tx, p_peer->rtp_ssrc_rx, base_seq,
        p_status, NULL, count, ref_time, p_peer->twcc_fb_count, pkt, (int)sizeof(pkt));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    int pkt_len = ret;

    ret = send_rtcp_klb_webrtc(p_peer, pkt, pkt_len);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->twcc_fb_count++;
    p_peer->twcc_tx_pkts++;

    return 0;
}


int klb_webrtc_rtcp_twcc_flush(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return KLB_NETCODE_INVAL;
    }

    if (0 >= p_peer->twcc_count)
    {
        return 0;
    }

    int ret = ensure_media_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    int count = p_peer->twcc_count;
    uint8_t delta[KLB_WEBRTC_TWCC_MAX];
    KLB_MEMSET(delta, 0, sizeof(delta));

    uint32_t ref_ms = 0;
    for (int i = 0; i < count; i++)
    {
        if (0 != p_peer->twcc_arrive[i])
        {
            ref_ms = p_peer->twcc_arrive[i];
            break;
        }
    }

    // 逐已收包计算到达增量: 单位 0.25ms, 超 255 截断
    uint32_t prev = ref_ms;
    for (int i = 0; i < count; i++)
    {
        if ((0 == p_peer->twcc_status[i]) || (0 == p_peer->twcc_arrive[i]))
        {
            continue;
        }

        uint32_t d = p_peer->twcc_arrive[i] - prev;
        uint32_t q = d * 4u;
        delta[i] = (uint8_t)((255u < q) ? 255u : q);
        prev = p_peer->twcc_arrive[i];
    }

    uint8_t pkt[512];
    uint32_t ref_time = (ref_ms / 64u) & 0x00FFFFFFu;

    ret = klb_rtcp_twcc_build(p_peer->rtp_ssrc_tx, p_peer->rtp_ssrc_rx, p_peer->twcc_base,
        p_peer->twcc_status, delta, count, ref_time, p_peer->twcc_fb_count, pkt, (int)sizeof(pkt));
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_ERR;
    }

    int pkt_len = ret;

    ret = send_rtcp_klb_webrtc(p_peer, pkt, pkt_len);
    if (0 > ret)
    {
        return ret;
    }

    p_peer->twcc_fb_count++;
    p_peer->twcc_tx_pkts++;
    p_peer->twcc_count = 0;

    return 0;
}


static void rtcp_periodic_klb_webrtc(klb_webrtc_peer_t* p_peer)
{
    if ((NULL == p_peer->p_srtp_tx) && (NULL == p_peer->p_dtls))
    {
        return;
    }

    uint64_t now = klb_tick_count64();

    // TWCC 反馈自动冲刷: 窗口达到阈值即回送, 无需脚本显式调用
    if (KLB_WEBRTC_TWCC_FLUSH_PKTS <= p_peer->twcc_count)
    {
        klb_webrtc_rtcp_twcc_flush(p_peer);
    }

    if (0 == p_peer->rtcp_last_report_ms)
    {
        p_peer->rtcp_last_report_ms = now;
        return;
    }

    if (KLB_WEBRTC_REPORT_MS > (now - p_peer->rtcp_last_report_ms))
    {
        return;
    }

    p_peer->rtcp_last_report_ms = now;

    if (0 < p_peer->rtp_tx_pkts)
    {
        int code = 0;
        klb_webrtc_rtcp_sr_send(p_peer, &code);      // SR 同时携带接收报告块
    }
    else if (0 < p_peer->rtp_rx_pkts)
    {
        klb_webrtc_rtcp_rr_send(p_peer);
    }
}


int klb_webrtc_peer_event_poll(klb_webrtc_peer_t* p_peer, klb_webrtc_event_t* p_event)
{
    if ((NULL == p_peer) || (NULL == p_event))
    {
        return KLB_NETCODE_INVAL;
    }

    collect_events_klb_webrtc(p_peer);

    if (0 >= p_peer->evt_count)
    {
        return 0;
    }

    *p_event = p_peer->evt[p_peer->evt_head];
    p_peer->evt_head = (p_peer->evt_head + 1) % KLB_WEBRTC_EVT_SLOTS;
    p_peer->evt_count--;

    return 1;
}


int klb_webrtc_peer_stats(klb_webrtc_peer_t* p_peer, klb_webrtc_stats_t* p_stats)
{
    if ((NULL == p_peer) || (NULL == p_stats))
    {
        return KLB_NETCODE_INVAL;
    }

    KLB_MEMSET(p_stats, 0, sizeof(*p_stats));

    klb_ice_stats_t ice;
    int ret = klb_ice_agent_stats(p_peer->p_ice, &ice);
    if (0 > ret)
    {
        return ret;
    }

    p_stats->state = ice.state;
    p_stats->has_selected = ice.has_selected;
    p_stats->local_count = ice.local_count;
    p_stats->remote_count = ice.remote_count;
    p_stats->pair_count = ice.pair_count;
    p_stats->dtls_ready = p_peer->dtls_done ? 1 : 0;
    p_stats->srtp_ready = ((NULL != p_peer->p_srtp_tx) && (NULL != p_peer->p_srtp_rx)) ? 1 : 0;

    copy_text_klb_webrtc(p_stats->local_ip, (int)sizeof(p_stats->local_ip),
        ice.local_ip, (int)strlen(ice.local_ip));
    p_stats->local_port = ice.local_port;
    copy_text_klb_webrtc(p_stats->remote_ip, (int)sizeof(p_stats->remote_ip),
        ice.remote_ip, (int)strlen(ice.remote_ip));
    p_stats->remote_port = ice.remote_port;

    p_stats->check_sent = ice.check_sent;
    p_stats->check_recv = ice.check_recv;
    p_stats->data_sent_pkts = ice.data_sent_pkts;
    p_stats->data_sent_bytes = ice.data_sent_bytes;
    p_stats->data_recv_pkts = ice.data_recv_pkts;
    p_stats->data_recv_bytes = ice.data_recv_bytes;
    p_stats->srtp_tx_pkts = p_peer->srtp_tx_pkts;
    p_stats->srtp_rx_pkts = p_peer->srtp_rx_pkts;
    p_stats->dtls_tx_pkts = p_peer->dtls_tx_pkts;
    p_stats->dtls_rx_pkts = p_peer->dtls_rx_pkts;

    p_stats->rtp_ssrc_tx = p_peer->rtp_ssrc_tx;
    p_stats->rtp_ssrc_rx = p_peer->rtp_ssrc_rx;
    p_stats->rtp_tx_pkts = p_peer->rtp_tx_pkts;
    p_stats->rtp_rx_pkts = p_peer->rtp_rx_pkts;
    p_stats->rtcp_tx_pkts = p_peer->rtcp_tx_pkts;
    p_stats->rtcp_rx_pkts = p_peer->rtcp_rx_pkts;
    p_stats->nack_tx_pkts = p_peer->nack_tx_pkts;
    p_stats->nack_rx_pkts = p_peer->nack_rx_pkts;
    p_stats->retrans_pkts = p_peer->retrans_pkts;
    p_stats->rtp_lost_pkts = p_peer->rtp_lost_pkts;
    p_stats->jb_pending = (uint64_t)klb_webrtc_jb_pending(p_peer->p_jb);

    klb_webrtc_jb_stats_t jb;
    if (0 == klb_webrtc_jb_stats(p_peer->p_jb, &jb))
    {
        p_stats->jb_dup_pkts = jb.dup_pkts;
        p_stats->jb_late_pkts = jb.late_pkts;
        p_stats->jb_overflow_pkts = jb.overflow_pkts;
        p_stats->jb_reorder_pkts = jb.reorder_pkts;
    }

    p_stats->rtp_rx_expected = p_peer->rr_expected;
    p_stats->rtp_rx_lost = (p_peer->rtp_rx_pkts < p_peer->rr_expected)
        ? (p_peer->rr_expected - p_peer->rtp_rx_pkts) : 0;
    p_stats->rr_jitter = p_peer->rr_jitter;
    p_stats->rr_lsr = p_peer->rr_lsr;
    if (0 != p_peer->rr_lsr)
    {
        uint64_t d = klb_tick_count64() - p_peer->rr_last_sr_ms;
        p_stats->rr_dlsr = (uint32_t)((d * 65536u) / 1000u);
    }

    p_stats->twcc_tx_pkts = p_peer->twcc_tx_pkts;
    p_stats->twcc_rx_pkts = p_peer->twcc_rx_pkts;
    p_stats->report_tx_pkts = p_peer->report_tx_pkts;
    p_stats->report_rx_pkts = p_peer->report_rx_pkts;
    p_stats->target_bps = p_peer->target_bps;
    p_stats->twcc_loss_permille = p_peer->twcc_loss_permille;
    p_stats->twcc_fb_rounds = p_peer->twcc_fb_rounds;

    return 0;
}


int klb_webrtc_peer_dc_open(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = pump_dtls_klb_webrtc(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    if (1 == ret)
    {
        return KLB_NETCODE_NOTCONN;
    }

    if (NULL == p_peer->p_dc)
    {
        // DTLS 客户端 (answer) 即 SCTP 客户端
        bool is_client = (KLB_WEBRTC_ROLE_ANSWER == p_peer->role) ? true : false;
        int code = 0;

        p_peer->p_dc = klb_webrtc_dc_create(is_client, &code);
        if (NULL == p_peer->p_dc)
        {
            return (0 != code) ? code : KLB_NETCODE_ERR;
        }

        ret = klb_webrtc_dc_open(p_peer->p_dc);
        if (0 > ret)
        {
            return ret;
        }
    }

    return flush_dc_klb_webrtc(p_peer);
}


int klb_webrtc_peer_dc_is_open(klb_webrtc_peer_t* p_peer)
{
    if (NULL == p_peer)
    {
        return 0;
    }

    return klb_webrtc_dc_is_open(p_peer->p_dc);
}


int klb_webrtc_peer_dc_is_open_stream(klb_webrtc_peer_t* p_peer, int stream_id)
{
    if (NULL == p_peer)
    {
        return 0;
    }

    return klb_webrtc_dc_is_open_stream(p_peer->p_dc, stream_id);
}


int klb_webrtc_peer_dc_open_stream(klb_webrtc_peer_t* p_peer, int stream_id)
{
    if ((NULL == p_peer) || (0 > stream_id))
    {
        return KLB_NETCODE_INVAL;
    }

    int ret = klb_webrtc_peer_dc_open(p_peer);
    if (0 > ret)
    {
        return ret;
    }

    if (NULL == p_peer->p_dc)
    {
        return KLB_NETCODE_NOTCONN;
    }

    ret = klb_webrtc_dc_open_stream(p_peer->p_dc, stream_id);
    if (0 > ret)
    {
        return ret;
    }

    return flush_dc_klb_webrtc(p_peer);
}


int klb_webrtc_peer_dc_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len, int is_binary)
{
    return klb_webrtc_peer_dc_send_stream(p_peer, 0, p_data, len, is_binary);
}


int klb_webrtc_peer_dc_send_stream(klb_webrtc_peer_t* p_peer, int stream_id,
                                   const uint8_t* p_data, int len, int is_binary)
{
    if ((NULL == p_peer) || (NULL == p_data) || (0 >= len))
    {
        return KLB_NETCODE_INVAL;
    }

    if ((NULL == p_peer->p_dc) || (NULL == p_peer->p_dtls))
    {
        return KLB_NETCODE_NOTCONN;
    }

    int ppid = (0 != is_binary) ? KLB_WEBRTC_DC_PPID_BINARY : KLB_WEBRTC_DC_PPID_STRING;
    uint8_t pkt[KLB_WEBRTC_PKT_MAX];
    int out_len = 0;

    int ret = klb_webrtc_dc_output_stream(p_peer->p_dc, stream_id, p_data, len, ppid,
        pkt, (int)sizeof(pkt), &out_len);
    if (0 > ret)
    {
        return ret;
    }

    ret = klb_socket_send(p_peer->p_dtls, pkt, out_len);
    if (0 >= ret)
    {
        return (0 > ret) ? ret : KLB_NETCODE_AGAIN;
    }

    p_peer->dtls_tx_pkts++;

    // 分片余片经队列泵送出
    return flush_dc_klb_webrtc(p_peer);
}


/// @brief 接收 DataChannel 应用消息 (可带出流号)
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_stream_id   消息所属流号; 可为 NULL
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int >0.字节数; 0.无消息; 小于 0.klb_netcode_e
static int recv_dc_klb_webrtc(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap,
                              int* p_stream_id, int* p_code)
{
    if ((NULL == p_peer) || (NULL == p_buf) || (0 >= cap))
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return KLB_NETCODE_INVAL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    if (NULL == p_peer->p_dc)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_NOTCONN;
        }

        return KLB_NETCODE_NOTCONN;
    }

    for (int guard = 0; guard < 32; guard++)
    {
        if (NULL != p_peer->p_dtls)
        {
            uint8_t pkt[KLB_WEBRTC_PKT_MAX];
            int dn = klb_socket_recv(p_peer->p_dtls, pkt, (int)sizeof(pkt));

            if (0 < dn)
            {
                p_peer->dtls_rx_pkts++;

                int msg_len = 0;
                int ir = klb_webrtc_dc_input_ex(p_peer->p_dc, pkt, dn, p_buf, cap, &msg_len,
                    p_stream_id);
                if (0 > ir)
                {
                    if (NULL != p_code)
                    {
                        *p_code = ir;
                    }

                    return ir;
                }

                int fr = flush_dc_klb_webrtc(p_peer);
                if (0 > fr)
                {
                    if (NULL != p_code)
                    {
                        *p_code = fr;
                    }

                    return fr;
                }

                if (0 < msg_len)
                {
                    return msg_len;
                }

                continue;
            }

            if (0 > dn)
            {
                if (NULL != p_code)
                {
                    *p_code = KLB_NETCODE_IO;
                }

                return dn;
            }
        }

        int got = pull_klb_webrtc(p_peer);
        if (0 > got)
        {
            if (NULL != p_code)
            {
                *p_code = got;
            }

            return got;
        }

        if (0 == got)
        {
            return 0;
        }

        if (NULL != p_peer->p_cert_pem)
        {
            int hs = pump_dtls_klb_webrtc(p_peer);
            if (0 > hs)
            {
                if (NULL != p_code)
                {
                    *p_code = hs;
                }

                return hs;
            }
        }
    }

    return 0;
}


int klb_webrtc_peer_dc_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code)
{
    return recv_dc_klb_webrtc(p_peer, p_buf, cap, NULL, p_code);
}


int klb_webrtc_peer_dc_recv_ex(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap,
                               int* p_stream_id, int* p_code)
{
    return recv_dc_klb_webrtc(p_peer, p_buf, cap, p_stream_id, p_code);
}


// end
