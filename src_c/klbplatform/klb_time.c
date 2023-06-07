// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/klb_time.h"
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

int64_t klb_gmt_time_ms()
{
    int64_t t64 = 0;

#ifdef _WIN32
    SYSTEMTIME systime;
    GetSystemTime(&systime);

    FILETIME filetime;
    if (SystemTimeToFileTime(&systime, &filetime))
    {
        t64 = filetime.dwHighDateTime;
        t64 <<= 32;
        t64 += filetime.dwLowDateTime;
        t64 /= 10000; //毫秒
        t64 -= 11644473600000L; //毫秒
    }
    else
    {
        assert(false);
    }
#else
    struct timeval tv = { 0 };
    if (0 == gettimeofday(&tv, NULL))
    {
        t64 = tv.tv_sec;
        t64 *= 1000;
        t64 += tv.tv_usec / 1000;
    }
    else
    {
        assert(false);
    }
#endif

    return t64;
}
