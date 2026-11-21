// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_webrtc.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WebRTC PeerConnection 会话 (SDP + ICE + DTLS + SRTP + DataChannel)
/// @version 1.1
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2-1.0 接口随实现落地: 会话创建 / ICE / SDP / DTLS-SRTP / DataChannel / 媒体面收发 / 事件队列 / 统计; SDP 多 m-line 协商与 rtpmap 频率/声道; TWCC 冲刷与目标码率/丢包率统计
/// @warning DTLS 未配置证书时 send/recv 走裸 ICE 应用数据通道
///  \n DataChannel 为单流单通道, 不支持分片/SACK/重传
///  \n 媒体面 (rtp_*/h264_*/rtcp_*) 须 DTLS-SRTP 就绪; 单 SSRC; 抖动缓冲 64 槽
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WEBRTC_H__
#define __KLB_WEBRTC_H__

#include "klb_type.h"
#include "klbnet/klb_netmulti.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_webrtc_role_e
/// @brief 会话角色
typedef enum klb_webrtc_role_e_
{
    KLB_WEBRTC_ROLE_OFFER  = 0,         ///< 发起方 (offer)
    KLB_WEBRTC_ROLE_ANSWER = 1,         ///< 应答方 (answer)
}klb_webrtc_role_e;


/// @brief PeerConnection (不透明)
typedef struct klb_webrtc_peer_t_ klb_webrtc_peer_t;


/// @brief 创建 PeerConnection
/// @param [in] *p_netmulti     复用模块; 不可为 NULL
/// @param [in] role            角色: klb_webrtc_role_e
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_webrtc_peer_t* NULL.失败 (桩恒 NULL); 非 NULL.会话
KLB_API klb_webrtc_peer_t* klb_webrtc_peer_create(klb_netmulti_t* p_netmulti, int role, int* p_code);


/// @brief 销毁 PeerConnection
/// @param [in] *p_peer         会话; 可为 NULL
/// @return 无
KLB_API void klb_webrtc_peer_destroy(klb_webrtc_peer_t* p_peer);


/// @brief 生成 offer SDP
/// @param [in]  *p_peer        会话
/// @param [out] *p_sdp         输出缓存
/// @param [in]  cap            缓存大小
/// @return int >0.SDP 长度; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_create_offer(klb_webrtc_peer_t* p_peer, char* p_sdp, int cap);


/// @brief 生成 answer SDP
/// @param [in]  *p_peer        会话
/// @param [in]  *p_remote_sdp  远端 offer
/// @param [out] *p_sdp         输出缓存
/// @param [in]  cap            缓存大小
/// @return int >0.SDP 长度; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_create_answer(klb_webrtc_peer_t* p_peer, const char* p_remote_sdp,
                                          char* p_sdp, int cap);


/// @brief 设置远端 SDP (offer 或 answer)
/// @param [in] *p_peer         会话
/// @param [in] *p_sdp          远端 SDP
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_set_remote_sdp(klb_webrtc_peer_t* p_peer, const char* p_sdp);


/// @brief 添加远端 ICE 候选
/// @param [in] *p_peer         会话
/// @param [in] *p_candidate    候选串
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_add_ice_candidate(klb_webrtc_peer_t* p_peer, const char* p_candidate);


/// @brief 取 ICE 连通状态
/// @param [in] *p_peer         会话
/// @return int klb_ice_state_e; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_state(klb_webrtc_peer_t* p_peer);


/// @brief 取状态名
/// @param [in] state           klb_ice_state_e
/// @return const char* 静态串; 未知为 "unknown"
KLB_API const char* klb_webrtc_state_name(int state);


/// @brief 经已建立的媒体通道发送数据
/// @param [in] *p_peer         会话
/// @param [in] *p_data         数据
/// @param [in] len             长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len);


/// @brief 接收媒体数据
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int >0.字节数; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code);


/// @brief 经 SRTP 发送一个已构建的 RTP 包 (UDP 载荷上限 1200 字节)
/// @param [in] *p_peer         会话
/// @param [in] *p_data         RTP 包 (含 12 字节固定头)
/// @param [in] len             长度
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 发送成功后进入 32 包重传缓存, 供对端 NACK 触发重发
KLB_API int klb_webrtc_peer_rtp_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len);


