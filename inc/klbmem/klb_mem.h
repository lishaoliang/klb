///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mem.h
/// @brief   基础内存申请/释放
/// @version 0.2
/// @history 修改历史
///   \n 2019 0.1 创建文件
///   \n 2026 0.2 明确内存使用原则: fatal-on-OOM / fail-fast
/// @warning 没有警告
/// @note    内存使用原则: 
///             fatal-on-OOM / fail-fast
///             堆分配失败视为不可恢复错误, 进程立即崩溃(快速失败)
///             系统调用 ENOMEM 等内存不足同等对待(assert, 禁止回滚返回)
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


#if !defined(_WIN32)

/// @brief 释放按对齐申请的内存 (Linux 等: posix_memalign 配 free)
#define klb_aligned_free   free

#else

#include <malloc.h>

/// @brief 释放按对齐申请的内存 (Windows: _aligned_free)
#define klb_aligned_free   _aligned_free

#endif


/// @brief 按对齐申请内存
/// @param [in] size        需要申请的内存大小
/// @param [in] align       对齐系数: 2^N
/// @return void* 申请到的内存
/// @note 分配失败时 assert 终止, 不返回 NULL (见文件 @note 内存使用原则)
KLB_API void* klb_aligned_malloc(size_t size, size_t align);


/// @brief 申请内存
/// @param [in] size        需要申请的内存大小; 0 时按 1 字节申请(避免 malloc(0) 歧义)
/// @return void* 申请到的内存
/// @note 分配失败时 assert 终止, 不返回 NULL (见文件 @note 内存使用原则)
KLB_API void* klb_malloc(size_t size);


/// @brief 申请内存并清零
/// @param [in] size        需要清零的字节数
/// @return void* 申请到的内存
/// @note 经 klb_malloc + KLB_MEMSET assert 链 fail-fast; size==0 时仍申请 1 字节, memset 0 字节
KLB_API void* klb_mallocz(size_t size);


/// @brief 重新申请内存
/// @param [in] p           原内存块; NULL 时等同 malloc
/// @param [in] size        需要申请的内存大小; 0 时释放 p 并返回 NULL
/// @return void* 申请到的内存; size>0 且失败返回 NULL, 原块保留
/// @note 与 fail-fast 宏不同, 失败可返回 NULL, 调用方须处理; 勿在失败时将返回值直接赋给原指针
KLB_API void* klb_realloc(void* p, size_t size);


/// @brief 重新申请内存并清零
/// @param [in] p           原内存块; NULL 时等同 klb_mallocz
/// @param [in] size        需要申请的内存大小
/// @return void* 申请到的内存; 失败返回 NULL, 原块保留
/// @note 成功时整块清零; 失败可返回 NULL, 调用方须处理
KLB_API void* klb_reallocz(void* p, size_t size);


/// @def   KLB_MALLOC
/// @brief 内存申请; 按结构体数目 + 对齐字节
/// @note  fail-fast: 经 klb_malloc 内部 assert; 分配后直接使用, 不 recover (见文件头 @note)
#define KLB_MALLOC(ST_, NUM_, PADDING_)             (ST_*)klb_malloc(sizeof(ST_) * (NUM_) + (PADDING_))

/// @def   KLB_MALLOCZ
/// @brief 内存申请(清零); 按结构体数目 + 对齐字节
/// @note  fail-fast: 经 klb_mallocz assert 链 (见文件头 @note)
#define KLB_MALLOCZ(ST_, NUM_, PADDING_)            (ST_*)klb_mallocz(sizeof(ST_) * (NUM_) + (PADDING_))

/// @def   KLB_REALLOC
/// @brief realloc内存申请; 按结构体数目 + 对齐字节
/// @note  非 fail-fast: 失败可返回 NULL, 旧块保留, 调用方须检 NULL
#define KLB_REALLOC(OLD_PTR_, ST_, NUM_, PADDING_)  (ST_*)klb_realloc((OLD_PTR_), sizeof(ST_) * (NUM_) + (PADDING_))

/// @def   KLB_REALLOCZ
/// @brief reallocz内存申请(带清0); 按结构体数目 + 对齐字节
/// @note  非 fail-fast: 失败可返回 NULL; 成功时整块清零, 调用方须检 NULL
#define KLB_REALLOCZ(OLD_PTR_, ST_, NUM_, PADDING_) (ST_*)klb_reallocz((OLD_PTR_), sizeof(ST_) * (NUM_) + (PADDING_))

/// @def   KLB_MEMSET
/// @brief memset, 加入了断言
#define KLB_MEMSET(PTR_, VAL_, SIZE_)       {assert(NULL!=(PTR_));memset(PTR_,VAL_,SIZE_);}


/// @def   KLB_FREE
/// @brief 释放内存, 并将指针置空
#define KLB_FREE(PTR_)                      {if(NULL!=(PTR_)){free(PTR_);(PTR_)=NULL;}}


/// @def   KLB_FREE_BY
/// @brief 通过释放函数释放结构体对象
#define KLB_FREE_BY(PTR_, FUNC_FREE_)       {if(NULL!=(PTR_)){(FUNC_FREE_)(PTR_);(PTR_)=NULL;}}


/// @def   KLB_MALLOC_ALIGNED
/// @brief 申请对齐内存; 按结构体数目 + 对齐字节; ALIGN_对齐(一般2^N对齐, 4, 8, 4K等)
/// @note  fail-fast: 经 klb_aligned_malloc 内部 assert (见文件头 @note)
#define KLB_MALLOC_ALIGNED(ST_, NUM_, PADDING_, ALIGN_) (ST_*)klb_aligned_malloc(sizeof(ST_) * (NUM_) + (PADDING_), ALIGN_)


/// @def   KLB_FREE_ALIGNED
/// @brief 释放对齐内存, 并将指针置空
#define KLB_FREE_ALIGNED(PTR_)              {if(NULL!=(PTR_)){klb_aligned_free(PTR_);(PTR_)=NULL;}}


/// @def   KLB_DELETE
/// @brief 删除指针, 并置空
#if defined(__cplusplus)
#define KLB_DELETE(PTR_)                    {if(NULL!=(PTR_)){ delete (PTR_);(PTR_)=NULL;}}
#endif


#ifdef __cplusplus
}
#endif

#endif // __KLB_MEM_H__
//end
