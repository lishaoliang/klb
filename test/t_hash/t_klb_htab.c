///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    t_klb_htab.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "t_klb.h"
#include "hash/klb_htab.h"
#include "mem/klb_mem.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int cb_klb_htab_clean(void* p_obj, void* p_data)
{
    return 0;
}

int t_klb_htab_main(int argc, char** argv, char** p_print)
{
    int ht_size = 10;
    klb_htab_t* p_htab = klb_htab_create(ht_size, KLB_HTAB_AUTO_MALLOC);

    for (int i = 1; i < ht_size * 1000 + 1; i++)
    {
        char key[68] = { 0 };
        snprintf(key, 64, "%d", i);

        klb_htab_push(p_htab, key, strlen(key), (void*)i, NULL);
    }

    klb_htab_find(p_htab, "1000", strlen("1000"));

    klb_htab_clean(p_htab, cb_klb_htab_clean, NULL);
    KLB_FREE_BY(p_htab, klb_htab_destroy);

    return 0;
}