/// @brief 发送一个 H.264 NALU (自动单包 / FU-A 分片, 自增序号)
/// @param [in] *p_peer         会话
/// @param [in] *p_nalu         NALU; 允许带 Annex-B 起始码
/// @param [in] len             NALU 长度
/// @param [in] timestamp       RTP 时间戳 (90 kHz)
/// @param [in] marker          标记位; 非 0 时最后一包置 M
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_h264_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_nalu,
    int len, uint32_t timestamp, int marker);


/// @brief 接收一个有序 RTP 包 (经抖动缓冲重排序)
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存 (完整 RTP 包)
/// @param [in]  cap            缓存大小
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int 大于 0.字节数; 0.暂无; 小于 0.klb_netcode_e
/// @note 出现缺口时按 KLB_WEBRTC_NACK_MAX 上限自动发 NACK; 缺口持续超时后跳过并计入丢包
KLB_API int klb_webrtc_peer_rtp_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code);


/// @brief 接收一个 H.264 NALU (FU-A 自动重组)
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存 (NALU; 不含起始码)
/// @param [in]  cap            缓存大小
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int 大于 0.NALU 字节数; 0.暂无完整 NALU; 小于 0.klb_netcode_e
/// @note STAP-A 拆分为多个 NALU 逐个返回; 分片丢失时丢弃整个 NALU
KLB_API int klb_webrtc_peer_h264_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf, int cap, int* p_code);


/// @brief 经 SRTCP 发送一个已构建的 RTCP 包 (SR/RR/NACK/PLI 等)
/// @param [in] *p_peer         会话
/// @param [in] *p_data         RTCP 包
/// @param [in] len             长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_rtcp_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data, int len);


/// @brief 发送 RTCP NACK 请求重传
/// @param [in] *p_peer         会话
/// @param [in] *p_seqs         丢失序号数组
/// @param [in] num             序号个数 (1 ~ KLB_WEBRTC_NACK_MAX)
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_nack_send(klb_webrtc_peer_t* p_peer, const uint16_t* p_seqs, int num);


/// @def   KLB_WEBRTC_MLINE_MAX
/// @brief 单会话可协商的媒体 m-line 上限 (不含 DataChannel)
#define KLB_WEBRTC_MLINE_MAX            4

/// @def   KLB_WEBRTC_CODEC_MAX
/// @brief 单条 m-line 可声明的编码上限
#define KLB_WEBRTC_CODEC_MAX            4

/// @def   KLB_WEBRTC_PT_MAX
/// @brief 单条 m-line 可解析的载荷类型上限
#define KLB_WEBRTC_PT_MAX               8

/// @def   KLB_WEBRTC_CODEC_NAME
/// @brief 编码名文本缓冲长度
#define KLB_WEBRTC_CODEC_NAME           24

/// @def   KLB_WEBRTC_FMTP_MAX
/// @brief a=fmtp 参数文本缓冲长度
#define KLB_WEBRTC_FMTP_MAX             128


/// @enum  klb_webrtc_kind_e
/// @brief 媒体类型
typedef enum klb_webrtc_kind_e_
{
    KLB_WEBRTC_KIND_VIDEO   = 0,        ///< 视频
    KLB_WEBRTC_KIND_AUDIO   = 1,        ///< 音频
}klb_webrtc_kind_e;


/// @enum  klb_webrtc_fb_e
/// @brief RTCP 反馈能力位 (a=rtcp-fb)
typedef enum klb_webrtc_fb_e_
{
    KLB_WEBRTC_FB_NACK      = 0x01,     ///< generic NACK
    KLB_WEBRTC_FB_PLI       = 0x02,     ///< 图像丢包指示
    KLB_WEBRTC_FB_FIR       = 0x04,     ///< 全帧刷新
    KLB_WEBRTC_FB_REMB      = 0x08,     ///< 接收端估计最大码率
    KLB_WEBRTC_FB_TWCC      = 0x10,     ///< 传输层拥塞控制反馈
}klb_webrtc_fb_e;


