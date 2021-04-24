///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/04
//
/// @file    klua_kthread.h
/// @brief   klua多线程部分接口
///        方式1. 由klua_kthread模块完全托管: klua_kthread负责线程的完整生命周期
///        方式2. 由外部管理线程生命周期, klua_kthread仅仅负责做线程之间沟通
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/04 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_KTHREAD_H__
#define __KLUA_KTHREAD_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klbutil/klb_list.h"
#include "klbthird/sds.h"
#include "klua/klua_data.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_env_t klua_env_t;


//typedef struct klua_thread_msg_t_
//{
//    int         flag;           // 0, 1, 2
//    sds         ex_name;
//    sds         func_name;
//    sds         sequence;
//}klua_thread_msg_t;


/// @brief 放入消息
/// @return int 0
KLB_API int klua_kthread_push_msg(const char* p_name, klua_msg_t* p_msg);


/// @brief 取消息
/// @return int 0
KLB_API int klua_kthread_get_msg(const char* p_name, klb_list_t* p_list);



KLB_API int klua_kthread_register(const char* p_name, klua_env_t* p_env);

KLB_API int klua_kthread_unregister(const char* p_name);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_KTHREAD_H__
//end
