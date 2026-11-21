// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_tls.h
/// @brief   TLS socket 操作接口 (基于 klb_socket_t; mbedTLS)
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_TLS_H__
#define __KLB_SOCKET_TLS_H__

#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_socket_tls_server_ctx_t_ klb_socket_tls_server_ctx_t;


/// @brief 创建一个异步 TLS socket (客户端)
/// @param [in]  fd             socket fd
/// @return klb_socket_t* NULL.创建失败; 非NULL
/// @note no-ssl 或 mbedtls init 失败时返回 NULL; 失败不关闭 fd
KLB_API klb_socket_t* klb_socket_async_create_tls(klb_socket_fd fd);


/// @brief 客户端 TLS 握手一轮 (非阻塞)
/// @param [in]  *p_socket      须为 klb_socket_async_create_tls 产物
/// @return 0.握手完成; 1.需再调(WANT_READ/WRITE); -1.失败
/// @note no-ssl 恒返回 -1
KLB_API int klb_socket_tls_handshake(klb_socket_t* p_socket);


/// @brief 设置 TLS SNI / 期望主机名
/// @param [in]  *p_socket      客户端 TLS socket
/// @param [in]  *p_hostname    主机名; NULL/空串清除
/// @return 0.成功; 非0.失败
/// @note 须在握手完成前调用; no-ssl 恒非0
KLB_API int klb_socket_tls_set_hostname(klb_socket_t* p_socket, const char* p_hostname);


/// @brief 设置对端证书校验模式
/// @param [in]  *p_socket      客户端 TLS socket
/// @param [in]  verify         0=NONE; 1=OPTIONAL; 2=REQUIRED
/// @return 0.成功; 非0.失败
/// @note 须在握手完成前; REQUIRED 时宜先 set_ca_pem; 默认 NONE
KLB_API int klb_socket_tls_set_verify(klb_socket_t* p_socket, int verify);


/// @brief 设置 CA 证书链 PEM (用于校验对端)
/// @param [in]  *p_socket      客户端 TLS socket
/// @param [in]  *p_ca_pem      CA PEM; 可为多证书拼接
/// @param [in]  ca_len         字节数; 不含结尾 0 时内部补 1
/// @return 0.成功; 非0.失败
/// @note 须在握手完成前; 重复调用替换
KLB_API int klb_socket_tls_set_ca_pem(klb_socket_t* p_socket, const char* p_ca_pem, int ca_len);


/// @brief 设置 ALPN 协议列表
/// @param [in]  *p_socket      客户端 TLS socket
/// @param [in]  *p_alpn_csv    逗号分隔协议名, 如 "h2,http/1.1"; NULL/空清除
/// @return 0.成功; 非0.失败
/// @note 须在握手完成前; 内部拷贝
KLB_API int klb_socket_tls_set_alpn(klb_socket_t* p_socket, const char* p_alpn_csv);


/// @brief 设置对端证书 SHA256 pin (hex)
/// @param [in]  *p_socket      客户端 TLS socket
/// @param [in]  *p_hex         64 位小写/大写 hex; NULL/空清除
/// @return 0.成功; 非0.失败
/// @note 握手完成后由 klb_socket_tls_check_pin 校验; 失败应断开
KLB_API int klb_socket_tls_set_pin_sha256(klb_socket_t* p_socket, const char* p_hex);


/// @brief 握手完成后校验 pin (若已设置)
/// @param [in]  *p_socket      客户端 TLS socket
/// @return 0.通过或未设置 pin; 非0.失败
KLB_API int klb_socket_tls_check_pin(klb_socket_t* p_socket);


/// @brief 创建 TLS 服务端上下文
/// @param [in]  *p_cert_pem    证书 PEM (可含中间证书)
/// @param [in]  cert_len       p_cert_pem 字节数; 不含结尾 0 时内部补 1
/// @param [in]  *p_key_pem     私钥 PEM
/// @param [in]  key_len        p_key_pem 字节数; 不含结尾 0 时内部补 1
/// @return klb_socket_tls_server_ctx_t* NULL.失败; 非NULL
/// @note no-ssl / 证书或私钥解析失败返回 NULL; destroy 为引用计数
KLB_API klb_socket_tls_server_ctx_t* klb_socket_tls_server_ctx_create(const char* p_cert_pem, int cert_len, const char* p_key_pem, int key_len);


/// @brief 释放 TLS 服务端上下文
/// @param [in]  *p_ctx         上下文; 可为 NULL
/// @return 无
/// @note 引用计数减 1; 至 0 时释放
KLB_API void klb_socket_tls_server_ctx_destroy(klb_socket_tls_server_ctx_t* p_ctx);


/// @brief 创建一个异步 TLS socket (服务端)
/// @param [in]  fd             已 accept 的 socket fd
/// @param [in]  *p_ctx         TLS 服务端上下文; 不可为 NULL
/// @return klb_socket_t* NULL.创建失败; 非NULL
/// @note 成功后持有 ctx 引用; 失败不关闭 fd
KLB_API klb_socket_t* klb_socket_async_create_tls_server(klb_socket_fd fd, klb_socket_tls_server_ctx_t* p_ctx);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SOCKET_TLS_H__

// end
