///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/02
//
/// @file    klb_socket.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/02 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_H__
#define __KLB_SOCKET_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET          klb_socket_fd;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>

    typedef int             klb_socket_fd;
    #define INVALID_SOCKET  -1
#endif


#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_H__
//end
