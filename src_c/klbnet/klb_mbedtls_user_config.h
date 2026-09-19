// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mbedtls_user_config.h
/// @brief   mbedtls MBEDTLS_USER_CONFIG_FILE overlay for klbnet
///          Keep upstream mbedtls_config.h; only tweak klb-specific options
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MBEDTLS_USER_CONFIG_H__
#define __KLB_MBEDTLS_USER_CONFIG_H__

// klb uses klb_socket + mbedtls_ssl_set_bio; do not compile mbedtls POSIX/Win sockets
#undef MBEDTLS_NET_C

#endif // __KLB_MBEDTLS_USER_CONFIG_H__

// end
