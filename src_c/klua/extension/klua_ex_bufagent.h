///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_bufagent.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   代理内存池 扩展
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_BUFAGENT_H__
#define __KLUA_EX_BUFAGENT_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_bufpoolfix.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_ex_bufagent_t_ klua_ex_bufagent_t;


/// @brief 注册
/// @return int 
int klua_ex_register_bufagent(klua_env_t* p_env);

/// @brief 获取
klua_ex_bufagent_t* klua_ex_get_bufagent(klua_env_t* p_env);


/// @brief 获取内存池指针
klb_bufpoolfix_t* klua_ex_bufagent_get_bufpoolfix(klua_ex_bufagent_t* p_ex);


/// @brief 获取内存代理
int klua_ex_bufagent_get_bufpoolfix2(klua_ex_bufagent_t* p_ex, klb_bufagent_t* p_out);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_BUFAGENT_H__
//end
