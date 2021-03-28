///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_transmit.h
/// @brief   socket数据异步传输
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_TRANSMIT_H__
#define __KLB_SOCKET_TRANSMIT_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"


#if defined(__cplusplus)
extern "C" {
#endif



typedef struct klb_socket_transmit_t_ klb_socket_transmit_t;


klb_socket_transmit_t* klb_socket_transmit_create();
void klb_socket_transmit_destroy(klb_socket_transmit_t* p_transmit);


int klb_socket_transmit_start(klb_socket_transmit_t* p_transmit, int cpu_idx, const char* p_name);
void klb_socket_transmit_stop(klb_socket_transmit_t* p_transmit);





#ifdef __cplusplus
}
#endif

#endif // __KLB_SOCKET_TRANSMIT_H__
//end