/// @struct klb_webrtc_codec_t
/// @brief  单个编码声明
typedef struct klb_webrtc_codec_t_
{
    int     pt;                         ///< 载荷类型
    char    name[KLB_WEBRTC_CODEC_NAME];    ///< 编码名, 如 "H264" / "opus"
    int     clock_rate;                 ///< 时钟频率; 0 时视频取 90000 / 音频取 48000
    int     channels;                   ///< 声道数; 仅音频, 0 表 1
    char    fmtp[KLB_WEBRTC_FMTP_MAX];  ///< a=fmtp 参数串; 空串表未声明
    int     fb_mask;                    ///< klb_webrtc_fb_e 位掩码; 0 表未声明
}klb_webrtc_codec_t;


/// @struct klb_webrtc_mline_t
/// @brief  SDP 中的一条媒体 m-line (解析结果)
typedef struct klb_webrtc_mline_t_
{
    char    kind[16];                   ///< 媒体类型: "video" / "audio" / "application"
    int     port;                       ///< 端口
    int     pt_num;                     ///< 载荷类型个数
    int     pts[KLB_WEBRTC_PT_MAX];     ///< 载荷类型
    char    codec[KLB_WEBRTC_PT_MAX][KLB_WEBRTC_CODEC_NAME];    ///< a=rtpmap 编码名
    int     rate[KLB_WEBRTC_PT_MAX];            ///< a=rtpmap 时钟频率; 0.未声明
    int     channels[KLB_WEBRTC_PT_MAX];        ///< a=rtpmap 声道数; 0.未声明
    char    fmtp[KLB_WEBRTC_PT_MAX][KLB_WEBRTC_FMTP_MAX];   ///< a=fmtp 参数串
    int     fb_mask[KLB_WEBRTC_PT_MAX];         ///< klb_webrtc_fb_e 位掩码
}klb_webrtc_mline_t;


/// @brief 增加一条媒体 m-line (video / audio)
/// @param [in] *p_peer         会话
/// @param [in] kind            klb_webrtc_kind_e
/// @param [in] *p_codecs       编码数组; 不可为 NULL
/// @param [in] num             编码个数; 1..KLB_WEBRTC_CODEC_MAX
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 须在 create_offer 之前调用; 最多 KLB_WEBRTC_MLINE_MAX 条; BUNDLE 中 mid 从 1 开始
KLB_API int klb_webrtc_peer_add_media(klb_webrtc_peer_t* p_peer, int kind,
    const klb_webrtc_codec_t* p_codecs, int num);


/// @def   KLB_WEBRTC_TWCC_MAX
/// @brief 单次 TWCC 反馈可携带的最大包状态数
#define KLB_WEBRTC_TWCC_MAX             256

/// @def   KLB_WEBRTC_TWCC_FLUSH_PKTS
/// @brief TWCC 窗口自动冲刷阈值 (包数); recv 泵驱动
#define KLB_WEBRTC_TWCC_FLUSH_PKTS      32

/// @def   KLB_WEBRTC_BPS_INIT / MIN / MAX
/// @brief TWCC 自适应目标码率初始值 / 下限 / 上限 (bps)
#define KLB_WEBRTC_BPS_INIT             1000000
#define KLB_WEBRTC_BPS_MIN              64000
#define KLB_WEBRTC_BPS_MAX              8000000

/// @def   KLB_WEBRTC_REPORT_MS
/// @brief SR/RR 周期上报间隔 (毫秒)
#define KLB_WEBRTC_REPORT_MS            1000


/// @brief 发送 RTCP SR (发送者报告)
/// @param [in]  *p_peer        会话
/// @param [out] *p_code        可选; 小于 0.klb_netcode_e
/// @return uint32_t SR 的 NTP 中间 32 位; 返回 0 且 p_code 小于 0 表失败
/// @note 报告块取本端接收统计 (对端媒体 SSRC / 丢包 / 抖动 / LSR / DLSR)
KLB_API uint32_t klb_webrtc_rtcp_sr_send(klb_webrtc_peer_t* p_peer, int* p_code);


/// @brief 发送 RTCP RR (接收者报告)
/// @param [in] *p_peer         会话
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_rtcp_rr_send(klb_webrtc_peer_t* p_peer);


/// @brief 发送 RTCP TWCC (传输层拥塞控制反馈)
/// @param [in] *p_peer         会话
/// @param [in] *p_status       包状态数组; 0.未收到; 非 0.已收到
/// @param [in] count           包状态数; 1..KLB_WEBRTC_TWCC_MAX
/// @param [in] base_seq        起始序号
/// @param [in] ref_time        参考时间 (24 位, 64ms 单位)
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_rtcp_twcc_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_status,
                                      int count, uint16_t base_seq, uint32_t ref_time);


