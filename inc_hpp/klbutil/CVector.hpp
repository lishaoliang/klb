///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CVector.hpp
/// @brief   vector
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CVECTOR_HPP__
#define __KLB_CVECTOR_HPP__

#include "klb_type.h"
#include "klbutil/klb_nvector.h"

namespace klb {

/// @class CVector
/// @brief vector
KLB_EXTERN class KLB_API_CPP CVector
{
public:
    CVector();
    ~CVector();


private:
    klb_nvector_t*    m_vector;
};

} // namespace klb

#endif // __KLB_CVECTOR_HPP__
