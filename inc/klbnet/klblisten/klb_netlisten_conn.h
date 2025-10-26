///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netlisten_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   监听连接
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETLISTEN_CONN_H__
#define __KLB_NETLISTEN_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netmulti.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建listen连接
/// @return klb_netconn_t*          listen 连接
KLB_API klb_netconn_t* klb_netlisten_conn_create(klb_netmulti_t* p_multi);


/// @brief 释放连接
KLB_API void klb_netlisten_conn_free(klb_netconn_t* p_conn);


/// @brief 监听: 端口
/// @return int 0.成功; 非0.错误码
KLB_API int klb_netlisten_conn_open(klb_netconn_t* p_conn, int port, int max_connect);


/// @brief 开启TCP监听: unix路径
/// @param [in]  *p_path                unix路径
/// @param [in]  max_connect            操作系统缓存最大的socket数
/// @return int 0.成功; 非0.错误码
KLB_API int klb_netlisten_conn_open_unix(klb_netconn_t* p_conn, const char* p_path, int max_connect);


/// @brief 获取监听端口
/// @return int 端口号
KLB_API int klb_netlisten_conn_get_port(klb_netconn_t* p_conn);


/// @struct klb_netlisten_socket_t
/// @brief  监听得到的 socket 参数
typedef struct klb_netlisten_socket_t_
{
    klb_socket_fd               fd;         ///< socket fd
    struct sockaddr_in          addr;       ///< ip地址
    bool                        tls;        ///< 
}klb_netlisten_socket_t;


/// @brief 当监听到socket连接建立时回调函数
/// @param [in] *ptr            用户数据指针
/// @param [in] fd              socket的系统fd
/// @param [in] *p_addr         socket信息
/// @return int 0.成功; 非0.失败
typedef int(*klb_netlisten_conn_accept_cb)(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param);


/// @brief 设置当监听到socket连接建立时回调
/// @param [in]  cb_accept          接收socket回调函数
/// @param [in]  *ptr               cb_accept函数, 附加用户数据指针
/// @return int 0
KLB_API int klb_netlisten_conn_set_accept(klb_netconn_t* p_conn, klb_netlisten_conn_accept_cb cb_accept, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETLISTEN_CONN_H__
//end
