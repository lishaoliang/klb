#include "t_klb.h"
#include "hash/klb_hlist.h"
#include "mem/klb_mem.h"
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

        snprintf(key, 64, "%d", i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "%d.%d", i, i);
        ret = klb_hlist_push_head(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "%d.%d.%d", i, i, i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);
    }

    for (uint32_t i = 1; i < ht_max * step + 1; i++)
    {
        //if (0 == i % 4)
        {
            char key[68] = { 0 };
            snprintf(key, 64, "%d", i);

            void* ptr = klb_hlist_find(p_hlist, key, strlen(key));
            assert(NULL != ptr);

            ptr = klb_hlist_remove_bykey(p_hlist, key, strlen(key));
            assert(NULL != ptr);
        }

        char key[68] = { 0 };

        snprintf(key, 64, "_%d", i);
        ret = klb_hlist_push_tail(p_hlist, key, strlen(key), (void*)i);
        assert(0 == ret);

        snprintf(key, 64, "_%d.%d", i, i);
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
            snprintf(key, 64, "%d.%d", i, i);
            klb_hlist_remove_bykey(p_hlist, key, strlen(key));
        }

        if (0 == i % 11)
        {
            char key[68] = { 0 };
            snprintf(key, 64, "%d.%d.%d", i, i, i);
            klb_hlist_remove_bykey(p_hlist, key, strlen(key));
        }
    }

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
