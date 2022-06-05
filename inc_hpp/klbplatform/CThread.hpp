///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CThread.hpp
/// @brief   线程
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CTHREAD_HPP__
#define __KLB_CTHREAD_HPP__


#include "klb_type.h"
#include "klbplatform/klb_thread.h"
#include "klbthird/sds.h"


namespace klb {

    KLB_EXTERN class KLB_API_CPP CThread
    {
    public:
        CThread();
        CThread(const char* p_name);
        CThread(const char* p_name, int cpu_idx);
        ~CThread();

    bool Start(klb_thread_cb cb_thread, void* p_obj, bool wait = true);
    void Stop();

    private:
        klb_thread_t*   m_thread;
        sds             m_name;
        int             m_cpu_idx;
    };
}

#endif // __KLB_CTHREAD_HPP__
