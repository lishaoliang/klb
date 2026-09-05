///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_object.h
/// @brief   object 扩展: 支持 klb_obj_t
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_OBJECT_H__
#define __KLUA_EX_OBJECT_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbutil/klb_obj.h"

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


int klua_ex_object_register_ops(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops);
int klua_ex_object_register_ops_array(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops);

const klb_obj_ops_t* klua_ex_object_get_ops(klua_ex_object_t* p_ex, const char* p_module, const char* p_name);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_OBJECT_H__
//end
