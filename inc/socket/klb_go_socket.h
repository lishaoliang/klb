#ifndef __KLB_GO_SOCKET_H__
#define __KLB_GO_SOCKET_H__

#include "klb_type.h"
#include "socket/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_go_socket_t_ klb_go_socket_t;


typedef void(*klb_go_socket_destroy_cb)(klb_go_socket_t* p_socket);
typedef int(*klb_go_socket_send_cb)(klb_go_socket_t* p_socket, const char* p_buf, int buf_len);
typedef int(*klb_go_socket_recv_cb)(klb_go_socket_t* p_socket, char* p_buf, int buf_len);
typedef int(*klb_go_socket_sendto_cb)(klb_go_socket_t* p_socket, const char* p_buf, int buf_len, const struct sockaddr* p_addr, int addr_len);
typedef int(*klb_go_socket_recvfrom_cb)(klb_go_socket_t* p_socket, char* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len);


/// @struct klb_go_socket_t
/// @brief  提供给go使用的socket
typedef struct klb_go_socket_t_
{
    klb_socket_fd                   fd;             ///< 系统fd

    struct
    {
        klb_go_socket_destroy_cb    cb_destroy;     ///< 销毁

        klb_go_socket_send_cb       cb_send;        ///< TCP发送
        klb_go_socket_recv_cb       cb_recv;        ///< TCP接收
        klb_go_socket_sendto_cb     cb_sendto;      ///< UDP发送
        klb_go_socket_recvfrom_cb   cb_recvfrom;    ///< UDP接收
    };
}klb_go_socket_t;


/// @brief 创建Go Socket
/// @param [in] fd           系统fd
/// @return klb_go_socket_t* go socket对象
klb_go_socket_t* klb_go_socket_create(klb_socket_fd fd);


/// @brief 销毁Go Socket
/// @param [in] *p_socket   go socket对象
/// @return 无
void klb_go_socket_destroy(klb_go_socket_t* p_socket);


/// @brief TCP发送数据
/// @param [in] *p_socket   go socket对象
/// @param [in] *p_buf      发送的缓存
/// @param [in] buf_len     缓存长度
/// @return int 
int klb_go_socket_send(klb_go_socket_t* p_socket, const char* p_buf, int buf_len);


/// @brief TCP接收数据
/// @param [in] *p_socket   go socket对象
/// @param [in] *p_buf      接收的缓存
/// @param [in] buf_len     缓存长度
/// @return int
int klb_go_socket_recv(klb_go_socket_t* p_socket, char* p_buf, int buf_len);


/// @brief UDP发送数据
/// @param [in] *p_socket   go socket对象
/// @param [in] *p_buf      发送的缓存
/// @param [in] buf_len     缓存长度
/// @param [in] *p_addr     目标地址
/// @param [in] addr_len    地址长度
/// @return int 
int klb_go_socket_sendto(klb_go_socket_t* p_socket, const char* p_buf, int buf_len, const struct sockaddr* p_addr, int addr_len);


/// @brief UDP接收数据数据
/// @param [in] *p_socket   go socket对象
/// @param [in] *p_buf      接收的缓存
/// @param [in] buf_len     缓存长度
/// @param [in] *p_addr     收到数据的地址
/// @param [in] addr_len    地址长度
/// @return int
int klb_go_socket_recvfrom(klb_go_socket_t* p_socket, char* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GO_SOCKET_H__
//end
