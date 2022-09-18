///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CList.hpp
/// @brief   仿std::list,托管内存
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CLIST_HPP__
#define __KLB_CLIST_HPP__


#include "klb_type.h"
#include "klbutil/klb_adt.h"
#include "klbutil/klb_list.h"
#include <string>

namespace klb {

KLB_EXTERN class KLB_API_CPP CListItem
{
public:
    CListItem();
    ~CListItem();

protected:
    klb_adt_t*          m_adt;
};

KLB_EXTERN class KLB_API_CPP CListIter
{
public:
    CListIter();
    ~CListIter();

protected:
    klb_list_iter_t*    m_iter;
};


/// @class CList
/// @brief list
KLB_EXTERN class KLB_API_CPP CList
{
public:
    CList();
    CList(const CList& t);
    ~CList();

    /////////////////////////////////////
    // 
    const klb_list_t* GetList();

    void Attach(klb_list_t* p_list); // 托管内存
    void Clear();
    int  Size();

    /////////////////////////////////////
    // 操作符

    CList&     operator=(const CList& t);
    CList&     operator=(const klb_list_t* p_list);

    CListItem& operator[](int idx);

    /////////////////////////////////////
    // PushHead

    void PushHeadNull();
    void PushHead(const bool b);
    void PushHead(const int64_t i64);
    void PushHead(const std::string& str);

    /////////////////////////////////////
    // PushTail

    void PushTailNull();
    void PushTail(const bool b);
    void PushTail(const int64_t i64);
    void PushTail(const std::string& str);

    //CListItem& PopHead();
    //CListItem& PopTail();

    //CListItem Head();
    //CListItem Tail();

    void Remove(const CListIter& iter);

    //CListIter Begin();
    //CListIter End();

protected:
    klb_adt_t* GetNoneAdt();

private:
    void InitKlbList();
    void QuitKlbList();

protected:
    klb_list_t      m_list;         ///< list

    klb_adt_t       m_none_adt;     ///< adt
    CListItem       m_none_item;    ///< none

public:
    static int Test();
};

} // namespace klb

#endif // __KLB_CLIST_HPP__
