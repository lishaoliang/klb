///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CShmem.hpp
/// @brief   (IPC)进程之间共享内存
///         shmget,shmat,shmdt,shmctl
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CSHMEM_HPP__
#define __KLB_CSHMEM_HPP__

#include "klb_type.h"

namespace klb {

/// @class CShmem
/// @brief 进程之间共享内存
KLB_EXTERN class KLB_API_CPP CShmem
{
public:
    CShmem();
    ~CShmem();

private:

};

} // namespace klb

#endif // __KLB_CSHMEM_HPP__
