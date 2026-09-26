// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_httpclient_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   HTTP客户端连接(klb HTTP client connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_HTTPCLIENT_CONN_H__
#define __KLB_HTTPCLIENT_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建HTTP客户端连接
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_socket         socket
/// @return klb_netconn_t* 连接对象
klb_netconn_t* klb_httpclient_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief HTTP连接目标
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_host           主机
/// @param [in] port              端口
/// @param [in] tls               是否 TLS; false.明文; true.TLS
/// @return klb_netconn_t* 连接对象; 失败为 NULL
klb_netconn_t* klb_httpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls);


/// @brief 释放连接
/// @param [in] *p_conn           连接对象
/// @return 无
void klb_httpclient_conn_free(klb_netconn_t* p_conn);


/// @brief 发送HTTP请求(头 + 可选body)
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           请求头
/// @param [in] head_len          请求头长度
/// @param [in] *p_body           请求体; 可为 NULL
/// @param [in] body_len          请求体长度
/// @return int 0.成功; 非0.失败
int klb_httpclient_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送HTTP数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.失败
int klb_httpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 发送HTTP请求头 + 本地文件体
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           请求头; 可为 NULL
/// @param [in] head_len          请求头长度
/// @param [in] *p_path           本地文件路径
/// @param [in] offset            文件起始偏移; 字节
/// @param [in] length            发送字节数; 0 则只发头
/// @return int 0.成功; 非0.失败
/// @note 先打开文件再入队头; 失败不发送头. 非内核 sendfile; on_send 分块读入写队列
int klb_httpclient_conn_send_file(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len,
    const char* p_path, int64_t offset, int64_t length);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_httpclient_wbuf_is_empty(klb_netconn_t* p_conn);


#ifdef __cplusplus
}
#endif

#endif // __KLB_HTTPCLIENT_CONN_H__

// end
