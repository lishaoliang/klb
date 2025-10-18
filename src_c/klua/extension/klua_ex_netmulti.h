///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_netmulti.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n [2025-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_NETMULTI_H__
#define __KLUA_EX_NETMULTI_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netmulti.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_netmulti_t_ klua_ex_netmulti_t;


/// @brief 注册复用扩展
/// @param [in] *p_env           lua环境
/// @return int 0
int klua_ex_register_netmulti(klua_env_t* p_env);


/// @brief 获取复用扩展
/// @param [in] *p_env           lua环境
/// @return klua_ex_netmulti_t* 扩展的指针
klua_ex_netmulti_t* klua_ex_get_netmulti(klua_env_t* p_env);


/// @brief 获取复用
/// @param [in] *p_ex            扩展的指针
/// @return klb_netmulti_t* 复用的指针
klb_netmulti_t* klua_ex_netmulti_get(klua_ex_netmulti_t* p_ex);


klb_netmulti_t* klua_ex_netmulti_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_NETMULTI_H__
//end
