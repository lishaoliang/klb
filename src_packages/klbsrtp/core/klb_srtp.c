// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_srtp.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SRTP/SRTCP 包保护 (RFC 3711; AES128_CM_HMAC_SHA1_80)
/// @version 0.4
/// @history 修改历史
///  \n 2026 0.1 创建文件(桩)
///  \n [2026-10] 0.2 实现 KDF / AES-CM / HMAC-SHA1 / 重放窗口
///  \n [2026-10] 0.3 增加可选 MKI 与 SRTCP 64 位重放窗口
///  \n [2026-10] 0.4 增加允许重传包 (NACK 重发同序号) 开关
/// @warning 仅支持 AES128_CM_HMAC_SHA1_80; 每上下文最多 8 个 SSRC
///  \n SRTCP 按单包处理: 仅加密首个 RTCP 包头(8 字节)之后的部分
///////////////////////////////////////////////////////////////////////////
#include "klbsrtp/core/klb_srtp.h"
#include "klbnet/klb_netcode.h"

#if defined(__KLB_MBEDTLS__)

#include "klbmem/klb_mem.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include <string.h>


/// @def   KLB_SRTP_MAX_SSRC
/// @brief 每上下文支持的 SSRC 状态表容量
#define KLB_SRTP_MAX_SSRC               8

/// @def   KLB_SRTP_AUTH_TAG_LEN
/// @brief 认证标签截断长度 (HMAC-SHA1 取前 10 字节)
#define KLB_SRTP_AUTH_TAG_LEN           10

/// @def   KLB_SRTP_HMAC_LEN
/// @brief HMAC-SHA1 完整摘要长度
#define KLB_SRTP_HMAC_LEN               20

/// @def   KLB_SRTP_SESSION_KEY_LEN
/// @brief 会话加密密钥长度 (AES-128)
#define KLB_SRTP_SESSION_KEY_LEN        16

/// @def   KLB_SRTP_AUTH_KEY_LEN
/// @brief 会话认证密钥长度 (HMAC-SHA1 20 字节)
#define KLB_SRTP_AUTH_KEY_LEN           20

/// @def   KLB_SRTP_SESSION_SALT_LEN
/// @brief 会话盐长度 (14 字节)
#define KLB_SRTP_SESSION_SALT_LEN       14

/// @def   KLB_SRTP_BLOCK_LEN
/// @brief AES 分组长度
#define KLB_SRTP_BLOCK_LEN              16

/// @def   KLB_SRTP_RTP_HDR_LEN
/// @brief RTP 固定头长度
#define KLB_SRTP_RTP_HDR_LEN            12

/// @def   KLB_SRTP_SRTCP_HDR_LEN
/// @brief SRTCP 固定头长度
#define KLB_SRTP_SRTCP_HDR_LEN          8

/// @def   KLB_SRTP_SRTCP_TRAILER_LEN
/// @brief SRTCP 尾部 E 标志 + 31 位索引长度
#define KLB_SRTP_SRTCP_TRAILER_LEN      4

/// @dev   KLB_SRTP_SRTCP_E_FLAG
/// @brief SRTCP 索引字最高位 (E 标志)
#define KLB_SRTP_SRTCP_E_FLAG           0x80000000u

/// @def   KLB_SRTP_SRTCP_INDEX_MASK
/// @brief SRTCP 31 位索引掩码
#define KLB_SRTP_SRTCP_INDEX_MASK       0x7FFFFFFFu

/// @def   KLB_SRTP_KDF_LABEL_ENC
/// @brief KDF 标签: 会话加密密钥
#define KLB_SRTP_KDF_LABEL_ENC          0x00

/// @def   KLB_SRTP_KDF_LABEL_AUTH
/// @brief KDF 标签: 会话认证密钥
#define KLB_SRTP_KDF_LABEL_AUTH         0x01

/// @def   KLB_SRTP_KDF_LABEL_SALT
/// @brief KDF 标签: 会话盐
#define KLB_SRTP_KDF_LABEL_SALT         0x02


/// @struct klb_srtp_stream_t
/// @brief 单 SSRC 的收发状态
typedef struct klb_srtp_stream_t_
{
    uint32_t    ssrc;           ///< 同步源标识
    uint32_t    roc;            ///< RTP 回绕计数器 (rollover counter)
    uint32_t    srtcp_index;    ///< SRTCP 发送索引 (31 位)
    uint32_t    srtcp_last;     ///< 最近一次 SRTCP 接收索引 (31 位)
    uint64_t    srtcp_replay;   ///< SRTCP 重放位图; bit0 对应最高索引
    uint16_t    seq_last;       ///< 最近一次 RTP 序号
    uint64_t    replay_map;     ///< 重放位图; bit0 对应最高序号
    bool        tx_valid;       ///< 发送序号已初始化
    bool        rx_valid;       ///< 接收序号已初始化
    bool        srtcp_rx_valid; ///< SRTCP 接收索引已初始化
}klb_srtp_stream_t;


