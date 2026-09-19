// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbplatform/klb_spinlock.h"
#include "klbplatform/klb_atomic.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klb_spinlock_t
/// @brief  自旋锁
typedef struct klb_spinlock_t_
{
    klb_atomic_t    lock;       ///< 0.未占用; 1.已占用
}klb_spinlock_t;


klb_spinlock_t* klb_spinlock_create()
{
    klb_spinlock_t* p_spinlock = KLB_MALLOCZ(klb_spinlock_t, 1, 0);

    return p_spinlock;
}

void klb_spinlock_destroy(klb_spinlock_t* p_spinlock)
{
    assert(NULL != p_spinlock);
    KLB_FREE(p_spinlock);
}

void klb_spinlock_lock(klb_spinlock_t* p_spinlock)
{
    assert(NULL != p_spinlock);
    klb_atomic_lock(&p_spinlock->lock);
}

int klb_spinlock_trylock(klb_spinlock_t* p_spinlock)
{
    assert(NULL != p_spinlock);

    if (klb_atomic_try_lock(&p_spinlock->lock))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void klb_spinlock_unlock(klb_spinlock_t* p_spinlock)
{
    assert(NULL != p_spinlock);
    klb_atomic_unlock(&p_spinlock->lock);
}

// end
