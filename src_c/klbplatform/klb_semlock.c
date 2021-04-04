#include "klbplatform/klb_semlock.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#ifdef _WIN32

typedef struct klb_semlock_t_
{
    int a;
}klb_semlock_t;


/// @brief 创建进程锁
/// @return klb_semlock_t* 进程锁
klb_semlock_t* klb_semlock_create(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOC(klb_semlock_t, 1, 0);
    KLB_MEMSET(p_semlock, 0, sizeof(klb_semlock_t));


    return p_semlock;
}


/// @brief 销毁进程锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_destroy(klb_semlock_t* p_semlock)
{

}


/// @brief 加锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_lock(klb_semlock_t* p_semlock)
{

}


/// @brief 解锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_unlock(klb_semlock_t* p_semlock)
{

}

#else

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

typedef struct klb_semlock_t_
{
    int a;
}klb_semlock_t;


/// @brief 创建进程锁
/// @return klb_semlock_t* 进程锁
klb_semlock_t* klb_semlock_create(const char* p_name)
{
    klb_semlock_t* p_semlock = KLB_MALLOC(klb_semlock_t, 1, 0);
    KLB_MEMSET(p_semlock, 0, sizeof(klb_semlock_t));

    // semget

    return p_semlock;
}


/// @brief 销毁进程锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_destroy(klb_semlock_t* p_semlock)
{

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
}


/// @brief 解锁
/// @param [in]  *p_semlock     进程锁
/// @return 无
void klb_semlock_unlock(klb_semlock_t* p_semlock)
{

}

#endif
