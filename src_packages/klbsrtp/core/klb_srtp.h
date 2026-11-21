// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_srtp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SRTP/SRTCP 包保护 (RFC 3711, AES128_CM_HMAC_SHA1_80)
/// @version 0.5
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 实现 AES-CM 加解密 / HMAC-SHA1-80 / ROC 与 64 位重放窗口 / SRTCP E+索引; 0.3 增加可选 MKI 与 SRTCP 64 位重放窗口; 0.4 增加允许重传包 (NACK 重发同序号) 开关
/// @warning 仅 AES128_CM_HMAC_SHA1_80
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SRTP_H__
#define __KLB_SRTP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_SRTP_MASTER_KEY_MAX
/// @brief 主密钥最大长度 (AES-128)
#define KLB_SRTP_MASTER_KEY_MAX     16

/// @def   KLB_SRTP_MASTER_SALT_MAX
/// @brief 主盐最大长度
#define KLB_SRTP_MASTER_SALT_MAX    14

/// @def   KLB_SRTP_MKI_MAX
/// @brief MKI 最大长度
#define KLB_SRTP_MKI_MAX            16


/// @enum  klb_srtp_profile_e
/// @brief SRTP 保护套件
typedef enum klb_srtp_profile_e_
{
    KLB_SRTP_PROFILE_AES128_CM_HMAC_SHA1_80 = 0,    ///< SRTP_AES128_CM_HMAC_SHA1_80
}klb_srtp_profile_e;


/// @brief SRTP 上下文 (不透明)
typedef struct klb_srtp_ctx_t_ klb_srtp_ctx_t;


/// @brief 创建 SRTP 上下文
/// @param [in] profile         保护套件: klb_srtp_profile_e
/// @param [out] *p_code        可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_srtp_ctx_t* NULL.失败 (桩恒 NULL); 非 NULL.上下文
KLB_API klb_srtp_ctx_t* klb_srtp_create(int profile, int* p_code);


/// @brief 销毁 SRTP 上下文
/// @param [in] *p_ctx          上下文; 可为 NULL
/// @return 无
KLB_API void klb_srtp_destroy(klb_srtp_ctx_t* p_ctx);


/// @brief 设置密钥与盐 (通常来自 DTLS-SRTP 导出)
/// @param [in] *p_ctx          上下文
/// @param [in] *p_key          主密钥
/// @param [in] key_len         密钥长度
/// @param [in] *p_salt         主盐
/// @param [in] salt_len        盐长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_srtp_set_key(klb_srtp_ctx_t* p_ctx, const uint8_t* p_key, int key_len,
                             const uint8_t* p_salt, int salt_len);


/// @brief 设置 MKI (可选; 0 长度表示不携带)
/// @param [in] *p_ctx          上下文
/// @param [in] *p_mki          MKI 字节; mki_len 为 0 时可为 NULL
/// @param [in] mki_len         MKI 长度 (0 ~ KLB_SRTP_MKI_MAX)
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note SRTP 中 MKI 位于加密载荷之后, SRTCP 中位于 E+索引之后, 均在认证标签之前
KLB_API int klb_srtp_set_mki(klb_srtp_ctx_t* p_ctx, const uint8_t* p_mki, int mki_len);


/// @brief 设置是否允许重复接收同一包索引 (NACK 重传对端)
/// @param [in] *p_ctx          上下文
/// @param [in] enable          1.允许; 0.禁止 (默认禁止)
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 开启后重放窗口不再拒绝已接收过的索引, 与 libsrtp allow_repeat_tx 等价;
///  \n 仅接收侧上下文需要开启, 用于接收 NACK 触发的同序号重传包
KLB_API int klb_srtp_set_repeat(klb_srtp_ctx_t* p_ctx, int enable);


/// @brief 加密 RTP 包 (原地)
/// @param [in]  *p_ctx         上下文
/// @param [in]  *p_pkt         RTP 包; 需预留认证标签空间
/// @param [in]  len            输入长度
/// @param [in]  cap            缓存容量
/// @param [out] *p_out_len     输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_srtp_protect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int cap, int* p_out_len);


/// @brief 解密 RTP 包 (原地)
/// @param [in]  *p_ctx         上下文
/// @param [in]  *p_pkt         SRTP 包
/// @param [in]  len            输入长度
/// @param [out] *p_out_len     输出长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_srtp_unprotect(klb_srtp_ctx_t* p_ctx, uint8_t* p_pkt, int len, int* p_out_len);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SRTP_H__

// end