/// @struct klb_srtp_ctx_t_
/// @brief SRTP 上下文 (对外不透明)
struct klb_srtp_ctx_t_
{
    int                     profile;            ///< 保护套件
    bool                    key_set;            ///< 密钥已装入
    uint8_t                 master_key[KLB_SRTP_MASTER_KEY_MAX];        ///< 主密钥
    uint8_t                 master_salt[KLB_SRTP_MASTER_SALT_MAX];      ///< 主盐
    uint8_t                 session_key[KLB_SRTP_SESSION_KEY_LEN];      ///< 会话加密密钥
    uint8_t                 auth_key[KLB_SRTP_AUTH_KEY_LEN];            ///< 会话认证密钥
    uint8_t                 session_salt[KLB_SRTP_SESSION_SALT_LEN];    ///< 会话盐
    mbedtls_aes_context     aes;                ///< AES 上下文 (会话密钥)
    mbedtls_md_context_t    hmac;               ///< HMAC-SHA1 上下文 (认证密钥)
    int                     stream_num;         ///< 已用 SSRC 数
    int                     mki_len;            ///< MKI 长度; 0 表不携带
    bool                    repeat_tx;          ///< 允许重复索引 (NACK 重传)
    uint8_t                 mki[KLB_SRTP_MKI_MAX];  ///< MKI 字节
    klb_srtp_stream_t       streams[KLB_SRTP_MAX_SSRC];                 ///< SSRC 状态表
};


/// @brief AES-CM 计数器模式异或; 原地处理
/// @param [in] *p_aes      AES 上下文 (须已装密钥)
/// @param [in] p_iv        16 字节初始向量; 末 2 字节为块计数器
/// @param [in,out] *p_data 数据; 长度 len
/// @param [in] len         数据长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int ctr_xor_klb_srtp(mbedtls_aes_context* p_aes, const uint8_t p_iv[KLB_SRTP_BLOCK_LEN],
                            uint8_t* p_data, int len)
{
    uint8_t iv[KLB_SRTP_BLOCK_LEN];
    uint8_t block[KLB_SRTP_BLOCK_LEN];

    memcpy(iv, p_iv, sizeof(iv));

    int off = 0;
    while (off < len)
    {
        int ret = mbedtls_aes_crypt_ecb(p_aes, MBEDTLS_AES_ENCRYPT, iv, block);
        if (0 != ret)
        {
            return KLB_NETCODE_EIO;
        }

        int n = len - off;
        if (n > KLB_SRTP_BLOCK_LEN)
        {
            n = KLB_SRTP_BLOCK_LEN;
        }

        for (int i = 0; i < n; ++i)
        {
            p_data[off + i] ^= block[i];
        }

        off += n;

        // 块计数器: IV 末 2 字节按大端递增
        uint16_t counter = (uint16_t)(((uint16_t)iv[14] << 8) | iv[15]);
        counter = (uint16_t)(counter + 1);
        iv[14] = (uint8_t)(counter >> 8);
        iv[15] = (uint8_t)counter;
    }

    return 0;
}


/// @brief RFC 3711 4.3 节 KDF: 由主密钥/主盐派生会话材料 (key_derivation_rate = 0)
/// @param [in] *p_ctx      上下文; aes 须已装主密钥
/// @param [in] label       KDF 标签
/// @param [out] *p_out     输出缓冲
/// @param [in] out_len     输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note IV = 主盐(14B, 右补 0 到 16B); label 异或到字节 7; 块计数器置 0
static int kdf_derive_klb_srtp(klb_srtp_ctx_t* p_ctx, uint8_t label, uint8_t* p_out, int out_len)
{
    uint8_t iv[KLB_SRTP_BLOCK_LEN];

    KLB_MEMSET(iv, 0, sizeof(iv));
    memcpy(iv, p_ctx->master_salt, KLB_SRTP_MASTER_SALT_MAX);
    iv[7] ^= label;

    KLB_MEMSET(p_out, 0, (size_t)out_len);

    return ctr_xor_klb_srtp(&p_ctx->aes, iv, p_out, out_len);
}


