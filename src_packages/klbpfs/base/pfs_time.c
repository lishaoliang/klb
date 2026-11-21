// Doc Encode : UTF-8 BOM, Unix(LF)
// 平台时间 (单调 tick 等)
#include "base/pfs_time.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

uint32_t pfs_get_tickcount(void)
{
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timespec ts;

    if (0 != clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        return 0;
    }
    // 秒与纳秒合成毫秒; 截断为 uint32 (与 GetTickCount 同宽, 自然回绕)
    return (uint32_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u);
#endif
}

// end
