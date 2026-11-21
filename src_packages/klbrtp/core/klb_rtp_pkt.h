// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtp_pkt.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTP/RTCP 打包解包 (RFC 3550 / 6184)
/// @version 0.6
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 RTP/RTCP 头解析 / H.264 单包与 FU-A 打包 / NACK·PLI·FIR·REMB 反馈; 0.3 增加 SR/RR 报告块构建与解析 / TWCC 传输层反馈构建与解析; 0.4 TWCC 构建补齐到达时间增量字节与 32 位对齐; 0.5 TWCC 构建增加 p_delta 入参: 真实到达增量 (0.25ms)
/// @warning 不含媒体引擎统计; SR/RR 报告块数值由调用方计算
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTP_PKT_H__
#define __KLB_RTP_PKT_H__

#include "klb_type.h"
#include "klbnet/klbrtp/klb_rtp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_rtcp_type_e
/// @brief RTCP 包类型 (RFC 3550)
typedef enum klb_rtcp_type_e_
{
    KLB_RTCP_SR      = 200,             ///< 发送者报告
    KLB_RTCP_RR      = 201,             ///< 接收者报告
    KLB_RTCP_SDES    = 202,             ///< 源描述
    KLB_RTCP_BYE     = 203,             ///< 结束
    KLB_RTCP_APP     = 204,             ///< 应用自定义
    KLB_RTCP_RTPFB   = 205,             ///< 传输层反馈 (NACK 等)
    KLB_RTCP_PSFB    = 206,             ///< 载荷层反馈 (PLI / REMB 等)
}klb_rtcp_type_e;


/// @struct klb_rtcp_head_t
/// @brief  RTCP 公共头 (4 字节)
typedef struct klb_rtcp_head_t_
{
    uint8_t     version : 2;            ///< 版本; 恒 2
    uint8_t     padding : 1;            ///< 填充位
    uint8_t     count : 5;              ///< 报告/包计数 (RC); 反馈包中为 FMT

    uint8_t     type;                   ///< 包类型: klb_rtcp_type_e
    uint16_t    length;                 ///< 长度; 以 32 位字为单位, 不含本头
}klb_rtcp_head_t;


/// @struct klb_rtcp_fb_t
/// @brief  RTCP 反馈包解析结果 (粗解析)
typedef struct klb_rtcp_fb_t_
{
    uint8_t     type;                   ///< KLB_RTCP_RTPFB / KLB_RTCP_PSFB
    uint8_t     fmt;                    ///< 反馈格式: NACK=1 / PLI=1 / FIR=4 / REMB=15
    uint32_t    sender_ssrc;            ///< 发送端 SSRC
    uint32_t    media_ssrc;             ///< 媒体源 SSRC
    uint16_t    pid;                    ///< NACK 起始序号; 非 NACK 为 0
    uint16_t    blp;                    ///< NACK 位图; 非 NACK 为 0
}klb_rtcp_fb_t;


/// @brief 解析 RTP 头
/// @param [in]  *p_data        RTP 包
/// @param [in]  len            长度
/// @param [out] *p_head        输出头
/// @param [out] *p_head_len    输出头长度 (含 CSRC / 扩展); 可 NULL
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_rtp_head_parse(const uint8_t* p_data, int len, klb_rtp_head_t* p_head, int* p_head_len);


/// @brief 解析 RTCP 头
/// @param [in]  *p_data        RTCP 包
/// @param [in]  len            长度
/// @param [out] *p_head        输出头
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_head_parse(const uint8_t* p_data, int len, klb_rtcp_head_t* p_head);


/// @brief 解析 RTCP 反馈包 (NACK / PLI / FIR / REMB 公共部分)
/// @param [in]  *p_data        RTCP 包
/// @param [in]  len            长度
/// @param [out] *p_fb          输出反馈信息
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_fb_parse(const uint8_t* p_data, int len, klb_rtcp_fb_t* p_fb);