/// @brief 构造 SRTP/SRTCP 的 AES-CM 初始向量
/// @param [in] *p_ctx      上下文 (取会话盐)
/// @param [in] ssrc        同步源
/// @param [in] index       48 位包索引 (SRTCP 为 E 标志 + 31 位索引)
/// @param [out] iv         16 字节初始向量
/// @return 无
static void make_iv_klb_srtp(const klb_srtp_ctx_t* p_ctx, uint32_t ssrc, uint64_t index,
                             uint8_t iv[KLB_SRTP_BLOCK_LEN])
{
    KLB_MEMSET(iv, 0, KLB_SRTP_BLOCK_LEN);
    memcpy(iv, p_ctx->session_salt, KLB_SRTP_SESSION_SALT_LEN);

    // SSRC 异或到字节 4..7
    iv[4] ^= (uint8_t)(ssrc >> 24);
    iv[5] ^= (uint8_t)(ssrc >> 16);
    iv[6] ^= (uint8_t)(ssrc >> 8);
    iv[7] ^= (uint8_t)ssrc;

    // 包索引异或到字节 8..13 (48 位大端)
    iv[8]  ^= (uint8_t)(index >> 40);
    iv[9]  ^= (uint8_t)(index >> 32);
    iv[10] ^= (uint8_t)(index >> 24);
    iv[11] ^= (uint8_t)(index >> 16);
    iv[12] ^= (uint8_t)(index >> 8);
    iv[13] ^= (uint8_t)index;
}


/// @brief 计算 HMAC-SHA1 摘要
/// @param [in] *p_ctx      上下文
/// @param [in] *p_pkt      主数据
/// @param [in] len         主数据长度
/// @param [in] *p_tail     附加数据; 可为 NULL
/// @param [in] tail_len    附加数据长度
/// @param [out] *p_tag     20 字节摘要输出
/// @return int 0.成功; 小于 0.klb_netcode_e
static int hmac_calc_klb_srtp(klb_srtp_ctx_t* p_ctx, const uint8_t* p_pkt, int len,
                              const uint8_t* p_tail, int tail_len, uint8_t* p_tag)
{
    int ret = mbedtls_md_hmac_reset(&p_ctx->hmac);
    if (0 != ret)
    {
        return KLB_NETCODE_EIO;
    }

    ret = mbedtls_md_hmac_update(&p_ctx->hmac, p_pkt, (size_t)len);
    if (0 != ret)
    {
        return KLB_NETCODE_EIO;
    }

    if (NULL != p_tail && tail_len > 0)
    {
        ret = mbedtls_md_hmac_update(&p_ctx->hmac, p_tail, (size_t)tail_len);
        if (0 != ret)
        {
            return KLB_NETCODE_EIO;
        }
    }

    ret = mbedtls_md_hmac_finish(&p_ctx->hmac, p_tag);
    if (0 != ret)
    {
        return KLB_NETCODE_EIO;
    }

    return 0;
}


/// @brief 认证标签定长时间比较
/// @param [in] *p_a         标签 A
/// @param [in] *p_b         标签 B
/// @param [in] len          比较长度
/// @return bool true.相同; false.不同
static bool tag_equal_klb_srtp(const uint8_t* p_a, const uint8_t* p_b, int len)
{
    uint8_t diff = 0;

    for (int i = 0; i < len; ++i)
    {
        diff |= (uint8_t)(p_a[i] ^ p_b[i]);
    }

    return (0 == diff);
}


/// @brief 按 SSRC 查找状态; 不存在时按需创建
/// @param [in] *p_ctx      上下文
/// @param [in] ssrc        同步源
/// @param [in] create      是否允许新建
/// @return klb_srtp_stream_t* NULL.未找到/表满; 非 NULL.状态
static klb_srtp_stream_t* stream_get_klb_srtp(klb_srtp_ctx_t* p_ctx, uint32_t ssrc, bool create)
{
    for (int i = 0; i < p_ctx->stream_num; ++i)
    {
        if (ssrc == p_ctx->streams[i].ssrc)
        {
            return &p_ctx->streams[i];
        }
    }

    if (!create)
    {
        return NULL;
    }

    if (p_ctx->stream_num >= KLB_SRTP_MAX_SSRC)
    {
        return NULL;
    }

    klb_srtp_stream_t* p_stream = &p_ctx->streams[p_ctx->stream_num];
    p_ctx->stream_num++;
    KLB_MEMSET(p_stream, 0, sizeof(*p_stream));
    p_stream->ssrc = ssrc;

    return p_stream;
}


/// @brief 解析 RTP 头; 跳过 CSRC 与扩展头
/// @param [in] *p_pkt      RTP 包
/// @param [in] len         包长
/// @param [out] *p_off     payload 起始偏移
/// @return int 0.成功; 小于 0.klb_netcode_e
static int rtp_payload_offset_klb_srtp(const uint8_t* p_pkt, int len, int* p_off)
{
    if (len < KLB_SRTP_RTP_HDR_LEN)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (2 != (p_pkt[0] >> 6))
    {
        return KLB_NETCODE_PROTO_VERSION;
    }

    int off = KLB_SRTP_RTP_HDR_LEN + 4 * (p_pkt[0] & 0x0F);
    if (off > len)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    // 扩展头: 4 字节 (profile 2 + length 2); length 以 32 位字计
    if (0 != (p_pkt[0] & 0x10))
    {
        if ((off + 4) > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }

        int ext_words = ((int)p_pkt[off + 2] << 8) | (int)p_pkt[off + 3];
        off += 4 + 4 * ext_words;
        if (off > len)
        {
            return KLB_NETCODE_PROTO_PARSE;
        }
    }

    *p_off = off;

    return 0;
}


