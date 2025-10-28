///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smpserverpc_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SMP协议服务RPC连接(klb SMP serve RPC connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMPSERVERPC_CONN_H__
#define __KLB_SMPSERVERPC_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"
#include "klbbase/klb_smp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建SMP连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_smpserverpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);

/// @brief 释放连接
KLB_API void klb_smpserverpc_conn_free(klb_netconn_t* p_conn);


/// @brief 发送RPC数据
KLB_API int klb_smpserverpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送RPC数据
KLB_API int klb_smpserverpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SMPSERVERPC_CONN_H__
//end
