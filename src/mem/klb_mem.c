﻿#include "mem/klb_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#if !defined(_MSC_VER)

/// @brief 按对齐申请内存
/// @param [in] size        需要申请的内存大小
/// @param [in] align       对齐系数: 2^N
/// @return void* 申请到的内存
void* _aligned_malloc(size_t size, size_t align)
{
    void* ptr = NULL;
    int ret = posix_memalign(&ptr, align, size);

    assert(NULL != ptr);
    return ptr;
}

#endif
