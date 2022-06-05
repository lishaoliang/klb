///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_atomic.h
/// @brief   原子变量
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
///  \n 2022 0.2 修改使用方式, 直接提供封装接口(夸系统平台)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ATOMIC_H__
#define __KLB_ATOMIC_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API void klb_atomic_set_zero(long volatile* p_atomic);
KLB_API int klb_atomic_set_value(long volatile* p_atomic, int value);
KLB_API int klb_atomic_get_value(long volatile* p_atomic);
KLB_API int klb_atomic_add(long volatile* p_atomic);
KLB_API int klb_atomic_sub(long volatile* p_atomic);
KLB_API bool klb_atomic_is_zero(long volatile* p_atomic);
KLB_API void klb_atomic_lock(long volatile* p_atomic);
KLB_API bool klb_atomic_try_lock(long volatile* p_atomic);
KLB_API void klb_atomic_unlock(long volatile* p_atomic);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ATOMIC_H__
//end
