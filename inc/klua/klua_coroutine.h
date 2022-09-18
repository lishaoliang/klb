///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_coroutine.h
/// @brief   Lua协程扩展: 标准扩展协程导出函数
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_COROUTINE_H__
#define __KLUA_COROUTINE_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_coroutine_t_ klua_ex_coroutine_t;


/// @brief 获取协程扩展
/// @param [in]  *p_env         Lua环境 
/// @return klua_ex_coroutine_t* 
KLB_API klua_ex_coroutine_t* klua_coroutine_get(klua_env_t* p_env);


/// @brief 获取协程扩展
/// @param [in]  *L             lua_State 
/// @return klua_ex_coroutine_t* 
KLB_API klua_ex_coroutine_t* klua_coroutine_get_by_L(lua_State* L);


/// @brief 将协程入口函数放入栈顶
/// @param [in]  *p_ex         协程扩展
/// @param [in]  *p_co         当前协程
/// @return lua_State* 注册协程的主程; 可能为 NULL
KLB_API lua_State* klua_coroutine_rawgeti(klua_ex_coroutine_t* p_ex, lua_State* p_co);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_COROUTINE_H__
//end
