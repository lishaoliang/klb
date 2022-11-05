///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_thread.h
/// @brief   klua多线程部分接口
///        方式1. 由klua_thread模块完全托管: klua_thread负责线程的完整生命周期
///        方式2. 由外部管理线程生命周期, klua_kthread仅仅负责做线程之间沟通
/// @version 0.1
/// @history 修改历史
///  \n 2021 0.1 创建文件
///  \n 2022 0.2 调整LPC/LPC模块实现机制
///   从原来LPC接口绑定线程与klua_env_t; 调整为与线程无关,可以任意定义模块并接收处理
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_THREAD_H__
#define __KLUA_THREAD_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 设置lua预加载库函数
/// @param [in] cb          预加载库函数
/// @return 无
KLB_API void klua_thread_set_preload(klua_openlibs_cb cb);


/// @brief 注册外部的 klua_env_t*
/// @param [in] *p_name     名称
/// @param [in] *p_env      环境
/// @return int 0.成功; 非0.失败
KLB_API int klua_thread_register(const char* p_name, klua_env_t* p_env);
KLB_API int klua_thread_unregister(const char* p_name);


/// @brief 注册lpc模块
/// @param [in] name        名称
/// @param [in] *p_env      环境
/// @return int 0.成功; 非0.失败
KLB_API int klua_thread_register_lpc_module(const sds name, klua_env_t* p_env);
KLB_API int klua_thread_unregister_lpc_module(const sds name);


/// @brief 注册lpc
/// @param [in] name        名称
/// @param [in] *p_env      环境
/// @return int 0.成功; 非0.失败
KLB_API sds klua_thread_register_lpc(klua_env_t* p_env);
KLB_API int klua_thread_unregister_lpc(const sds name);


/// @brief 放入lpc消息
/// @param [in] *p_name     名称
/// @param [in] *p_msg      消息
/// @return int 0.成功; 非0.失败
KLB_API int klua_thread_push_lpc_msg(const char* p_name, klua_msg_t* p_msg);



#ifdef __cplusplus
}
#endif

#endif // __KLUA_THREAD_H__
//end
