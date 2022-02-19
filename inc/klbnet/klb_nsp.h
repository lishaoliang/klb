///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_nsp.h
/// @brief   net server preprocess, 网络连接预处理模块
///          处理监听等,主要为识别协议
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NSP_H__
#define __KLB_NSP_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_multiplex.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_nsp_t_ klb_nsp_t;


/// @brief 创建nsp(net server preprocess); 网络连接预处理模块
/// @param [in]  *p_multi               复用模块
/// @return klb_nsp_t* 管理模块
KLB_API klb_nsp_t* klb_nsp_create(klb_multiplex_t* p_multi);


/// @brief 销毁nsp
/// @param [in]  *p_nsp                 nsp模块
/// @return 无
KLB_API void klb_nsp_destroy(klb_nsp_t* p_nsp);


typedef int(*klb_nsp_accept_cb)(void* ptr, int protocol, klb_socket_t* p_socket, const klb_buf_t* p_buf);


KLB_API int klb_nsp_set_accept(klb_nsp_t* p_nsp, klb_nsp_accept_cb cb_accept, void* ptr);


/// @brief 放入socket
/// @param [in]  *p_nsp                 nsp模块
/// @return 无
KLB_API void klb_nsp_push(klb_nsp_t* p_nsp, klb_socket_t* p_socket);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NSP_H__
//end
