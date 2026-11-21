// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnp_crypto.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   MNP/SMP 帧内 CRYPTO_PSK 加码; 分方向状态; XOR/AES_*/ChaCha20 均可选
/// @version 0.5
/// @history 修改历史
///  \n [2026-10] 0.3 禁 AES_ECB/XOR(弱算法); fill_key 改 CSPRNG; 复用约束用换钥(rekey); 0.2 AES_CTR/OFB/ECB/CBC + ChaCha20; xform 增 encrypt; 创建文件
/// @warning XOR/AES_ECB 为弱算法; 仅显式选型时使用, 默认仍 CHACHA20/AES_CTR
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNP_CRYPTO_H__
#define __KLB_MNP_CRYPTO_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 加码错误码(与 codec 数值对齐习惯)
typedef enum klb_mnp_crypto_err_e_
{
    KLB_MNP_CRYPTO_OK       = 0,    ///< 成功
    KLB_MNP_CRYPTO_EPROTO   = 1,    ///< 协议/状态非法(保持旧态)
    KLB_MNP_CRYPTO_ENOSUP   = 2,    ///< 本端不支持该 format(保持旧态)
}klb_mnp_crypto_err_e;


/// @brief 单方向(发送或接收)加码状态
typedef struct klb_mnp_crypto_side_t_
{
    uint32_t        format;     ///< klb_mnp_crypto_e; NULL=无加码
    uint32_t        key_size;   ///< 密钥材料长度
    uint8_t*        p_key;      ///< 密钥材料; 本侧持有
}klb_mnp_crypto_side_t;


/// @brief 连接加码策略: 发送/接收两套互不干扰
typedef struct klb_mnp_crypto_policy_t_
{
    klb_mnp_crypto_side_t   tx;     ///< 本端发送加码
    klb_mnp_crypto_side_t   rx;     ///< 本端接收解码
}klb_mnp_crypto_policy_t;


/// @brief 初始化策略(默认两侧 CRYPTO_NULL)
KLB_API void klb_mnp_crypto_policy_init(klb_mnp_crypto_policy_t* p_pol);


/// @brief 释放两侧密钥并复位为 NULL
KLB_API void klb_mnp_crypto_policy_clear(klb_mnp_crypto_policy_t* p_pol);


/// @brief 本端是否支持该 format(NULL / XOR / AES_CTR / AES_OFB / AES_ECB / AES_CBC / CHACHA20)
/// @note  0.4 起 XOR/AES_ECB 重新开放(弱; 须客户端显式选型)
KLB_API bool klb_mnp_crypto_format_supported(uint32_t format);


/// @brief packtype 是否受 crypto 控制(子头之后加/解码)
KLB_API bool klb_mnp_crypto_packtype_controlled(int packtype);


/// @brief packtype 是否完全不受控(PING/PONG/OPTIONS)
KLB_API bool klb_mnp_crypto_packtype_exempt(int packtype);


/// @brief 业务子头字节数; 未知返回 -1
KLB_API int klb_mnp_crypto_subhdr_size(int packtype);


/// @brief 切换单侧状态; 失败保持旧态
/// @note  禁止加密态切回 NULL; 同 format 可换钥与 key_size
KLB_API int klb_mnp_crypto_apply_psk(klb_mnp_crypto_side_t* p_side,
    uint32_t format, const uint8_t* p_key, uint32_t key_size);


/// @brief 设置本端发送侧(TX); 失败保持旧 TX
KLB_API int klb_mnp_crypto_set_tx(klb_mnp_crypto_policy_t* p_pol,
    uint32_t format, const uint8_t* p_key, uint32_t key_size);


/// @brief 解析 OPTIONS 载荷并应用到 RX; 非 CRYPTO_PSK 返回 OK 且不改状态
/// @note  校验失败返回非 0, RX 维持原样(调用方勿因此断连)
KLB_API int klb_mnp_crypto_on_options_recv(klb_mnp_crypto_policy_t* p_pol,
    const uint8_t* p_payload, int payload_len);


/// @brief 组装 CRYPTO_PSK OPTIONS 载荷(不含 smp/mnp 帧头)
/// @return klb_buf_t* 成功非 NULL; 失败 NULL
KLB_API klb_buf_t* klb_mnp_crypto_pack_psk(int sequence,
    uint32_t format, const uint8_t* p_key, uint32_t key_size);


/// @brief 对业务载荷子头之后做加/解码; format=NULL 为无操作
/// @param [in] p_side        本侧方向状态; IV/nonce 取自密钥材料尾部
/// @param [in] encrypt       true=TX 加密; false=RX 解密(对称算法同向可逆)
/// @note  密钥材料布局见下宏; keystream 复用的约束手段为换钥(rekey_after)
KLB_API int klb_mnp_crypto_xform_payload(const klb_mnp_crypto_side_t* p_side,
    int packtype, uint8_t* p_data, int data_len, bool encrypt);


/// 密钥材料上限(与 pack_psk 硬顶一致)
#define KLB_MNP_CRYPTO_KEY_SIZE_MAX     ((uint32_t)(KLB_MNP_BLOCK_SIZE_MAX - 8 - 12 - 8))

/// AES_CTR: key[16]+iv[8]
#define KLB_MNP_CRYPTO_AES_CTR_KEY_N    (24u)
/// AES_OFB / AES_CBC: key[16]+iv[16]
#define KLB_MNP_CRYPTO_AES_IV_KEY_N     (32u)
/// AES_ECB: key[16]
#define KLB_MNP_CRYPTO_AES_ECB_KEY_N    (16u)
/// ChaCha20: key[32]+nonce[12]
#define KLB_MNP_CRYPTO_CHACHA20_KEY_N   (44u)
/// XOR: 循环密钥最小长度
#define KLB_MNP_CRYPTO_XOR_KEY_MIN      (1u)


/// @brief 填充随机密钥材料(CSPRNG: 系统熵源 /dev/urandom | BCrypt | arc4random)
KLB_API void klb_mnp_crypto_fill_key(uint8_t* p_key, uint32_t key_size);


/// @brief 按名解析 format; 成功写 *p_format 并返回 0; 失败返回非 0
/// @note  名: XOR / AES_CTR / AES_OFB / AES_ECB / CHACHA20 / AES_CBC(大小写不敏感)
KLB_API int klb_mnp_crypto_format_from_name(const char* p_name, uint32_t* p_format);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_MNP_CRYPTO_H__

// end
