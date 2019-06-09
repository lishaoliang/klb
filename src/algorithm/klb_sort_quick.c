///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_sort_quick.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   快速排序
///////////////////////////////////////////////////////////////////////////
#include "algorithm/klb_sort_quick.h"
#include <string.h>
#include <assert.h>


#define KLB_SORT_TMP_BUF            64
#define KLB_SORT_B0_IDX(IDX_)       ((IDX_) - 1)


#define KLB_SORT_EXCHANGE(BASE_, A_, B_, S_) \
do \
{ \
    memcpy(tmp, (BASE_) + width * KLB_SORT_B0_IDX(A_), S_); \
    memcpy((BASE_) + width * KLB_SORT_B0_IDX(A_), (BASE_) + width * KLB_SORT_B0_IDX(B_), S_); \
    memcpy((BASE_) + width * KLB_SORT_B0_IDX(B_), tmp, S_); \
} while (0);


static size_t partition(uint8_t* p_base, size_t p, size_t r, size_t width, klb_sort_compare_cb cb_compare)
{
    uint8_t tmp[KLB_SORT_TMP_BUF];

    uint8_t x[KLB_SORT_TMP_BUF];
    memcpy(x, p_base + width * KLB_SORT_B0_IDX(r), width);

    size_t i = p - 1;
    
    for (size_t j = p; j < r; j++)
    {
        if (cb_compare(p_base + width * KLB_SORT_B0_IDX(j), x) < 0)
        {
            i = i + 1;
            
            KLB_SORT_EXCHANGE(p_base, i, j, width);
        }
    }

    KLB_SORT_EXCHANGE(p_base, i + 1, r, width);

    return i + 1;
}

static size_t partition_ptr(uintptr_t* p_base, size_t p, size_t r, size_t width, klb_sort_compare_cb cb_compare)
{
    uintptr_t tmp;
    uintptr_t x = p_base[KLB_SORT_B0_IDX(r)];
    size_t i = p - 1;

    for (size_t j = p; j < r; j++)
    {
        if (cb_compare(&(p_base[KLB_SORT_B0_IDX(j)]), &x) < 0)
        {
            i = i + 1;

            tmp = p_base[KLB_SORT_B0_IDX(i)];
            p_base[KLB_SORT_B0_IDX(i)] = p_base[KLB_SORT_B0_IDX(j)];
            p_base[KLB_SORT_B0_IDX(j)] = tmp;
        }
    }

    tmp = p_base[KLB_SORT_B0_IDX(i + 1)];
    p_base[KLB_SORT_B0_IDX(i + 1)] = p_base[KLB_SORT_B0_IDX(r)];
    p_base[KLB_SORT_B0_IDX(r)] = tmp;

    return i + 1;
}

static size_t partition_ptr_1(uintptr_t* p_base, size_t p, size_t r, size_t width, klb_sort_compare_cb cb_compare)
{
    uintptr_t* p_r = p_base + KLB_SORT_B0_IDX(r);
    //uintptr_t x = *p_r;
    uintptr_t* p_i = p_base + KLB_SORT_B0_IDX(p) - 1;

    uintptr_t tmp;
    uintptr_t* p_j;
    for (p_j = p_base + KLB_SORT_B0_IDX(p); p_j < p_r; p_j++)
    {
        if (cb_compare(p_j, p_r) < 0)
        {
            p_i++;

            tmp = *p_i;
            *p_i = *p_j;
            *p_j = tmp;
        }
    }

    p_i++;

    tmp = *p_i;
    *p_i = *p_r;
    *p_r = tmp;

    return p_i - p_base + 1;
}

static void quick_sort(uint8_t* p_base, size_t p, size_t r, size_t width, klb_sort_compare_cb cb_compare)
{
    if (p < r)
    {
        //size_t q = partition(p_base, p, r, width, cb_compare);
        size_t q = partition_ptr_1((uintptr_t*)p_base, p, r, width, cb_compare);
        quick_sort(p_base, p, q - 1, width, cb_compare);
        quick_sort(p_base, q + 1, r, width, cb_compare);
    }
}

void klb_sort_quick(void* p_base, size_t nitems, size_t width, klb_sort_compare_cb cb_compare)
{
    if (width <= KLB_SORT_TMP_BUF)
    {
        // 使用线程栈 存储中间值
        quick_sort((uint8_t*)p_base, 1, nitems, width, cb_compare);
    }
    else
    {
        // 使用动态分配内存 存储中间值
    }
}
