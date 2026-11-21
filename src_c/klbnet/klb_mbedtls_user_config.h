// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mbedtls_user_config.h
/// @brief   mbedtls MBEDTLS_USER_CONFIG_FILE overlay for klbnet
///          Keep upstream mbedtls_config.h; only tweak klb-specific options
/// @version 0.2
/// @history 修改历史
///  \n 2026 0.1 创建文件
///  \n [2026-10] 0.2 开启 DTLS-SRTP (RFC 5764), 供 klb_socket_dtls 导出 SRTP 密钥
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MBEDTLS_USER_CONFIG_H__
#define __KLB_MBEDTLS_USER_CONFIG_H__

// klb uses klb_socket + mbedtls_ssl_set_bio; do not compile mbedtls POSIX/Win sockets
#undef MBEDTLS_NET_C

// WebRTC: use_srtp 扩展协商 + DTLS-SRTP 密钥导出 (依赖 MBEDTLS_SSL_PROTO_DTLS 已开)
#define MBEDTLS_SSL_DTLS_SRTP

#endif // __KLB_MBEDTLS_USER_CONFIG_H__

// end
