///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/19
//
/// @file    klb_mem.h
/// @brief   基础内存申请/释放
/// @author  李绍良
/// @version 0.1
/// @history 修改历史
///   \n 2019/05/19 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MEM_H__
#define __KLB_MEM_H__

#include "klb_type.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif


#if 0

/// @brief 申请内存
/// @param [in] size    需申请的内存大小
/// @return void* 申请到的内存地址
KLB_API void* klb_malloc(size_t size);


/// @brief 释放内存
/// @param [in] *p      待释放的内存地址
/// @return 无
KLB_API void klb_free(void* p);

#endif


/// @def   KLB_MALLOC
/// @brief 内存申请; 按结构体数目 + 对齐字节
#define KLB_MALLOC(ST_, NUM_, PADDING_)     (ST_*)malloc(sizeof(ST_) * (NUM_) + (PADDING_))


/// @def   KLB_MEMSET
/// @brief memset, 加入了断言
#define KLB_MEMSET(PTR_, VAL_, SIZE_)       {assert(NULL!=(PTR_));memset(PTR_,VAL_,SIZE_);}


/// @def   KLB_FREE
/// @brief 释放内存, 并将指针置空
#define KLB_FREE(PTR_)                      {if(NULL!=(PTR_)){free(PTR_);(PTR_)=NULL;}}


/// @def   KLB_FREE_BY
/// @brief 通过释放函数释放结构体对象
#define KLB_FREE_BY(PTR_, FUNC_FREE_)       {if(NULL!=(PTR_)){(FUNC_FREE_)(PTR_);(PTR_)=NULL;}}



#ifdef __cplusplus
}
#endif

#endif // __KLB_MEM_H__
//end
