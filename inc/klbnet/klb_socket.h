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
    klb_socket_destroy_cb   cb_destroy;     ///< 销毁
    klb_socket_send_cb      cb_send;        ///< TCP发送数据
    klb_socket_recv_cb      cb_recv;        ///< TCP接收数据
    klb_socket_sendto_cb    cb_sendto;      ///< UDP发送数据
    klb_socket_recvfrom_cb  cb_recvfrom;    ///< UDP接收数据
}klb_socket_vtable_t;


/// @struct klb_socket_status_e
/// @brief  状态
typedef enum klb_socket_status_e_
{
    KLB_SOCKET_OK,                      ///< 正常
    KLB_SOCKET_ERR,                     ///< 网络错误(读写socket返回-1引发): 通用错误
    KLB_SOCKET_DISCONN,                 ///< 连接已断开(读写socket返回0引发): 对方已经断开连接
    KLB_SOCKET_TIMEOUT,                 ///< 超时错误: 读写数据超时
    KLB_SOCKET_ERR_PROTOCOL,            ///< 协议解析错误: 不符合协议规范的数据或格式等
    KLB_SOCKET_ERR_MAX,                 ///< 最大值
}klb_socket_status_e;


/// @struct klb_socket_t
/// @brief  socket基础定义
typedef struct klb_socket_t_
{
    klb_socket_vtable_t vtable;         ///< 虚表

    klb_socket_fd       fd;             ///< socket fd

    uint16_t            nonblock : 1;   ///< 0(false).阻塞模式; 1(true).非阻塞
    uint16_t            sending : 1;    ///< 0(false).无数据发送; 1(true).有数据发送
    uint16_t            reading : 1;    ///< 0(false).无需读取; 1(true).需读取
    uint16_t            rsv1 : 13;

    uint16_t            status : 4;     ///< 状态: klb_socket_status_e
    uint16_t            rsv2 : 12;

    int64_t             last_recv_tc;   ///< 上次接收数据时间
    int64_t             last_send_tc;   ///< 上次发送数据时间

    void*               p_udata;        ///< user data; 用户附加数据

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
void klb_socket_destroy(klb_socket_t* p_socket);

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

/// @brief 设置发送
void klb_socket_set_sending(klb_socket_t* p_socket, bool sending);

/// @brief 获取是否需要发送数据
bool klb_socket_is_sending(klb_socket_t* p_socket);

/// @brief 设置读取
void klb_socket_set_reading(klb_socket_t* p_socket, bool reading);

/// @brief 获取是否需要读取数据
bool klb_socket_is_reading(klb_socket_t* p_socket);

/// @brief 设置状态
void klb_socket_set_status(klb_socket_t* p_socket, int status);

/// @brief 获取状态
int  klb_socket_get_status(klb_socket_t* p_socket);

/// @brief 关闭socket
void klb_socket_close(klb_socket_fd fd);
#define KLB_SOCKET_CLOSE(FD_) {if(INVALID_SOCKET!=(FD_)){klb_socket_close((FD_));(FD_)=INVALID_SOCKET;}}


/// @brief 端口释放后立即就可以被再次使用
/// @param [in]  fd             socket fd
/// @return int 0.成功; 非0.失败
int klb_socket_set_reuseaddr(klb_socket_fd fd);


/// @brief 设置阻塞/非阻塞
/// @param [in]  fd             socket fd
/// @param [in]  blocking       true.阻塞; false.非阻塞
/// @return 无
void klb_socket_set_block(klb_socket_fd fd, bool blocking);


/// @brief 使用TCP连接服务器 域名:端口
/// @param [in]  *p_host        域名: eg. "www.baidu.com", "127.0.0.1"
/// @param [in]  port           端口: eg. 80
/// @param [in]  time_out       超时(毫秒): 0. 表示创建非阻塞socket; 大于0. 表示创建阻塞socket并等待超时
/// @return klb_socket_fd INVALID_SOCKET, 大于0
klb_socket_fd klb_socket_connect(const char* p_host, int port, int time_out);


/// @brief 使用TCP监听端口
/// @param [in]  port           端口: eg. 80
/// @param [in]  max_connect    监听列表最大数目
/// @return klb_socket_fd INVALID_SOCKET, 大于0
klb_socket_fd klb_socket_listen(int port, int max_connect);


/// @brief 使用TCP监听unix节点
/// @param [in]  *p_path        unix节点路径
/// @param [in]  max_connect    监听列表最大数目
/// @return klb_socket_fd INVALID_SOCKET, 大于0
klb_socket_fd klb_socket_listen_unix(const char* p_path, int max_connect);


/// @brief 接收监听到的连接
/// @param [in]  fd_listen      监听fd
/// @param [out] *p_addr        输出监听到的地址
/// @return klb_socket_fd INVALID_SOCKET, 大于0
klb_socket_fd klb_socket_accept(klb_socket_fd fd_listen, struct sockaddr_in* p_addr);


/// @brief 创建一个异步socket
/// @param [in]  fd             socket fd
/// @return klb_socket_t* NULL.创建失败; 非NULL
klb_socket_t* klb_socket_async_create(klb_socket_fd fd);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_H__
//end