/// @brief 重放窗口校验; 接受时更新位图与最高序号
/// @param [in,out] *p_stream   单 SSRC 状态
/// @param [in] roc             包所属 ROC
/// @param [in] seq             RTP 序号
/// @return bool true.接受; false.重复或窗口外
static bool replay_accept_klb_srtp(klb_srtp_ctx_t* p_ctx, klb_srtp_stream_t* p_stream, uint32_t roc, uint16_t seq)
{
    uint64_t index = ((uint64_t)roc << 16) | seq;

    if (!p_stream->rx_valid)
    {
        p_stream->rx_valid = true;
        p_stream->roc = roc;
        p_stream->seq_last = seq;
        p_stream->replay_map = 1;

        return true;
    }

    uint64_t highest = ((uint64_t)p_stream->roc << 16) | p_stream->seq_last;
    if (index > highest)
    {
        uint64_t delta = index - highest;
        if (delta >= 64)
        {
            p_stream->replay_map = 0;
        }
        else
        {
            p_stream->replay_map <<= (unsigned int)delta;
        }

        p_stream->replay_map |= 1;
        p_stream->roc = roc;
        p_stream->seq_last = seq;

        return true;
    }

    uint64_t delta = highest - index;
    if (delta >= 64)
    {
        return false;
    }

    uint64_t mask = (uint64_t)1 << delta;
    if (0 != (p_stream->replay_map & mask))
    {
        // 已接收过的索引: 开启 repeat_tx 时按 NACK 重传包放行
        return p_ctx->repeat_tx;
    }

    p_stream->replay_map |= mask;

    return true;
}


/// @brief SRTCP 重放窗口校验; 接受时更新位图与最高索引
/// @param [in,out] *p_stream   单 SSRC 状态
/// @param [in] index           SRTCP 31 位索引
/// @return bool true.接受; false.重复或窗口外
static bool replay_accept_srtcp_klb_srtp(klb_srtp_stream_t* p_stream, uint32_t index)
{
    index &= KLB_SRTP_SRTCP_INDEX_MASK;

    if (!p_stream->srtcp_rx_valid)
    {
        p_stream->srtcp_rx_valid = true;
        p_stream->srtcp_last = index;
        p_stream->srtcp_replay = 1;

        return true;
    }

    if (index > p_stream->srtcp_last)
    {
        uint32_t delta = index - p_stream->srtcp_last;
        if (delta >= 64)
        {
            p_stream->srtcp_replay = 0;
        }
        else
        {
            p_stream->srtcp_replay <<= delta;
        }

        p_stream->srtcp_replay |= 1;
        p_stream->srtcp_last = index;

        return true;
    }

    uint32_t delta = p_stream->srtcp_last - index;
    if (delta >= 64)
    {
        return false;
    }

    uint64_t mask = (uint64_t)1 << delta;
    if (0 != (p_stream->srtcp_replay & mask))
    {
        return false;
    }

    p_stream->srtcp_replay |= mask;

    return true;
}


/// @brief 保护 RTP 包 (原地加密并追加认证标签)
/// @param [in] *p_ctx      上下文
/// @param [in,out] *p_pkt  RTP 包
/// @param [in] len         输入长度
/// @param [in] cap          缓存容量
/// @param [out] *p_out_len 输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int protect_rtp_klb_srtp(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int cap, int* p_out_len)
{
    int off = 0;
    int ret = rtp_payload_offset_klb_srtp(p_pkt, len, &off);
    if (0 != ret)
    {
        return ret;
    }

    if (cap < len || (cap - len) < (KLB_SRTP_AUTH_TAG_LEN + p_ctx->mki_len))
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    uint16_t seq = KLB_RB16(p_pkt + 2);
    uint32_t ssrc = KLB_RB32(p_pkt + 8);
    klb_srtp_stream_t* p_stream = stream_get_klb_srtp(p_ctx, ssrc, true);
    if (NULL == p_stream)
    {
        return KLB_NETCODE_LIMIT;
    }

    // step1. 发送侧 ROC 维护 (序号回绕判定)
    if (p_stream->tx_valid)
    {
        if (p_stream->seq_last > 0x8000 && seq < 0x8000)
        {
            p_stream->roc++;
        }
        else if (p_stream->seq_last < 0x8000 && seq > 0x8000 && p_stream->roc > 0)
        {
            p_stream->roc--;
        }
    }
    else
    {
        p_stream->tx_valid = true;
    }

    uint64_t index = ((uint64_t)p_stream->roc << 16) | seq;

    // step2. AES-CM 加密 payload
    uint8_t iv[KLB_SRTP_BLOCK_LEN];
    make_iv_klb_srtp(p_ctx, ssrc, index, iv);

    ret = ctr_xor_klb_srtp(&p_ctx->aes, iv, p_pkt + off, len - off);
    if (0 != ret)
    {
        return ret;
    }

    // step3. 追加 MKI (位于加密载荷之后, 认证标签之前)
    int body_len = len;
    if (0 < p_ctx->mki_len)
    {
        memcpy(p_pkt + body_len, p_ctx->mki, (size_t)p_ctx->mki_len);
        body_len += p_ctx->mki_len;
    }

    // step4. HMAC-SHA1(加密后整包(含 MKI) || ROC)
    uint8_t roc_be[4];
    uint8_t tag[KLB_SRTP_HMAC_LEN];
    KLB_WB32(roc_be, p_stream->roc);

    ret = hmac_calc_klb_srtp(p_ctx, p_pkt, body_len, roc_be, (int)sizeof(roc_be), tag);
    if (0 != ret)
    {
        return ret;
    }

    memcpy(p_pkt + body_len, tag, KLB_SRTP_AUTH_TAG_LEN);

    // step5. 提交状态
    p_stream->seq_last = seq;
    *p_out_len = body_len + KLB_SRTP_AUTH_TAG_LEN;

    return 0;
}


