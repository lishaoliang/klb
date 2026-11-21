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
/// @param [out] *p_code          可选; 失败时 klb_netcode_e; 成功为 0
/// @return klb_netconn_t* 连接对象; 失败为 NULL
klb_netconn_t* klb_httpclient_connect(klb_netmulti_t* p_netmulti, const char* p_host, int port, bool tls, int* p_code);


/// @brief 置/清 CONNECT 应答模式
/// @param [in] *p_conn           连接对象
/// @param [in] connect           true.下一次响应按 CONNECT 2xx 无体解析; false.普通响应
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 正向代理 https 隧道: 发 CONNECT 前置 true; tls_upgrade 成功后自动复位
int klb_httpclient_conn_set_connect(klb_netconn_t* p_conn, bool connect);


/// @brief 明文连接就地升级为 TLS (正向代理隧道)
/// @param [in] *p_conn           连接对象
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 仅客户端; 须已发 CONNECT 且读缓存已空; fd 不变, 原 socket 换为 TLS
int klb_httpclient_conn_tls_upgrade(klb_netconn_t* p_conn);


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
/// @return int 0.成功; 非0.klb_netcode_e
int klb_httpclient_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送HTTP数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.klb_netcode_e
int klb_httpclient_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 发送HTTP请求头 + 本地文件体
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           请求头; 可为 NULL
/// @param [in] head_len          请求头长度
/// @param [in] *p_path           本地文件路径
/// @param [in] offset            文件起始偏移; 字节
/// @param [in] length            发送字节数; 0 则只发头
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 先打开文件再入队头; 失败不发送头. 非内核 sendfile; on_send 分块读入写队列
int klb_httpclient_conn_send_file(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len,
    const char* p_path, int64_t offset, int64_t length);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_httpclient_wbuf_is_empty(klb_netconn_t* p_conn);


/// @brief 设置读/写/idle 超时(毫秒; 0=关闭)
/// @param [in] *p_conn           连接对象
/// @param [in] read_ms           读超时: 有未完成响应且无收包超过此值
/// @param [in] write_ms          写超时: 写队列非空且无发包超过此值
/// @param [in] idle_ms           空闲超时: 无进行中读写且无收包超过此值
/// @return 无
void klb_httpclient_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms);


/// @brief 设置响应体上限(字节; 0=不限制)
/// @param [in] *p_conn           连接对象
/// @param [in] max_body          上限; 超限上抛 KLB_NETCODE_PROTO_TOO_LARGE
/// @return 无
void klb_httpclient_conn_set_max_body(klb_netconn_t* p_conn, int64_t max_body);


#ifdef __cplusplus
}
#endif

#endif // __KLB_HTTPCLIENT_CONN_H__

// end
