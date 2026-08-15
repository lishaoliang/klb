// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/klb_atomic.h"
#include <assert.h>


#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#endif


#ifdef _WIN32

#if defined(_WIN64)
#define KLB_ATOMIC_PTR_(p_)  ((volatile LONGLONG*)(p_))
#define KLB_ATOMIC_VAL_(v_)  ((LONGLONG)(v_))
#define KLB_InterlockedExchange          InterlockedExchange64
#define KLB_InterlockedExchangeAdd       InterlockedExchangeAdd64
#define KLB_InterlockedCompareExchange   InterlockedCompareExchange64
#else
#define KLB_ATOMIC_PTR_(p_)  ((volatile LONG*)(p_))
#define KLB_ATOMIC_VAL_(v_)  ((LONG)(v_))
#define KLB_InterlockedExchange          InterlockedExchange
#define KLB_InterlockedExchangeAdd       InterlockedExchangeAdd
#define KLB_InterlockedCompareExchange   InterlockedCompareExchange
#endif

void klb_atomic_set_zero(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    KLB_InterlockedExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(0));
}

int klb_atomic_set_value(klb_atomic_t volatile* p_atomic, klb_atomic_t value)
{
    assert(NULL != p_atomic);
    return (int)KLB_InterlockedExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(value));
}

int klb_atomic_get_value(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    return (int)KLB_InterlockedExchangeAdd(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(0));
}

int klb_atomic_add(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    return (int)KLB_InterlockedExchangeAdd(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(1));
}

int klb_atomic_sub(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    return (int)KLB_InterlockedExchangeAdd(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(-1));
}

bool klb_atomic_is_zero(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);

    if (0 == KLB_InterlockedCompareExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(0), KLB_ATOMIC_VAL_(0)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void klb_atomic_lock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);

    int wait = 0;

    while (0 != KLB_InterlockedCompareExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(1), KLB_ATOMIC_VAL_(0)))
    {
#if 0
        wait += 1;

        if (4096 < wait)
        {
            assert(false);
        }
#endif
    };
}

bool klb_atomic_try_lock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);

    if (0 != KLB_InterlockedCompareExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(1), KLB_ATOMIC_VAL_(0)))
    {
        // 没有锁成功
        return false;
    }
    else
    {
        // 锁成功
        return true;
    }
}

void klb_atomic_unlock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    KLB_InterlockedExchange(KLB_ATOMIC_PTR_(p_atomic), KLB_ATOMIC_VAL_(0));
}

#else

void klb_atomic_set_zero(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    __sync_lock_release(p_atomic);
}

int klb_atomic_set_value(klb_atomic_t volatile* p_atomic, klb_atomic_t value)
{
    assert(NULL != p_atomic);
    return __sync_lock_test_and_set(p_atomic, value);
}

int klb_atomic_get_value(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    uintptr_t ffff = -1;
    return __sync_fetch_and_and(p_atomic, ffff/*0xffffffff*/);
}

int klb_atomic_add(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    return __sync_fetch_and_add(p_atomic, 1);
}

int klb_atomic_sub(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    return __sync_fetch_and_sub(p_atomic, 1);
}

bool klb_atomic_is_zero(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);

    if (__sync_bool_compare_and_swap(p_atomic, 0, 0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void klb_atomic_lock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    while (!__sync_bool_compare_and_swap(p_atomic, 0, 1));
}

bool klb_atomic_try_lock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    if (__sync_bool_compare_and_swap(p_atomic, 0, 1))
    {
        // 加锁成功
        return true;
    }
    else
    {
        // 锁失败
        return false;
    }
}

void klb_atomic_unlock(klb_atomic_t volatile* p_atomic)
{
    assert(NULL != p_atomic);
    __sync_lock_release(p_atomic);
}

#endif
