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


KLB_API void klb_atomic_set_zero(klb_atomic_t volatile* p_atomic);
KLB_API int klb_atomic_set_value(klb_atomic_t volatile* p_atomic, klb_atomic_t value);
KLB_API int klb_atomic_get_value(klb_atomic_t volatile* p_atomic);
KLB_API int klb_atomic_add(klb_atomic_t volatile* p_atomic);
KLB_API int klb_atomic_sub(klb_atomic_t volatile* p_atomic);
KLB_API bool klb_atomic_is_zero(klb_atomic_t volatile* p_atomic);
KLB_API void klb_atomic_lock(klb_atomic_t volatile* p_atomic);
KLB_API bool klb_atomic_try_lock(klb_atomic_t volatile* p_atomic);
KLB_API void klb_atomic_unlock(klb_atomic_t volatile* p_atomic);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ATOMIC_H__
//end
