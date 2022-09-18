///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CKlbList.hpp
/// @brief   沿用原始klb_list_t内存处理思路
///     存放的指针必须自行处理
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CKLB_LIST_HPP__
#define __CKLB_LIST_HPP__

#include "klb_type.h"
#include "klbutil/klb_nlist.h"


namespace klb {


class CKlbList;


/// @class CListIter
/// @brief 链表
KLB_EXTERN class KLB_API_CPP CKlbListIter
{
    friend class CKlbList;

public:
    CKlbListIter();
    CKlbListIter(klb_nlist_iter_t* p_iter);
    CKlbListIter(const CKlbListIter& t);
    ~CKlbListIter();

    CKlbListIter& operator=(const CKlbListIter& t);
    CKlbListIter& operator++();
    CKlbListIter& operator--();
    bool operator==(const void* ptr);

    CKlbListIter& Next();
    CKlbListIter& Prev();

    bool IsNull();

    void* Data();

protected:
    void SetIter(klb_nlist_iter_t* p_iter);

private:
    void CopyTo(const CKlbListIter& t);

protected:
    klb_nlist_iter_t*    m_iter;
};


/// @class CList
/// @brief 链表
KLB_EXTERN class KLB_API_CPP CKlbList
{
protected:
    CKlbList& operator=(const CKlbList& t); // 禁止拷贝

public:
    CKlbList();
    ~CKlbList();

    void PushHead(void* p_data);
    void PushTail(void* p_data);

    void* PopHead();
    void* PopTail();

    void* Head();
    void* Tail();

    int Size();

    void* Remove(const CKlbListIter& iter);

    CKlbListIter Begin();
    CKlbListIter End();

private:
    klb_nlist_t*    m_list;
};

} // namespace klb

#endif // __CKLB_LIST_HPP__
