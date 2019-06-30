///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_sort_insert.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   插入排序
/// 中文版<<算法导论(原书第三版)>>: 2.1 插入排序, P.10
/// -------------------------------------------------------------------
/// INSERTION-SORT(A)
///  for j = 2 to A.length
///      key = A[j]
///      // Insert A[j] into the sorted sequence A[1..j-1].
///      i = j -1
///      while i > 0 and A[i] > key
///          A[i + 1] = A[i]
///          i = i - 1
///      A[i + 1] = key
///////////////////////////////////////////////////////////////////////////
#include "algorithm/klb_sort_insert.h"
#include "mem/klb_mem.h"
#include <string.h>
#include <assert.h>

#define KLB_SORT_TMP_BUF            64
#define KLB_SORT_B0_IDX(IDX_)       ((IDX_) - 1)


static void klb_sort_insert_b(uint8_t* p_base, size_t nitems, size_t width, klb_sort_compare_cb cb_compare)
{
    assert(width <= KLB_SORT_TMP_BUF);
    uint8_t key[KLB_SORT_TMP_BUF];

    for (size_t j = 2; j <= nitems; j++)
    {
        memcpy(key, p_base + width * KLB_SORT_B0_IDX(j), width);

        size_t i = j - 1;
        while (0 < i && 0 < cb_compare((p_base + width * KLB_SORT_B0_IDX(i)), &key))
        {
            memcpy(p_base + width * (KLB_SORT_B0_IDX(i) + 1), (p_base + width * KLB_SORT_B0_IDX(i)), width);
            i = i - 1;
        }

        memcpy(p_base + width * (KLB_SORT_B0_IDX(i) + 1), &key, width);
    }
}

static void klb_sort_insert_m(uint8_t* p_base, size_t nitems, size_t width, klb_sort_compare_cb cb_compare)
{
    uint8_t* p_key = KLB_MALLOC(uint8_t, width, 0);

    for (size_t j = 2; j <= nitems; j++)
    {
        memcpy(p_key, p_base + width * KLB_SORT_B0_IDX(j), width);

        size_t i = j - 1;
        while (0 < i && 0 < cb_compare((p_base + width * KLB_SORT_B0_IDX(i)), p_key))
        {
            memcpy(p_base + width * (KLB_SORT_B0_IDX(i) + 1), (p_base + width * KLB_SORT_B0_IDX(i)), width);
            i = i - 1;
        }

        memcpy(p_base + width * (KLB_SORT_B0_IDX(i) + 1), p_key, width);
    }

    KLB_FREE(p_key);
}

void klb_sort_insert(void* p_base, size_t nitems, size_t width, klb_sort_compare_cb cb_compare)
{
    if (width <= KLB_SORT_TMP_BUF)
    {
        // 使用线程栈 存储中间值
        klb_sort_insert_b((uint8_t*)p_base, nitems, width, cb_compare);
    }
    else
    {
        // 使用动态分配内存 存储中间值
        klb_sort_insert_m((uint8_t*)p_base, nitems, width, cb_compare);
    }
}
