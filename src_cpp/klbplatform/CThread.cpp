// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbplatform/CThread.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>

namespace klb {

    CThread::CThread()
    {
        m_thread = NULL;
        m_name = NULL;
        m_cpu_idx = -1;
    }

    CThread::CThread(const char* p_name)
    {
        m_thread = NULL;
        m_name = sdsnew(p_name);
        m_cpu_idx = -1;
    }

    CThread::CThread(const char* p_name, int cpu_idx)
    {
        m_thread = NULL;
        m_name = sdsnew(p_name);
        m_cpu_idx = cpu_idx;
    }

    CThread::~CThread()
    {
        Stop();
        KLB_FREE_BY(m_name, sdsfree);
    }

    bool CThread::Start(klb_thread_cb cb_thread, void* p_obj, bool wait)
    {
        Stop();

        m_thread = klb_thread_create(cb_thread, p_obj, m_cpu_idx, m_name);
        assert(NULL != m_thread);

        if (m_thread && wait)
        {
            klb_thread_wait(m_thread);
        }

        return (NULL != m_thread) ? true : false;
    }

    void CThread::Stop()
    {
        KLB_FREE_BY(m_thread, klb_thread_destroy);
    }
}
