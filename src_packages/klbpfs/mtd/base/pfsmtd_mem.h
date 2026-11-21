// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_mem.h
/// @brief   平台堆内存封装
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    Win/Linux 平台堆; 失败返回 NULL, 由调用方处理
///  \n 同步自 portfs base/pfs_mem.*; 本库自持, 勿 include portfs 内部头
///  \n 业务代码勿直接 malloc/free; 对齐块用 pfsmtd_aligned_*
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_MEM_H_
#define _PFSMTD_MEM_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 分配堆内存
void *pfsmtd_malloc(size_t size);

/// @brief 分配并清零
void *pfsmtd_mallocz(size_t size);

/// @brief 分配 nmemb*size 并清零
void *pfsmtd_calloc(size_t nmemb, size_t size);

/// @brief 调整已分配块大小
void *pfsmtd_realloc(void *ptr, size_t size);

/// @brief 释放堆内存
void pfsmtd_free(void *ptr);

/// @brief 分配对齐堆内存
/// @param [in] align         须为 2 的幂且 >= sizeof(void*)
/// @param [in] size          字节数
/// @return void* 失败返回 NULL; 须用 pfsmtd_aligned_free 释放
void *pfsmtd_aligned_malloc(size_t align, size_t size);

/// @brief 分配对齐堆内存并清零
void *pfsmtd_aligned_mallocz(size_t align, size_t size);

/// @brief 释放对齐堆内存 (勿与 pfsmtd_free 混用)
void pfsmtd_aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_MEM_H_

// end
