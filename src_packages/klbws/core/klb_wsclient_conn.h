// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wsclient_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WS客户端连接(klb web socket client connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025] 创建文件
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


/// @brief 创建WS客户端连接
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_socket         socket
/// @return klb_netconn_t* 连接对象
klb_netconn_t* klb_wsclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief WS连接目标
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_host           主机
/// @param [in] port              端口
/// @param [in] tls               是否 TLS; false.明文; true.TLS
/// @return klb_netconn_t* 连接对象; 失败为 NULL
klb_netconn_t* klb_wsclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls);


/// @brief 释放连接
/// @param [in] *p_conn           连接对象
/// @return 无
void klb_wsclient_conn_free(klb_netconn_t* p_conn);


/// @brief 发送数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           握手阶段为HTTP头; WS阶段并入payload
/// @param [in] head_len          头长度
/// @param [in] *p_body           数据体; 可为 NULL
/// @param [in] body_len          数据体长度
/// @return int 0.成功; 非0.失败
/// @note 握手完成前按HTTP原文发送; 之后按 WS TEXT 帧发送(带mask)
int klb_wsclient_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.失败
int klb_wsclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_wsclient_wbuf_is_empty(klb_netconn_t* p_conn);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WSCLIENT_CONN_H__

// end
