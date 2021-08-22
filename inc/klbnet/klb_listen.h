///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_listen.h
/// @brief   网路监听
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LISTEN_H__
#define __KLB_LISTEN_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbbase/klb_multiplex.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_listen_t_ klb_listen_t;


/// @brief 创建listen
/// @return klb_listen_t* 管理模块
KLB_API klb_listen_t* klb_listen_create(klb_multiplex_t* p_multi);


/// @brief 销毁listen
/// @param [in]  *p_listen          listen模块
/// @return 无
KLB_API void klb_listen_destroy(klb_listen_t* p_listen);


KLB_API int klb_listen_open(klb_listen_t* p_listen, int port, int max_connect);
KLB_API int klb_listen_open_unix(klb_listen_t* p_listen, int port, int max_connect);
KLB_API int klb_listen_close(klb_listen_t* p_listen);


typedef int(*klb_listen_accept_cb)(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr);


KLB_API int klb_listen_set_accept(klb_listen_t* p_listen, klb_listen_accept_cb cb_accept, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_LISTEN_H__
//end
