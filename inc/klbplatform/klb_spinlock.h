// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_spinlock.h
/// @brief   自旋锁
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SPINLOCK_H__
#define __KLB_SPINLOCK_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


#if !defined(__EMSCRIPTEN__)


/// @struct klb_spinlock_t
/// @brief  自旋锁
typedef struct klb_spinlock_t_ klb_spinlock_t;


/// @brief 创建锁
/// @return klb_spinlock_t* 锁对象
KLB_API klb_spinlock_t* klb_spinlock_create();


/// @brief 销毁锁
/// @param [in] *p_spinlock  锁对象
/// @return 无
KLB_API void klb_spinlock_destroy(klb_spinlock_t* p_spinlock);


/// @brief 加锁
/// @param [in] *p_spinlock  锁对象
/// @return 无
/// @note 非递归; 临界区须短, 持锁期间禁止阻塞
KLB_API void klb_spinlock_lock(klb_spinlock_t* p_spinlock);


/// @brief 尝试加锁
/// @param [in] *p_spinlock  锁对象
/// @return int 0.加锁成功; 非0.加锁失败
KLB_API int klb_spinlock_trylock(klb_spinlock_t* p_spinlock);


/// @brief 解锁
/// @param [in] *p_spinlock  锁对象
/// @return 无
/// @note 解锁 klb_spinlock_lock / klb_spinlock_trylock
KLB_API void klb_spinlock_unlock(klb_spinlock_t* p_spinlock);


#endif

#ifdef __cplusplus
}
#endif

#endif // __KLB_SPINLOCK_H__

// end
