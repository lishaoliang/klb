///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_fpool.h
/// @brief   klb fix pool固定大小内存池
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_FPOOL_H__
#define __KLB_FPOOL_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "string.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_fpool_t_ klb_fpool_t;


KLB_API klb_fpool_t* klb_fpool_create(int item_size, int item_num, int aligned);
KLB_API void klb_fpool_destroy(klb_fpool_t* p_pool);

/// @brief 获取内存池, 使用的内存总大小
/// @return size_t 大小
KLB_API size_t klb_fpool_total_mem_size(klb_fpool_t* p_pool);


KLB_API size_t klb_fpool_item_size(klb_fpool_t* p_pool);
KLB_API size_t klb_fpool_idle_num(klb_fpool_t* p_pool);


KLB_API klb_buf_t* klb_fpool_malloc(void* p_pool, size_t size);


KLB_API int klb_fpool_ref(klb_buf_t* p_buf);
KLB_API int klb_fpool_unref(klb_buf_t* p_buf);


#ifdef __cplusplus
}
#endif

#endif // __KLB_FPOOL_H__
//end
