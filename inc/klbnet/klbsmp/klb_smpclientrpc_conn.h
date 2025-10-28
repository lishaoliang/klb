///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_smpclientrpc_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   SMP协议 客户端RPC连接(klb SMP client RPC connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SMPCLIENTRPC_CONN_H__
#define __KLB_SMPCLIENTRPC_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"
#include "klbbase/klb_smp.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建SMP-RPC连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_smpclientrpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief SMP-RPC连接目标
KLB_API klb_netconn_t* klb_smpclientrpc_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port);


/// @brief 释放连接
KLB_API void klb_smpclientrpc_conn_free(klb_netconn_t* p_conn);


/// @brief 发送RPC数据
KLB_API int klb_smpclientrpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送RPC数据
KLB_API int klb_smpclientrpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data);


/// @brief 写缓存 是否为空
KLB_API bool klb_smpclientrpc_wbuf_is_empty(klb_netconn_t* p_conn);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SMPCLIENTRPC_CONN_H__
//end
