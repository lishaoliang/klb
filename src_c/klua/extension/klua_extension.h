///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_extension.h
/// @brief   标准lua扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EXTENSION_H__
#define __KLUA_EXTENSION_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief (内部)注册标准 C扩展
/// @return int 0.成功; 非0. 失败
int klua_register_extension_std(klua_env_t* p_env);


#if !defined(__KLB_NO_CPP__)
/// @brief (内部)注册标准 CPP扩展
/// @return int 0.成功; 非0. 失败
///  由 .\klb\src_cpp\klua\extension\CKluaExtension.cpp 实现本函数
int klua_register_extension_std_cpp(klua_env_t* p_env);


/// @brief 支持CPP-gui
int klua_using_cpp_gui(klua_env_t* p_env);
#endif


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EXTENSION_H__
//end
