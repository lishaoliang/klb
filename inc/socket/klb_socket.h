///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/02
//
/// @file    klb_socket.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/02 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_H__
#define __KLB_SOCKET_H__

#include "klb_type.h"

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET          klb_socket_fd;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>

    typedef int             klb_socket_fd;
    #define INVALID_SOCKET  -1
#endif


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_socket_t_ klb_socket_t;


/// @brief 销毁
typedef void(*klb_socket_destroy_cb)(klb_socket_t* p_socket);

/// @brief TCP发送数据
typedef int(*klb_socket_send_cb)(klb_socket_t* p_socket, const uint8_t* p_data, int len);

/// @brief TCP接收数据
typedef int(*klb_socket_recv_cb)(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len);

/// @brief UDP发送数据
typedef int(*klb_socket_sendto_cb)(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len);

/// @brief UDP接收数据
typedef int(*klb_socket_recvfrom_cb)(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len);


/// @struct klb_socket_vtable_t
/// @brief  socket虚表
typedef struct klb_socket_vtable_t_
{
    //klb_socket_destroy_cb   cb_destroy;
    klb_socket_send_cb      cb_send;
    klb_socket_recv_cb      cb_recv;
    klb_socket_sendto_cb    cb_sendto;
    klb_socket_recvfrom_cb  cb_recvfrom;
}klb_socket_vtable_t;


typedef struct klb_socket_user_t_
{
    uint32_t    tick_count;
}klb_socket_user_t;


typedef struct klb_socket_t_
{
    klb_socket_vtable_t vtable;         ///< 虚表
    void*               p_obj;          ///< vtable附加对象

    klb_socket_fd       fd;             ///< socket fd

    uint32_t            nonblock : 1;   ///< 0(false). 阻塞模式; 1(true),非阻塞
    uint32_t            resv : 31;

    klb_socket_user_t   user;           ///< 一些参数

    char                extra[];        ///< 附加数据
}klb_socket_t;


/// @brief 系统socket模块初始化
/// @return 无
/// @note WIN32网络初始化
int klb_socket_init();


/// @brief 系统socket模块退出
/// /// @return 无
/// @note WIN32网络退出
void klb_socket_quit();


/// @brief 销毁
//void klb_socket_destroy(klb_socket_t* p_socket);

/// @brief TCP发送数据
int klb_socket_send(klb_socket_t* p_socket, const uint8_t* p_data, int len);

/// @brief TCP接收数据
int klb_socket_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len);

/// @brief UDP发送数据
int klb_socket_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len);

/// @brief UDP接收数据
int klb_socket_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len);

/// @brief 附加到socket fd
void klb_socket_attach_fd(klb_socket_t* p_socket, klb_socket_fd fd);

/// @brief 分离出socket fd
klb_socket_fd klb_socket_detach_fd(klb_socket_t* p_socket);


/// @brief 关闭socket
void klb_socket_close(klb_socket_fd fd);
#define KLB_SOCKET_CLOSE(FD_) {if(INVALID_SOCKET!=(FD_)){klb_socket_close((FD_));(FD_)=INVALID_SOCKET;}}


/// @brief 设置阻塞/非阻塞
void klb_socket_set_block(klb_socket_fd fd, bool blocking);


/// @brief 使用TCP连接服务器 域名:端口
/// @param [in] *p_host     域名: eg. "www.baidu.com", "127.0.0.1"
/// @param [in] port        端口: eg. 80
/// @param [in] time_out    超时(毫秒): -1. 表示创建非阻塞socket; 大于等于0. 表示创建阻塞socket并等待超时
/// @return klb_socket_fd INVALID_SOCKET, 大于0
klb_socket_fd klb_socket_connect(const char* p_host, int port, int time_out);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_H__
//end
