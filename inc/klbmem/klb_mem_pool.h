///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mem_pool.h
/// @brief   内存池
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MEM_POOL_H__
#define __KLB_MEM_POOL_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_mem_pool_t_ klb_mem_pool_t;


klb_mem_pool_t* klb_mem_pool_create();
void klb_mem_pool_destroy(klb_mem_pool_t* p_pool);

klb_buf_t* klb_mem_pool_malloc(klb_mem_pool_t* p_pool);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MEM_POOL_H__
//end
