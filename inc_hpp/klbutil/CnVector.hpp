///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CnVector.hpp
/// @brief   vector : 存放的指针,调用者需自行释放
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CNVECTOR_HPP__
#define __KLB_CNVECTOR_HPP__

#include "klb_type.h"
#include "klbutil/klb_nvector.h"


namespace klb {


/// @class CnVector
/// @brief vector : 存放的指针,调用者需自行释放
KLB_EXTERN class KLB_API_CPP CnVector
{
protected:
    CnVector& operator=(const CnVector& t); // 禁止拷贝

public:
    CnVector();
    ~CnVector();

    void* operator[](int idx);

    klb_nvector_t* GetVector();

    void Clear(klb_nvector_clear_cb cb_clear, void* p_obj);
    int Size();

    CnVector& Append(void* p_data);

    int PushTail(void* p_data);

    void* PopHead();
    void* PopTail();

    void* Head();
    void* Tail();

    void* Get(int idx);

    void* Update(int idx, void* p_data);
    void* Remove(int idx);

private:
    klb_nvector_t*      m_vector;

public:
    static int Test();
};

} // namespace klb

#endif // __KLB_CNVECTOR_HPP__
