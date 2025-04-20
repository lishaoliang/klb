// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/klb_rwlock.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

#ifdef _WIN32

#include <windows.h>


/// @struct klb_rwlock_t
/// @brief  读写锁
typedef struct klb_rwlock_t_
{
    SRWLOCK         srw_lock;       ///< 读写锁
}klb_rwlock_t;

/// @brief 创建锁
klb_rwlock_t* klb_rwlock_create()
{
    klb_rwlock_t* p_rwlock = KLB_MALLOCZ(klb_rwlock_t, 1, 0);

    InitializeSRWLock(&p_rwlock->srw_lock);

    return p_rwlock;
}

/// @brief 销毁锁
void klb_rwlock_destroy(klb_rwlock_t* p_rwlock)
{
    KLB_FREE(p_rwlock);
}

/// @brief (读)加锁
void klb_rwlock_rdlock(klb_rwlock_t* p_rwlock)
{
    // 读锁, 共享访问
    AcquireSRWLockShared(&p_rwlock->srw_lock);
}

/// @brief (读)尝试加锁
int klb_rwlock_try_rdlock(klb_rwlock_t* p_rwlock)
{
    if (TryAcquireSRWLockShared(&p_rwlock->srw_lock))
    {
        return 0;
    }

    return 1;
}

/// @brief (读)解锁
void klb_rwlock_rdunlock(klb_rwlock_t* p_rwlock)
{
    // 因windows 读写 释放接口不同, 解锁接口分离
    ReleaseSRWLockShared(&p_rwlock->srw_lock);
}

/// @brief (写)加锁
void klb_rwlock_wrlock(klb_rwlock_t* p_rwlock)
{
    // 写锁, 独占访问
    AcquireSRWLockExclusive(&p_rwlock->srw_lock);
}

/// @brief (写)尝试加锁
int klb_rwlock_try_wrlock(klb_rwlock_t* p_rwlock)
{
    if (TryAcquireSRWLockExclusive(&p_rwlock->srw_lock))
    {
        return 0;
    }

    return 1;
}

/// @brief (写)解锁
void klb_rwlock_wrunlock(klb_rwlock_t* p_rwlock)
{
    // 因windows 读写 释放接口不同, 解锁接口分离
    ReleaseSRWLockExclusive(&p_rwlock->srw_lock);
}

#else

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifndef __APPLE__
#include <sys/prctl.h>
#endif

/// @struct klb_rwlock_t
/// @brief  读写锁
typedef struct klb_rwlock_t_
{
    pthread_rwlock_t    rwlock;
}klb_rwlock_t;

/// @brief 创建锁
klb_rwlock_t* klb_rwlock_create()
{
    klb_rwlock_t* p_rwlock = KLB_MALLOCZ(klb_rwlock_t, 1, 0);

    if (0 != pthread_rwlock_init(&p_rwlock->rwlock, NULL))
    {
        assert(false);
    }

    return p_rwlock;
}

/// @brief 销毁锁
void klb_rwlock_destroy(klb_rwlock_t* p_rwlock)
{
    pthread_rwlock_destroy(&p_rwlock->rwlock);

    KLB_FREE(p_rwlock);
}

/// @brief (读)加锁
void klb_rwlock_rdlock(klb_rwlock_t* p_rwlock)
{
    if (0 != pthread_rwlock_rdlock(&p_rwlock->rwlock))
    {
        assert(false);
    }
}

/// @brief (读)尝试加锁
int klb_rwlock_try_rdlock(klb_rwlock_t* p_rwlock)
{
    if (0 == pthread_rwlock_tryrdlock(&p_rwlock->rwlock))
    {
        return 0;
    }

    return 1;
}

/// @brief (读)解锁
void klb_rwlock_rdunlock(klb_rwlock_t* p_rwlock)
{
    if (0 != pthread_rwlock_unlock(&p_rwlock->rwlock))
    {
        assert(false);
    }
}

/// @brief (写)加锁
void klb_rwlock_wrlock(klb_rwlock_t* p_rwlock)
{
    if (0 != pthread_rwlock_wrlock(&p_rwlock->rwlock))
    {
        assert(false);
    }
}

/// @brief (写)尝试加锁
int klb_rwlock_try_wrlock(klb_rwlock_t* p_rwlock)
{
    if (0 == pthread_rwlock_trywrlock(&p_rwlock->rwlock))
    {
        return 0;
    }

    return 1;
}

/// @brief (写)解锁
void klb_rwlock_wrunlock(klb_rwlock_t* p_rwlock)
{
    if (0 != pthread_rwlock_unlock(&p_rwlock->rwlock))
    {
        assert(false);
    }
}

#endif // _WIN32
//end
