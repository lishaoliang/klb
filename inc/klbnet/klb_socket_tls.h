///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/03/27
//
/// @file    klb_socket_tls.h
/// @brief   加密socket
///         __KLB_USE_OPENSSL__
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/03/27 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_TLS_H__
#define __KLB_SOCKET_TLS_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief openssl库初始化
int klb_socket_tls_init();


/// @brief openssl库退出
void klb_socket_tls_quit();


/// @brief 创建一个加密异步socket
/// @param [in]  fd             socket fd
/// @return klb_socket_t* NULL.创建失败; 非NULL
klb_socket_t* klb_socket_tls_async_create(klb_socket_fd fd);



#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_TLS_H__
//end
