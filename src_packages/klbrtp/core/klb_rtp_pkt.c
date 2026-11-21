// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtp_pkt.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTP/RTCP 打包解包 (RFC 3550 / 6184)
/// @version 0.6
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 实现 RTP/RTCP 头解析 / H.264 单包与 FU-A 打包 / NACK·PLI·FIR·REMB 反馈; 0.3 增加 SR/RR 报告块构建与解析 / TWCC 传输层反馈构建与解析; 0.4 TWCC 构建补齐到达时间增量字节与 32 位对齐; 0.5 TWCC 构建增加 p_delta 入参: 真实到达增量 (0.25ms)
/// @warning RTP 仅解析到扩展头; SR/RR 报告块数值与丢包统计由调用方计算
///////////////////////////////////////////////////////////////////////////
#include "klbrtp/core/klb_rtp_pkt.h"
#include "klbnet/klb_netcode.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @def   KLB_RTP_FIXED_LEN
/// @brief RTP 固定头长度
#define KLB_RTP_FIXED_LEN               12

/// @def   KLB_RTP_FU_HEADS
/// @brief FU-A 分片附加头长度 (指示字节 + 头字节)
#define KLB_RTP_FU_HEADS                2

/// @def   KLB_RTCP_FB_HEADS
/// @brief RTCP 反馈固定头长度 (公共头 + 发送端 SSRC + 媒体源 SSRC)
#define KLB_RTCP_FB_HEADS               12

/// @def   KLB_RTCP_SR_HEADS
/// @brief SR 发送者信息长度 (SSRC + NTP 秒/小数 + RTP 时间戳 + 包数 + 字节数)
#define KLB_RTCP_SR_HEADS               24

/// @def   KLB_RTCP_RR_HEADS
/// @brief RR 发送端 SSRC 长度
#define KLB_RTCP_RR_HEADS               4

/// @def   KLB_RTCP_BLOCK_LEN
/// @brief 单个报告块长度
#define KLB_RTCP_BLOCK_LEN              24

/// @def   KLB_RTCP_TWCC_HEADS
/// @brief TWCC 公共字段长度 (起始序号 + 状态数 + 参考时间 + 反馈计数)
#define KLB_RTCP_TWCC_HEADS             8

/// @def   KLB_RTCP_TWCC_RUN_MAX
/// @brief TWCC 游程块可编码的最大包数 (13 位)
#define KLB_RTCP_TWCC_RUN_MAX           8191

/// @def   KLB_RTP_STAP_MAX_NALU
/// @brief 单次 STAP-A 聚合可容纳的 NALU 个数
#define KLB_RTP_STAP_MAX_NALU           16

/// @def   KLB_RTP_PT_H264
/// @brief 本模块固定使用的 H.264 动态载荷类型
#define KLB_RTP_PT_H264                 96


/// @brief 读取 16 位大端
static uint16_t get_u16_klb_rtp(const uint8_t* p_buf)
{
    return (uint16_t)(((uint16_t)p_buf[0] << 8) | (uint16_t)p_buf[1]);
}


/// @brief 写入 16 位大端
static void put_u16_klb_rtp(uint8_t* p_buf, uint16_t val)
{
    p_buf[0] = (uint8_t)(val >> 8);
    p_buf[1] = (uint8_t)(val & 0xFF);
}


/// @brief 读取 32 位大端
static uint32_t get_u32_klb_rtp(const uint8_t* p_buf)
{
    return ((uint32_t)p_buf[0] << 24) | ((uint32_t)p_buf[1] << 16)
        | ((uint32_t)p_buf[2] << 8) | (uint32_t)p_buf[3];
}


/// @brief 写入 32 位大端
static void put_u32_klb_rtp(uint8_t* p_buf, uint32_t val)
{
    p_buf[0] = (uint8_t)(val >> 24);
    p_buf[1] = (uint8_t)((val >> 16) & 0xFF);
    p_buf[2] = (uint8_t)((val >> 8) & 0xFF);
    p_buf[3] = (uint8_t)(val & 0xFF);
}