/// @brief 保护 SRTCP 包 (原地加密并追加 E+索引与认证标签)
/// @param [in] *p_ctx      上下文
/// @param [in,out] *p_pkt  RTCP 包
/// @param [in] len         输入长度
/// @param [in] cap          缓存容量
/// @param [out] *p_out_len 输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int protect_rtcp_klb_srtp(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int cap, int* p_out_len)
{
    if (len < KLB_SRTP_SRTCP_HDR_LEN)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (2 != (p_pkt[0] >> 6))
    {
        return KLB_NETCODE_PROTO_VERSION;
    }

    int need = KLB_SRTP_SRTCP_TRAILER_LEN + p_ctx->mki_len + KLB_SRTP_AUTH_TAG_LEN;
    if (cap < len || (cap - len) < need)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    uint32_t ssrc = KLB_RB32(p_pkt + 4);
    klb_srtp_stream_t* p_stream = stream_get_klb_srtp(p_ctx, ssrc, true);
    if (NULL == p_stream)
    {
        return KLB_NETCODE_LIMIT;
    }

    // step1. 取当前 SRTCP 索引; IV 用 E 标志 + 31 位索引
    uint32_t srtcp_index = p_stream->srtcp_index & KLB_SRTP_SRTCP_INDEX_MASK;
    uint8_t iv[KLB_SRTP_BLOCK_LEN];
    make_iv_klb_srtp(p_ctx, ssrc, (uint64_t)(srtcp_index | KLB_SRTP_SRTCP_E_FLAG), iv);

    // step2. 加密第 8 字节起的 payload
    int ret = ctr_xor_klb_srtp(&p_ctx->aes, iv, p_pkt + KLB_SRTP_SRTCP_HDR_LEN,
                               len - KLB_SRTP_SRTCP_HDR_LEN);
    if (0 != ret)
    {
        return ret;
    }

    // step3. 追加 E+索引
    KLB_WB32(p_pkt + len, srtcp_index | KLB_SRTP_SRTCP_E_FLAG);

    // step4. 追加 MKI (位于 E+索引之后, 认证标签之前)
    int body_len = len + KLB_SRTP_SRTCP_TRAILER_LEN;
    if (0 < p_ctx->mki_len)
    {
        memcpy(p_pkt + body_len, p_ctx->mki, (size_t)p_ctx->mki_len);
        body_len += p_ctx->mki_len;
    }

    // step5. HMAC-SHA1(含 E+索引与 MKI 的整包)
    uint8_t tag[KLB_SRTP_HMAC_LEN];
    ret = hmac_calc_klb_srtp(p_ctx, p_pkt, body_len, NULL, 0, tag);
    if (0 != ret)
    {
        return ret;
    }

    memcpy(p_pkt + body_len, tag, KLB_SRTP_AUTH_TAG_LEN);

    // step6. 提交状态
    p_stream->srtcp_index = (srtcp_index + 1) & KLB_SRTP_SRTCP_INDEX_MASK;
    *p_out_len = body_len + KLB_SRTP_AUTH_TAG_LEN;

    return 0;
}


