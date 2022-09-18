// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CMap.hpp"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_map_serialize.h"
#include <iostream>
#include <sstream>
#include <assert.h>

namespace klb {

//////////////////////////////////////////////////////////////////////////
//  CMapItem

CMapItem::CMapItem()
{
    InitAdt();
}

CMapItem::CMapItem(const CMapItem& t)
{
    InitAdt();
    CopyTo(t);
}

CMapItem::~CMapItem()
{
    QuitAdt();
}

CMapItem& CMapItem::operator=(const CMapItem& t)
{
    CopyTo(t);
    return *this;
}

CMapItem& CMapItem::operator=(const bool b)
{
    return Set(b);
}

CMapItem& CMapItem::operator=(const int64_t i64)
{
    return Set(i64);
}

CMapItem& CMapItem::operator=(const uint64_t u64)
{
    return Set(u64);
}

CMapItem& CMapItem::operator=(const double d)
{
    return Set(d);
}

CMapItem& CMapItem::operator=(const std::string& str)
{
    return Set(str);
}

CMapItem& CMapItem::operator=(const char* p_str)
{
    return Set(p_str);
}

CMapItem& CMapItem::operator=(const CMap& m)
{
    return Set(m);
}

bool CMapItem::operator==(const bool b)
{
    // 注意: 必须 数据类型 和 值 都对应 才返回true
    if (KLB_ADT_bool == klb_adt_type(m_ref_adt))
    {
        return (b == klb_adt_to_bool(m_ref_adt)) ? true : false;
    }
    return false;
}

bool CMapItem::operator==(const int64_t i64)
{
    if (KLB_ADT_int64 == klb_adt_type(m_ref_adt))
    {
        int64_t v = klb_adt_to_int64(m_ref_adt);
        return (i64 == v) ? true : false;
    }
    return false;
}

bool CMapItem::operator==(const uint64_t u64)
{
    if (KLB_ADT_uint64 == klb_adt_type(m_ref_adt))
    {
        uint64_t v = klb_adt_to_uint64(m_ref_adt);
        return (u64 == v) ? true : false;
    }
    return false;
}

bool CMapItem::operator==(const std::string& str)
{
    if (KLB_ADT_string == klb_adt_type(m_ref_adt))
    {
        const char* ptr = klb_adt_to_string(m_ref_adt);
        return (str == ptr) ? true : false;
    }
    return false;
}

bool CMapItem::operator==(const char* p_str)
{
    if (NULL != p_str && KLB_ADT_string == klb_adt_type(m_ref_adt))
    {
        const char* ptr = klb_adt_to_string(m_ref_adt);
        return (0 == strcmp(ptr, p_str)) ? true : false;
    }
    return false;
}

CMapItem& CMapItem::Set(const bool b)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, b);
        }
        else
        {
            m_ref_map->Set(m_key, b);
        }
    }
    else
    {
        klb_adt_set_bool(m_ref_adt, b);
    }

    return *this;
}

CMapItem& CMapItem::Set(const int64_t i64)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, i64);
        }
        else
        {
            m_ref_map->Set(m_key, i64);
        }
    }
    else
    {
        klb_adt_set_int64(m_ref_adt, i64);
    }

    return *this;
}

CMapItem& CMapItem::Set(const uint64_t u64)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, u64);
        }
        else
        {
            m_ref_map->Set(m_key, u64);
        }
    }
    else
    {
        klb_adt_set_uint64(m_ref_adt, u64);
    }

    return *this;
}

CMapItem& CMapItem::Set(const double d)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, d);
        }
        else
        {
            m_ref_map->Set(m_key, d);
        }
    }
    else
    {
        klb_adt_set_double(m_ref_adt, d);
    }

    return *this;
}

CMapItem& CMapItem::Set(const std::string& str)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, str);
        }
        else
        {
            m_ref_map->Set(m_key, str);
        }
    }
    else
    {
        klb_adt_set_string(m_ref_adt, str.c_str());
    }

    return *this;
}

CMapItem& CMapItem::Set(const char* p_str)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, p_str);
        }
        else
        {
            m_ref_map->Set(m_key, p_str);
        }
    }
    else
    {
        klb_adt_set_string(m_ref_adt, p_str);
    }

    return *this;
}

