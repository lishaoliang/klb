///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CSemLock.hpp
/// @brief   (IPC)进程之间信号量(Semaphore)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CSEMLOCK_HPP__
#define __KLB_CSEMLOCK_HPP__

#include "klb_type.h"

namespace klb {

/// @class CSemLock
/// @brief 进程之间信号量(Semaphore)
KLB_EXTERN class KLB_API_CPP CSemLock
{
public:
    CSemLock();
    ~CSemLock();

private:

};

} // namespace klb

#endif // __KLB_CSEMLOCK_HPP__