/// @brief 解保护 RTP 包 (校验认证标签, 重放检查并原地解密)
/// @param [in] *p_ctx      上下文
/// @param [in,out] *p_pkt  SRTP 包
/// @param [in] len         输入长度
/// @param [out] *p_out_len 输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int unprotect_rtp_klb_srtp(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int* p_out_len)
{
    if (len < (KLB_SRTP_RTP_HDR_LEN + KLB_SRTP_AUTH_TAG_LEN + p_ctx->mki_len))
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    int body_len = len - KLB_SRTP_AUTH_TAG_LEN - p_ctx->mki_len;

    // step0. 校验 MKI
    if (0 < p_ctx->mki_len)
    {
        if (0 != memcmp(p_pkt + body_len, p_ctx->mki, (size_t)p_ctx->mki_len))
        {
            return KLB_NETCODE_EINVAL;
        }
    }

    int off = 0;
    int ret = rtp_payload_offset_klb_srtp(p_pkt, body_len, &off);
    if (0 != ret)
    {
        return ret;
    }

    uint16_t seq = KLB_RB16(p_pkt + 2);
    uint32_t ssrc = KLB_RB32(p_pkt + 8);
    klb_srtp_stream_t* p_stream = stream_get_klb_srtp(p_ctx, ssrc, true);
    if (NULL == p_stream)
    {
        return KLB_NETCODE_LIMIT;
    }

    // step1. 估计当前 ROC (序号回绕判定)
    uint32_t roc = p_stream->roc;
    if (p_stream->rx_valid)
    {
        if (p_stream->seq_last > 0x8000 && seq < 0x8000)
        {
            roc++;
        }
        else if (p_stream->seq_last < 0x8000 && seq > 0x8000 && roc > 0)
        {
            roc--;
        }
    }

    // step2. 校验 HMAC: 当前 ROC 失败则用 ROC-1 重试一次
    uint8_t roc_be[4];
    uint8_t tag[KLB_SRTP_HMAC_LEN];
    KLB_WB32(roc_be, roc);

    int sign_len = body_len + p_ctx->mki_len;
    const uint8_t* p_tag = p_pkt + sign_len;

    ret = hmac_calc_klb_srtp(p_ctx, p_pkt, sign_len, roc_be, (int)sizeof(roc_be), tag);
    if (0 != ret)
    {
        return ret;
    }

    if (!tag_equal_klb_srtp(tag, p_tag, KLB_SRTP_AUTH_TAG_LEN))
    {
        if (0 == roc)
        {
            return KLB_NETCODE_EINVAL;
        }

        roc--;
        KLB_WB32(roc_be, roc);

        ret = hmac_calc_klb_srtp(p_ctx, p_pkt, sign_len, roc_be, (int)sizeof(roc_be), tag);
        if (0 != ret)
        {
            return ret;
        }

        if (!tag_equal_klb_srtp(tag, p_tag, KLB_SRTP_AUTH_TAG_LEN))
        {
            return KLB_NETCODE_EINVAL;
        }
    }

    // step3. 重放窗口
    if (!replay_accept_klb_srtp(p_ctx, p_stream, roc, seq))
    {
        return KLB_NETCODE_EINVAL;
    }

    // step4. 解密 payload
    uint8_t iv[KLB_SRTP_BLOCK_LEN];
    make_iv_klb_srtp(p_ctx, ssrc, ((uint64_t)roc << 16) | seq, iv);

    ret = ctr_xor_klb_srtp(&p_ctx->aes, iv, p_pkt + off, body_len - off);
    if (0 != ret)
    {
        return ret;
    }

    *p_out_len = body_len;

    return 0;
}


