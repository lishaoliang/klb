///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_fix_pool.h
/// @brief   固定大小内存池
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_FIX_POOL_H__
#define __KLB_FIX_POOL_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_fix_pool_t_ klb_fix_pool_t;


klb_fix_pool_t* klb_fix_pool_create();
void klb_fix_pool_destroy(klb_fix_pool_t* p_pool);


klb_buf_t* klb_fix_pool_malloc(klb_fix_pool_t* p_pool);


#ifdef __cplusplus
}
#endif

#endif // __KLB_FIX_POOL_H__
//end
