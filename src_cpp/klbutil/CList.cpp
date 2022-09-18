// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CList.hpp"
#include "klbmem/klb_mem.h"


namespace klb {

//////////////////////////////////////////////////////////////////////////
// CListItem

CListItem::CListItem()
{

}

CListItem::~CListItem()
{

}

//////////////////////////////////////////////////////////////////////////
// CListIter

CListIter::CListIter()
{

}

CListIter::~CListIter()
{

}


//////////////////////////////////////////////////////////////////////////
// CList

CList::CList()
{

}

CList::CList(const CList& t)
{

}

CList::~CList()
{

}

void CList::InitKlbList()
{

}

void CList::QuitKlbList()
{

}

//////////////////////////////////////
// 

const klb_list_t* CList::GetList()
{
    return NULL;
}

void CList::Attach(klb_list_t* p_list)
{

}

void CList::Clear()
{

}

int CList::Size()
{
    return klb_list_size(&m_list);
}

/////////////////////////////////////
// 操作符

CList& CList::operator=(const CList& t)
{
    return *this;
}

CList& CList::operator=(const klb_list_t* p_list)
{
    return *this;
}

CListItem& CList::operator[](int idx)
{
    return m_none_item;
}

//////////////////////////////////////
// CList::PushHead

void CList::PushHeadNull()
{

}

void CList::PushHead(const bool b)
{

}

void CList::PushHead(const int64_t i64)
{

}

void CList::PushHead(const std::string& str)
{

}

//////////////////////////////////////
// CList::PushTail

void CList::PushTailNull()
{

}

void CList::PushTail(const bool b)
{

}

void CList::PushTail(const int64_t i64)
{

}

void CList::PushTail(const std::string& str)
{

}

//////////////////////////////////////

void CList::Remove(const CListIter& iter)
{

}

klb_adt_t* CList::GetNoneAdt()
{
    klb_adt_set_null(&m_none_adt);
    return &m_none_adt;
}

//////////////////////////////////////////////////////////////////////////


int CList::Test()
{



    return 0;
}

} // namespace klb