/// @brief 写 RTP 固定头
static void put_rtp_head_klb_rtp(uint8_t* p_pkt, bool marker, uint8_t payload_type,
                                 uint16_t sequence, uint32_t timestamp, uint32_t ssrc)
{
    p_pkt[0] = 0x80;
    p_pkt[1] = (uint8_t)(((marker ? 1 : 0) << 7) | (payload_type & 0x7F));
    put_u16_klb_rtp(p_pkt + 2, sequence);
    put_u32_klb_rtp(p_pkt + 4, timestamp);
    put_u32_klb_rtp(p_pkt + 8, ssrc);
}


int klb_rtp_head_parse(const uint8_t* p_data, int len, klb_rtp_head_t* p_head, int* p_head_len)
{
    if ((NULL == p_data) || (NULL == p_head))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (KLB_RTP_FIXED_LEN > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (2 != (p_data[0] >> 6))
    {
        return KLB_NETCODE_PROTO_VERSION;
    }

    p_head->version    = 2;
    p_head->padding    = (uint8_t)((p_data[0] >> 5) & 0x01);
    p_head->extension  = (uint8_t)((p_data[0] >> 4) & 0x01);
    p_head->csrc_count = (uint8_t)(p_data[0] & 0x0F);
    p_head->marker     = (uint8_t)(p_data[1] >> 7);
    p_head->payload_type = (uint8_t)(p_data[1] & 0x7F);
    p_head->sequence   = get_u16_klb_rtp(p_data + 2);
    p_head->timestamp  = get_u32_klb_rtp(p_data + 4);
    p_head->ssrc       = get_u32_klb_rtp(p_data + 8);

    int off = KLB_RTP_FIXED_LEN + (int)p_head->csrc_count * 4;
    if (off > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (0 != p_head->extension)
    {
        if ((off + 4) > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        int ext_words = (int)get_u16_klb_rtp(p_data + off + 2);
        off += 4 + ext_words * 4;
        if (off > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }
    }

    if (NULL != p_head_len)
    {
        *p_head_len = off;
    }

    return 0;
}


int klb_rtcp_head_parse(const uint8_t* p_data, int len, klb_rtcp_head_t* p_head)
{
    if ((NULL == p_data) || (NULL == p_head))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (4 > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (2 != (p_data[0] >> 6))
    {
        return KLB_NETCODE_PROTO_VERSION;
    }

    p_head->version = 2;
    p_head->padding = (uint8_t)((p_data[0] >> 5) & 0x01);
    p_head->count   = (uint8_t)(p_data[0] & 0x1F);
    p_head->type    = p_data[1];
    p_head->length  = get_u16_klb_rtp(p_data + 2);

    int total = ((int)p_head->length + 1) * 4;
    if (total > len)
    {
        return KLB_NETCODE_PROTO_TOO_LARGE;
    }

    return 0;
}


/// @brief 扫描 Annex-B 字节流中的 NALU
/// @param [in]  *p_buf     输入字节流
/// @param [in]  len        长度
/// @param [out] *p_nalu    NALU 起始地址
/// @param [out] *p_len     NALU 长度
/// @param [in]  max_num    数组容量
/// @return int 大于 0.NALU 个数; 小于 0.klb_netcode_e
/// @note 无起始码时整体视为一个 NALU; NALU 之间的填充零被裁剪
static int scan_annexb_klb_rtp(const uint8_t* p_buf, int len,
                               const uint8_t* p_nalu[], int p_len[], int max_num)
{
    int count = 0;
    int start = -1;
    int i = 0;

    while ((i + 3) <= len)
    {
        if ((0x00 == p_buf[i]) && (0x00 == p_buf[i + 1]) && (0x01 == p_buf[i + 2]))
        {
            if (0 <= start)
            {
                int n = i - start;
                while ((0 < n) && (0x00 == p_buf[start + n - 1]))
                {
                    n--;
                }

                if (0 < n)
                {
                    if (count >= max_num)
                    {
                        return count;
                    }

                    p_nalu[count] = p_buf + start;
                    p_len[count] = n;
                    count++;
                }
            }

            i += 3;
            start = i;
            continue;
        }

        i++;
    }

    if (0 > start)
    {
        p_nalu[0] = p_buf;
        p_len[0] = len;

        return 1;
    }

    int n = len - start;
    if ((0 < n) && (count < max_num))
    {
        p_nalu[count] = p_buf + start;
        p_len[count] = n;
        count++;
    }

    return (0 < count) ? count : KLB_NETCODE_EINVAL;
}


/// @brief 打包单个 NALU (单包 / FU-A)
/// @return int 大于 0.包数; 小于 0.klb_netcode_e
static int packetize_body_klb_rtp(const uint8_t* p_body, int body_len, int mtu, int marker,
                                  uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                                  uint8_t* p_out, int cap, int* p_out_len)
{
    int pos = 0;
    int count = 0;
    uint16_t seq = sequence;

    // step1. 单包: 固定头 + 完整 NALU
    if ((KLB_RTP_FIXED_LEN + body_len) <= mtu)
    {
        if ((pos + KLB_RTP_FIXED_LEN + body_len) > cap)
        {
            return KLB_NETCODE_EMSGSIZE;
        }

        put_rtp_head_klb_rtp(p_out + pos, (0 != marker), KLB_RTP_PT_H264,
            seq, timestamp, ssrc);
        memcpy(p_out + pos + KLB_RTP_FIXED_LEN, p_body, (size_t)body_len);
        pos += KLB_RTP_FIXED_LEN + body_len;
        count++;

        *p_out_len = pos;
        return count;
    }

    // step2. FU-A 分片: 固定头 + FU 指示 + FU 头 + 分片
    int chunk = mtu - KLB_RTP_FIXED_LEN - KLB_RTP_FU_HEADS;
    if (0 >= chunk)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    int off = 1;                        // 跳过原 NALU 头
    while (off < body_len)
    {
        int n = body_len - off;
        if (n > chunk)
        {
            n = chunk;
        }

        bool end = ((off + n) >= body_len) ? true : false;
        if ((pos + KLB_RTP_FIXED_LEN + KLB_RTP_FU_HEADS + n) > cap)
        {
            return KLB_NETCODE_EMSGSIZE;
        }

        put_rtp_head_klb_rtp(p_out + pos, ((0 != marker) && end), KLB_RTP_PT_H264,
            seq, timestamp, ssrc);

        uint8_t* p_fu = p_out + pos + KLB_RTP_FIXED_LEN;
        p_fu[0] = (uint8_t)((p_body[0] & 0xE0) | KLB_RTP_NALU_FU_A);
        p_fu[1] = (uint8_t)(((off == 1) ? 0x80 : 0x00) | (end ? 0x40 : 0x00)
            | (p_body[0] & 0x1F));

        memcpy(p_fu + KLB_RTP_FU_HEADS, p_body + off, (size_t)n);

        pos += KLB_RTP_FIXED_LEN + KLB_RTP_FU_HEADS + n;
        count++;
        seq++;
        off += n;
    }

    *p_out_len = pos;
    return count;
}


int klb_rtp_h264_packetize(const uint8_t* p_nalu, int nalu_len, int mtu, int marker,
                           uint16_t sequence, uint32_t timestamp, uint32_t ssrc,
                           uint8_t* p_out, int cap, int* p_out_len)
{
    if ((NULL == p_nalu) || (NULL == p_out) || (NULL == p_out_len) || (0 >= nalu_len))
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((0 >= mtu) || (mtu <= KLB_RTP_FIXED_LEN))
    {
        return KLB_NETCODE_EINVAL;
    }

    // step1. 拆分 Annex-B 得到多个 NALU
    const uint8_t* p_nalu_tab[KLB_RTP_STAP_MAX_NALU];
    int nalu_len_tab[KLB_RTP_STAP_MAX_NALU];
    int nalu_num = scan_annexb_klb_rtp(p_nalu, nalu_len, p_nalu_tab, nalu_len_tab,
        KLB_RTP_STAP_MAX_NALU);
    if (0 >= nalu_num)
    {
        return (0 > nalu_num) ? nalu_num : KLB_NETCODE_EINVAL;
    }

    // step2. 单 NALU: 单包 / FU-A
    if (1 == nalu_num)
    {
        return packetize_body_klb_rtp(p_nalu_tab[0], nalu_len_tab[0], mtu, marker,
            sequence, timestamp, ssrc, p_out, cap, p_out_len);
    }

    // step3. 多 NALU 且总长不超 MTU: STAP-A 聚合
    int stap_len = 1;                   // STAP-A 头
    for (int i = 0; i < nalu_num; i++)
    {
        stap_len += 2 + nalu_len_tab[i];
    }

    if (((KLB_RTP_FIXED_LEN + stap_len) <= mtu) && ((KLB_RTP_FIXED_LEN + stap_len) <= cap)
        && (65535 >= stap_len))
    {
        put_rtp_head_klb_rtp(p_out, (0 != marker), KLB_RTP_PT_H264, sequence, timestamp, ssrc);

        uint8_t* p_body = p_out + KLB_RTP_FIXED_LEN;
        p_body[0] = (uint8_t)((p_nalu_tab[0][0] & 0xE0) | KLB_RTP_NALU_STAP_A);

        int off = 1;
        for (int i = 0; i < nalu_num; i++)
        {
            put_u16_klb_rtp(p_body + off, (uint16_t)nalu_len_tab[i]);
            off += 2;
            memcpy(p_body + off, p_nalu_tab[i], (size_t)nalu_len_tab[i]);
            off += nalu_len_tab[i];
        }

        *p_out_len = KLB_RTP_FIXED_LEN + stap_len;

        return 1;
    }

    // step4. 超 MTU: 逐 NALU 打包, 仅最后一个 NALU 携带 marker
    int pos = 0;
    int count = 0;
    uint16_t seq = sequence;

    for (int i = 0; i < nalu_num; i++)
    {
        int sub_len = 0;
        int sub_marker = ((0 != marker) && ((nalu_num - 1) == i)) ? 1 : 0;

        int sub_count = packetize_body_klb_rtp(p_nalu_tab[i], nalu_len_tab[i], mtu, sub_marker,
            seq, timestamp, ssrc, p_out + pos, cap - pos, &sub_len);
        if (0 >= sub_count)
        {
            return (0 > sub_count) ? sub_count : KLB_NETCODE_EPERM;
        }

        pos += sub_len;
        count += sub_count;
        seq = (uint16_t)(seq + sub_count);
    }

    *p_out_len = pos;

    return count;
}


int klb_rtcp_fb_parse(const uint8_t* p_data, int len, klb_rtcp_fb_t* p_fb)
{
    if ((NULL == p_data) || (NULL == p_fb))
    {
        return KLB_NETCODE_EINVAL;
    }

    klb_rtcp_head_t head;
    int ret = klb_rtcp_head_parse(p_data, len, &head);
    if (0 > ret)
    {
        return ret;
    }

    if ((KLB_RTCP_RTPFB != head.type) && (KLB_RTCP_PSFB != head.type))
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    if (KLB_RTCP_FB_HEADS > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    KLB_MEMSET(p_fb, 0, sizeof(klb_rtcp_fb_t));
    p_fb->type        = head.type;
    p_fb->fmt         = head.count;
    p_fb->sender_ssrc = get_u32_klb_rtp(p_data + 4);
    p_fb->media_ssrc  = get_u32_klb_rtp(p_data + 8);

    if ((KLB_RTCP_RTPFB == head.type) && (1 == head.count) && ((KLB_RTCP_FB_HEADS + 4) <= len))
    {
        p_fb->pid = get_u16_klb_rtp(p_data + KLB_RTCP_FB_HEADS);
        p_fb->blp = get_u16_klb_rtp(p_data + KLB_RTCP_FB_HEADS + 2);
    }

    return 0;
}


/// @brief 写 RTCP 反馈公共头
static void put_fb_head_klb_rtp(uint8_t* p_pkt, uint8_t fmt, uint8_t type, int total,
                                uint32_t sender_ssrc, uint32_t media_ssrc)
{
    p_pkt[0] = (uint8_t)(0x80 | (fmt & 0x1F));
    p_pkt[1] = type;
    put_u16_klb_rtp(p_pkt + 2, (uint16_t)((total / 4) - 1));
    put_u32_klb_rtp(p_pkt + 4, sender_ssrc);
    put_u32_klb_rtp(p_pkt + 8, media_ssrc);
}


int klb_rtcp_nack_build(uint32_t sender_ssrc, uint32_t media_ssrc,
                        const uint16_t* p_seqs, int seq_num, uint8_t* p_out, int cap)
{
    if ((NULL == p_seqs) || (NULL == p_out) || (0 >= seq_num))
    {
        return KLB_NETCODE_EINVAL;
    }

    int total = KLB_RTCP_FB_HEADS + seq_num * 4;
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    put_fb_head_klb_rtp(p_out, 1, KLB_RTCP_RTPFB, total, sender_ssrc, media_ssrc);

    for (int i = 0; i < seq_num; i++)
    {
        uint8_t* p_fci = p_out + KLB_RTCP_FB_HEADS + i * 4;
        put_u16_klb_rtp(p_fci, p_seqs[i]);
        put_u16_klb_rtp(p_fci + 2, 0);
    }

    return total;
}


int klb_rtcp_pli_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint8_t* p_out, int cap)
{
    if (NULL == p_out)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (KLB_RTCP_FB_HEADS > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    put_fb_head_klb_rtp(p_out, 1, KLB_RTCP_PSFB, KLB_RTCP_FB_HEADS, sender_ssrc, media_ssrc);

    return KLB_RTCP_FB_HEADS;
}


int klb_rtcp_fir_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint8_t* p_out, int cap)
{
    if (NULL == p_out)
    {
        return KLB_NETCODE_EINVAL;
    }

    int total = KLB_RTCP_FB_HEADS + 8;
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    put_fb_head_klb_rtp(p_out, 4, KLB_RTCP_PSFB, total, sender_ssrc, media_ssrc);

    // FCI: SSRC(4) + 序号(1) + 保留(3)
    put_u32_klb_rtp(p_out + KLB_RTCP_FB_HEADS, media_ssrc);
    p_out[KLB_RTCP_FB_HEADS + 4] = 0;
    p_out[KLB_RTCP_FB_HEADS + 5] = 0;
    p_out[KLB_RTCP_FB_HEADS + 6] = 0;
    p_out[KLB_RTCP_FB_HEADS + 7] = 0;

    return total;
}


int klb_rtcp_remb_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint32_t bitrate_bps,
                        const uint32_t* p_ssrcs, int ssrc_num, uint8_t* p_out, int cap)
{
    if ((NULL == p_out) || (0 > ssrc_num) || ((0 < ssrc_num) && (NULL == p_ssrcs)))
    {
        return KLB_NETCODE_EINVAL;
    }

    int total = KLB_RTCP_FB_HEADS + 8 + ssrc_num * 4;
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    put_fb_head_klb_rtp(p_out, 15, KLB_RTCP_PSFB, total, sender_ssrc, media_ssrc);

    // FCI: 发送端 SSRC(4) + 指数/尾数(4) + SSRC 列表
    uint8_t* p_fci = p_out + KLB_RTCP_FB_HEADS;
    put_u32_klb_rtp(p_fci, sender_ssrc);

    uint8_t exp = 0;
    uint32_t mantissa = bitrate_bps;
    while (mantissa > 0x3FFFF)
    {
        mantissa >>= 1;
        exp++;
    }

    p_fci[4] = (uint8_t)((exp << 2) | 0x00);
    p_fci[5] = (uint8_t)((mantissa >> 16) & 0xFF);
    p_fci[6] = (uint8_t)((mantissa >> 8) & 0xFF);
    p_fci[7] = (uint8_t)(mantissa & 0xFF);

    for (int i = 0; i < ssrc_num; i++)
    {
        put_u32_klb_rtp(p_fci + 8 + i * 4, p_ssrcs[i]);
    }

    return total;
}


/// @brief 读取 24 位大端
static uint32_t get_u24_klb_rtp(const uint8_t* p_buf)
{
    return ((uint32_t)p_buf[0] << 16) | ((uint32_t)p_buf[1] << 8) | (uint32_t)p_buf[2];
}


/// @brief 写入 24 位大端
static void put_u24_klb_rtp(uint8_t* p_buf, uint32_t val)
{
    p_buf[0] = (uint8_t)((val >> 16) & 0xFF);
    p_buf[1] = (uint8_t)((val >> 8) & 0xFF);
    p_buf[2] = (uint8_t)(val & 0xFF);
}


/// @brief 写入一个 SR/RR 报告块
/// @param [out] *p_out     24 字节输出
/// @param [in]  *p_block   报告块
/// @return 无
static void put_block_klb_rtp(uint8_t* p_out, const klb_rtcp_report_block_t* p_block)
{
    put_u32_klb_rtp(p_out, p_block->ssrc);
    p_out[4] = p_block->fraction_lost;
    put_u24_klb_rtp(p_out + 5, (uint32_t)p_block->cumulative_lost & 0x00FFFFFFu);
    put_u32_klb_rtp(p_out + 8, p_block->ext_highest_seq);
    put_u32_klb_rtp(p_out + 12, p_block->jitter);
    put_u32_klb_rtp(p_out + 16, p_block->lsr);
    put_u32_klb_rtp(p_out + 20, p_block->dlsr);
}


/// @brief 读取一个 SR/RR 报告块
/// @param [in]  *p_in      24 字节输入
/// @param [out] *p_block   报告块
/// @return 无
static void get_block_klb_rtp(const uint8_t* p_in, klb_rtcp_report_block_t* p_block)
{
    p_block->ssrc = get_u32_klb_rtp(p_in);
    p_block->fraction_lost = p_in[4];

    uint32_t lost = get_u24_klb_rtp(p_in + 5);
    if (0 != (lost & 0x00800000u))
    {
        lost |= 0xFF000000u;
    }

    p_block->cumulative_lost = (int32_t)lost;
    p_block->ext_highest_seq = get_u32_klb_rtp(p_in + 8);
    p_block->jitter = get_u32_klb_rtp(p_in + 12);
    p_block->lsr = get_u32_klb_rtp(p_in + 16);
    p_block->dlsr = get_u32_klb_rtp(p_in + 20);
}


int klb_rtcp_sr_build(uint32_t sender_ssrc, uint32_t ntp_sec, uint32_t ntp_frac,
                      uint32_t rtp_ts, uint32_t pkt_count, uint32_t octet_count,
                      const klb_rtcp_report_block_t* p_blocks, int block_num,
                      uint8_t* p_out, int cap)
{
    if ((NULL == p_out) || (0 > block_num) || (KLB_RTCP_BLOCK_MAX < block_num)
        || ((0 < block_num) && (NULL == p_blocks)))
    {
        return KLB_NETCODE_EINVAL;
    }

    int total = 4 + KLB_RTCP_SR_HEADS + block_num * KLB_RTCP_BLOCK_LEN;
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    p_out[0] = (uint8_t)(0x80 | (block_num & 0x1F));
    p_out[1] = KLB_RTCP_SR;
    put_u16_klb_rtp(p_out + 2, (uint16_t)((total / 4) - 1));
    put_u32_klb_rtp(p_out + 4, sender_ssrc);
    put_u32_klb_rtp(p_out + 8, ntp_sec);
    put_u32_klb_rtp(p_out + 12, ntp_frac);
    put_u32_klb_rtp(p_out + 16, rtp_ts);
    put_u32_klb_rtp(p_out + 20, pkt_count);
    put_u32_klb_rtp(p_out + 24, octet_count);

    for (int i = 0; i < block_num; i++)
    {
        put_block_klb_rtp(p_out + 4 + KLB_RTCP_SR_HEADS + i * KLB_RTCP_BLOCK_LEN, p_blocks + i);
    }

    return total;
}


int klb_rtcp_rr_build(uint32_t sender_ssrc, const klb_rtcp_report_block_t* p_blocks,
                      int block_num, uint8_t* p_out, int cap)
{
    if ((NULL == p_out) || (0 > block_num) || (KLB_RTCP_BLOCK_MAX < block_num)
        || ((0 < block_num) && (NULL == p_blocks)))
    {
        return KLB_NETCODE_EINVAL;
    }

    int total = 4 + KLB_RTCP_RR_HEADS + block_num * KLB_RTCP_BLOCK_LEN;
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    p_out[0] = (uint8_t)(0x80 | (block_num & 0x1F));
    p_out[1] = KLB_RTCP_RR;
    put_u16_klb_rtp(p_out + 2, (uint16_t)((total / 4) - 1));
    put_u32_klb_rtp(p_out + 4, sender_ssrc);

    for (int i = 0; i < block_num; i++)
    {
        put_block_klb_rtp(p_out + 4 + KLB_RTCP_RR_HEADS + i * KLB_RTCP_BLOCK_LEN, p_blocks + i);
    }

    return total;
}


int klb_rtcp_sr_parse(const uint8_t* p_data, int len, klb_rtcp_sr_t* p_sr)
{
    if ((NULL == p_data) || (NULL == p_sr))
    {
        return KLB_NETCODE_EINVAL;
    }

    klb_rtcp_head_t head;
    int ret = klb_rtcp_head_parse(p_data, len, &head);
    if (0 > ret)
    {
        return ret;
    }

    if (KLB_RTCP_SR != head.type)
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    int total = ((int)head.length + 1) * 4;
    if ((4 + KLB_RTCP_SR_HEADS) > total)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    KLB_MEMSET(p_sr, 0, sizeof(*p_sr));
    p_sr->sender_ssrc = get_u32_klb_rtp(p_data + 4);
    p_sr->ntp_sec = get_u32_klb_rtp(p_data + 8);
    p_sr->ntp_frac = get_u32_klb_rtp(p_data + 12);
    p_sr->rtp_ts = get_u32_klb_rtp(p_data + 16);
    p_sr->pkt_count = get_u32_klb_rtp(p_data + 20);
    p_sr->octet_count = get_u32_klb_rtp(p_data + 24);

    int blocks = (int)head.count;
    if (KLB_RTCP_BLOCK_MAX < blocks)
    {
        blocks = KLB_RTCP_BLOCK_MAX;
    }

    for (int i = 0; i < blocks; i++)
    {
        int off = 4 + KLB_RTCP_SR_HEADS + i * KLB_RTCP_BLOCK_LEN;
        if ((off + KLB_RTCP_BLOCK_LEN) > total)
        {
            break;
        }

        get_block_klb_rtp(p_data + off, p_sr->blocks + i);
        p_sr->block_num++;
    }

    return 0;
}


int klb_rtcp_rr_parse(const uint8_t* p_data, int len, klb_rtcp_rr_t* p_rr)
{
    if ((NULL == p_data) || (NULL == p_rr))
    {
        return KLB_NETCODE_EINVAL;
    }

    klb_rtcp_head_t head;
    int ret = klb_rtcp_head_parse(p_data, len, &head);
    if (0 > ret)
    {
        return ret;
    }

    if (KLB_RTCP_RR != head.type)
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    int total = ((int)head.length + 1) * 4;
    if ((4 + KLB_RTCP_RR_HEADS) > total)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    KLB_MEMSET(p_rr, 0, sizeof(*p_rr));
    p_rr->sender_ssrc = get_u32_klb_rtp(p_data + 4);

    int blocks = (int)head.count;
    if (KLB_RTCP_BLOCK_MAX < blocks)
    {
        blocks = KLB_RTCP_BLOCK_MAX;
    }

    for (int i = 0; i < blocks; i++)
    {
        int off = 4 + KLB_RTCP_RR_HEADS + i * KLB_RTCP_BLOCK_LEN;
        if ((off + KLB_RTCP_BLOCK_LEN) > total)
        {
            break;
        }

        get_block_klb_rtp(p_data + off, p_rr->blocks + i);
        p_rr->block_num++;
    }

    return 0;
}


int klb_rtcp_twcc_build(uint32_t sender_ssrc, uint32_t media_ssrc, uint16_t base_seq,
                        const uint8_t* p_status, const uint8_t* p_delta, int count,
                        uint32_t ref_time, uint8_t fb_count, uint8_t* p_out, int cap)
{
    if ((NULL == p_status) || (NULL == p_out) || (0 >= count) || (0xFFFF < count))
    {
        return KLB_NETCODE_EINVAL;
    }

    // step1. 统计游程块数与到达时间增量字节数 (每 1 个已收包 1 字节)
    int chunks = 0;
    int deltas = 0;
    for (int i = 0; i < count; )
    {
        int j = i + 1;
        while ((j < count) && (p_status[j] == p_status[i]) && ((j - i) < KLB_RTCP_TWCC_RUN_MAX))
        {
            j++;
        }

        chunks++;
        if (0 != p_status[i])
        {
            deltas += (j - i);
        }

        i = j;
    }

    int total = KLB_RTCP_FB_HEADS + KLB_RTCP_TWCC_HEADS + chunks * 2 + deltas;
    total = (total + 3) & ~3;                   // RTCP 包按 32 位对齐
    if (total > cap)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    // step2. 公共头与 TWCC 公共字段
    put_fb_head_klb_rtp(p_out, 15, KLB_RTCP_RTPFB, total, sender_ssrc, media_ssrc);

    uint8_t* p_tw = p_out + KLB_RTCP_FB_HEADS;
    put_u16_klb_rtp(p_tw, base_seq);
    put_u16_klb_rtp(p_tw + 2, (uint16_t)count);
    put_u24_klb_rtp(p_tw + 4, ref_time & 0x00FFFFFFu);
    p_tw[7] = fb_count;

    // step3. 游程块: T=0 | 游程长度(13) | 状态(2)
    uint8_t* p_chunk = p_tw + KLB_RTCP_TWCC_HEADS;
    for (int i = 0; i < count; )
    {
        int j = i + 1;
        while ((j < count) && (p_status[j] == p_status[i]) && ((j - i) < KLB_RTCP_TWCC_RUN_MAX))
        {
            j++;
        }

        uint16_t run = (uint16_t)(j - i);
        uint16_t symbol = (0 != p_status[i]) ? 1 : 0;
        put_u16_klb_rtp(p_chunk, (uint16_t)((run << 2) | symbol));
        p_chunk += 2;

        if (0 != symbol)
        {
            // 到达时间增量: 每已收包 1 字节
            for (int k = 0; k < run; k++)
            {
                p_chunk[k] = (NULL != p_delta) ? p_delta[i + k] : 0;
            }

            p_chunk += run;
        }

        i = j;
    }

    // step4. 尾部对齐填充置 0
    if (p_chunk < (p_out + total))
    {
        memset(p_chunk, 0, (size_t)(p_out + total - p_chunk));
    }

    return total;
}


int klb_rtcp_twcc_parse(const uint8_t* p_data, int len, klb_rtcp_twcc_t* p_twcc,
                        uint8_t* p_status, int cap)
{
    if ((NULL == p_data) || (NULL == p_twcc) || (NULL == p_status))
    {
        return KLB_NETCODE_EINVAL;
    }

    klb_rtcp_head_t head;
    int ret = klb_rtcp_head_parse(p_data, len, &head);
    if (0 > ret)
    {
        return ret;
    }

    if ((KLB_RTCP_RTPFB != head.type) || (15 != head.count))
    {
        return KLB_NETCODE_PROTO_UNEXPECTED;
    }

    int total = ((int)head.length + 1) * 4;
    if ((KLB_RTCP_FB_HEADS + KLB_RTCP_TWCC_HEADS) > total)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    KLB_MEMSET(p_twcc, 0, sizeof(*p_twcc));
    p_twcc->sender_ssrc = get_u32_klb_rtp(p_data + 4);
    p_twcc->media_ssrc = get_u32_klb_rtp(p_data + 8);

    const uint8_t* p_tw = p_data + KLB_RTCP_FB_HEADS;
    p_twcc->base_seq = get_u16_klb_rtp(p_tw);
    p_twcc->count = get_u16_klb_rtp(p_tw + 2);
    p_twcc->ref_time = get_u24_klb_rtp(p_tw + 4);
    p_twcc->fb_count = p_tw[7];

    int count = (int)p_twcc->count;
    if (cap < count)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    // step1. 解块: 游程块 / 状态向量块; 到达时间增量按符号宽度跳过
    int off = KLB_RTCP_FB_HEADS + KLB_RTCP_TWCC_HEADS;
    int idx = 0;
    while ((idx < count) && ((off + 2) <= total))
    {
        uint16_t word = get_u16_klb_rtp(p_data + off);
        off += 2;

        if (0 == (word & 0x8000))
        {
            int run = (int)((word >> 2) & 0x1FFF);
            int symbol = (int)(word & 0x03);
            if (0 == run)
            {
                break;
            }

            for (int k = 0; (k < run) && (idx < count); k++)
            {
                p_status[idx++] = (uint8_t)((0 != symbol) ? 1 : 0);
            }

            if (1 == symbol)
            {
                off += run;
            }
            else if (2 == symbol)
            {
                off += run * 2;
            }

            continue;
        }

        if (0 == (word & 0x4000))
        {
            int delta = 0;
            for (int k = 0; k < 14; k++)
            {
                int bit = (word >> (13 - k)) & 0x01;
                if (idx < count)
                {
                    p_status[idx++] = (uint8_t)bit;
                }

                if (0 != bit)
                {
                    delta++;
                }
            }

            off += delta;
        }
        else
        {
            int delta = 0;
            for (int k = 0; k < 7; k++)
            {
                int symbol = (word >> (12 - k * 2)) & 0x03;
                if (idx < count)
                {
                    p_status[idx++] = (uint8_t)((0 != symbol) ? 1 : 0);
                }

                if (1 == symbol)
                {
                    delta += 1;
                }
                else if (2 == symbol)
                {
                    delta += 2;
                }
            }

            off += delta;
        }
    }

    return idx;
}


// end
