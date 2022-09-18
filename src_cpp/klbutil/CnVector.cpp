// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CnVector.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>

namespace klb {

// 禁止拷贝
CnVector& CnVector::operator=(const CnVector& t)
{
    assert(false);
    return *this;
}

CnVector::CnVector()
{
    m_vector = klb_nvector_create();
}

CnVector::~CnVector()
{
    KLB_FREE_BY(m_vector, klb_nvector_destroy);
}

void* CnVector::operator[](int idx)
{
    return Get(idx);
}

klb_nvector_t* CnVector::GetVector()
{
    return m_vector;
}

void CnVector::Clean(klb_nvector_clean_cb cb_clean, void* p_obj)
{
    klb_nvector_clean(m_vector, cb_clean, p_obj);
}

int CnVector::Size()
{
    return klb_nvector_size(m_vector);
}

CnVector& CnVector::Append(void* p_data)
{
    klb_nvector_push_tail(m_vector, p_data);
    return *this;
}

int CnVector::PushTail(void* p_data)
{
    return klb_nvector_push_tail(m_vector, p_data);
}

void* CnVector::PopHead()
{
    return klb_nvector_pop_head(m_vector);
}

void* CnVector::PopTail()
{
    return klb_nvector_pop_tail(m_vector);
}

void* CnVector::Head()
{
    return klb_nvector_head(m_vector);
}

void* CnVector::Tail()
{
    return klb_nvector_tail(m_vector);
}

void* CnVector::Get(int idx)
{
    return klb_nvector_get(m_vector, idx);
}

void* CnVector::Update(int idx, void* p_data)
{
    return klb_nvector_update(m_vector, idx, p_data);
}

void* CnVector::Remove(int idx)
{
    return klb_nvector_remove(m_vector, idx);
}

//////////////////////////////////////////////////////////////////////////

int CnVector::Test()
{
    return 0;
}

} // namespace klb
