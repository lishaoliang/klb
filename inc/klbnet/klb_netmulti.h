///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netmulti.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   (klb net multiplex)使用select/epool等多路复用; 适用于异步流程
/// @version 0.1
/// @history 修改历史
///  \n [2025-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETMULTI_H__
#define __KLB_NETMULTI_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_netmulti_t
/// @brief  多路复用(klb net multiplex)
typedef struct klb_netmulti_t_ klb_netmulti_t;


/// @brief 创建
/// @return klb_netmulti_t*             复用对象
KLB_API klb_netmulti_t* klb_netmulti_create(int64_t tc);


/// @brief 销毁
/// @param [in]  *p_multi               复用对象
/// @return 无
KLB_API void klb_netmulti_destroy(klb_netmulti_t* p_multi);


/// @brief 放入连接对象
/// @param [in]  *p_multi               复用对象
/// @param [in]  *p_conn                连接对象
/// @return int 0.成功; 非0. 失败
/// @note 只负责使用, 并不负责p_conn销毁
KLB_API int klb_netmulti_push(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);


/// @brief 移除连接对象
/// @return int 0.成功; 非0. 失败
KLB_API int klb_netmulti_remove(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);
KLB_API klb_netconn_t* klb_netmulti_remove_by_name(klb_netmulti_t* p_multi, const char* p_name);


/// @brief 调用一次; 需要定期调用
/// @param [in] *p_multi                复用对象
/// @param [in] now                     当前系统滴答数
/// @return int 0
KLB_API int klb_netmulti_loop_once(klb_netmulti_t* p_multi, int64_t now);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETMULTI_H__
//end
