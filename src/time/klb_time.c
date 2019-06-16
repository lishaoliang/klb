///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_time.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "time/klb_time.h"
#include <assert.h>

#ifdef _MSC_VER
#include <windows.h>

uint32_t klb_tick_count()
{
    return GetTickCount();
}

#else

#include <time.h>
#include <sys/time.h>

uint32_t klb_tick_count()
{
    struct timespec tp = { 0 };
    if (!clock_gettime(CLOCK_MONOTONIC, &tp) < 0)
    {
        assert(false);
    }

    return (uint32_t)(tp.tv_sec) * 1000 + (uint32_t)(tp.tv_nsec) / 1000000;
}

#endif