/// @brief H.264 NALU 打包为 RTP 载荷 (单包 / STAP-A / FU-A)
/// @param [in]  *p_nalu        NALU 或 Annex-B 字节流 (可含多个 NALU)
/// @param [in]  nalu_len       长度
/// @param [in]  mtu           单包上限 (含 RTP 头)
/// @param [in]  marker         标记位; 仅最后一个分片/最后一个 NALU 置位
/// @param [in]  sequence       起始序列号
/// @param [in]  timestamp      时间戳
/// @param [in]  ssrc           同步源
/// @param [out] *p_out         输出缓存; 多个 RTP 包顺序拼接
/// @param [in]  cap            缓存容量
/// @param [out] *p_out_len     输出长度
/// @return int >0.包数; 小于 0.klb_netcode_e
/// @note 多个小 NALU 且总长不超 MTU 时聚合为 STAP-A (RFC 6184)
KLB_API int klb_rtp_h264_packetize(const uint8_t* p_nalu, int nalu_len, int mtu, int marker,
                                   uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                                   uint8_t* p_out, int cap, int* p_out_len);


/// @brief 构建 RTCP NACK (RTPFB FMT=1)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  media_ssrc     媒体源 SSRC
/// @param [in]  *p_seqs        丢失序号数组
/// @param [in]  seq_num        序号个数; 每个序号单独一个 FCI
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_nack_build(uint32_t sender_ssrc, uint32_t media_ssrc,
                                const uint16_t* p_seqs, int seq_num, uint8_t* p_out, int cap);


/// @brief 构建 RTCP PLI (PSFB FMT=1)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  media_ssrc     媒体源 SSRC
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_pli_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint8_t* p_out, int cap);


/// @brief 构建 RTCP FIR (PSFB FMT=4)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  media_ssrc     媒体源 SSRC
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_fir_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint8_t* p_out, int cap);


/// @brief 构建 RTCP REMB (PSFB FMT=15)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  media_ssrc     媒体源 SSRC
/// @param [in]  bitrate_bps    目标码率 (bit/s)
/// @param [in]  *p_ssrcs       受控 SSRC 列表; 可 NULL
/// @param [in]  ssrc_num       受控 SSRC 个数; 0 表示不限
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_remb_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint32_t bitrate_bps,
                                const uint32_t* p_ssrcs, int ssrc_num, uint8_t* p_out, int cap);


/// @def   KLB_RTCP_BLOCK_MAX
/// @brief SR/RR 单包可携带的报告块上限
#define KLB_RTCP_BLOCK_MAX              4


/// @struct klb_rtcp_report_block_t
/// @brief  SR/RR 报告块 (24 字节, RFC 3550 §6.4.1)
typedef struct klb_rtcp_report_block_t_
{
    uint32_t    ssrc;                   ///< 报告源 SSRC
    uint8_t     fraction_lost;          ///< 自上次报告以来的丢包率 (0..255, 即 256 进制)
    int32_t     cumulative_lost;        ///< 累计丢包数 (24 位有符号)
    uint32_t    ext_highest_seq;        ///< 扩展最高接收序号
    uint32_t    jitter;                 ///< 抖动 (RTP 时钟单位)
    uint32_t    lsr;                    ///< 最近一次 SR 的 NTP 中间 32 位
    uint32_t    dlsr;                   ///< 距最近一次 SR 的延迟 (1/65536 秒)
}klb_rtcp_report_block_t;


/// @struct klb_rtcp_sr_t
/// @brief  SR (发送者报告) 解析结果
typedef struct klb_rtcp_sr_t_
{
    uint32_t                sender_ssrc;        ///< 发送端 SSRC
    uint32_t                ntp_sec;            ///< NTP 秒
    uint32_t                ntp_frac;           ///< NTP 小数
    uint32_t                rtp_ts;             ///< 对应 RTP 时间戳
    uint32_t                pkt_count;          ///< 累计发送包数
    uint32_t                octet_count;        ///< 累计发送字节数
    int                     block_num;          ///< 报告块数
    klb_rtcp_report_block_t blocks[KLB_RTCP_BLOCK_MAX];   ///< 报告块
}klb_rtcp_sr_t;


/// @struct klb_rtcp_rr_t
/// @brief  RR (接收者报告) 解析结果
typedef struct klb_rtcp_rr_t_
{
    uint32_t                sender_ssrc;        ///< 发送端 SSRC
    int                     block_num;          ///< 报告块数
    klb_rtcp_report_block_t blocks[KLB_RTCP_BLOCK_MAX];   ///< 报告块
}klb_rtcp_rr_t;


