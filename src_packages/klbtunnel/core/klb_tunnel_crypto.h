// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_tunnel_crypto.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   alua-tunnel PSK SHA256-CTR + HMAC 标签密封 (包内; EtM AEAD)
/// @version 0.3
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TUNNEL_CRYPTO_H__
#define __KLB_TUNNEL_CRYPTO_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLB_TUNNEL_FLAG_CRYPTO     0x01
#define KLB_TUNNEL_NONCE_LEN       16
#define KLB_TUNNEL_SHA256_LEN      32
#define KLB_TUNNEL_TAG_LEN         16


/// @brief SHA256 摘要
/// @param [out] p_out  至少 32 字节
void klb_tunnel_sha256(const uint8_t* p_msg, int msg_len, uint8_t* p_out);

/// @brief 会话键 = SHA256("alua-tunnel-sess|" + psk + "|" + nc + "|" + ns)
void klb_tunnel_derive_session_key(const uint8_t* p_psk, int psk_len,
    const uint8_t* p_nc, int nc_len, const uint8_t* p_ns, int ns_len, uint8_t* p_out32);

/// @brief 密封: seq_be32 + ciphertext + tag16; out_max >= 4 + plain_len + 16
/// @return 写入字节数; 失败 -1
int klb_tunnel_seal(uint8_t* p_out, int out_max, const uint8_t* p_key32,
    uint32_t seq, const uint8_t* p_plain, int plain_len);

/// @brief 开封并校验 tag16
/// @param [out] p_seq  可 NULL
/// @return 明文长度; 失败 -1 (含 tag 不符)
int klb_tunnel_open(uint8_t* p_out, int out_max, const uint8_t* p_key32,
    const uint8_t* p_blob, int blob_len, uint32_t* p_seq);

/// @brief 弱熵 nonce (非 CSPRNG; AEAD/CSPRNG 见 P1-2)
int klb_tunnel_random_nonce(uint8_t* p_out, int out_len);

/// @brief HELLO crypto payload: flag + token_len_be16 + token + nonce_c
int klb_tunnel_encode_hello_crypto(uint8_t* p_out, int out_max,
    const uint8_t* p_token, int token_len, const uint8_t* p_nonce_c);

/// @brief 解析 HELLO payload
/// @param [out] p_crypto  1=crypto
int klb_tunnel_parse_hello_payload(const uint8_t* p, int len,
    int* p_crypto, char* p_token, int token_max, int* p_token_len,
    uint8_t* p_nonce_c);

/// @brief HELLO_OK crypto: flag + nonce_s
int klb_tunnel_encode_hello_ok_crypto(uint8_t* p_out, int out_max, const uint8_t* p_nonce_s);

/// @brief 解析 HELLO_OK payload
int klb_tunnel_parse_hello_ok_payload(const uint8_t* p, int len,
    int* p_crypto, uint8_t* p_nonce_s);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_TUNNEL_CRYPTO_H__

// end
