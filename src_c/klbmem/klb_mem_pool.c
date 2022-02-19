///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_mpool.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   内存池
///////////////////////////////////////////////////////////////////////////
#include "klbmem/klb_mem_pool.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_fpool.h"
#include <assert.h>


typedef struct klb_mem_pool_t_
{
    int a;
}klb_mem_pool_t;


klb_mem_pool_t* klb_mem_pool_create()
{
    return NULL;
}

void klb_mem_pool_destroy(klb_mem_pool_t* p_pool)
{

}

klb_buf_t* klb_mem_pool_malloc(klb_mem_pool_t* p_pool)
{
    return NULL;
}
