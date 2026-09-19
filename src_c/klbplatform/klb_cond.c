// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_cond.h"
#include "klbplatform/klb_mutex_in.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

#ifdef _WIN32
#include <windows.h>

/// @struct klb_cond_t
/// @brief  条件变量
typedef struct klb_cond_t_
{
    CONDITION_VARIABLE  cond;       ///< 条件变量
}klb_cond_t;

klb_cond_t* klb_cond_create()
{
    klb_cond_t* p_cond = KLB_MALLOC(klb_cond_t, 1, 0);
    KLB_MEMSET(p_cond, 0, sizeof(klb_cond_t));

    InitializeConditionVariable(&p_cond->cond);

    return p_cond;
}

void klb_cond_destroy(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);

    KLB_FREE(p_cond);
}

void klb_cond_wait(klb_cond_t* p_cond, klb_mutex_t* p_mutex)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    if (0 == SleepConditionVariableCS(&p_cond->cond, &p_mutex->section, INFINITE))
    {
        assert(false);
    }
}

int klb_cond_timedwait(klb_cond_t* p_cond, klb_mutex_t* p_mutex, uint32_t ms)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    if (0 != SleepConditionVariableCS(&p_cond->cond, &p_mutex->section, ms))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void klb_cond_signal(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);
    WakeConditionVariable(&p_cond->cond);
}

void klb_cond_broadcast(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);
    WakeAllConditionVariable(&p_cond->cond);
}

#else

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

/// @struct klb_cond_t
/// @brief  条件变量
typedef struct klb_cond_t_
{
    pthread_cond_t      cond;       ///< 条件变量
}klb_cond_t;

static void abstime_klb_cond(struct timespec* p_ts, uint32_t ms)
{
    struct timeval tv = { 0 };
    if (0 != gettimeofday(&tv, NULL))
    {
        assert(false);
    }

    p_ts->tv_sec = tv.tv_sec + (time_t)(ms / 1000);
    long nsec = tv.tv_usec * 1000L + (long)(ms % 1000) * 1000000L;
    if (nsec >= 1000000000L)
    {
        p_ts->tv_sec += 1;
        nsec -= 1000000000L;
    }

    p_ts->tv_nsec = nsec;
}

klb_cond_t* klb_cond_create()
{
    klb_cond_t* p_cond = KLB_MALLOC(klb_cond_t, 1, 0);
    KLB_MEMSET(p_cond, 0, sizeof(klb_cond_t));

    if (0 != pthread_cond_init(&p_cond->cond, NULL))
    {
        assert(false);
    }

    return p_cond;
}

void klb_cond_destroy(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);

    pthread_cond_destroy(&p_cond->cond);
    KLB_FREE(p_cond);
}

void klb_cond_wait(klb_cond_t* p_cond, klb_mutex_t* p_mutex)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    if (0 != pthread_cond_wait(&p_cond->cond, &p_mutex->mutex))
    {
        assert(false);
    }
}

int klb_cond_timedwait(klb_cond_t* p_cond, klb_mutex_t* p_mutex, uint32_t ms)
{
    assert(NULL != p_cond);
    assert(NULL != p_mutex);

    struct timespec ts = { 0 };
    abstime_klb_cond(&ts, ms);

    int rc = pthread_cond_timedwait(&p_cond->cond, &p_mutex->mutex, &ts);
    if (0 == rc)
    {
        return 0;
    }
    else if (ETIMEDOUT == rc)
    {
        return 1;
    }

    assert(false);
    return 1;
}

void klb_cond_signal(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);

    if (0 != pthread_cond_signal(&p_cond->cond))
    {
        assert(false);
    }
}

void klb_cond_broadcast(klb_cond_t* p_cond)
{
    assert(NULL != p_cond);

    if (0 != pthread_cond_broadcast(&p_cond->cond))
    {
        assert(false);
    }
}

#endif

// end
