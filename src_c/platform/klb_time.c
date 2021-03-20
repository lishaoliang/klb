///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_time.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "platform/klb_time.h"
#include <assert.h>

#ifdef _WIN32
#include <windows.h>

uint32_t klb_tick_count()
{
    return GetTickCount();
}

uint64_t klb_tick_count64()
{
    return (uint64_t)GetTickCount();
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

uint64_t klb_tick_count64()
{
    struct timespec tp = { 0 };
    if (!clock_gettime(CLOCK_MONOTONIC, &tp) < 0)
    {
        assert(false);
    }

    return (uint64_t)(tp.tv_sec) * 1000 + (uint64_t)(tp.tv_nsec) / 1000000;
}

#endif

int64_t klb_tick_counti64()
{
    uint64_t u64 = klb_tick_count64();

    return (int64_t)(u64 & 0x7fffffffffffffff);
}