/// @brief 按本端 TWCC 到达窗口发送反馈 (含真实到达时间增量)
/// @param [in] *p_peer          会话
/// @return int 0.成功 (0 == 无可上报包); 小于 0.klb_netcode_e
/// @note 取内部轮询累积的包状态与到达 tick; 成功后窗口清空
KLB_API int klb_webrtc_rtcp_twcc_flush(klb_webrtc_peer_t* p_peer);


/// @enum klb_webrtc_event_e
/// @brief 会话事件类型
typedef enum klb_webrtc_event_e_
{
    KLB_WEBRTC_EVT_NONE         = 0,    ///< 无事件
    KLB_WEBRTC_EVT_ICE_STATE    = 1,    ///< ICE 状态变化; code = klb_ice_state_e; name = "ice_state"
    KLB_WEBRTC_EVT_DTLS_READY   = 2,    ///< DTLS 握手完成; name = "dtls_ready"
    KLB_WEBRTC_EVT_SRTP_READY   = 3,    ///< SRTP 收发密钥就绪; name = "srtp_ready"
    KLB_WEBRTC_EVT_DC_OPEN      = 4,    ///< DataChannel 已打开; name = "dc_open"
}klb_webrtc_event_e;


/// @struct klb_webrtc_event_t
/// @brief  会话事件 (从事件队列取出)
typedef struct klb_webrtc_event_t_
{
    int     type;                       ///< klb_webrtc_event_e
    int     code;                       ///< 附加码: ICE 状态 / 错误码; 无则 0
    char    name[32];                   ///< 事件名文本
}klb_webrtc_event_t;


/// @brief 取一个待处理会话事件 (非阻塞)
/// @param [in]  *p_peer        会话
/// @param [out] *p_event       事件
/// @return int 1.取到事件; 0.暂无事件; 小于 0.klb_netcode_e
/// @note 事件由 ICE 状态变化 / DTLS 与 SRTP 就绪 / DataChannel 打开时产生; 队列满时丢最旧
KLB_API int klb_webrtc_peer_event_poll(klb_webrtc_peer_t* p_peer, klb_webrtc_event_t* p_event);


/// @def   KLB_WEBRTC_FP_MAX
/// @brief 指纹文本缓冲长度 (大于 "sha-256 " + 95)
#define KLB_WEBRTC_FP_MAX               128

/// @def   KLB_WEBRTC_NACK_MAX
/// @brief 单次 NACK 可携带的序号上限
#define KLB_WEBRTC_NACK_MAX             16


/// @brief 配置 DTLS 证书; 未配置则 DTLS/SRTP 不启用
/// @param [in] *p_peer         会话
/// @param [in] *p_cert_pem     本端证书 PEM
/// @param [in] *p_key_pem      本端私钥 PEM
/// @param [in] *p_ca_pem       对端校验 CA PEM; 可为 NULL
/// @param [in] verify_peer     0(false).不校验; 1(true).校验对端证书
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 须在 create_offer / create_answer 之前调用; 文本被复制保存
KLB_API int klb_webrtc_peer_set_dtls_cert(klb_webrtc_peer_t* p_peer, const char* p_cert_pem,
    const char* p_key_pem, const char* p_ca_pem, int verify_peer);