/// @brief 解保护 SRTCP 包 (校验认证标签并原地解密)
/// @param [in] *p_ctx      上下文
/// @param [in,out] *p_pkt  SRTCP 包
/// @param [in] len         输入长度
/// @param [out] *p_out_len 输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
static int unprotect_rtcp_klb_srtp(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int* p_out_len)
{
    int need = KLB_SRTP_SRTCP_HDR_LEN + KLB_SRTP_SRTCP_TRAILER_LEN + p_ctx->mki_len
        + KLB_SRTP_AUTH_TAG_LEN;
    if (len < need)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    if (2 != (p_pkt[0] >> 6))
    {
        return KLB_NETCODE_PROTO_VERSION;
    }

    int body_len = len - KLB_SRTP_SRTCP_TRAILER_LEN - p_ctx->mki_len - KLB_SRTP_AUTH_TAG_LEN;

    // step1. 读取尾部 E+索引; 无 E 标志视为非法
    uint32_t word = KLB_RB32(p_pkt + body_len);
    if (0 == (word & KLB_SRTP_SRTCP_E_FLAG))
    {
        return KLB_NETCODE_EINVAL;
    }

    uint32_t srtcp_index = word & KLB_SRTP_SRTCP_INDEX_MASK;
    uint32_t ssrc = KLB_RB32(p_pkt + 4);
    klb_srtp_stream_t* p_stream = stream_get_klb_srtp(p_ctx, ssrc, true);
    if (NULL == p_stream)
    {
        return KLB_NETCODE_LIMIT;
    }

    // step2. 校验 MKI
    if (0 < p_ctx->mki_len)
    {
        if (0 != memcmp(p_pkt + body_len + KLB_SRTP_SRTCP_TRAILER_LEN,
                p_ctx->mki, (size_t)p_ctx->mki_len))
        {
            return KLB_NETCODE_EINVAL;
        }
    }

    // step3. 校验 HMAC (覆盖含 E+索引与 MKI 的整包)
    int sign_len = body_len + KLB_SRTP_SRTCP_TRAILER_LEN + p_ctx->mki_len;
    uint8_t tag[KLB_SRTP_HMAC_LEN];
    int ret = hmac_calc_klb_srtp(p_ctx, p_pkt, sign_len, NULL, 0, tag);
    if (0 != ret)
    {
        return ret;
    }

    if (!tag_equal_klb_srtp(tag, p_pkt + sign_len, KLB_SRTP_AUTH_TAG_LEN))
    {
        return KLB_NETCODE_EINVAL;
    }

    // step4. 重放窗口
    if (!replay_accept_srtcp_klb_srtp(p_stream, srtcp_index))
    {
        return KLB_NETCODE_EINVAL;
    }

    // step5. 解密第 8 字节起的 payload
    uint8_t iv[KLB_SRTP_BLOCK_LEN];
    make_iv_klb_srtp(p_ctx, ssrc, (uint64_t)(srtcp_index | KLB_SRTP_SRTCP_E_FLAG), iv);

    ret = ctr_xor_klb_srtp(&p_ctx->aes, iv, p_pkt + KLB_SRTP_SRTCP_HDR_LEN,
                           body_len - KLB_SRTP_SRTCP_HDR_LEN);
    if (0 != ret)
    {
        return ret;
    }

    *p_out_len = body_len;

    return 0;
}


klb_srtp_ctx_t* klb_srtp_create(int profile, int* p_code)
{
    if (KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80 != profile)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_EOPNOTSUPP;
        }

        return NULL;
    }

    klb_srtp_ctx_t* p_ctx = KLB_MALLOCZ(klb_srtp_ctx_t, 1, 0);
    p_ctx->profile = profile;
    p_ctx->key_set = false;

    mbedtls_aes_init(&p_ctx->aes);
    mbedtls_md_init(&p_ctx->hmac);

    const mbedtls_md_info_t* p_md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (NULL == p_md)
    {
        klb_srtp_destroy(p_ctx);
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_EOPNOTSUPP;
        }

        return NULL;
    }

    int ret = mbedtls_md_setup(&p_ctx->hmac, p_md, 1);
    if (0 != ret)
    {
        klb_srtp_destroy(p_ctx);
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_ENOMEM;
        }

        return NULL;
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_ctx;
}


void klb_srtp_destroy(klb_srtp_ctx_t* p_ctx)
{
    if (NULL == p_ctx)
    {
        return;
    }

    mbedtls_aes_free(&p_ctx->aes);
    mbedtls_md_free(&p_ctx->hmac);
    KLB_FREE(p_ctx);
}


int klb_srtp_set_key(klb_srtp_ctx_t* p_ctx, const uint8_t* p_key, int key_len,
                     const uint8_t* p_salt, int salt_len)
{
    if (NULL == p_ctx || NULL == p_key || NULL == p_salt)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (KLB_SRTP_MASTER_KEY_MAX != key_len || KLB_SRTP_MASTER_SALT_MAX != salt_len)
    {
        return KLB_NETCODE_EINVAL;
    }

    KLB_MEMSET(p_ctx->master_key, 0, sizeof(p_ctx->master_key));
    KLB_MEMSET(p_ctx->master_salt, 0, sizeof(p_ctx->master_salt));
    memcpy(p_ctx->master_key, p_key, (size_t)key_len);
    memcpy(p_ctx->master_salt, p_salt, (size_t)salt_len);

    // step1. 主密钥先装入 AES, 供 KDF 使用
    int ret = mbedtls_aes_setkey_enc(&p_ctx->aes, p_ctx->master_key, 128);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return KLB_NETCODE_EINVAL;
    }

    // step2. RFC 3711 4.3 节 KDF: 加密密钥 / 认证密钥 / 会话盐
    ret = kdf_derive_klb_srtp(p_ctx, KLB_SRTP_KDF_LABEL_ENC,
                              p_ctx->session_key, KLB_SRTP_SESSION_KEY_LEN);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return ret;
    }

    ret = kdf_derive_klb_srtp(p_ctx, KLB_SRTP_KDF_LABEL_AUTH,
                              p_ctx->auth_key, KLB_SRTP_AUTH_KEY_LEN);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return ret;
    }

    ret = kdf_derive_klb_srtp(p_ctx, KLB_SRTP_KDF_LABEL_SALT,
                              p_ctx->session_salt, KLB_SRTP_SESSION_SALT_LEN);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return ret;
    }

    // step3. AES 改用会话密钥; HMAC 装入认证密钥
    ret = mbedtls_aes_setkey_enc(&p_ctx->aes, p_ctx->session_key, 128);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return KLB_NETCODE_EINVAL;
    }

    ret = mbedtls_md_hmac_starts(&p_ctx->hmac, p_ctx->auth_key, KLB_SRTP_AUTH_KEY_LEN);
    if (0 != ret)
    {
        p_ctx->key_set = false;
        return KLB_NETCODE_EIO;
    }

    // step4. 复位所有 SSRC 状态
    KLB_MEMSET(p_ctx->streams, 0, sizeof(p_ctx->streams));
    p_ctx->stream_num = 0;
    p_ctx->key_set = true;

    return 0;
}


