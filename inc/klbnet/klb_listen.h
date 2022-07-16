///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_listen.h
/// @brief   网路监听: 监听单个端口/或unix路径
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LISTEN_H__
#define __KLB_LISTEN_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_listen_t_ klb_listen_t;


/// @brief 创建listen
/// @return klb_listen_t*               listen模块
KLB_API klb_listen_t* klb_listen_create(klb_multiplex_t* p_multi);


/// @brief 销毁listen
/// @param [in]  *p_listen              listen模块
/// @return 无
KLB_API void klb_listen_destroy(klb_listen_t* p_listen);


/// @brief 开启TCP监听端口
/// @param [in]  *p_listen              listen模块
/// @param [in]  port                   端口
/// @param [in]  max_connect            操作系统缓存最大的socket数
/// @return klb_listen_t* listen模块
KLB_API int klb_listen_open(klb_listen_t* p_listen, int port, int max_connect);


/// @brief 开启TCP监听端口: unix路径
/// @param [in]  *p_listen              listen模块
/// @param [in]  *p_path                unix路径
/// @param [in]  max_connect            操作系统缓存最大的socket数
/// @return klb_listen_t* listen模块
KLB_API int klb_listen_open_unix(klb_listen_t* p_listen, const char* p_path, int max_connect);


/// @brief 设置当监听为tls及相关参数
/// @param [in]  *p_listen          listen模块
/// @return int 0
KLB_API int klb_listen_set_tls(klb_listen_t* p_listen, const klb_socket_tls_param_t* p_tls_param);


/// @brief 关闭监听端口
/// @param [in]  *p_listen              listen模块
/// @return 无
KLB_API void klb_listen_close(klb_listen_t* p_listen);


/// @brief 获取鉴定端口
/// @return int 端口
KLB_API int klb_listen_port(klb_listen_t* p_listen);


/// @brief 当监听到socket连接建立时回调函数
/// @param [in] *ptr            用户数据指针
/// @param [in] fd              socket的系统fd
/// @param [in] *p_addr         socket信息
/// @return int 0.成功; 非0.失败
typedef int(*klb_listen_accept_cb)(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param);


/// @brief 设置当监听到socket连接建立时回调
/// @param [in]  *p_listen          listen模块
/// @param [in]  cb_accept          接收socket回调函数
/// @param [in]  *ptr               cb_accept函数, 附加用户数据指针
/// @return int 0
KLB_API int klb_listen_set_accept(klb_listen_t* p_listen, klb_listen_accept_cb cb_accept, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LISTEN_H__
//end
