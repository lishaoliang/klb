///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_iopoll_in.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   网络底层IO模型
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_IOPOLL_IN_H__
#define __KLB_IOPOLL_IN_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_netmulti_t_ klb_netmulti_t;


typedef struct klb_iopoll_ops_t_ 
{
    /// @brief 创建
    /// @return void* 
    klb_netmulti_t* (*create)(int64_t tc);

    /// @brief 销毁
    /// @param [in] *ptr            句柄
    /// @return 无
    void (*destroy)(klb_netmulti_t* p_multi);


    int (*push)(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);

    int (*remove)(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);
    klb_netconn_t* (*remove_by_name)(klb_netmulti_t* ptr, const char* p_name);

    int (*closing)(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);
    int (*loop_once)(klb_netmulti_t* p_multi, int64_t now);
} klb_iopoll_ops_t;


/// @struct klb_netmulti_t
/// @brief  多路复用(klb net multiplex)
typedef struct klb_netmulti_t_
{
    klb_iopoll_ops_t    ops;
    char                extra[];
}klb_netmulti_t;


extern klb_iopoll_ops_t g_klb_iopoll_ops;


#ifdef __cplusplus
}
#endif

#endif // __KLB_IOPOLL_IN_H__
//end
