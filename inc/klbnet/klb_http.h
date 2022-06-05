///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_http.h
/// @brief   http/https客户端
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_HTTP_H__
#define __KLB_HTTP_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbthird/http_parser.h"
#include "klbthird/sds.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_http_t_ klb_http_t;


/// @brief 创建http
/// @return klb_http_t* http模块
KLB_API klb_http_t* klb_http_create(enum http_parser_type parser_type);


/// @brief 销毁http
/// @param [in]  *p_http          http模块
/// @return 无
KLB_API void klb_http_destroy(klb_http_t* p_http);

int klb_http_init(klb_http_t* p_http, const uint8_t* p_data, int data_len);

/// @brief 放入发送数据
int klb_http_push(klb_http_t* p_http, klb_socket_t* p_socket, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);

/// @brief 发送数据
int klb_http_send(klb_http_t* p_http, klb_socket_t* p_socket);


/// @brief 从网络上接收数据
int klb_http_recv(klb_http_t* p_http, klb_socket_t* p_socket);


/// @brief 获取接收数据的结果
int klb_http_get(klb_http_t* p_http, klb_buf_t** p_data);


#ifdef __cplusplus
}
#endif

#endif // __KLB_HTTP_H__
//end
