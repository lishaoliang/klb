///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_multiplex.h
/// @brief   异步复用( socket/fd )
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_MULTIPLEX_H__
#define __KLUA_MULTIPLEX_H__

#include "klb_type.h"
#include "klbnet/klb_multiplex.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 获取复用
/// @param [in]  *p_env         Lua环境 
/// @return klb_multiplex_t* 
KLB_API klb_multiplex_t* klua_multiplex_get(klua_env_t* p_env);


/// @brief 获取复用
/// @param [in]  *L             lua_State 
/// @return klb_multiplex_t* 
KLB_API klb_multiplex_t* klua_multiplex_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_MULTIPLEX_H__
//end