/// @struct klb_webrtc_stats_t
/// @brief  会话统计 (ICE 连通 + DTLS/SRTP 状态 + 收发计数)
typedef struct klb_webrtc_stats_t_
{
    int         state;              ///< klb_ice_state_e
    int         has_selected;       ///< 1.已选中工作对; 0.未选中
    int         local_count;        ///< 本地候选数
    int         remote_count;       ///< 远端候选数
    int         pair_count;         ///< 候选对数
    int         dtls_ready;         ///< 1.DTLS 握手完成
    int         srtp_ready;         ///< 1.SRTP 收发密钥就绪
    char        local_ip[64];       ///< 选中本地候选 IP
    int         local_port;         ///< 选中本地候选端口
    char        remote_ip[64];      ///< 选中远端候选 IP
    int         remote_port;        ///< 选中远端候选端口
    uint64_t    check_sent;         ///< STUN 连通性检查发送数
    uint64_t    check_recv;         ///< STUN 连通性检查接收数
    uint64_t    data_sent_pkts;     ///< ICE 应用数据发送包数
    uint64_t    data_sent_bytes;    ///< ICE 应用数据发送字节数
    uint64_t    data_recv_pkts;     ///< ICE 应用数据接收包数
    uint64_t    data_recv_bytes;    ///< ICE 应用数据接收字节数
    uint64_t    srtp_tx_pkts;       ///< SRTP/SRTCP 发送包数
    uint64_t    srtp_rx_pkts;       ///< SRTP/SRTCP 接收包数
    uint64_t    dtls_tx_pkts;       ///< DTLS 应用数据发送包数
    uint64_t    dtls_rx_pkts;       ///< DTLS 应用数据接收包数
    uint32_t    rtp_ssrc_tx;        ///< 本端媒体 SSRC
    uint32_t    rtp_ssrc_rx;        ///< 对端媒体 SSRC; 未收到为 0
    uint64_t    rtp_tx_pkts;        ///< RTP 发送包数
    uint64_t    rtp_rx_pkts;        ///< RTP 接收包数
    uint64_t    rtcp_tx_pkts;       ///< RTCP 发送包数
    uint64_t    rtcp_rx_pkts;       ///< RTCP 接收包数
    uint64_t    nack_tx_pkts;       ///< NACK 发送包数
    uint64_t    nack_rx_pkts;       ///< NACK 接收包数
    uint64_t    retrans_pkts;       ///< 按 NACK 重传的 RTP 包数
    uint64_t    rtp_lost_pkts;      ///< 抖动缓冲跳过缺口丢弃的序号数
    uint64_t    jb_pending;         ///< 抖动缓冲当前积压包数
    uint64_t    jb_dup_pkts;        ///< 抖动缓冲重复包数
    uint64_t    jb_late_pkts;       ///< 抖动缓冲迟到包数
    uint64_t    jb_overflow_pkts;   ///< 抖动缓冲越窗丢弃包数
    uint64_t    jb_reorder_pkts;    ///< 抖动缓冲乱序到达包数
    uint64_t    rtp_rx_expected;    ///< 期望收到的 RTP 包数 (按序号跨度)
    uint64_t    rtp_rx_lost;        ///< 累计丢包估计 (期望 - 实收)
    uint64_t    rr_jitter;          ///< 接收抖动 (RTP 时钟单位)
    uint32_t    rr_lsr;             ///< 最近 SR 的 NTP 中间 32 位
    uint32_t    rr_dlsr;            ///< 距最近 SR 的延迟 (1/65536 秒)
    uint64_t    twcc_tx_pkts;       ///< TWCC 反馈发送包数
    uint64_t    twcc_rx_pkts;       ///< TWCC 反馈接收包数
    uint64_t    report_tx_pkts;     ///< SR/RR 发送包数
    uint64_t    report_rx_pkts;     ///< SR/RR 接收包数
    uint32_t    target_bps;         ///< TWCC 自适应目标码率 (bps)
    uint32_t    twcc_loss_permille; ///< 最近一轮 TWCC 丢包率 (千分比)
    uint64_t    twcc_fb_rounds;     ///< 已处理的 TWCC 反馈轮数
}klb_webrtc_stats_t;


/// @brief 取会话统计
/// @param [in]  *p_peer        会话
/// @param [out] *p_stats       统计结果
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_stats(klb_webrtc_peer_t* p_peer, klb_webrtc_stats_t* p_stats);


/// @brief 打开 DataChannel (须 DTLS 已就绪)
/// @param [in] *p_peer         会话
/// @return int 0.成功; 小于 0.klb_netcode_e; KLB_NETCODE_ENOTCONN 表尚未连通
/// @note 客户端在此发出 SCTP INIT; 服务端等待对端 INIT
KLB_API int klb_webrtc_peer_dc_open(klb_webrtc_peer_t* p_peer);


/// @brief DataChannel 是否已完成 DCEP 握手
/// @param [in] *p_peer         会话
/// @return int 1.已打开; 0.未打开
KLB_API int klb_webrtc_peer_dc_is_open(klb_webrtc_peer_t* p_peer);


