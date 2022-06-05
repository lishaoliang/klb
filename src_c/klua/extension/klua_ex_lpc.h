///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_lpc.h
/// @brief   LPC: 本地(跨Lua线程)通信(Local Procedure Call Protocol)
/// @version 0.1
/// @history 修改历史
///  \n 2021 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_LPC_H__
#define __KLUA_EX_LPC_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klua/klua_data.h"
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


typedef int(*klua_ex_lpc_msg_cb)(void* ptr, klua_msg_t* p_msg);


int klua_ex_lpc_new_module(klua_ex_lpc_t* p_ex, const sds name, klua_ex_lpc_msg_cb cb_msg, void* ptr);
sds klua_ex_lpc_new_lpc(klua_ex_lpc_t* p_ex, klua_ex_lpc_msg_cb cb_msg, void* ptr);
int klua_ex_lpc_delete(klua_ex_lpc_t* p_ex, const sds name);

#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_LPC_H__
//end
