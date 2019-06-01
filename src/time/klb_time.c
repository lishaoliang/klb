#include "time/klb_time.h"
#include <time.h>
#include <assert.h>

#ifdef _MSC_VER
#include <windows.h>

uint32_t klb_tick_count()
{
    return GetTickCount();
}

#else

#include <sys/time.h>

uint32_t klb_tick_count()
{
    struct timespec tp = { 0 };
    if (!clock_gettime(CLOCK_MONOTONIC, &tp) < 0)
    {
        assert(FALSE);
    }

    return (uint32_t)(tp.tv_sec) * 1000 + (uint32_t)(tp.tv_nsec) / 1000000;
}

#endif
