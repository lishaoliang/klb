// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_tls_cert.h
/// @brief   TLS 证书工具: mbedtls 自签 RSA PEM / 读有效期
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TLS_CERT_H__
#define __KLB_TLS_CERT_H__


#include "klb_type.h"
#include "klbnet/klb_netcode.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// 建议 PEM 缓冲区下限 (RSA-2048 自签通常够用)
#define KLB_TLS_CERT_PEM_MIN    4096


/// @brief 自签选项; 字段可为默认
typedef struct klb_tls_cert_selfsign_opt_t_
{
    int             bits;           ///< RSA 位数; <=0 则 2048
    int             days;           ///< 有效天数; <=0 则 3650
    const char*     subject;        ///< 如 "O=klb,OU=alua,CN=alua-serve"; NULL 用默认
    const char*     san_csv;        ///< 如 "DNS:localhost,IP:127.0.0.1"; NULL 用默认 SAN
}klb_tls_cert_selfsign_opt_t;


/// @brief 是否具备自签能力 (编入 mbedtls)
/// @return true 可用; false 为 no-ssl 或未编 mbedtls
KLB_API bool klb_tls_cert_available(void);


/// @brief 生成自签 RSA 证书与私钥 (PEM 文本)
/// @param [in]  *p_opt         选项; 可为 NULL (全默认)
/// @param [out] *p_cert_pem    证书 PEM 输出缓冲
/// @param [in]  cert_cap       p_cert_pem 容量 (建议 >= KLB_TLS_CERT_PEM_MIN)
/// @param [out] *p_cert_len    写出字节数 (含结尾 0)
/// @param [out] *p_key_pem     私钥 PEM 输出缓冲
/// @param [in]  key_cap        p_key_pem 容量
/// @param [out] *p_key_len     写出字节数 (含结尾 0)
/// @return int 0.成功; 非0.klb_netcode_e
/// @note no-ssl 时返回 NOTSUP; 缓冲不足返回 NOMEM; 默认 subject=O=klb,OU=alua,CN=alua-serve
KLB_API int klb_tls_cert_selfsign_rsa(const klb_tls_cert_selfsign_opt_t* p_opt,
    char* p_cert_pem, int cert_cap, int* p_cert_len,
    char* p_key_pem, int key_cap, int* p_key_len);


/// @brief 生成自签 RSA 并写入文件
/// @param [in]  *p_opt         选项; 可为 NULL
/// @param [in]  *p_cert_path   证书路径
/// @param [in]  *p_key_path    私钥路径
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 覆盖已存在文件; 目录须已存在
KLB_API int klb_tls_cert_selfsign_rsa_to_files(const klb_tls_cert_selfsign_opt_t* p_opt,
    const char* p_cert_path, const char* p_key_path);


/// @brief 解析 PEM 证书 notAfter
/// @param [in]  *p_cert_pem    PEM 文本
/// @param [in]  cert_len       字节数; <=0 则 strlen
/// @param [out] *p_not_after   valid_to 的 unix 秒(UTC); 可为 NULL
/// @param [out] *p_remaining   not_after - now(秒, 可为负); 可为 NULL
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_tls_cert_pem_not_after(const char* p_cert_pem, int cert_len,
    int64_t* p_not_after, int64_t* p_remaining);


/// @brief 从证书文件解析 notAfter
/// @param [in]  *p_cert_path   PEM 文件路径
/// @param [out] *p_not_after   同 klb_tls_cert_pem_not_after
/// @param [out] *p_remaining   同 klb_tls_cert_pem_not_after
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_tls_cert_file_not_after(const char* p_cert_path,
    int64_t* p_not_after, int64_t* p_remaining);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_TLS_CERT_H__

// end
