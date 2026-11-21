///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netlisten_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   监听连接
/// @version 0.2
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETLISTEN_CONN_H__
#define __KLB_NETLISTEN_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建listen连接
/// @return klb_netconn_t*          listen 连接
KLB_API klb_netconn_t* klb_netlisten_conn_create(klb_netmulti_t* p_multi);


/// @brief 释放连接
KLB_API void klb_netlisten_conn_free(klb_netconn_t* p_conn);


/// @struct klb_netlisten_open_opts_t
/// @brief  open_addr 可选参数
typedef struct klb_netlisten_open_opts_t_
{
    int                             family;         ///< AF_INET / AF_INET6 / AF_UNSPEC(dual)
    bool                            reuseport;      ///< SO_REUSEPORT
    bool                            nodelay;        ///< 对 accept 所得 fd 设 TCP_NODELAY
    bool                            keepalive;      ///< 对 accept 所得 fd 设 SO_KEEPALIVE
}klb_netlisten_open_opts_t;


/// @brief 监听: 端口
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netlisten_conn_open(klb_netconn_t* p_conn, int port, int max_connect);


/// @brief 监听: 指定绑定地址
/// @param [in]  *p_host            绑定地址; NULL/""/"0.0.0.0"/"*" 表示任意地址
/// @param [in]  port               端口
/// @param [in]  max_connect        操作系统缓存最大的socket数
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netlisten_conn_open_addr(klb_netconn_t* p_conn, const char* p_host, int port, int max_connect);


/// @brief 监听: 指定绑定地址 + 族/套接字选项
/// @param [in]  *p_opts            可选; NULL 等价 family=AF_INET
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netlisten_conn_open_addr_ex(klb_netconn_t* p_conn, const char* p_host, int port,
    int max_connect, const klb_netlisten_open_opts_t* p_opts);


/// @brief 回读监听本地地址 (getsockname); 用于 port=0 临时端口回读
/// @param [out] *p_addr            输出本地地址 (仅 AF_INET 成功)
/// @return int 0.成功; 非0.失败
KLB_API int klb_netlisten_conn_get_local_addr(klb_netconn_t* p_conn, struct sockaddr_in* p_addr);


/// @brief 回读监听本地地址 (AF_INET / AF_INET6)
/// @return int 0.成功; 非0.失败
KLB_API int klb_netlisten_conn_get_local_sa(klb_netconn_t* p_conn, struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 取监听 native fd
/// @return klb_socket_fd; 未 open 为 INVALID_SOCKET
KLB_API klb_socket_fd klb_netlisten_conn_native_fd(klb_netconn_t* p_conn);


/// @brief 同步关闭监听 (立即关闭 fd, 不经复用模块延迟关闭)
/// @note 为 rebind 而设: 关闭后端口立即可被重新 bind; 既有已 accept 连接不受影响
KLB_API void klb_netlisten_conn_close_sync(klb_netconn_t* p_conn);


/// @brief 开启TCP监听: unix路径
/// @param [in]  *p_path                unix路径
/// @param [in]  max_connect            操作系统缓存最大的socket数
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netlisten_conn_open_unix(klb_netconn_t* p_conn, const char* p_path, int max_connect);


/// @brief 获取监听端口
/// @return int 端口号
KLB_API int klb_netlisten_conn_get_port(klb_netconn_t* p_conn);


/// @struct klb_netlisten_socket_t
/// @brief  监听得到的 socket 参数
typedef struct klb_netlisten_socket_t_
{
    klb_socket_fd               fd;         ///< socket fd
    struct sockaddr_in          addr;       ///< ip地址 (IPv4; IPv6 对端时为零)
    bool                        tls;        ///< 
}klb_netlisten_socket_t;


/// @brief 当监听到socket连接建立时回调函数
/// @param [in] *ptr            用户数据指针
/// @param [in] fd              socket的系统fd
/// @param [in] *p_addr         socket信息 (IPv4; IPv4-mapped 已折算; 纯 IPv6 为零)
/// @return int 0.成功; 非0.klb_netcode_e
typedef int(*klb_netlisten_conn_accept_cb)(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls);


/// @brief 当监听到socket连接建立时回调 (sockaddr_storage; 优先于 accept_cb)
typedef int(*klb_netlisten_conn_accept_sa_cb)(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd,
    const struct sockaddr_storage* p_addr, bool tls);


/// @brief accept 系统错误回调 (非 EAGAIN/EINTR)
/// @param [in]  code           klb_netcode_e
typedef int(*klb_netlisten_conn_accept_err_cb)(klb_netconn_t* p_conn, void* ptr, int code);


/// @brief 设置当监听到socket连接建立时回调
/// @param [in]  cb_accept          接收socket回调函数
/// @param [in]  *ptr               cb_accept函数, 附加用户数据指针
/// @return int 0
KLB_API int klb_netlisten_conn_set_accept(klb_netconn_t* p_conn, klb_netlisten_conn_accept_cb cb_accept, void* ptr);


/// @brief 设置 accept_sa 回调 (优先; 供 netmux IPv6)
/// @return int 0
KLB_API int klb_netlisten_conn_set_accept_sa(klb_netconn_t* p_conn, klb_netlisten_conn_accept_sa_cb cb_accept, void* ptr);


/// @brief 设置 accept 错误回调
/// @return int 0
KLB_API int klb_netlisten_conn_set_accept_err(klb_netconn_t* p_conn, klb_netlisten_conn_accept_err_cb cb_err, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETLISTEN_CONN_H__

// end
