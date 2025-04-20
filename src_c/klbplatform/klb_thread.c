// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/klb_thread.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbplatform/klb_time.h"
#include "klbthird/sds.h"
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>


/// @struct klb_thread_t
/// @brief  线程对象
typedef struct klb_thread_t_
{
    HANDLE          hnd;            ///< 线程句柄
    volatile int    run;            ///< 是否运行: true.在运行; false.未运行

    klb_thread_cb   cb_thread;      ///< 线程启动入口
    void*           p_obj;          ///< 传递的对象参数

    int             cpu_idx;        ///< 需要设置CPU序号
    sds             name;           ///< 需要设置的线程名称

    bool            wait;           ///< 需创建者等待
}klb_thread_t;


static DWORD WINAPI cb_klb_thread(void* p_obj)
{
    klb_thread_t* p_thread = (klb_thread_t*)(p_obj);

    // 初始随机值
    srand(klb_tick_count() + klb_thread_tid());

    // 等待标记
    p_thread->wait = false;

    if (NULL != p_thread->cb_thread)
    {
        p_thread->cb_thread(p_thread->p_obj, &p_thread->run);
    }

    return 0;
}

klb_thread_t* klb_thread_create(klb_thread_cb cb_thread, void* p_obj, int cpu_idx, const char* p_name)
{
    assert(NULL != cb_thread);

    klb_thread_t* p_thread = KLB_MALLOC(klb_thread_t, 1, 0);
    KLB_MEMSET(p_thread, 0, sizeof(klb_thread_t));

    p_thread->cb_thread = cb_thread;
    p_thread->p_obj = p_obj;
    p_thread->cpu_idx = cpu_idx;

    p_thread->run = true;
    p_thread->wait = true;

    p_thread->hnd = CreateThread(0, 0, cb_klb_thread, (void *)p_thread, 0, NULL);

    if (NULL == p_thread->hnd)
    {
        KLB_LOG_S("sys error!klb thread create!");

        KLB_FREE_BY(p_thread->name, sdsfree);
        KLB_FREE(p_thread);
        return NULL;
    }

    return p_thread;
}


void klb_thread_destroy(klb_thread_t* p_thread)
{
    assert(NULL != p_thread);

    p_thread->run = false;
    WaitForSingleObject(p_thread->hnd, INFINITE);
    CloseHandle(p_thread->hnd);

    KLB_FREE_BY(p_thread->name, sdsfree);
    KLB_FREE(p_thread);
}

void klb_thread_wait(klb_thread_t* p_thread)
{
    assert(NULL != p_thread);

    while (p_thread->wait)
    {
        klb_sleep(1);
    }
}

/// @brief 获取进程ID
int klb_thread_pid()
{
    return GetProcessIdOfThread(GetCurrentThread());
}


/// @brief 获取线程ID
int klb_thread_tid()
{
    return GetCurrentThreadId();
}

void klb_sleep(uint32_t ms)
{
    Sleep(ms);
}

void klb_sleep_ns(uint32_t ns)
{
    assert(ns <= 999999999);
    assert(false);
}

#else

#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifndef __APPLE__
#include <sys/prctl.h>
#endif

/// @struct klb_thread_t
/// @brief  线程对象
typedef struct klb_thread_t_
{
    pthread_t       hnd;            ///< 线程句柄
    volatile int    run;            ///< 是否运行: true.在运行; false.未运行

    klb_thread_cb   cb_thread;      ///< 线程启动入口
    void*           p_obj;          ///< 传递的对象参数

    int             cpu_idx;        ///< 需要设置CPU序号
    sds             name;           ///< 需要设置的线程名称

    bool            wait;           ///< 需创建者等待
}klb_thread_t;


static void* cb_klb_thread(void* p_obj)
{
    klb_thread_t* p_thread = (klb_thread_t*)(p_obj);

#ifndef __APPLE__
    // 设置CPU
    if (0 <= p_thread->cpu_idx)
    {
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(p_thread->cpu_idx, &mask);

        int tid = syscall(__NR_gettid);;
        if (-1 == sched_setaffinity(tid, sizeof(mask), &mask))
        {
            KLB_LOG_E("error!klb thread set cpu idx:%d\n", p_thread->cpu_idx);
        }
    }

    // 设置线程名称
    if (NULL != p_thread->name && 0 < sdslen(p_thread->name))
    {
        prctl(PR_SET_NAME, p_thread->name);
    }
#endif

    // 初始随机值
    srand(klb_tick_count() + klb_thread_tid());

    // 等待标记
    p_thread->wait = false;

    if (NULL != p_thread->cb_thread)
    {
        p_thread->cb_thread(p_thread->p_obj, &p_thread->run);
    }

    return 0;
}

klb_thread_t* klb_thread_create(klb_thread_cb cb_thread, void* p_obj, int cpu_idx, const char* p_name)
{
    assert(NULL != cb_thread);

    klb_thread_t* p_thread = KLB_MALLOC(klb_thread_t, 1, 0);
    KLB_MEMSET(p_thread, 0, sizeof(klb_thread_t));

    p_thread->cb_thread = cb_thread;
    p_thread->p_obj = p_obj;
    p_thread->cpu_idx = cpu_idx;
    p_thread->name = sdsnew(p_name);

    p_thread->run = true;
    p_thread->wait = true;

    if (0 != pthread_create(&(p_thread->hnd), 0, cb_klb_thread, p_thread))
    {
        KLB_LOG_S("sys error!klb thread create!");

        KLB_FREE_BY(p_thread->name, sdsfree);
        KLB_FREE(p_thread);
        return NULL;
    }

    return p_thread;
}

void klb_thread_destroy(klb_thread_t* p_thread)
{
    assert(NULL != p_thread);

    p_thread->run = false;
    pthread_join(p_thread->hnd, NULL);

    KLB_FREE_BY(p_thread->name, sdsfree);
    KLB_FREE(p_thread);
}

void klb_thread_wait(klb_thread_t* p_thread)
{
    assert(NULL != p_thread);

    while (p_thread->wait)
    {
        klb_sleep(1);
    }
}

/// @brief 获取进程ID
int klb_thread_pid()
{
    int pid = getpid();
    return pid;
}


/// @brief 获取线程ID
int klb_thread_tid()
{
    int tid = syscall(__NR_gettid);
    return tid;
}


/// @brief 通过 select 函数休眠
static void klb_sleep_by_select(uint32_t ms)
{
    if (0 == ms) { ms = 1; } // 最小1毫秒

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    select(0, &rfds, NULL, NULL, &tv);
}

void klb_sleep(uint32_t ms)
{
    if (ms <= 10)
    {
        // Bug. 部分嵌入式平台, clock_nanosleep 函数无效
        // 这里替换成 select 休眠

        // (0毫秒, 10毫秒]休眠
        klb_sleep_by_select(ms);
    }
    else if (ms <= 60000)
    {
        // (10毫秒, 60秒]采用 usleep
        usleep(ms * 1000);
    }
    else
    {
        // (60秒,N] 60秒以上, 忽略秒以下休眠时间
        uint32_t s = ms / 1000;

        do
        {
            int left = sleep(s);

            if (left < s)
            {
                s = left;
            }
            else
            {
                s = 0;
            }

        } while (0 < s);
    }
}

void klb_sleep_ns(uint32_t ns)
{
    assert(ns <= 999999999);

    struct timespec wait;
    wait.tv_sec = 0;
    wait.tv_nsec = ns;

    // !注意: 部分嵌入式平台, clock_nanosleep 函数无效

#ifndef __APPLE__
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wait, NULL);
#else
    nanosleep(&wait, NULL);
#endif
}

#endif
