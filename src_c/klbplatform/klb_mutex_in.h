// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mutex_in.h
/// @brief   互斥量内部结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MUTEX_IN_H__
#define __KLB_MUTEX_IN_H__

#include "klbplatform/klb_mutex.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif


#ifdef _WIN32

/// @struct klb_mutex_t
/// @brief  通用(互斥量)锁
struct klb_mutex_t_
{
    CRITICAL_SECTION    section;    ///< 互斥量
};

#else

/// @struct klb_mutex_t
/// @brief  通用(互斥量)锁
struct klb_mutex_t_
{
    pthread_mutex_t     mutex;      ///< 互斥量
};

#endif


#if defined(__cplusplus)
}
#endif

#endif // __KLB_MUTEX_IN_H__

// end