int klb_srtp_set_mki(klb_srtp_ctx_t* p_ctx, const uint8_t* p_mki, int mki_len)
{
    if (NULL == p_ctx)
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((0 > mki_len) || (KLB_SRTP_MKI_MAX < mki_len))
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((0 < mki_len) && (NULL == p_mki))
    {
        return KLB_NETCODE_EINVAL;
    }

    KLB_MEMSET(p_ctx->mki, 0, sizeof(p_ctx->mki));
    if (0 < mki_len)
    {
        memcpy(p_ctx->mki, p_mki, (size_t)mki_len);
    }

    p_ctx->mki_len = mki_len;

    return 0;
}


int klb_srtp_set_repeat(klb_srtp_ctx_t* p_ctx, int enable)
{
    if (NULL == p_ctx)
    {
        return KLB_NETCODE_EINVAL;
    }

    p_ctx->repeat_tx = (0 != enable) ? true : false;

    return 0;
}


int klb_srtp_protect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int cap, int* p_out_len)
{
    if (NULL == p_ctx || NULL == p_pkt || NULL == p_out_len || len < 0 || cap < 0)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (!p_ctx->key_set)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (len < 2)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    // PT 192..223 (RTCP 头 byte1 为完整 PT) 视为 RTCP, 其余按 RTP
    int pt = (int)p_pkt[1];
    if (pt >= 192 && pt <= 223)
    {
        return protect_rtcp_klb_srtp(p_ctx, p_pkt, len, cap, p_out_len);
    }

    return protect_rtp_klb_srtp(p_ctx, p_pkt, len, cap, p_out_len);
}


int klb_srtp_unprotect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int* p_out_len)
{
    if (NULL == p_ctx || NULL == p_pkt || NULL == p_out_len || len < 0)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (!p_ctx->key_set)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (len < 2)
    {
        return KLB_NETCODE_PROTO_PARSE;
    }

    int pt = (int)p_pkt[1];
    if (pt >= 192 && pt <= 223)
    {
        return unprotect_rtcp_klb_srtp(p_ctx, p_pkt, len, p_out_len);
    }

    return unprotect_rtp_klb_srtp(p_ctx, p_pkt, len, p_out_len);
}

#else // !__KLB_MBEDTLS__


klb_srtp_ctx_t* klb_srtp_create(int profile, int* p_code)
{
    (void)profile;

    if (NULL != p_code)
    {
        *p_code = KLB_NETCODE_EOPNOTSUPP;
    }

    return NULL;
}


void klb_srtp_destroy(klb_srtp_ctx_t* p_ctx)
{
    (void)p_ctx;
}


int klb_srtp_set_key(klb_srtp_ctx_t* p_ctx, const uint8_t* p_key, int key_len,
                     const uint8_t* p_salt, int salt_len)
{
    (void)p_ctx;
    (void)p_key;
    (void)key_len;
    (void)p_salt;
    (void)salt_len;

    return KLB_NETCODE_EOPNOTSUPP;
}


int klb_srtp_set_mki(klb_srtp_ctx_t* p_ctx, const uint8_t* p_mki, int mki_len)
{
    (void)p_ctx;
    (void)p_mki;
    (void)mki_len;

    return KLB_NETCODE_EOPNOTSUPP;
}


int klb_srtp_set_repeat(klb_srtp_ctx_t* p_ctx, int enable)
{
    (void)p_ctx;
    (void)enable;

    return KLB_NETCODE_EOPNOTSUPP;
}


int klb_srtp_protect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int cap, int* p_out_len)
{
    (void)p_ctx;
    (void)p_pkt;
    (void)len;
    (void)cap;
    (void)p_out_len;

    return KLB_NETCODE_EOPNOTSUPP;
}


int klb_srtp_unprotect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int* p_out_len)
{
    (void)p_ctx;
    (void)p_pkt;
    (void)len;
    (void)p_out_len;

    return KLB_NETCODE_EOPNOTSUPP;
}

#endif // __KLB_MBEDTLS__


// end
