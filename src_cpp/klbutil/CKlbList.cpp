// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CKlbList.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>


namespace klb {

//////////////////////////////////////////////////////////////////////////
// CKlbListIter

CKlbListIter::CKlbListIter()
{
    m_iter = NULL;
}

CKlbListIter::CKlbListIter(klb_nlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

CKlbListIter::CKlbListIter(const CKlbListIter& t)
{
    m_iter = NULL;

    CopyTo(t);
}

CKlbListIter::~CKlbListIter()
{

}

void CKlbListIter::CopyTo(const CKlbListIter& t)
{
    m_iter = t.m_iter;
}

CKlbListIter& CKlbListIter::operator=(const CKlbListIter& t)
{ 
    CopyTo(t);
    return (*this);
};

CKlbListIter& CKlbListIter::operator++()
{
    return Next();
}

CKlbListIter& CKlbListIter::operator--()
{
    return Prev();
}

bool CKlbListIter::operator==(const void* ptr)
{
    return (ptr == m_iter) ? true : false;
}

CKlbListIter& CKlbListIter::Next()
{
    if (NULL != m_iter)
    {
        m_iter = klb_nlist_next(m_iter);
    }

    return (*this);
}

CKlbListIter& CKlbListIter::Prev()
{
    if (NULL != m_iter)
    {
        m_iter = klb_nlist_prev(m_iter);
    }

    return (*this);
}

bool CKlbListIter::IsNull()
{
    return (NULL == m_iter) ? true : false;
}

void* CKlbListIter::Data()
{
    if (NULL != m_iter)
    {
        return klb_nlist_data(m_iter);
    }
    return NULL;
}

void CKlbListIter::SetIter(klb_nlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

//////////////////////////////////////////////////////////////////////////
// CKlbList

CKlbList& CKlbList::operator=(const CKlbList& t)
{
    assert(false);
    return *this;
}

CKlbList::CKlbList()
{
    m_list = klb_nlist_create();
}

CKlbList::~CKlbList()
{
    KLB_FREE_BY(m_list, klb_nlist_destroy);
}

void CKlbList::PushHead(void* p_data)
{
    klb_nlist_push_head(m_list, p_data);
}

void CKlbList::PushTail(void* p_data)
{
    klb_nlist_push_tail(m_list, p_data);
}

void* CKlbList::PopHead()
{
    return klb_nlist_pop_head(m_list);
}

void* CKlbList::PopTail()
{
    return klb_nlist_pop_tail(m_list);
}

void* CKlbList::Head()
{
    return klb_nlist_head(m_list);
}

void* CKlbList::Tail()
{
    return klb_nlist_tail(m_list);
}

int CKlbList::Size()
{
    return (int)klb_nlist_size(m_list);
}

void* CKlbList::Remove(const CKlbListIter& iter)
{
    assert(NULL != iter.m_iter);
    return klb_nlist_remove(m_list, iter.m_iter);
}

CKlbListIter CKlbList::Begin()
{
    return CKlbListIter(klb_nlist_begin(m_list));
}

CKlbListIter CKlbList::End()
{
    return CKlbListIter(klb_nlist_end(m_list));
}

} // namespace klb
