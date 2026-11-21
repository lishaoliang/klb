// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnpserverpc_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   MNP 服务 RPC 连接(klb MNP serve RPC connect)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNPSERVERPC_CONN_H__
#define __KLB_MNPSERVERPC_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建 MNP-RPC 服务连接
/// @return klb_netconn_t*
klb_netconn_t* klb_mnpserverpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief 释放连接
void klb_mnpserverpc_conn_free(klb_netconn_t* p_conn);


/// @brief 发送 RPC 数据
int klb_mnpserverpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送 RPC 数据
int klb_mnpserverpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MNPSERVERPC_CONN_H__

// end
