///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mpool.h
/// @brief   内存池
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MPOOL_H__
#define __KLB_MPOOL_H__

#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_mpool_t_ klb_mpool_t;


klb_mpool_t* klb_mpool_create();
void klb_mpool_destroy(klb_mpool_t* p_pool);

klb_buf_t* klb_mpool_malloc(klb_mpool_t* p_pool);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MPOOL_H__
//end
