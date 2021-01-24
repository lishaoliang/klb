///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_base.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_BASE_H__
#define __KLB_SOCKET_BASE_H__

#include "klb_type.h"
#include "socket/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


void klb_socket_base_construct(klb_socket_t* p_socket, klb_socket_fd fd);
klb_socket_fd klb_socket_base_destruct(klb_socket_t* p_socket);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_BASE_H__
//end
