///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_fpool.h
/// @brief   klb fix pool固定大小内存池
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_FPOOL_H__
#define __KLB_FPOOL_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_fpool_t_ klb_fpool_t;


klb_fpool_t* klb_fpool_create();
void klb_fpool_destroy(klb_fpool_t* p_pool);


klb_buf_t* klb_fpool_malloc(klb_fpool_t* p_pool);


#ifdef __cplusplus
}
#endif

#endif // __KLB_FPOOL_H__
//end
