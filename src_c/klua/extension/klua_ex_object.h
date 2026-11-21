///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_object.h
/// @brief   object 扩展
/// @version 0.2
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_OBJECT_H__
#define __KLUA_EX_OBJECT_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLUAEX_object     "_KLUAEX-object_"


typedef struct klua_ex_object_t_ klua_ex_object_t;


/// @brief 注册
/// @param [in]  *p_env         Lua环境
/// @return int 0
int klua_ex_register_object(klua_env_t* p_env);


klua_ex_object_t* klua_ex_get_object(klua_env_t* p_env);
klua_ex_object_t* klua_ex_get_object_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_OBJECT_H__
// end
