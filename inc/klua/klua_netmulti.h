///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_netmulti.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klua net multiplex 复用
/// @version 0.1
/// @history 修改历史
///  \n [2025-09] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_NETMULTI_H__
#define __KLUA_NETMULTI_H__

#include "klb_type.h"
#include "klbnet/klb_netmulti.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 获取复用
/// @param [in]  *p_env         Lua环境 
/// @return klb_netmulti_t* 
KLB_API klb_netmulti_t* klua_netmulti_get(klua_env_t* p_env);


/// @brief 获取复用
/// @param [in]  *L             lua_State 
/// @return klb_netmulti_t* 
KLB_API klb_netmulti_t* klua_netmulti_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_NETMULTI_H__
//end