CMapItem& CMapItem::Set(const char* p_str, int str_len)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, p_str, str_len);
        }
        else
        {
            m_ref_map->Set(m_key, p_str, str_len);
        }
    }
    else
    {
        klb_adt_set_lstring(m_ref_adt, p_str, str_len);
    }

    return *this;
}

CMapItem& CMapItem::Set(const void* ptr1, const void* ptr2)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, ptr1, ptr2);
        }
        else
        {
            m_ref_map->Set(m_key, ptr1, ptr2);
        }
    }
    else
    {
        klb_adt_set_ptr(m_ref_adt, ptr1, ptr2);
    }

    return *this;
}

CMapItem& CMapItem::Set(const CMap& m)
{
    if (NULL != m_ref_map)
    {
        if (KLB_CMAP_ITEM_array == m_type)
        {
            m_ref_map->Set(m_idx, m);
        }
        else
        {
            m_ref_map->Set(m_key, m);
        }
    }
    else
    {
        klb_adt_set_map_clone(m_ref_adt, &m.m_map);
    }

    return *this;
}

klb_adt_type_e CMapItem::Type()
{
    return klb_adt_type(m_ref_adt);
}

CMapItem& CMapItem::Get(bool& b)
{
    b = klb_adt_to_bool(m_ref_adt);
    return *this;
}

CMapItem& CMapItem::Get(int64_t& i64)
{
    i64 = klb_adt_to_int64(m_ref_adt);
    return *this;
}

CMapItem& CMapItem::Get(uint64_t& u64)
{
    u64 = klb_adt_to_uint64(m_ref_adt);
    return *this;
}

CMapItem& CMapItem::Get(double& d)
{
    d = klb_adt_to_double(m_ref_adt);
    return *this;
}

CMapItem& CMapItem::Get(std::string& str)
{
    str = klb_adt_to_string(m_ref_adt);
    return *this;
}

CMapItem& CMapItem::Get(const char** p_out_str, int* p_out_len)
{
    const char* p_str = klb_adt_to_lstring(m_ref_adt, p_out_len);

    if (NULL != p_out_str)
    {
        *p_out_str = p_str;
    }

    return *this;
}

CMapItem& CMapItem::Get(const void** p_out_ptr1, const void** p_out_ptr2)
{
    const void* p_ptr1 = klb_adt_to_ptr(m_ref_adt, p_out_ptr2);

    if (NULL != p_out_ptr1)
    {
        *p_out_ptr1 = p_ptr1;
    }

    return *this;
}

CMapItem& CMapItem::Get(CMap& m)
{
    klb_map_t* ptr = klb_adt_to_map(m_ref_adt);

    klb_map_copy(&m.m_map, ptr);
    return *this;
}

bool CMapItem::ToBool()
{
    return klb_adt_to_bool(m_ref_adt);
}

int64_t CMapItem::ToInt64()
{
    return klb_adt_to_int64(m_ref_adt);
}

uint64_t CMapItem::ToUint64()
{
    return klb_adt_to_uint64(m_ref_adt);
}

double CMapItem::ToDouble()
{
    return klb_adt_to_double(m_ref_adt);
}

std::string CMapItem::ToString()
{
    const char* p_str = klb_adt_to_string(m_ref_adt);
    return (NULL != p_str) ? p_str : "";
}

const char* CMapItem::ToString(int* p_out_len)
{
    return klb_adt_to_lstring(m_ref_adt, p_out_len);
}

const void* CMapItem::ToPtr(const void** p_out_ptr2)
{
    return klb_adt_to_ptr(m_ref_adt, p_out_ptr2);
}

std::string CMapItem::Print()
{
    switch (Type())
    {
    case KLB_ADT_null:
        return "null";
        break;
    case KLB_ADT_bool:
        return ToBool() ? "true" : "false";
        break;
    case KLB_ADT_string:
        {
            std::ostringstream ss;
            ss << "\"" << ToString() << "\"";
            return ss.str();
        }
        break;
    case KLB_ADT_double:
        {
            std::ostringstream ss;
            ss << ToDouble();
            return ss.str();
        }
        break;
    case KLB_ADT_uint64:
        {
            std::ostringstream ss;
            ss << ToUint64();
            return ss.str();
        }
        break;
    case KLB_ADT_int64:
        {
            std::ostringstream ss;
            ss << ToInt64();
            return ss.str();
        }
        break;
    case KLB_ADT_map:
        {
            klb::CMap map;
            Get(map);
            return map.PrintJson();
        }
        break;
    case KLB_ADT_ptr:
        {
            std::ostringstream ss;
            ss << "\"0x" << ToPtr(NULL) << "\"";
            return ss.str();
        }
        break;
    default:
        break;
    }

    return "null";
}

