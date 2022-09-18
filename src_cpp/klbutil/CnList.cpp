// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CnList.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>


namespace klb {

//////////////////////////////////////////////////////////////////////////
// CnListIter

CnListIter::CnListIter()
{
    m_iter = NULL;
}

CnListIter::CnListIter(klb_nlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

CnListIter::CnListIter(const CnListIter& t)
{
    m_iter = NULL;

    CopyTo(t);
}

CnListIter::~CnListIter()
{

}

void CnListIter::CopyTo(const CnListIter& t)
{
    m_iter = t.m_iter;
}

CnListIter& CnListIter::operator=(const CnListIter& t)
{ 
    CopyTo(t);
    return (*this);
};

CnListIter& CnListIter::operator++()
{
    return Next();
}

CnListIter& CnListIter::operator--()
{
    return Prev();
}

bool CnListIter::operator==(const void* ptr)
{
    return (ptr == m_iter) ? true : false;
}

void* CnListIter::operator*()
{
    return Data();
}

CnListIter& CnListIter::Next()
{
    if (NULL != m_iter)
    {
        m_iter = klb_nlist_next(m_iter);
    }

    return (*this);
}

CnListIter& CnListIter::Prev()
{
    if (NULL != m_iter)
    {
        m_iter = klb_nlist_prev(m_iter);
    }

    return (*this);
}

bool CnListIter::IsNull()
{
    return (NULL == m_iter) ? true : false;
}

void* CnListIter::Data()
{
    if (NULL != m_iter)
    {
        return klb_nlist_data(m_iter);
    }
    return NULL;
}

void CnListIter::SetIter(klb_nlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

//////////////////////////////////////////////////////////////////////////
// CnList

CnList& CnList::operator=(const CnList& t)
{
    assert(false);
    return *this;
}

CnList::CnList()
{
    m_list = klb_nlist_create();
}

CnList::~CnList()
{
    KLB_FREE_BY(m_list, klb_nlist_destroy);
}

void CnList::PushHead(void* p_data)
{
    klb_nlist_push_head(m_list, p_data);
}

void CnList::PushTail(void* p_data)
{
    klb_nlist_push_tail(m_list, p_data);
}

void* CnList::PopHead()
{
    return klb_nlist_pop_head(m_list);
}

void* CnList::PopTail()
{
    return klb_nlist_pop_tail(m_list);
}

void* CnList::Head()
{
    return klb_nlist_head(m_list);
}

void* CnList::Tail()
{
    return klb_nlist_tail(m_list);
}

int CnList::Size()
{
    return (int)klb_nlist_size(m_list);
}

void* CnList::Remove(const CnListIter& iter)
{
    assert(NULL != iter.m_iter);
    return klb_nlist_remove(m_list, iter.m_iter);
}

CnListIter& CnList::Begin()
{
    klb_nlist_iter_t* p_iter = klb_nlist_begin(m_list);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnListIter& CnList::End()
{
    klb_nlist_iter_t* p_iter = klb_nlist_end(m_list);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

} // namespace klb
