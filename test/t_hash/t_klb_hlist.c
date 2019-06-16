///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    t_klb_hlist.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "t_klb.h"
#include "hash/klb_hlist.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include "time/klb_time.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


static int t_klb_hlist_range(uint32_t ht_max, uint32_t step)
{
    //uint32_t ht_max = 1;
    //uint32_t step = 10000;
    klb_hlist_t* p_hlist = klb_hlist_create(ht_max);

    int ret = klb_hlist_push_tail(p_hlist, "", strlen(""), (void*)1000);
    assert(0 == ret);

    for (uint32_t i = 1; i < ht_max * step + 1; i++)
    {
        char key[68] = { 0 };

        snprintf(key, 64, "%1d", i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "%1d.%1d", i, i);
        ret = klb_hlist_push_head(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "%1d.%1d.%1d", i, i, i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);
    }

    for (uint32_t i = 1; i < ht_max * step + 1; i++)
    {
        if (0 == i % 4)
        {
            char key[68] = { 0 };
            snprintf(key, 64, "%1d", i);

            void* ptr = klb_hlist_find(p_hlist, key, strlen(key));
            assert(NULL != ptr);

            ptr = klb_hlist_remove_bykey(p_hlist, key, strlen(key));
            assert(NULL != ptr);
        }

        char key[68] = { 0 };

        snprintf(key, 64, "a%1d", i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "a%1d.%1d", i, i);
        ret = klb_hlist_push_head(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);
    }

    for (uint32_t i = 1; i < ht_max * step + 1; i++)
    {
        if (0 == i % 5)
        {
            void* ptr = klb_hlist_pop_head(p_hlist);
            assert(NULL != ptr);
        }

        if (0 == i % 7)
        {
            void* ptr = klb_hlist_pop_tail(p_hlist);
            assert(NULL != ptr);
        }

        if (0 == i % 9)
        {
            char key[68] = { 0 };
            snprintf(key, 64, "%1d.%1d", i, i);
            klb_hlist_remove_bykey(p_hlist, key, strlen(key));
        }

        if (0 == i % 11)
        {
            char key[68] = { 0 };
            snprintf(key, 64, "%1d.%1d.%1d", i, i, i);
            klb_hlist_remove_bykey(p_hlist, key, strlen(key));
        }
    }

    uint32_t tc_qs_1 = klb_tick_count();
    klb_hlist_qsort(p_hlist);
    uint32_t tc_qs_2 = klb_tick_count();

    KLB_LOG("hlist ht max=[%u],step=[%u], dt=[%u]\n", ht_max, step, tc_qs_2 - tc_qs_1);

    //klb_hlist_iter_t* p_iter = klb_hlist_begin(p_hlist);
    //while (NULL != p_iter)
    //{
    //    uint32_t key_len = 0;
    //    char key[68] = { 0 };
    //    char* p_key = (char*)klb_hlist_key(p_iter, &key_len);
    //    memcpy(key, p_key, key_len);

    //    KLB_LOG("key: [%s]\n", key);

    //    p_iter = klb_hlist_next(p_iter);
    //}

    klb_hlist_clean(p_hlist, NULL, NULL);
    KLB_FREE_BY(p_hlist, klb_hlist_destroy);
    return 0;
}

int t_klb_hlist_main(int argc, char** argv, char** p_print)
{
    int ret = t_klb_hlist_range(1, 1000);
    ret += t_klb_hlist_range(10, 100);
    ret += t_klb_hlist_range(100, 10);
    ret += t_klb_hlist_range(1000, 10);
    ret += t_klb_hlist_range(10000, 10);

    return ret;
}
