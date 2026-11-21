// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mnpclientrpc_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   MNP 客户端 RPC 连接(klb MNP client RPC connect)
/// @version 0.1
/// @history 修改历史
///  \n [2026-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MNPCLIENTRPC_CONN_H__
#define __KLB_MNPCLIENTRPC_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"
#include "klbbase/klb_mnp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建 MNP-RPC 连接
/// @return klb_netconn_t*
klb_netconn_t* klb_mnpclientrpc_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief MNP-RPC 连接目标
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_host           主机
/// @param [in] port              端口
/// @param [in] tls               true=TLS(mnprpcs)
/// @param [out] *p_code          可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_netconn_t* 失败为 NULL
klb_netconn_t* klb_mnpclientrpc_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code);


/// @brief 释放连接
void klb_mnpclientrpc_conn_free(klb_netconn_t* p_conn);


/// @brief 发送 RPC 数据
int klb_mnpclientrpc_conn_send(klb_netconn_t* p_conn, int rpctype, int method, int sequence, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送 RPC 数据
int klb_mnpclientrpc_conn_send_buf(klb_netconn_t* p_conn, int rpctype, int method, int sequence, klb_buf_t* p_data);


/// @brief 写缓存是否为空
bool klb_mnpclientrpc_wbuf_is_empty(klb_netconn_t* p_conn);



void klb_mnpclientrpc_conn_set_op_deadline(klb_netconn_t* p_conn, int64_t deadline_tc);
void klb_mnpclientrpc_conn_set_max_msg(klb_netconn_t* p_conn, int64_t max_msg);

#ifdef __cplusplus
}
#endif

#endif // __KLB_MNPCLIENTRPC_CONN_H__

// end
