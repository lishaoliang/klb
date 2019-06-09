///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/09
//
/// @file    klb_sort_insert.h
/// @brief   插入排序
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/09 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SORT_INSERT_H__
#define __KLB_SORT_INSERT_H__

#include "klb_type.h"
#include "algorithm/klb_algorithm.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 插入排序
/// @param [in] *p_base     待排序的指针
/// @param [in] nitems      元素个数
/// @param [in] width       每个元素大小(字节)
/// @param [in] cb_compare  比较函数
/// @return 无
KLB_API void klb_sort_insert(void* p_base, size_t nitems, size_t width, klb_sort_compare_cb cb_compare);



#ifdef __cplusplus
}
#endif

#endif // __KLB_SORT_INSERT_H__
//end