/// @brief 指定流上的 DataChannel 是否已完成 DCEP 握手
/// @param [in] *p_peer         会话
/// @param [in] stream_id       流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @return int 1.已打开; 0.未打开
KLB_API int klb_webrtc_peer_dc_is_open_stream(klb_webrtc_peer_t* p_peer, int stream_id);


/// @brief 在指定流上打开一条 DataChannel (须 DTLS 已就绪)
/// @param [in] *p_peer         会话
/// @param [in] stream_id       流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_dc_open_stream(klb_webrtc_peer_t* p_peer, int stream_id);


/// @brief 经 DataChannel 发送一条消息
/// @param [in] *p_peer         会话
/// @param [in] *p_data         消息内容
/// @param [in] len             长度
/// @param [in] is_binary       0.文本(PPID 51); 非 0.二进制(PPID 53)
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_dc_send(klb_webrtc_peer_t* p_peer, const uint8_t* p_data,
    int len, int is_binary);


/// @brief 经指定流的 DataChannel 发送一条消息 (超过 MTU 自动分片)
/// @param [in] *p_peer         会话
/// @param [in] stream_id       流号; 0..KLB_WEBRTC_DC_STREAMS-1
/// @param [in] *p_data         消息内容
/// @param [in] len             长度
/// @param [in] is_binary       0.文本(PPID 51); 非 0.二进制(PPID 53)
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_dc_send_stream(klb_webrtc_peer_t* p_peer, int stream_id,
    const uint8_t* p_data, int len, int is_binary);


/// @brief 经 DataChannel 接收一条消息
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int 大于 0.消息长度; 0.暂无消息; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_dc_recv(klb_webrtc_peer_t* p_peer, uint8_t* p_buf,
    int cap, int* p_code);


/// @brief 经 DataChannel 接收一条消息 (同时输出所属流号)
/// @param [in]  *p_peer        会话
/// @param [out] *p_buf         输出缓存
/// @param [in]  cap            缓存大小
/// @param [out] *p_stream_id   消息所属流号; 可为 NULL
/// @param [out] *p_code        可选; klb_netcode_e
/// @return int 大于 0.消息长度; 0.暂无消息; 小于 0.klb_netcode_e
KLB_API int klb_webrtc_peer_dc_recv_ex(klb_webrtc_peer_t* p_peer, uint8_t* p_buf,
    int cap, int* p_stream_id, int* p_code);


/// @def   KLB_WEBRTC_MAX_CAND
/// @brief 单份 SDP 可解析的候选行上限
#define KLB_WEBRTC_MAX_CAND             16

/// @def   KLB_WEBRTC_CAND_MAX
/// @brief 单条候选串缓冲长度
#define KLB_WEBRTC_CAND_MAX             192


/// @struct klb_webrtc_sdp_t
/// @brief  SDP 解析结果 (ICE 凭据 / 候选 / DTLS 指纹子集)
typedef struct klb_webrtc_sdp_t_
{
    char    ufrag[64];              ///< a=ice-ufrag; 未出现为空串
    char    pwd[64];                ///< a=ice-pwd; 未出现为空串
    char    fingerprint[128];       ///< a=fingerprint 值 (形如 "sha-256 AB:CD:..."); 未出现为空串
    char    setup[16];              ///< a=setup 值 ("actpass"/"active"/"passive"); 未出现为空串
    int     candidate_num;          ///< 候选行数
    char    candidates[KLB_WEBRTC_MAX_CAND][KLB_WEBRTC_CAND_MAX];   ///< 候选行 (不含 "a=")
    int     mline_num;              ///< m-line 数 (含 application)
    klb_webrtc_mline_t mlines[KLB_WEBRTC_MLINE_MAX + 1];    ///< m-line 明细
}klb_webrtc_sdp_t;


/// @brief 解析 SDP 文本中的 ICE 凭据与候选
/// @param [in]  *p_sdp         SDP 文本; 不可为 NULL
/// @param [out] *p_info        解析结果
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 候选超出 KLB_WEBRTC_MAX_CAND 的部分被丢弃
KLB_API int klb_webrtc_sdp_parse(const char* p_sdp, klb_webrtc_sdp_t* p_info);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_WEBRTC_H__

// end
