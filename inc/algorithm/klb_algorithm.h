///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/09
//
/// @file    klb_algorithm.h
/// @brief   基础算法通用定义等
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/09 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ALGORITHM_H__
#define __KLB_ALGORITHM_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 排序比较回调函数
/// @param [in] *p_k1       前一个元素
/// @param [in] *p_k2       后一个元素
/// @return int 返回值[-1, 0, 1], 参考memcmp返回值
/// @note k1比k2相对位置靠前
typedef int(*klb_sort_compare_cb)(void* p_k1, void* p_k2);


#ifdef __cplusplus
}
#endif

#endif // __KLB_ALGORITHM_H__
//end
