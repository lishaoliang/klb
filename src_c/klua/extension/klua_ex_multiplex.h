///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_multiplex.h
/// @brief   复用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_MULTIPLEX_H__
#define __KLUA_EX_MULTIPLEX_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_multiplex.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_multiplex_t_ klua_ex_multiplex_t;


/// @brief 注册复用扩展
/// @param [in] *p_env           lua环境
/// @return int 0
int klua_ex_register_multiplex(klua_env_t* p_env);


/// @brief 获取复用扩展
/// @param [in] *p_env           lua环境
/// @return klua_ex_multiplex_t* 扩展的指针
klua_ex_multiplex_t* klua_ex_get_multiplex(klua_env_t* p_env);


/// @brief 获取复用
/// @param [in] *p_ex            扩展的指针
/// @return klb_multiplex_t* 复用的指针
klb_multiplex_t* klua_ex_multiplex_get(klua_ex_multiplex_t* p_ex);


klb_multiplex_t* klua_ex_multiplex_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_MULTIPLEX_H__
//end