void CMapItem::SetMapAdt(CMap* p_map, const std::string& key, klb_adt_t* p_adt)
{
    assert(NULL != p_map);
    assert(NULL != p_adt);

    m_ref_map = p_map;
    m_ref_adt = p_adt;

    m_type = KLB_CMAP_ITEM_keyvalue;
    
    KLB_FREE_BY(m_key, sdsfree);
    m_key = sdsnew(key.c_str());
}

void CMapItem::SetMapAdt(CMap* p_map, int idx, klb_adt_t* p_adt)
{
    assert(NULL != p_map);
    assert(NULL != p_adt);

    m_ref_map = p_map;
    m_ref_adt = p_adt;

    m_type = KLB_CMAP_ITEM_array;
    m_idx = idx;
}

void CMapItem::SetMapAdt(klb_adt_t* p_adt)
{
    m_ref_map = NULL;
    m_ref_adt = p_adt;
    m_type = 0;
}

void CMapItem::InitAdt()
{
    m_ref_map = NULL;
    m_ref_adt = NULL;
    m_type = 0;
    m_idx = 0;
    m_key = NULL;
}

void CMapItem::QuitAdt()
{
    m_ref_map = NULL;
    m_ref_adt = NULL;
    m_type = 0;
    m_idx = 0;
    KLB_FREE_BY(m_key, sdsfree);
}

void CMapItem::CopyTo(const CMapItem& t)
{
    m_ref_map = t.m_ref_map;
    m_ref_adt = t.m_ref_adt;
    m_type = t.m_type;
    m_idx = t.m_idx;

    KLB_FREE_BY(m_key, sdsfree);
    m_key = sdsnew(t.m_key);
}

//////////////////////////////////////////////////////////////////////////
//  CMapIter

CMapIter::CMapIter()
{
    m_map = NULL;
    m_iter = NULL;
}

CMapIter::CMapIter(const CMapIter& t)
{
    m_map = t.m_map;
    m_iter = t.m_iter;
}

CMapIter::CMapIter(CMap* p_map, klb_map_iter_t* p_iter)
{
    m_map = p_map;
    m_iter = p_iter;
}

CMapIter::~CMapIter()
{
    m_map = NULL;
    m_iter = NULL;
}

CMapIter& CMapIter::operator=(const CMapIter& t)
{
    m_map = t.m_map;
    m_iter = t.m_iter;
    return *this;
}

CMapIter& CMapIter::operator++(int i)
{
    return Next();
}

CMapIter& CMapIter::operator--(int d)
{
    return Prev();
}

bool CMapIter::IsNull()
{
    return (NULL == m_iter) ? true : false;
}

CMapIter& CMapIter::Next()
{
    if (NULL != m_iter)
    {
        m_iter = klb_map_next(m_iter);
    }
    return *this;
}

CMapIter& CMapIter::Prev()
{
    if (NULL != m_iter)
    {
        m_iter = klb_map_prev(m_iter);
    }
    return *this;
}

CMapItem CMapIter::Value()
{
    assert(NULL != m_map);

    klb_adt_t* p_adt = NULL;
    if (NULL != m_iter)
    {
        p_adt = klb_map_data(m_iter);
        assert(NULL != p_adt);
    }
    else
    {
        p_adt = m_map->GetNoneAdt();
    }

    CMapItem item;
    item.SetMapAdt(p_adt);
    return item;
}

std::string CMapIter::Key()
{
    const char* p_key = NULL;
    if (NULL != m_iter)
    {
        p_key = klb_map_key(m_iter);
    }

    return (NULL != p_key) ? p_key : "";
}

//////////////////////////////////////////////////////////////////////////
// CMap

CMap::CMap()
{
    m_share = false;
    InitKlbMap();
}

CMap::CMap(klb_map_t* p_map, bool share)
{
    m_share = true;

    m_map.p_item_hlist = p_map->p_item_hlist;
    m_map.p_item_vector = p_map->p_item_vector;
    klb_adt_int(&m_none_adt);
}

