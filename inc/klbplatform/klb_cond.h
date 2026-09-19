// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_cond.h
/// @brief   条件变量
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_COND_H__
#define __KLB_COND_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#if !defined(__EMSCRIPTEN__)

#include "klbplatform/klb_mutex.h"


/// @struct klb_cond_t
/// @brief  条件变量
typedef struct klb_cond_t_ klb_cond_t;


/// @brief 创建条件变量
/// @return klb_cond_t*     条件变量对象
KLB_API klb_cond_t* klb_cond_create();


/// @brief 销毁条件变量
/// @param [in] *p_cond     条件变量对象
/// @return 无
KLB_API void klb_cond_destroy(klb_cond_t* p_cond);


/// @brief 等待条件变量
/// @param [in] *p_cond     条件变量对象
/// @param [in] *p_mutex    已持有的互斥量
/// @return 无
/// @note 调用前须持有 *p_mutex; 等待期间会释放并在返回前重新持有
///  \n 可能虚假唤醒, 调用方须循环检查条件
KLB_API void klb_cond_wait(klb_cond_t* p_cond, klb_mutex_t* p_mutex);


/// @brief 限时等待条件变量
/// @param [in] *p_cond     条件变量对象
/// @param [in] *p_mutex    已持有的互斥量
/// @param [in] ms          超时毫秒
/// @return int 0.等到信号; 非0.超时或失败
/// @note 调用前须持有 *p_mutex; 等待期间会释放并在返回前重新持有
///  \n 可能虚假唤醒, 调用方须循环检查条件
KLB_API int klb_cond_timedwait(klb_cond_t* p_cond, klb_mutex_t* p_mutex, uint32_t ms);


/// @brief 唤醒一个等待线程
/// @param [in] *p_cond     条件变量对象
/// @return 无
KLB_API void klb_cond_signal(klb_cond_t* p_cond);


/// @brief 唤醒全部等待线程
/// @param [in] *p_cond     条件变量对象
/// @return 无
KLB_API void klb_cond_broadcast(klb_cond_t* p_cond);


#endif

#ifdef __cplusplus
}
#endif

#endif // __KLB_COND_H__

// end
