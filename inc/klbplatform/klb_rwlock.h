///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_rwlock.h
/// @brief   读写锁
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RWLOCK_H__
#define __KLB_RWLOCK_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(__EMSCRIPTEN__)

// 读写锁
typedef struct klb_rwlock_t_ klb_rwlock_t;


/// @brief 创建锁
/// @return klb_rwlock_t*    锁对象
KLB_API klb_rwlock_t* klb_rwlock_create();


/// @brief 销毁锁
/// @param [in] *p_rwlock    锁对象
/// @return 无
KLB_API void klb_rwlock_destroy(klb_rwlock_t* p_rwlock);


/// @brief (读)加锁
/// @param [in] *p_rwlock    锁对象
/// @return 无
KLB_API void klb_rwlock_rdlock(klb_rwlock_t* p_rwlock);

/// @brief (读)尝试加锁
/// @param [in] *p_rwlock    锁对象
/// @return int 0.加锁成功; 非0.加锁失败
KLB_API int klb_rwlock_try_rdlock(klb_rwlock_t* p_rwlock);

/// @brief (读)解锁
/// @param [in] *p_rwlock    锁对象
/// @return 无
KLB_API void klb_rwlock_rdunlock(klb_rwlock_t* p_rwlock);


/// @brief (写)加锁
/// @param [in] *p_rwlock    锁对象
/// @return 无
KLB_API void klb_rwlock_wrlock(klb_rwlock_t* p_rwlock);

/// @brief (写)尝试加锁
/// @param [in] *p_rwlock    锁对象
/// @return int 0.加锁成功; 非0.加锁失败
KLB_API int klb_rwlock_try_wrlock(klb_rwlock_t* p_rwlock);

/// @brief (写)解锁
/// @param [in] *p_rwlock    锁对象
/// @return 无
KLB_API void klb_rwlock_wrunlock(klb_rwlock_t* p_rwlock);


#endif // __EMSCRIPTEN__

#ifdef __cplusplus
}
#endif

#endif // __KLB_RWLOCK_H__
//end
