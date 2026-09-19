// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wsserve_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   WS服务端连接(klb web socket serve connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WSSERVE_CONN_H__
#define __KLB_WSSERVE_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建WS服务连接
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_socket         已接受的 socket
/// @return klb_netconn_t* 连接对象
/// @note 创建后已加入复用
klb_netconn_t* klb_wsserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief 释放连接
/// @param [in] *p_conn           连接对象
/// @return 无
void klb_wsserve_conn_free(klb_netconn_t* p_conn);


/// @brief 发送数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           握手阶段为HTTP头; WS阶段并入payload
/// @param [in] head_len          头长度
/// @param [in] *p_body           数据体; 可为 NULL
/// @param [in] body_len          数据体长度
/// @return int 0.成功; 非0.失败
/// @note 握手完成前按HTTP原文发送; 之后按 WS TEXT 帧发送(无mask)
int klb_wsserve_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.失败
int klb_wsserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_wsserve_wbuf_is_empty(klb_netconn_t* p_conn);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WSSERVE_CONN_H__

// end
