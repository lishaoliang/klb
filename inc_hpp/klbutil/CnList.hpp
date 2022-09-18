///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CnList.hpp
/// @brief   沿用原始klb_list_t内存处理思路
///     存放的指针必须调用者自行释放
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CNLIST_HPP__
#define __KLB_CNLIST_HPP__

#include "klb_type.h"
#include "klbutil/klb_nlist.h"


namespace klb {


class CnList;


/// @class CnListIter
/// @brief 链表
KLB_EXTERN class KLB_API_CPP CnListIter
{
    friend class CnList;

public:
    CnListIter();
    CnListIter(klb_nlist_iter_t* p_iter);
    CnListIter(const CnListIter& t);
    ~CnListIter();

    CnListIter& operator=(const CnListIter& t);
    CnListIter& operator++();
    CnListIter& operator--();
    bool operator==(const void* ptr);
    void* operator*();

    CnListIter& Next();
    CnListIter& Prev();

    bool IsNull();

    void* Data();

protected:
    void SetIter(klb_nlist_iter_t* p_iter);

private:
    void CopyTo(const CnListIter& t);

protected:
    klb_nlist_iter_t*    m_iter;
};


/// @class CnList
/// @brief 链表: 存放的指针,调用者需自行释放
KLB_EXTERN class KLB_API_CPP CnList
{
protected:
    CnList& operator=(const CnList& t); // 禁止拷贝

public:
    CnList();
    ~CnList();

    void PushHead(void* p_data);
    void PushTail(void* p_data);

    void* PopHead();
    void* PopTail();

    void* Head();
    void* Tail();

    int Size();

    void* Remove(const CnListIter& iter);

    CnListIter& Begin();
    CnListIter& End();

private:
    klb_nlist_t*    m_list;
    CnListIter      m_tmp_iter;
};

} // namespace klb

#endif // __KLB_CNLIST_HPP__