/// @struct klb_rtcp_twcc_t
/// @brief  TWCC (传输层拥塞控制反馈, RTPFB FMT=15) 解析结果
typedef struct klb_rtcp_twcc_t_
{
    uint32_t    sender_ssrc;            ///< 发送端 SSRC
    uint32_t    media_ssrc;             ///< 媒体源 SSRC; transport-wide 约定为 0
    uint16_t    base_seq;               ///< 起始序号
    uint16_t    count;                  ///< 包状态数
    uint32_t    ref_time;               ///< 参考时间 (24 位, 64ms 单位)
    uint8_t     fb_count;               ///< 反馈包计数 (8 位)
}klb_rtcp_twcc_t;


/// @brief 构建 RTCP SR (发送者报告)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  ntp_sec        NTP 秒
/// @param [in]  ntp_frac       NTP 小数
/// @param [in]  rtp_ts         对应 RTP 时间戳
/// @param [in]  pkt_count      累计发送包数
/// @param [in]  octet_count    累计发送字节数
/// @param [in]  *p_blocks      报告块数组; 可为 NULL
/// @param [in]  block_num      报告块数; 0..KLB_RTCP_BLOCK_MAX
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_sr_build(uint32_t sender_ssrc, uint32_t ntp_sec, uint32_t ntp_frac,
                              uint32_t rtp_ts, uint32_t pkt_count, uint32_t octet_count,
                              const klb_rtcp_report_block_t* p_blocks, int block_num,
                              uint8_t* p_out, int cap);


/// @brief 构建 RTCP RR (接收者报告)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  *p_blocks      报告块数组; 可为 NULL
/// @param [in]  block_num      报告块数; 0..KLB_RTCP_BLOCK_MAX
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_rr_build(uint32_t sender_ssrc, const klb_rtcp_report_block_t* p_blocks,
                              int block_num, uint8_t* p_out, int cap);


/// @brief 解析 RTCP SR
/// @param [in]  *p_data        RTCP 包
/// @param [in]  len            长度
/// @param [out] *p_sr          输出 SR 信息
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_sr_parse(const uint8_t* p_data, int len, klb_rtcp_sr_t* p_sr);


/// @brief 解析 RTCP RR
/// @param [in]  *p_data        RTCP 包
/// @param [in]  len            长度
/// @param [out] *p_rr          输出 RR 信息
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_rr_parse(const uint8_t* p_data, int len, klb_rtcp_rr_t* p_rr);


/// @brief 构建 RTCP TWCC (RTPFB FMT=15)
/// @param [in]  sender_ssrc    发送端 SSRC
/// @param [in]  media_ssrc     媒体源 SSRC; transport-wide 约定为 0
/// @param [in]  base_seq       起始序号
/// @param [in]  *p_status      包状态数组; 0.未收到; 非 0.已收到
/// @param [in]  *p_delta       到达时间增量数组 (与 p_status 同序); 可 NULL
/// @param [in]  count          包状态数
/// @param [in]  ref_time       参考时间 (24 位, 64ms 单位)
/// @param [in]  fb_count       反馈包计数
/// @param [out] *p_out         输出缓存
/// @param [in]  cap            缓存容量
/// @return int >0.包长度; 小于 0.klb_netcode_e
/// @note 游程块编码; 每个已收包附 1 字节到达增量 (0.25ms 单位), p_delta 为 NULL 时置 0
KLB_API int klb_rtcp_twcc_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint16_t base_seq,
                                const uint8_t* p_status, const uint8_t* p_delta, int count,
                                uint32_t ref_time, uint8_t fb_count, uint8_t* p_out, int cap);


/// @brief 解析 RTCP TWCC
/// @param [in]  *p_data        RTCP 包
/// @param [in]  len            长度
/// @param [out] *p_twcc        输出 TWCC 信息
/// @param [out] *p_status      包状态输出缓冲; 1.已收到; 0.未收到
/// @param [in]  cap            状态缓冲容量; 须不小于包状态数
/// @return int >=0.状态数; 小于 0.klb_netcode_e
KLB_API int klb_rtcp_twcc_parse(const uint8_t* p_data, int len, klb_rtcp_twcc_t* p_twcc,
                                uint8_t* p_status, int cap);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_RTP_PKT_H__

// end
