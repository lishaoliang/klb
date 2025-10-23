///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wsclient_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WS客户端连接(klb web socket client connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WSCLIENT_CONN_H__
#define __KLB_WSCLIENT_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建WS连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_wsclient_conn_create();


/// @brief WS连接目标
KLB_API klb_netconn_t* klb_wsclient_connect();


#ifdef __cplusplus
}
#endif

#endif // __KLB_WSCLIENT_CONN_H__
//end