CMap::CMap(klb_map_t* p_map)
{
    m_share = false;

    if (NULL != p_map)
    {
        assert(NULL != p_map->p_item_hlist);
        assert(NULL != p_map->p_item_vector);

        memset(&m_map, 0, sizeof(m_map));

        m_map.p_item_hlist = p_map->p_item_hlist;
        m_map.p_item_vector = p_map->p_item_vector;

        klb_adt_int(&m_none_adt);

        KLB_FREE(p_map);
    }
    else
    {
        InitKlbMap();
    }
}

CMap::CMap(const CMap& t)
{
    m_share = false;

    InitKlbMap();
    klb_map_copy(&m_map, &t.m_map);
}

CMap::~CMap()
{
    QuitKlbMap();
}

void CMap::InitKlbMap()
{
    memset(&m_map, 0, sizeof(m_map));
    klb_map_init(&m_map);
    klb_adt_int(&m_none_adt);
}

void CMap::QuitKlbMap()
{
    if (!m_share)
    {
        klb_map_quit(&m_map);
        klb_adt_quit(&m_none_adt);
    }
}

//////////////////////////////////

const klb_map_t* CMap::GetMap()
{
    return &m_map;
}

void CMap::Attach(klb_map_t* p_map)
{
    assert(!m_share); // 不许可共享模式

    if (NULL != p_map)
    {
        QuitKlbMap();

        assert(NULL != p_map->p_item_hlist);
        assert(NULL != p_map->p_item_vector);

        memset(&m_map, 0, sizeof(m_map));

        m_map.p_item_hlist = p_map->p_item_hlist;
        m_map.p_item_vector = p_map->p_item_vector;

        klb_adt_int(&m_none_adt);

        KLB_FREE(p_map);
    }
    else
    {
        Clear();
    }
}

void CMap::Clear()
{
    klb_map_clear(&m_map);
}

int CMap::Size()
{
    return klb_map_size(&m_map);
}

CMap& CMap::operator=(const CMap& t)
{
    klb_map_copy(&m_map, &t.m_map);
    return *this;
}

CMap& CMap::operator=(const klb_map_t* p_map)
{
    klb_map_copy(&m_map, p_map);
    return *this;
}

CMapItem& CMap::operator[](const std::string& key)
{
    const klb_adt_t* p_adt = klb_map_to_adt(&m_map, key.c_str());
    klb_adt_t* p_dst = (NULL != p_adt) ? (klb_adt_t*)p_adt : GetNoneAdt();

    m_none_item.SetMapAdt(this, key, p_dst);
    return m_none_item;
}

CMapItem& CMap::operator[](const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";

    const klb_adt_t* p_adt = klb_map_to_adt(&m_map, key.c_str());
    klb_adt_t* p_dst = (NULL != p_adt) ? (klb_adt_t*)p_adt : GetNoneAdt();

    m_none_item.SetMapAdt(this, key, p_dst);
    return m_none_item;
}

CMapItem& CMap::operator[](int idx)
{
    const klb_adt_t* p_adt = klb_map_idx_to_adt(&m_map, idx);
    klb_adt_t* p_dst = (NULL != p_adt) ? (klb_adt_t*)p_adt : GetNoneAdt();

    m_none_item.SetMapAdt(this, idx, p_dst);
    return m_none_item;
}

//////////////////////////////////

CMap& CMap::SetNull(const std::string& key)
{
    klb_map_set_null(&m_map, key.c_str());
    return *this;
}

CMap& CMap::Set(const std::string& key, const bool b)
{
    klb_map_set_bool(&m_map, key.c_str(), b);
    return *this;
}

CMap& CMap::Set(const std::string& key, const int64_t i64)
{
    klb_map_set_int64(&m_map, key.c_str(), i64);
    return *this;
}

CMap& CMap::Set(const std::string& key, const uint64_t u64)
{
    klb_map_set_uint64(&m_map, key.c_str(), u64);
    return *this;
}

CMap& CMap::Set(const std::string& key, const double d)
{
    klb_map_set_double(&m_map, key.c_str(), d);
    return *this;
}

CMap& CMap::Set(const std::string& key, const std::string& str)
{
    klb_map_set_lstring(&m_map, key.c_str(), str.c_str(), str.size());
    return *this;
}

