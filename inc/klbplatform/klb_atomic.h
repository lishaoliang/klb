// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_atomic.h
/// @brief   原子变量
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2022 0.2 修改使用方式, 直接提供封装接口(夸系统平台)
///  \n 2023 0.3 修改执行原子变量的类型为 intptr_t, 用于兼容支持32,64位操作系统
/// @note 1. 原子变量尽可能申请在偶数地址, 且4字节对齐
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ATOMIC_H__
#define __KLB_ATOMIC_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @typedef klb_atomic_t
/// @brief   机器字长
typedef intptr_t klb_atomic_t;


/// @brief 原子置零
/// @param [in] *p_atomic   原子变量
/// @return 无
KLB_API void klb_atomic_set_zero(klb_atomic_t volatile* p_atomic);


/// @brief 原子设置值
/// @param [in] *p_atomic   原子变量
/// @param [in] value       新值
/// @return int 设置前的原值
KLB_API int klb_atomic_set_value(klb_atomic_t volatile* p_atomic, klb_atomic_t value);


/// @brief 原子读取值
/// @param [in] *p_atomic   原子变量
/// @return int 当前值
KLB_API int klb_atomic_get_value(klb_atomic_t volatile* p_atomic);


/// @brief 原子加1
/// @param [in] *p_atomic   原子变量
/// @return int 加1前的原值
KLB_API int klb_atomic_add(klb_atomic_t volatile* p_atomic);


/// @brief 原子减1
/// @param [in] *p_atomic   原子变量
/// @return int 减1前的原值
KLB_API int klb_atomic_sub(klb_atomic_t volatile* p_atomic);


/// @brief 原子判断是否为零
/// @param [in] *p_atomic   原子变量
/// @return bool true.为零; false.非零
KLB_API bool klb_atomic_is_zero(klb_atomic_t volatile* p_atomic);


/// @brief 原子自旋加锁
/// @param [in] *p_atomic   原子变量
/// @return 无
/// @note 非递归; 0.未锁, 1.已锁; 临界区须短, 持锁期间禁止阻塞
KLB_API void klb_atomic_lock(klb_atomic_t volatile* p_atomic);


/// @brief 原子尝试加锁
/// @param [in] *p_atomic   原子变量
/// @return bool true.加锁成功; false.加锁失败
KLB_API bool klb_atomic_try_lock(klb_atomic_t volatile* p_atomic);


/// @brief 原子解锁
/// @param [in] *p_atomic   原子变量
/// @return 无
/// @note 解锁 klb_atomic_lock / klb_atomic_try_lock
KLB_API void klb_atomic_unlock(klb_atomic_t volatile* p_atomic);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ATOMIC_H__

// end
