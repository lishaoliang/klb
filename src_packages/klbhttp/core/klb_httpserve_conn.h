// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_httpserve_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   HTTP服务连接(klb HTTP serve connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_HTTPSERVE_CONN_H__
#define __KLB_HTTPSERVE_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建HTTP服务连接
/// @param [in] *p_netmulti       复用模块
/// @param [in] *p_socket         已接受的 socket
/// @return klb_netconn_t* 连接对象
/// @note 创建后已加入复用
klb_netconn_t* klb_httpserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);


/// @brief 释放连接
/// @param [in] *p_conn           连接对象
/// @return 无
void klb_httpserve_conn_free(klb_netconn_t* p_conn);


/// @brief 发送HTTP响应(头 + 可选body)
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           响应头
/// @param [in] head_len          响应头长度
/// @param [in] *p_body           响应体; 可为 NULL
/// @param [in] body_len          响应体长度
/// @return int 0.成功; 非0.klb_netcode_e
int klb_httpserve_conn_send(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 按 buf 发送HTTP数据
/// @param [in] *p_conn           连接对象
/// @param [in] *p_data           待发送缓存; 所有权转入连接
/// @return int 0.成功; 非0.klb_netcode_e
int klb_httpserve_conn_send_buf(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 发送HTTP响应头 + 本地文件体
/// @param [in] *p_conn           连接对象
/// @param [in] *p_head           响应头; 可为 NULL
/// @param [in] head_len          响应头长度
/// @param [in] *p_path           本地文件路径
/// @param [in] offset            文件起始偏移; 字节
/// @param [in] length            发送字节数; 0 则只发头
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 先打开文件再入队头; 失败不发送头. 非内核 sendfile; on_send 分块读入写队列
int klb_httpserve_conn_send_file(klb_netconn_t* p_conn, const uint8_t* p_head, int head_len,
    const char* p_path, int64_t offset, int64_t length);


/// @brief 写缓存 是否为空
/// @param [in] *p_conn           连接对象
/// @return bool true.空; false.还有数据
bool klb_httpserve_wbuf_is_empty(klb_netconn_t* p_conn);


/// @brief 设置读/写/idle 超时(毫秒; 0=关闭)
/// @param [in] *p_conn           连接对象
/// @param [in] read_ms           读超时: 有未完成请求且无收包超过此值
/// @param [in] write_ms          写超时: 写队列非空且无发包超过此值
/// @param [in] idle_ms           空闲超时: 无进行中读写且无收包超过此值
/// @return 无
void klb_httpserve_conn_set_timeout(klb_netconn_t* p_conn, int read_ms, int write_ms, int idle_ms);


/// @brief 设置请求体上限(字节; 0=不限制)
/// @param [in] *p_conn           连接对象
/// @param [in] max_body          上限; 超限上抛 KLB_NETCODE_PROTO_TOO_LARGE
/// @return 无
void klb_httpserve_conn_set_max_body(klb_netconn_t* p_conn, int64_t max_body);


/// @brief 摘出 socket 与握手后剩余字节, 并销毁 HTTP 连接
/// @param [in]  *p_conn          连接对象; 成功后不可再使用
/// @param [out] *pp_socket       摘出的 socket
/// @param [out] *pp_tail         剩余字节; 无则为 NULL; 调用方释放
/// @return int 0.成功; 非0.klb_netcode_e 且连接仍有效
/// @note 仅 Upgrade 停解析之后, 且写队列已空
int klb_httpserve_conn_take(klb_netconn_t* p_conn, klb_socket_t** pp_socket, klb_buf_t** pp_tail);


#ifdef __cplusplus
}
#endif

#endif // __KLB_HTTPSERVE_CONN_H__

// end