CMap& CMap::Set(const std::string& key, const char* p_str)
{
    klb_map_set_string(&m_map, key.c_str(), p_str);
    return *this;
}

CMap& CMap::Set(const std::string& key, const char* p_str, int str_len)
{
    klb_map_set_lstring(&m_map, key.c_str(), p_str, str_len);
    return *this;
}

CMap& CMap::Set(const std::string& key, const void* ptr1, const void* ptr2)
{
    klb_map_set_ptr(&m_map, key.c_str(), ptr1, ptr2);
    return *this;
}

CMap& CMap::Set(const std::string& key, const CMap& m)
{
    klb_map_set_map_clone(&m_map, key.c_str(), &m.m_map);
    return *this;
}

CMap& CMap::SetNull(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return SetNull(key);
}

CMap& CMap::Set(const char* p_key, bool b)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, b);
}

CMap& CMap::Set(const char* p_key, int64_t i64)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, i64);
}

CMap& CMap::Set(const char* p_key, uint64_t u64)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, u64);
}

CMap& CMap::Set(const char* p_key, double d)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, d);
}

CMap& CMap::Set(const char* p_key, const std::string& str)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, str);
}

CMap& CMap::Set(const char* p_key, const char* p_str)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, p_str);
}

CMap& CMap::Set(const char* p_key, const char* p_str, int str_len)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, p_str, str_len);
}

CMap& CMap::Set(const char* p_key, const void* ptr1, const void* ptr2)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, ptr1, ptr2);
}

CMap& CMap::Set(const char* p_key, const CMap& m)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Set(key, m);
}

CMap& CMap::Get(const std::string& key, bool& b)
{
    b = klb_map_to_bool(&m_map, key.c_str());
    return *this;
}

CMap& CMap::Get(const std::string& key, int64_t& i64)
{
    i64 = klb_map_to_int64(&m_map, key.c_str());
    return *this;
}

CMap& CMap::Get(const std::string& key, uint64_t& u64)
{
    u64 = klb_map_to_uint64(&m_map, key.c_str());
    return *this;
}

CMap& CMap::Get(const std::string& key, double& d)
{
    d = klb_map_to_double(&m_map, key.c_str());
    return *this;
}

CMap& CMap::Get(const std::string& key, std::string& str)
{
    const char* p_str = klb_map_to_string(&m_map, key.c_str());
    str = (NULL != p_str) ? p_str : "";
    return *this;
}

CMap& CMap::Get(const std::string& key, const char** p_out_str, int* p_out_len)
{
    const char* p_str = klb_map_to_lstring(&m_map, key.c_str(), p_out_len);

    if (NULL != p_out_str)
    {
        *p_out_str = p_str;
    }

    return *this;
}

CMap& CMap::Get(const std::string& key, const void** p_out_ptr1, const void** p_out_ptr2)
{
    const void* ptr1 = klb_map_to_ptr(&m_map, key.c_str(), p_out_ptr2);

    if (NULL != p_out_ptr1)
    {
        *p_out_ptr1 = ptr1;
    }
    return *this;
}

CMap& CMap::Get(const std::string& key, CMap& m)
{
    klb_map_t* ptr = klb_map_to_map(&m_map, key.c_str());

    klb_map_copy(&m.m_map, ptr);
    return *this;
}

CMap& CMap::Get(const char* p_key, bool& b)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, b);
}

CMap& CMap::Get(const char* p_key, int64_t& i64)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, i64);
}

CMap& CMap::Get(const char* p_key, uint64_t& u64)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, u64);
}

CMap& CMap::Get(const char* p_key, double& d)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, d);
}

CMap& CMap::Get(const char* p_key, std::string& str)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, str);
}

CMap& CMap::Get(const char* p_key, const char** p_out_str, int* p_out_len)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, p_out_str, p_out_len);
}

CMap& CMap::Get(const char* p_key, const void** p_out_ptr1, const void** p_out_ptr2)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, p_out_ptr1, p_out_ptr2);
}

CMap& CMap::Get(const char* p_key, CMap& m)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Get(key, m);
}

klb_adt_type_e CMap::Type(const std::string& key)
{
    return (klb_adt_type_e)klb_map_type(&m_map, key.c_str());
}

klb_adt_type_e CMap::Type(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Type(key);
}

