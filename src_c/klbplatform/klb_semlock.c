// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/klb_semlock.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hash.h"
#include "klbutil/klb_log.h"
#include <assert.h>


#ifdef _WIN32

#include <windows.h>

typedef struct klb_semlock_t_
{
    HANDLE  mutex;
}klb_semlock_t;


/// @brief 创建进程锁
/// @return klb_semlock_t* 进程锁
klb_semlock_t* klb_semlock_create(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOCZ(klb_semlock_t, 1, 0);

    p_semlock->mutex = CreateMutex(NULL, FALSE, p_name);

    assert(NULL != p_semlock->mutex);

    return p_semlock;
}

klb_semlock_t* klb_semlock_open(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOCZ(klb_semlock_t, 1, 0);

    p_semlock->mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, p_name);

    assert(NULL != p_semlock->mutex);
    return p_semlock;
}

/// @brief 销毁进程锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_destroy(klb_semlock_t* p_semlock)
{
    assert(NULL != p_semlock);
    assert(NULL != p_semlock->mutex);

    if (NULL != p_semlock)
    {
        // Fixed Bug. [2026] destroy 须 CloseHandle 释放内核句柄, 非 ReleaseMutex
        KLB_FREE_BY(p_semlock->mutex, CloseHandle);
        KLB_FREE(p_semlock);
    }
}


/// @brief 加锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_lock(klb_semlock_t* p_semlock)
{
    assert(NULL != p_semlock);
    assert(NULL != p_semlock->mutex);

    if (p_semlock->mutex)
    {
        WaitForSingleObject(p_semlock->mutex, INFINITE);
    }
}


/// @brief 解锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_unlock(klb_semlock_t* p_semlock)
{
    assert(NULL != p_semlock);
    assert(NULL != p_semlock->mutex);

    if (p_semlock->mutex)
    {
        ReleaseMutex(p_semlock->mutex);
    }
}

#else

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

typedef struct klb_semlock_t_
{
    int     fd;
    int     uid;
}klb_semlock_t;


/// @brief 创建进程锁
/// @return klb_semlock_t* 进程锁
klb_semlock_t* klb_semlock_create(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOCZ(klb_semlock_t, 1, 0);

    p_semlock->fd = -1;
    p_semlock->uid = klb_hash32(p_name, strlen(p_name));

    // semget
    p_semlock->fd = semget(p_semlock->uid, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (p_semlock->fd < 0)
    {
        p_semlock->fd = semget(p_semlock->uid, 1, IPC_CREAT | 0666);
    }
    assert(0 <= p_semlock->fd);

    if (semctl(p_semlock->fd, 0, SETVAL, 1) < 0)
    {
        KLB_LOG_E("semlock semctl error!,fd:[%d]\n", p_semlock->fd);
    }

    return p_semlock;
}

klb_semlock_t* klb_semlock_open(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOCZ(klb_semlock_t, 1, 0);

    p_semlock->fd = -1;
    p_semlock->uid = klb_hash32(p_name, strlen(p_name));

    p_semlock->fd = semget(p_semlock->uid, 1, IPC_CREAT | 0666);
    assert(0 <= p_semlock->fd);

    return p_semlock;
}

/// @brief 销毁进程锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_destroy(klb_semlock_t* p_semlock)
{
    if (0 <= p_semlock->fd)
    {
        if (semctl(p_semlock->fd, 0, IPC_RMID) == -1)
        {
            KLB_LOG_E("semlock destroy: semctl IPC_RMID error!");
        }
    }

    KLB_FREE(p_semlock);
}


/// @brief 加锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_lock(klb_semlock_t* p_semlock)
{
    // semctl
    //struct sembuf sem_opt_wait1[1] = { 0, -1, SEM_UNDO };
    //struct sembuf sem_opt_wakeup1[1] = { 0, 1, SEM_UNDO };
    //struct sembuf sem_opt_wait2[1] = { 1, -1, SEM_UNDO };
    //struct sembuf sem_opt_wakeup2[1] = { 1, 1, SEM_UNDO };

    assert(NULL != p_semlock);

    if (0 <= p_semlock->fd)
    {
        struct sembuf sem = { 0 };

        sem.sem_num = 0;
        sem.sem_op = -1;
        sem.sem_flg = SEM_UNDO;

        if (semop(p_semlock->fd, &sem, 1) < 0)
        {
            KLB_LOG_E("semlock lock error!fd:[%d]\n", p_semlock->fd);
        }
    }
    else
    {
        KLB_LOG_E("semlock lock error!fd:[%d]\n", p_semlock->fd);
    }
}


/// @brief 解锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_unlock(klb_semlock_t* p_semlock)
{
    assert(NULL != p_semlock);

    if (0 <= p_semlock->fd)
    {
        struct sembuf sem = { 0 };

        sem.sem_num = 0;
        sem.sem_op = 1;
        sem.sem_flg = SEM_UNDO;

        if (semop(p_semlock->fd, &sem, 1) < 0)
        {
            KLB_LOG_E("semlock unlock error!fd:[%d]\n", p_semlock->fd);
        }
    }
    else
    {
        KLB_LOG_E("semlock unlock error!fd:[%d]\n", p_semlock->fd);
    }

}

#endif
