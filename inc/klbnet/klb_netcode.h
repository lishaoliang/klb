///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netcode.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   网络模块 消息码
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETCODE_H__
#define __KLB_NETCODE_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_netcode_e
/// @brief 网络模块 消息码
typedef enum klb_netcode_e_
{
    // [0 ~ KLB_SOCKET_STATUS_MAX], 沿用 klb_socket_status_e
    KLB_NETCODE_OK          = KLB_SOCKET_OK,            ///< 正常
    KLB_NETCODE_SOCKET_MAX  = KLB_SOCKET_STATUS_MAX,    ///< 最大值: 2^6

    KLB_NETCODE_WBUF_EMPTY  = 70,                       ///< 写缓存空(net code write buffer empty)

    KLB_NETCODE_RBUF_FULL   = 90,                       ///< 读缓存满(net code read buffer full)
}klb_netcode_e;


#if defined(__cplusplus)
}
#endif

#endif // __KLB_NETCODE_H__
//end
