///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_multiplex.h
/// @brief   多路复用
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_MULTIPLEX_H__
#define __KLUA_EX_MULTIPLEX_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "socket/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef klb_socket_fd   klua_ex_fd;
#define INVALID_EX_FD   INVALID_SOCKET

typedef struct klua_ex_multiplex_t_ klua_ex_multiplex_t;

int klua_ex_register_multiplex(klua_env_t* p_env);

klua_ex_multiplex_t* klua_ex_get_multiplex(klua_env_t* p_env);

typedef struct klua_ex_multiplex_obj_t_
{
    int (*cb_recv)(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now);
    int (*cb_send)(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now);

    void* p_lparam;
    void* p_wparam;
}klua_ex_multiplex_obj_t;

int klua_ex_multiplex_push_socket(klua_ex_multiplex_t* p_ex, int id, klb_socket_t* p_socket, const klua_ex_multiplex_obj_t* p_obj);

int klua_ex_multiplex_remove(klua_ex_multiplex_t* p_ex, int id);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_MULTIPLEX_H__
//end
