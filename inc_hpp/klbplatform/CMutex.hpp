///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CMutex.hpp
/// @brief   普通锁
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CMUTEX_HPP__
#define __CMUTEX_HPP__

#include "klb_type.h"
#include "klbplatform/klb_mutex.h"

namespace klb {

    /// @class CMutex
    /// @brief 普通锁
    KLB_EXTERN class KLB_API_CPP CMutex
    {
    public:
        CMutex();
        ~CMutex();

        /// @brief 加锁
        /// @return 无
        void Lock();

        /// @brief 尝试加锁
        /// @return bool true.成功; false.失败
        bool TryLock();

        /// @brief 解锁
        /// @return 无
        void UnLock();

    private:
        klb_mutex_t*    m_mutex;
    };
}

#endif // __CMUTEX_HPP__
