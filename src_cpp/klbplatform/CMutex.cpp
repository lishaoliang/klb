// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/CMutex.hpp"
#include "klbmem/klb_mem.h"


namespace klb {

CMutex::CMutex()
{
    m_mutex = klb_mutex_create();
}

CMutex::~CMutex()
{
    KLB_FREE_BY(m_mutex, klb_mutex_destroy);
}

void CMutex::Lock()
{
    klb_mutex_lock(m_mutex);
}

bool CMutex::TryLock()
{
    if (0 == klb_mutex_trylock(m_mutex))
    {
        return true;
    }

    return false;
}

void CMutex::UnLock()
{
    klb_mutex_unlock(m_mutex);
}

} // namespace klb
