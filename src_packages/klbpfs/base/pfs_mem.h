// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_mem.h
/// @brief   平台堆内存封装
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_MEM_H_
#define _PFS_MEM_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 平台堆内存封装 (Win/Linux); 失败返回 NULL, 由调用方处理
/// 业务代码勿直接 malloc/free; 对齐块用 pfs_aligned_*

void *pfs_malloc(size_t size);
void *pfs_mallocz(size_t size);                        ///< malloc 后 memset 0
void *pfs_calloc(size_t nmemb, size_t size);
void *pfs_realloc(void *ptr, size_t size);
void pfs_free(void *ptr);

/// align 须为 2 的幂且 >= sizeof(void*); 失败返回 NULL
/// 配对释放必须用 pfs_aligned_free (勿与 pfs_free 混用)
void *pfs_aligned_malloc(size_t align, size_t size);
void *pfs_aligned_mallocz(size_t align, size_t size);   ///< aligned_malloc 后 memset 0
void pfs_aligned_free(void *ptr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_MEM_H_

// end
