// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CVector.hpp"
#include "klbmem/klb_mem.h"


namespace klb {


CVectorItem::CVectorItem()
{

}

CVectorItem::CVectorItem(const CVectorItem& t)
{

}

CVectorItem::~CVectorItem()
{

}

void CVectorItem::InitAdt()
{

}

void CVectorItem::QuitAdt()
{

}

void CVectorItem::CopyTo(const CVectorItem& t)
{

}

CVectorItem& CVectorItem::operator=(const CVectorItem& t)
{
    return *this;
}

CVectorItem& CVectorItem::operator=(const int64_t i64)
{
    return *this;
}

bool CVectorItem::operator==(const int64_t i64)
{
    return false;
}

CVectorItem& CVectorItem::Set(const int64_t i64)
{
    return *this;
}

klb_adt_type_e CVectorItem::Type()
{
    return KLB_ADT_null;
}

CVectorItem& CVectorItem::Get(int64_t& i64)
{
    return *this;
}

int64_t CVectorItem::ToInt64()
{
    return 0;
}

void CVectorItem::SetVectorAdt(CVector* p_vector, int idx, klb_adt_t* p_adt)
{
    m_ref_vector = p_vector;
    m_ref_adt = p_adt;
    m_idx = idx; 
}

void CVectorItem::SetVectorAdt(klb_adt_t* p_adt)
{
    m_ref_vector = NULL;
    m_ref_adt = p_adt;
    m_idx = -1;
}

//////////////////////////////////////////////////////////////////////////
// CVector

CVector::CVector()
{
    klb_vector_init(&m_vector);
}

CVector::CVector(const CVector& t)
{

}

CVector::~CVector()
{
    klb_vector_quit(&m_vector);
}

void CVector::InitVector()
{

}

void CVector::QuitVector()
{

}

////////////////////////////////////////
// 操作符

CVector& CVector::operator=(const CVector& t)
{
    return *this;
}

CVector& CVector::operator=(const klb_vector_t* p_vector)
{
    return *this;
}

CVectorItem& CVector::operator[](int idx)
{
    return m_tmp_item;
}

////////////////////////////////////////

int CVector::Size()
{
    return klb_vector_size(&m_vector);
}

CVector& CVector::AppendNull()
{
    return *this;
}

CVector& CVector::Append(const int64_t i64)
{
    return *this;
}

CVector& CVector::SetNull(const int idx)
{
    return *this;
}

CVector& CVector::Set(const int idx, const int64_t i64)
{
    return *this;
}

CVector& CVector::Get(const int idx, int64_t& i64)
{
    return *this;
}

klb_adt_type_e CVector::Type(const int idx)
{
    return KLB_ADT_null;
}

CVector& CVector::Remove(const int idx)
{
    return *this;
}

CVector& CVector::RemoveTail(const int idx)
{
    return *this;
}

CVector& CVector::RemoveAll()
{
    return *this;
}

klb_adt_t* CVector::GetNoneAdt()
{
    klb_adt_set_null(&m_tmp_adt);
    return &m_tmp_adt;
}

//////////////////////////////////////////////////////////////////////////
// Test
int CVector::Test()
{
    return 0;
}

} // namespace klb
