///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/04
//
/// @file    klua_ex_lpc.h
/// @brief   LPC: 本地(跨Lua线程)过程调用(Local Procedure Call Protocol)
///        1. 调用本进程其他Lua线程注册的函数
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/04 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_LPC_H__
#define __KLUA_EX_LPC_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbthird/sds.h"
#include "klua/klua_kthread.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLUA_EX_LPC_NAME      "_KLUA_EX_LPC_"


typedef struct klua_ex_lpc_t_ klua_ex_lpc_t;


/// @brief 注册LPC: 本地(跨Lua线程)过程调用
/// @param [in]  *p_env         Lua环境 
/// @return int 0
int klua_ex_register_lpc(klua_env_t* p_env);


/// @brief 获取LPC: 本地(跨Lua线程)过程调用
/// @param [in]  *p_env         Lua环境 
/// @return klua_ex_rpc_t* LPC对象
klua_ex_lpc_t* klua_ex_get_lpc(klua_env_t* p_env);


/// @brief 注册函数
/// @param [in]  *p_ex          LPC对象
/// @return 无
void klua_ex_lpc_register_function(klua_ex_lpc_t* p_ex, const char* p_func_name, size_t name_len, int func_idx);


/// @brief 等待响应的回调函数
/// @param [in]  *ptr           用户指针
/// @param [in]  *p_ex          LPC对象
/// @param [in]  *p_msg         响应消息
/// @return int 0
typedef int (*klua_ex_lpc_response_cb)(void* ptr, klua_ex_lpc_t* p_ex, klua_lpc_msg_t* p_msg);


/// @brief 一个新的等待响应
/// @param [in]  *p_ex          LPC对象
/// @return sds 序列号
sds klua_ex_lpc_new_wait_response(klua_ex_lpc_t* p_ex, klua_ex_lpc_response_cb cb, void* ptr);


/// @brief 移除新的等待响应
/// @param [in]  *p_ex          LPC对象
/// @return 无
void klua_ex_lpc_remove_response(klua_ex_lpc_t* p_ex, sds sequence);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_LPC_H__
//end
