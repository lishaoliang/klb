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
#include "klbutil/klb_adt.h"
#include "klbutil/klb_vector.h"

namespace klb {

class CVector;

KLB_EXTERN class KLB_API_CPP CVectorItem
{
    friend class CVector;
public:
    CVectorItem();
    CVectorItem(const CVectorItem& t);
    ~CVectorItem();

    CVectorItem& operator=(const CVectorItem& t);
    CVectorItem& operator=(const int64_t i64);

    bool operator==(const int64_t i64);

    CVectorItem& Set(const int64_t i64);

    klb_adt_type_e Type();

    CVectorItem& Get(int64_t& i64);

    int64_t ToInt64(); 

protected:
    void SetVectorAdt(CVector* p_vector, int idx, klb_adt_t* p_adt);
    void SetVectorAdt(klb_adt_t* p_adt);

protected:
    void InitAdt();
    void QuitAdt();
    void CopyTo(const CVectorItem& t);

private:
    CVector*        m_ref_vector;   ///< CVector
    klb_adt_t*      m_ref_adt;      ///< CVector 节点数据的引用
    int             m_idx;          ///< 序号
};

/// @class CVector
/// @brief vector
KLB_EXTERN class KLB_API_CPP CVector
{
public:
    CVector();
    CVector(const CVector& t);
    ~CVector();

    CVector&     operator=(const CVector& t);
    CVector&     operator=(const klb_vector_t* p_vector);
    CVectorItem& operator[](int idx);

    int Size();

    CVector& AppendNull();
    CVector& Append(const int64_t i64);

    CVector& SetNull(const int idx);
    CVector& Set(const int idx, const int64_t i64);

    CVector& Get(const int idx, int64_t& i64);

    klb_adt_type_e Type(const int idx);

    CVector& Remove(const int idx);
    CVector& RemoveTail(const int idx);
    CVector& RemoveAll();

protected:
    klb_adt_t* GetNoneAdt();

private:
    void InitVector();
    void QuitVector();

private:
    klb_vector_t        m_vector;

    klb_adt_t           m_tmp_adt;
    CVectorItem         m_tmp_item;

public:
    static int Test();
};

} // namespace klb

#endif // __KLB_CVECTOR_HPP__