int CMap::KeyValueSize()
{
    return klb_map_key_value_size(&m_map);
}

bool CMap::Remove(const std::string& key)
{
    return klb_map_remove_by_key(&m_map, key.c_str());
}

bool CMap::Remove(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Remove(key);
}

bool CMap::Remove(const CMapIter& iter)
{
    klb_map_remove(&m_map, iter.m_iter);
    return true;
}

CMapIter CMap::Begin()
{
    klb_map_iter_t* p_iter = klb_map_begin(&m_map);
    return CMapIter(this, p_iter);
}

CMapIter CMap::End()
{
    klb_map_iter_t* p_iter = klb_map_end(&m_map);
    return CMapIter(this, p_iter);
}

CMap& CMap::AppendNull()
{
    klb_map_append_null(&m_map);
    return *this;
}

CMap& CMap::Append(const bool b)
{
    klb_map_append_bool(&m_map, b);
    return *this;
}

CMap& CMap::Append(const int64_t i64)
{
    klb_map_append_int64(&m_map, i64);
    return *this;
}

CMap& CMap::Append(const uint64_t u64)
{
    klb_map_append_uint64(&m_map, u64);
    return *this;
}

CMap& CMap::Append(const double d)
{
    klb_map_append_double(&m_map, d);
    return *this;
}

CMap& CMap::Append(const std::string& str)
{
    klb_map_append_string(&m_map, str.c_str());
    return *this;
}

CMap& CMap::Append(const char* p_str)
{
    klb_map_append_string(&m_map, p_str);
    return *this;
}

CMap& CMap::Append(const char* p_str, int str_len)
{
    klb_map_append_lstring(&m_map, p_str, str_len);
    return *this;
}

CMap& CMap::Append(const void* ptr1, const void* ptr2)
{
    klb_map_append_ptr(&m_map, ptr1, ptr2);
    return *this;
}

CMap& CMap::Append(const CMap& m)
{
    klb_map_append_map_clone(&m_map, &m.m_map);
    return *this;
}

CMap& CMap::SetNull(const int idx)
{
    klb_map_set_idx_null(&m_map, idx);
    return *this;
}

CMap& CMap::Set(const int idx, const bool b)
{
    klb_map_set_idx_bool(&m_map, idx, b);
    return *this;
}

CMap& CMap::Set(const int idx, const int64_t i64)
{
    klb_map_set_idx_int64(&m_map, idx, i64);
    return *this;
}

CMap& CMap::Set(const int idx, const uint64_t u64)
{
    klb_map_set_idx_uint64(&m_map, idx, u64);
    return *this;
}

CMap& CMap::Set(const int idx, const double d)
{
    klb_map_set_idx_double(&m_map, idx, d);
    return *this;
}

CMap& CMap::Set(const int idx, const std::string& str)
{
    klb_map_set_idx_string(&m_map, idx, str.c_str());
    return *this;
}

CMap& CMap::Set(const int idx, const char* p_str)
{
    klb_map_set_idx_string(&m_map, idx, p_str);
    return *this;
}

CMap& CMap::Set(const int idx, const char* p_str, int str_len)
{
    klb_map_set_idx_lstring(&m_map, idx, p_str, str_len);
    return *this;
}

CMap& CMap::Set(const int idx, const void* ptr1, const void* ptr2)
{
    klb_map_set_idx_ptr(&m_map, idx, ptr1, ptr2);
    return *this;
}

CMap& CMap::Set(const int idx, const CMap& m)
{
    klb_map_set_idx_map_clone(&m_map, idx, &m.m_map);
    return *this;
}

CMap& CMap::Get(const int idx, bool& b)
{
    b = klb_map_idx_to_bool(&m_map, idx);
    return *this;
}

CMap& CMap::Get(const int idx, int64_t& i64)
{
    i64 = klb_map_idx_to_int64(&m_map, idx);
    return *this;
}

CMap& CMap::Get(const int idx, uint64_t& u64)
{
    u64 = klb_map_idx_to_uint64(&m_map, idx);
    return *this;
}

CMap& CMap::Get(const int idx, double& d)
{
    d = klb_map_idx_to_double(&m_map, idx);
    return *this;
}

CMap& CMap::Get(const int idx, std::string& str)
{
    str = klb_map_idx_to_string(&m_map, idx);
    return *this;
}

