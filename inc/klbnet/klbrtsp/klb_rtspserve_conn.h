///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rtspserve_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   RTSP服务连接(klb rtsp serve connect)
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RTSPSERVE_CONN_H__
#define __KLB_RTSPSERVE_CONN_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netconn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 创建rtsp连接
/// @return klb_netconn_t*
KLB_API klb_netconn_t* klb_rtspserve_conn_create(klb_netmulti_t* p_netmulti, klb_socket_t* p_socket);



#ifdef __cplusplus
}
#endif


#endif // __KLB_RTSPSERVE_CONN_H__
//end
