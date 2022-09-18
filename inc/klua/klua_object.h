///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_object.h
/// @brief   object注册使用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_OBJECT_H__
#define __KLUA_OBJECT_H__

#include "klb_type.h"
#include "klbutil/klb_obj.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_object_t_ klua_ex_object_t;


/// @brief 获取object扩展
/// @param [in]  *p_env         Lua环境 
/// @return klua_ex_object_t* 
KLB_API klua_ex_object_t* klua_object_get(klua_env_t* p_env);


/// @brief 获取object扩展
/// @param [in]  *L             lua_State 
/// @return klua_ex_object_t* 
KLB_API klua_ex_object_t* klua_object_get_by_L(lua_State* L);


/// @brief 注册object ops
/// @param [in]  *p_ex          ojbect扩展
/// @param [in]  *p_ops         ops: p_ops->p_module, p_ops->p_name
/// @return 0.成功; 非0.失败
KLB_API int klua_object_register_ops(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops);


KLB_API int klua_object_register_ops_array(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops);


/// @brief 获取ops
/// @param [in]  *p_ex          ojbect扩展
/// @param [in]  *p_module      模块名
/// @param [in]  *p_name        名称
/// @return klb_obj_ops_t*
KLB_API const klb_obj_ops_t* klua_object_get_ops(klua_ex_object_t* p_ex, const char* p_module, const char* p_name);



#define KLUA_KOBJECT_HANDLE     "KLUA_KOBJECT_HANDLE"

#define to_klb_obj(L, UD_)      ((klb_obj_t*)luaL_checkudata(L, UD_, KLUA_KOBJECT_HANDLE))


#ifdef __cplusplus
}
#endif

#endif // __KLUA_OBJECT_H__
//end
