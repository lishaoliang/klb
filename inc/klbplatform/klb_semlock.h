///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_semlock.h
/// @brief   (IPC)进程之间信号量(Semaphore)
/// @version 0.1
/// @history 修改历史
///  \n 2021 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SEMLOCK_H__
#define __KLB_SEMLOCK_H__


#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_semlock_t_ klb_semlock_t;


/// @brief 创建进程锁
/// @return klb_semlock_t* 进程锁
KLB_API klb_semlock_t* klb_semlock_create(const char* p_name);


/// @brief 打开进程锁
/// @return klb_semlock_t* 进程锁
KLB_API klb_semlock_t* klb_semlock_open(const char* p_name);


/// @brief 销毁进程锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
KLB_API void klb_semlock_destroy(klb_semlock_t* p_semlock);


/// @brief 加锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
KLB_API void klb_semlock_lock(klb_semlock_t* p_semlock);


/// @brief 解锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
KLB_API void klb_semlock_unlock(klb_semlock_t* p_semlock);


#ifdef __cplusplus
}
#endif

#endif // __KLB_SEMLOCK_H__
//end
