///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/03/27
//
/// @file    klua_exs_thread.h
/// @brief   多线程支持
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/03/27 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EXS_THREAD_H__
#define __KLUA_EXS_THREAD_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klua_exs_thread_t_ klua_exs_thread_t;


/// @brief 注册线程扩展
/// @param [in] 输入参数
/// @param [out] 输出参数
/// @param [in,out] 输入输出参数
/// @return bool 返回值
/// @note 注意事项,参见GetQueryError()
/// @see GetQueryError()
int klua_exs_register_thread(klua_env_t* p_env);


/// @brief 获取线程扩展
/// @param [in] 输入参数
/// @param [out] 输出参数
/// @param [in,out] 输入输出参数
/// @return bool 返回值
/// @note 注意事项,参见GetQueryError()
/// @see GetQueryError()
klua_exs_thread_t* klua_exs_get_thread(klua_env_t* p_env);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EXS_THREAD_H__
//end