CMap& CMap::Get(const int idx, const char** p_out_str, int* p_out_len)
{
    const char* p_str = klb_map_idx_to_lstring(&m_map, idx, p_out_len);

    if (NULL != p_out_str)
    {
        *p_out_str = p_str;
    }

    return *this;
}

CMap& CMap::Get(const int idx, const void** p_out_ptr1, const void** p_out_ptr2)
{
    const void* ptr1 = klb_map_idx_to_ptr(&m_map, idx, p_out_ptr2);

    if (NULL != p_out_ptr1)
    {
        *p_out_ptr1 = ptr1;
    }

    return *this;
}

CMap& CMap::Get(const int idx, CMap& m)
{
    klb_map_t* ptr = klb_map_idx_to_map(&m_map, idx);

    klb_map_copy(&m.m_map, ptr);
    return *this;
}

klb_adt_type_e CMap::Type(const int idx)
{
    return (klb_adt_type_e)klb_map_array_type(&m_map, idx);
}

int CMap::ArraySize()
{
    return klb_map_array_size(&m_map);
}

bool CMap::Remove(int idx)
{
    return klb_map_idx_remove(&m_map, idx);
}

bool CMap::RemoveTail()
{
    return klb_map_idx_remove_tail(&m_map);
}

klb_adt_t* CMap::GetNoneAdt()
{
    klb_adt_set_null(&m_none_adt);
    return &m_none_adt;
}

//////////////////////////////////
klb_buf_t* CMap::Pack()
{
    return klb_map_seri_pack(&m_map);
}

void CMap::Unpack(const char* p_data, int data_len)
{
    Clear();

    klb_map_seri_unpack(&m_map, p_data, data_len);
}

//////////////////////////////////

static void CMapPrintArray(CMap& map, std::ostringstream& ss)
{
    for (int i = 0; i < map.ArraySize(); i++)
    {
        CMapItem& item = map[i];

        if (0 != i)
        {
            ss << ",";
        }

        ss << item.Print();
    }
}

static void CMapPrintKV(CMap& map, std::ostringstream& ss)
{
    bool first = true;
    CMapIter iter = map.Begin();
    while (!iter.IsNull())
    {
        std::string key = iter.Key();
        CMapItem item = iter.Value();

        if (first)
        {
            first = false;
        }
        else
        {
            ss << ",";
        }

        ss << "\"" << key << "\":" << item.Print();

        iter++;
    }
}

std::string CMap::PrintJson()
{
    std::ostringstream ss;

    if (0 < ArraySize())
    {
        if (0 < KeyValueSize())
        {
            ss << "[[";
            CMapPrintArray(*this, ss);
            ss << "],{";
            CMapPrintKV(*this, ss);
            ss << "}]";
        }
        else
        {
            ss << "[";
            CMapPrintArray(*this, ss);
            ss << "]";
        }
    }
    else
    {
        ss << "{";
        CMapPrintKV(*this, ss);
        ss << "}";
    }

    return ss.str();
}

//////////////////////////////////
// test

int CMap::Test()
{
    CMap a;
    a["1"] = "1";
    a["2"] = std::string("123456");
    a["3"] = true;
    a["4"] = (int64_t)1;
    a["5"] = 3.1415926;
    
    a[0] = (int64_t)20000000000;
    a[1] = (int64_t)1;
    a[8] = (int64_t)8;

    CMap b;
    b["b1"] = "1";
    b["b2"] = std::string("123456");
    b["b3"] = true;
    b["b4"] = int64_t(1);
    b["b5"] = 3.1415926;
    b["b6"].Set((const void*)CMap::Test, &b);
    b["b7"] = uint64_t(0xFFFFFFFF) - 1;
    b["b8"] = uint64_t(0xFFFFFFFF);
    b["b9"] = "";


    a["b"] = b;

    CMap c;
    c[0] = "c0";
    c[1] = "c1";
    c[2] = "c2";
    c[3] = "c3";
    c[4] = a;

    a["c"] = c;

    double e = a["5"].ToDouble();

    std::string s = a.PrintJson();

    klb_buf_t* p_buf = a.Pack();

    CMap z;
    z.Unpack(p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
    
    std::string sz = z.PrintJson();

    KLB_FREE(p_buf);
    return 0;
}

}; // namespace klb
