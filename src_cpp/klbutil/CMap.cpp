#include "klbutil/CMap.hpp"
#include "klbmem/klb_mem.h"
#include <sstream>
#include <assert.h>

namespace klb {

//////////////////////////////////////////////////////////////////////////
// CMapIter

CMapIter::CMapIter()
{
    m_iter = NULL;
}

CMapIter::CMapIter(klb_hlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

CMapIter::CMapIter(const CMapIter& t)
{
    CopyTo(t);
}

CMapIter::~CMapIter()
{

}

CMapIter& CMapIter::operator=(const CMapIter& t)
{
    CopyTo(t);
    return (*this);
}

CMapIter CMapIter::Next()
{
    klb_hlist_iter_t* p_next = NULL;
    if (NULL != m_iter)
    {
        p_next = klb_hlist_next(m_iter);
    }

    return CMapIter(p_next);
}

bool CMapIter::IsEnd()
{
    return (NULL == m_iter);
}

CMapObj& CMapIter::Data()
{
    CMapObj* ptr = (CMapObj*)klb_hlist_data(m_iter);
    return (*ptr);
}

void CMapIter::CopyTo(const CMapIter& t)
{
    m_iter = t.m_iter;
}

//////////////////////////////////////////////////////////////////////////
// CMapObj

CMapObj::CMapObj()
{
    Init();
}

CMapObj::CMapObj(const CMapObj& t)
{
    Init();
    CopyTo(t);
}

CMapObj::CMapObj(const std::string& key)
{
    Init();
    m_key = key;
}

CMapObj::CMapObj(const std::string& key, const CArray& v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, const CMap& v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, const void* func, const void* o)
{
    Init();
    m_key = key;
    Set(func, o);
}

CMapObj::CMapObj(const std::string& key, const void* v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, const char* v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, const char* data, int size)
{
    Init();
    m_key = key;
    Set(data, size);
}

CMapObj::CMapObj(const std::string& key, const std::string& v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, bool v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, double v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::CMapObj(const std::string& key, int64_t v)
{
    Init();
    m_key = key;
    Set(v);
}

CMapObj::~CMapObj()
{
    Quit();
}

CMapObj& CMapObj::operator=(const CArray& v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const CMap& v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const void* v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const char* v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const std::string& v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const bool& v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const double& v)
{
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const int64_t& v)
{
    Set(v);

    return (*this);
}

bool CMapObj::operator==(const void* v)
{
    void* ptr = NULL;
    if (Get(&ptr))
    {
        return (ptr == v);
    }

    return false;
}

bool CMapObj::operator==(const char* v)
{
    if (NULL == v)
    {
        return false;
    }

    std::string s;
    if (Get(s))
    {
        return (s == v);
    }

    return false;
}

bool CMapObj::operator==(const std::string& v)
{
    std::string s;
    if (Get(s))
    {
        return (s == v);
    }

    return false;
}

bool CMapObj::operator==(const bool& v)
{
    bool b = false;
    if (Get(b))
    {
        return (b == v);
    }

    return false;
}

bool CMapObj::operator==(const int64_t& v)
{
    int64_t i = 0;
    if (Get(i))
    {
        return (i == v);
    }

    return false;
}

bool CMapObj::Get(CArray& v)
{
    if (CMAP_CArray == m_type)
    {
        v = *((CArray*)m_value);
        return true;
    }

    return false;
}

bool CMapObj::Get(CMap& v)
{
    if (CMAP_CMap == m_type)
    {
        v = *((CMap*)m_value);
        return true;
    }

    return false;
}

bool CMapObj::Get(void** func, void** o)
{
    if (CMAP_Func == m_type)
    {
        if (func)
        {
            *func = m_value;
        }

        if (o)
        {
            *o = m_obj;
        }
        return true;
    }

    return false;
}

bool CMapObj::Get(void** v)
{
    if (CMAP_Ptr == m_type)
    {
        if (v)
        {
            *v = m_value;
        }

        return true;
    }

    return false;
}

bool CMapObj::Get(char** data, int* size)
{
    if (CMAP_Buf == m_type)
    {
        if (size)
        {
            *size = (int)m_value_int64;
        }

        if (data)
        {
            *data = (char*)m_value;
        }

        return true;
    }

    return false;
}

bool CMapObj::Get(std::string& v)
{
    if (CMAP_String == m_type)
    {
        v = *((std::string*)m_value);
        return true;
    }

    return false;
}

bool CMapObj::Get(bool& v)
{
    if (CMAP_Bool == m_type)
    {
        v = m_value_bool;
        return true;
    }

    return false;
}

bool CMapObj::Get(double& v)
{
    if (CMAP_Double == m_type)
    {
        v = m_value_double;
        return true;
    }

    return false;
}

bool CMapObj::Get(int64_t& v)
{
    if (CMAP_Int64 == m_type)
    {
        v = m_value_int64;
        return true;
    }

    return false;
}

CMapObjType CMapObj::Type()
{
    return m_type;
}

bool CMapObj::IsType(CMapObjType t)
{
    return (m_type == t);
}

CArray CMapObj::GetArray()
{
    if (CMAP_CArray == m_type)
    {
        return *((CArray*)m_value);
    }

    return CArray();
}

CMap CMapObj::GetMap()
{
    if (CMAP_CMap == m_type)
    {
        return *((CMap*)m_value);
    }

    return CMap();
}

void* CMapObj::GetFunc(void** o, const void* p_default)
{
    if (CMAP_Func == m_type)
    {
        if (o)
        {
            *o = m_obj;
        }
        return m_value;
    }

    return (void*)p_default;
}

void* CMapObj::GetPtr(const void* p_default)
{
    if (CMAP_Ptr == m_type)
    {
        return m_value;
    }

    return (void*)p_default;
}

char* CMapObj::GetBuf(int* size, const void* p_default)
{
    if (CMAP_Buf == m_type)
    {
        if (size)
        {
            *size = (int)m_value_int64;
        }

        return (char*)m_value;
    }

    return (char*)(p_default);
}

std::string CMapObj::GetString(const std::string v_default)
{
    if (CMAP_String == m_type)
    {
        return *((std::string*)m_value);
    }

    return v_default;
}

bool CMapObj::GetBool(const bool v_default)
{
    if (CMAP_Bool == m_type)
    {
        return m_value_bool;
    }

    return v_default;
}

double CMapObj::GetDouble(const double v_default)
{
    if (CMAP_Double == m_type)
    {
        return m_value_double;
    }

    return v_default;
}

int64_t CMapObj::GetInt64(const int64_t v_default)
{
    if (CMAP_Int64 == m_type)
    {
        return m_value_int64;
    }

    return v_default;
}

CMapObj& CMapObj::Set(const CArray& v)
{
    ClearValue();

    m_value = new CArray(v);
    m_type = CMAP_CArray;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const CMap& v)
{
    ClearValue();

    m_value = new CMap(v);
    m_type = CMAP_CMap;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const void* func, const void* o)
{
    ClearValue();

    m_value = (void*)func;
    m_obj = (void*)o;
    m_type = CMAP_Func;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, func, o);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, func, o);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const void* v)
{
    ClearValue();

    m_value = (void*)v;
    m_type = CMAP_Ptr;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const char* v)
{
    ClearValue();

    m_value = new std::string(v ? v : "");
    m_type = CMAP_String;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const char* data, int size)
{
    ClearValue();

    char* ptr = KLB_MALLOCZ(char, size, 4);
    if (NULL != data && 0 < size)
    {
        memcpy(ptr, data, size);
    }

    m_value = ptr;
    m_value_int64 = size;
    m_type = CMAP_Buf;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, data, size);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, data, size);
    }

    return (*this);
}

CMapObj& CMapObj::Set(const std::string& v)
{
    ClearValue();

    m_value = new std::string(v);
    m_type = CMAP_String;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(bool v)
{
    ClearValue();

    m_value_bool = v;
    m_type = CMAP_Bool;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(double v)
{
    ClearValue();

    m_value_double = v;
    m_type = CMAP_Double;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}

CMapObj& CMapObj::Set(int64_t v)
{
    ClearValue();

    m_value_int64 = v;
    m_type = CMAP_Int64;

    if (NULL != m_array)
    {
        m_array->Set(m_array_idx, v);
    }
    else if (NULL != m_map)
    {
        m_map->Set(m_key, v);
    }

    return (*this);
}


std::string CMapObj::GetKey()
{
    return m_key;
}

std::string CMapObj::Dump()
{
    std::ostringstream ss;

    switch (m_type)
    {
    case CMAP_CArray:
        {
            CArray v;
            Get(v);

            ss << "[" << m_key << ":" << v.Dump() << "]";
        }
        break;
    case CMAP_CMap:
        {
            CMap v;
            Get(v);

            ss << "[" << m_key << ":" << v.Dump() << "]";
        }
        break;
    case CMAP_Func:
        {
            void* func = NULL;
            void* o = NULL;
            Get(&func, &o);

            ss << "{" << m_key << ":0x" << func << "-0x" << o << "}";
        }
        break;
    case CMAP_Ptr:
        {
            void* v = NULL;
            Get(&v);

            ss << "{" << m_key << ":0x" << v << "}";
        }
        break;
    case CMAP_String:
        {
            std::string v;
            Get(v);

            ss << "{" << m_key << ":" << v << "}";
        }
        break;
    case CMAP_Bool:
        {
            bool v;
            Get(v);

            ss << "{" << m_key << ":" << (v ? "true" : "false") << "}";
        }
        break;
    case CMAP_Double:
        {
            double v;
            Get(v);

            ss << "{" << m_key << ":" << v << "}";
        }
        break;
    case CMAP_Int64:
        {
            int64_t v;
            Get(v);

            ss << "{" << m_key << ":" << v << "}";
        }
        break;
    default:
        {
            ss << "{" << m_key << ":none}";
        }
        break;
    }

    return ss.str();
}

void CMapObj::Init()
{
    m_array = NULL;
    m_array_idx = -1;

    m_map = NULL;
    m_key = "";

    m_type = CMAP_None;
    m_value = NULL;
    m_value_int64 = 0;
}

void CMapObj::Quit()
{
    ClearValue();
}

void CMapObj::Reset()
{
    ClearValue();
    Init();
}

void CMapObj::SetArray(CArray* p_array, int idx)
{
    m_array = p_array;
    m_array_idx = idx;
}

void CMapObj::SetMap(CMap* p_map)
{
    m_map = p_map;
}

void CMapObj::SetKey(const std::string& key)
{
    m_key = key;
}

void CMapObj::ClearValue()
{
    switch (m_type)
    {
    case CMAP_CArray:
        {
            CArray* ptr = (CArray*)m_value;
            delete ptr;
        }
        break;
    case CMAP_CMap:
        {
            CMap* ptr = (CMap*)m_value;
            delete ptr;
        }
        break;
    case CMAP_Buf:
        {
            char* ptr = (char*)m_value;
            KLB_FREE(ptr);
        }
        break;
    case CMAP_String:
        {
            std::string* ptr = (std::string*)m_value;
            delete ptr;
        }
        break;
    default:
        break;
    }

    m_value = NULL;
    m_value_int64 = 0;

    m_type = CMAP_None;
}

void CMapObj::CopyTo(const CMapObj& t)
{
    m_map = t.m_map;
    m_key = t.m_key;

    m_type = t.m_type;
    switch (t.m_type)
    {
    case CMAP_CArray:
        {
            const CArray* p_v = (const CArray*)t.m_value;
            m_value = new CArray(*p_v);
        }
        break;
    case CMAP_CMap:
        {
            const CMap* p_v = (const CMap*)t.m_value;
            m_value = new CMap(*p_v);
        }
        break;
    case CMAP_Func:
        {
            m_value = t.m_value;
            m_obj = t.m_obj;
        }
        break;
    case CMAP_Ptr:
        {
            m_value = t.m_value;
        }
        break;
    case CMAP_String:
        {
            const std::string* p_v = (const std::string*)t.m_value;
            m_value = new std::string(*p_v);
        }
        break;
    case CMAP_Bool:
        {
            m_value_bool = t.m_value_bool;
        }
        break;
    case CMAP_Double:
        {
            m_value_double = t.m_value_double;
        }
        break;
    case CMAP_Int64:
        {
            m_value_int64 = t.m_value_int64;
        }
        break;
    default:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
// CArray

CArray::CArray()
{
    Init();
}

CArray::CArray(const CArray& t)
{
    Init();
    CopyTo(t);
}

CArray::~CArray()
{
    Quit();
}

CArray& CArray::operator=(const CArray& t)
{
    Quit();
    Init();
    CopyTo(t);
    return (*this);
}

// 大小
int CArray::Size()
{
    return klb_vector_size(m_vector);
}

CMapObj& CArray::Get(int idx)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return *p_obj;
    }
    else
    {
        m_none.Reset();
        m_none.SetArray(this, -1);

        return m_none;
    }
}

CMapObj& CArray::operator[](int idx)
{
    return Get(idx);
}

bool CArray::Get(int idx, CArray& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, CMap& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, void** func, void** o)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(func, o);
    }

    return false;
}

bool CArray::Get(int idx, void** v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, char** data, int* size)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(data, size);
    }

    return false;
}

bool CArray::Get(int idx, std::string& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, bool& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, double& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

bool CArray::Get(int idx, int64_t& v)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Get(v);
    }

    return false;
}

CMapObjType CArray::Type(int idx)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->Type();
    }

    return CMAP_None;
}

bool CArray::IsType(int idx, CMapObjType t)
{
    CMapObj* p_obj = Find(idx);
    if (NULL != p_obj)
    {
        return p_obj->IsType(t);
    }

    return false;
}

CArray CArray::GetArray(int idx)
{
    CArray v;
    if (Get(idx, v))
    {
        return v;
    }

    return CArray();
}

CMap CArray::GetMap(int idx)
{
    CMap v;
    if (Get(idx, v))
    {
        return v;
    }

    return CMap();
}

void* CArray::GetFunc(int idx, void** o, const void* p_default)
{
    void* func = NULL;
    if (Get(idx, &func, o))
    {
        return func;
    }

    return (void*)p_default;
}

void* CArray::GetPtr(int idx, const void* p_default)
{
    void* ptr = NULL;
    if (Get(idx, &ptr))
    {
        return ptr;
    }

    return (void*)p_default;
}

char* CArray::GetBuf(int idx, int* size, const char* p_default)
{
    char* ptr = NULL;
    if (Get(idx, &ptr, size))
    {
        return ptr;
    }

    return (char*)p_default;
}

std::string CArray::GetString(int idx, const std::string v_default)
{
    std::string s;
    if (Get(idx, s))
    {
        return s;
    }

    return v_default;
}

bool CArray::GetBool(int idx, const bool v_default)
{
    bool b;
    if (Get(idx, b))
    {
        return b;
    }

    return v_default;
}

double CArray::GetDouble(int idx, const double v_default)
{
    double d;
    if (Get(idx, d))
    {
        return d;
    }

    return v_default;
}

int64_t CArray::GetInt64(int idx, const int64_t v_default)
{
    int64_t n;
    if (Get(idx, n))
    {
        return n;
    }

    return v_default;
}

CArray& CArray::Set(int idx, const CArray& v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, const CMap& v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, const void* func, const void* o)
{
    return SetValue(idx, new CMapObj("", func, o));
}

CArray& CArray::Set(int idx, const void* v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, const char* v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, const char* data, int size)
{
    return SetValue(idx, new CMapObj("", data, size));
}

CArray& CArray::Set(int idx, const std::string& v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, bool v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, double v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Set(int idx, int64_t v)
{
    return SetValue(idx, new CMapObj("", v));
}

CArray& CArray::Append(const CArray& v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(const CMap& v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(const void* func, const void* o)
{
    return AppendValue(new CMapObj("", func, o));
}

CArray& CArray::Append(const void* v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(const char* v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(const char* data, int size)
{
    return AppendValue(new CMapObj("", data, size));
}

CArray& CArray::Append(const std::string& v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(bool v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(double v)
{
    return AppendValue(new CMapObj("", v));
}

CArray& CArray::Append(int64_t v)
{
    return AppendValue(new CMapObj("", v));
}

// dump
std::string CArray::Dump()
{
    std::ostringstream ss;

    for (int i = 0; i < Size();i++)
    {
        CMapObj& item = Get(i);
        ss << item.Dump() << "\n";
    }

    return ss.str();
}

CArray& CArray::SetValue(int idx, CMapObj* p_v)
{
    assert(NULL != p_v);
    assert(CMAP_None != p_v->m_type);

    if (0 <= idx && idx < klb_vector_size(m_vector))
    {
        // 替换
        p_v->SetArray(this, idx);

        CMapObj* p_obj = (CMapObj*)klb_vector_update(m_vector, idx, p_v);
        assert(NULL != p_obj);

        delete p_obj;
    }
    else
    {
        // 追加
        int idx_push = klb_vector_push_tail(m_vector, p_v);
        p_v->SetArray(this, idx_push);
    }

    return *this;
}

CArray& CArray::AppendValue(CMapObj* p_v)
{
    assert(NULL != p_v);
    assert(CMAP_None != p_v->m_type);

    int idx = klb_vector_push_tail(m_vector, p_v);
    p_v->SetArray(this, idx);

    return *this;
}

void CArray::Init()
{
    m_vector = klb_vector_create();
}

void CArray::Quit()
{
    while (0 < klb_vector_size(m_vector))
    {
        CMapObj* p_obj = (CMapObj*)klb_vector_pop_tail(m_vector);
        delete p_obj;
    }

    KLB_FREE_BY(m_vector, klb_vector_destroy);
}

CMapObj* CArray::Find(int idx)
{
    if (0 <= idx && idx < klb_vector_size(m_vector))
    {
        CMapObj* p_obj = (CMapObj*)klb_vector_get(m_vector, idx);
        assert(NULL != p_obj);

        return p_obj;
    }

    return NULL;
}

void CArray::CopyTo(const CArray& t)
{
    int size = klb_vector_size(t.m_vector);
    for (int i = 0; i < size; i++)
    {
        CMapObj* p_obj = (CMapObj*)klb_vector_get(t.m_vector, i);
        assert(NULL != p_obj);

        CMapObj* p_new = new CMapObj(*p_obj);

        int idx = klb_vector_push_tail(m_vector, p_new);
        p_new->SetArray(this, idx);

        assert(idx == i);
    }
}


//////////////////////////////////////////////////////////////////////////
// CMap

CMap::CMap()
{
    Init();
}

CMap::CMap(bool is_array)
{
    Init();

    m_is_array = true;
}

CMap::CMap(const CArray& t)
{
    Init();
    CopyTo(t);
}

CMap::CMap(const CMap& t)
{
    Init();
    CopyTo(t);
}

CMap::~CMap()
{
    Quit();
}

CMap& CMap::operator=(const CArray& t)
{
    Quit();
    Init();
    CopyTo(t);
    return (*this);
}

CMap& CMap::operator=(const CMap& t)
{
    Quit();
    Init();
    CopyTo(t);
    return (*this);
}

//////////////////////////////////
// 

bool CMap::IsArray()
{
    return m_is_array;
}

CArray& CMap::Array()
{
    return m_array;
}

int CMap::Size()
{
    return klb_hlist_size(m_hlist);
}

CMap&  CMap::Delete(const std::string& key)
{
    klb_hlist_iter_t* p_iter = klb_hlist_find_iter(m_hlist, key.c_str(), key.length());
    if (NULL != p_iter)
    {
        CMapObj* ptr = (CMapObj*)klb_hlist_remove(m_hlist, p_iter);
        delete ptr;
    }

    return (*this);
}

//////////////////////////////////
// get

CMapObj& CMap::Get(const std::string& key)
{
    CMapObj* ptr = Find(key);
    if (NULL == ptr)
    {
        m_none.Reset();
        m_none.SetKey(key);
        m_none.SetMap(this);

        ptr = &m_none;
    }
    else
    {
        assert(CMAP_None != ptr->m_type);
        assert(this == ptr->m_map);
    }

    return (*ptr);
}

CMapObj& CMap::operator[](const std::string& key)
{
    return Get(key);
}

//////////////////////////////////
// get value

bool CMap::Get(const std::string& key, CArray& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, CMap& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, void** func, void** o)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(func, o);
    }

    return false;
}

bool CMap::Get(const std::string& key, void** v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, char** data, int* size)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(data, size);
    }

    return false;
}

bool CMap::Get(const std::string& key, std::string& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, bool& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, double& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}

bool CMap::Get(const std::string& key, int64_t& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->Get(v);
    }

    return false;
}


//////////////////////////////////
// 按类型获取值

CMapObjType CMap::Type(const std::string& key)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return ptr->m_type;
    }

    return CMAP_None;
}

bool CMap::IsType(const std::string& key, CMapObjType t)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        return (ptr->m_type == t);
    }

    return (CMAP_None == t);
}

CArray CMap::GetArray(const std::string& key)
{
    CArray v;
    if (Get(key, v))
    {
        return v;
    }

    return CArray();
}

CMap CMap::GetMap(const std::string& key)
{
    CMap v;
    if (Get(key, v))
    {
        return v;
    }

    return CMap();
}

void* CMap::GetFunc(const std::string& key, void** obj, const void* p_default)
{
    void* func = NULL;
    if (Get(key, &func, obj))
    {
        return func;
    }

    return (void*)p_default;
}

void* CMap::GetPtr(const std::string& key, const void* p_default)
{
    void* p_v = NULL;
    if (Get(key, &p_v))
    {
        return p_v;
    }

    return (void*)p_default;
}

char* CMap::GetBuf(const std::string& key, int* size, const char* p_default)
{
    char* buf = NULL;
    if (Get(key, &buf, size))
    {
        return buf;
    }

    return (char*)p_default;
}

std::string CMap::GetString(const std::string& key, const std::string v_default)
{
    std::string v;
    if (Get(key, v))
    {
        return v;
    }

    return v_default;
}

bool CMap::GetBool(const std::string& key, const bool v_default)
{
    bool v = false;
    if (Get(key, v))
    {
        return v;
    }

    return v_default;
}

double CMap::GetDouble(const std::string& key, const double v_default)
{
    double v = 0.0;
    if (Get(key, v))
    {
        return v;
    }

    return v_default;
}

int64_t CMap::GetInt64(const std::string& key, const int64_t v_default)
{
    int64_t v = 0;
    if (Get(key, v))
    {
        return v;
    }

    return v_default;
}

//////////////////////////////////
// set

CMap& CMap::Set(const std::string& key, const CArray& v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, const CMap& v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, const void* func, const void* o)
{
    return SetValue(key, new CMapObj(key, func, o));
}

CMap& CMap::Set(const std::string& key, const void* v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, const char* v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, const char* data, int size)
{
    return SetValue(key, new CMapObj(key, data, size));
}

CMap& CMap::Set(const std::string& key, const std::string& v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, bool v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, double v)
{
    return SetValue(key, new CMapObj(key, v));
}

CMap& CMap::Set(const std::string& key, int64_t v)
{
    return SetValue(key, new CMapObj(key, v));
}

//////////////////////////////////
// 遍历

CMapIter CMap::Begin()
{
    return CMapIter((klb_hlist_iter_t*)klb_hlist_begin(m_hlist));
}

//////////////////////////////////
// dump

std::string CMap::Dump()
{
    std::ostringstream ss;

    CMapIter iter = Begin();
    while (!iter.IsEnd())
    {
        CMapObj& item = iter.Data();

        ss << item.Dump() << "\n";

        iter = iter.Next();
    }

    return ss.str();
}

CMap& CMap::SetValue(const std::string& key, CMapObj* p_v)
{
    assert(NULL != p_v);
    assert(CMAP_None != p_v->m_type);

    p_v->SetMap(this);

    CMapObj* p_old = (CMapObj*)klb_hlist_update(m_hlist, key.c_str(), key.length(), p_v);
    if (NULL != p_old)
    {
        delete p_old;
    }
    else
    {
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(m_hlist, key.c_str(), key.length(), p_v);
        assert(NULL != p_iter);
    }

    return (*this);
}

//////////////////////////////////
// private

void CMap::Init()
{
    m_is_array = false;
    m_hlist = klb_hlist_create(0);
}

void CMap::Quit()
{
    if (NULL != m_hlist)
    {
        while (0 < klb_hlist_size(m_hlist))
        {
            CMapObj* ptr = (CMapObj*)klb_hlist_pop_head(m_hlist);

            delete ptr;
        }

        KLB_FREE_BY(m_hlist, klb_hlist_destroy);
    }
}

CMapObj* CMap::Find(const std::string& key)
{
    return (CMapObj*)klb_hlist_find(m_hlist, key.c_str(), key.length());
}

void CMap::CopyTo(const CMap& t)
{
    klb_hlist_iter_t* p_iter = klb_hlist_begin(t.m_hlist);
    while (NULL != p_iter)
    {
        CMapObj* ptr = (CMapObj*)klb_hlist_data(p_iter);
        CMapObj* p_new = new CMapObj(*ptr);

        SetValue(ptr->m_key, p_new);

        p_iter = klb_hlist_next(p_iter);
    }
    m_is_array = false;
}

void CMap::CopyTo(const CArray& t)
{
    m_array = t;
    m_is_array = true;
}

//////////////////////////////////
// test

class CTestMap {
public:
    CTestMap() { m_count = 0; }
    ~CTestMap() {}

    int Add(int a, int b) { return a + b; }
    int Dec(int a) { return a; }

    int m_count;
};

int CMap::Test()
{
    CMap a;
    a["1"] = "1";
    a["2"] = std::string("123456");
    a["3"] = true;
    a["4"] = int64_t(1);
    a["5"] = 3.1415926;
    a["6"] = CMap::Test;
    a.Set("7", int64_t(777));

    int (CTestMap::*add)(int a, int b) = &CTestMap::Add;
    CTestMap ta;
    int cc = (ta.*add)(1, 2);

    CMap b;
    b["b1"] = "1";
    b["b2"] = std::string("123456");
    b["b3"] = true;
    b["b4"] = int64_t(1);
    b["b5"] = 3.1415926;
    b["b6"].Set((void*)CMap::Test, &b);


    a["b"] = b;

    double e = a["5"].GetDouble();

    if (a["1"] == "1")
    {
        std::string s1 = a["1"].Dump();
    }

    CArray arr;
    arr[0] = int64_t(100);
    arr[1] = true;
    arr[2] = 3.1415926;
    arr[-1] = "poiefsf";

    arr.Append(int64_t(100)).Append("sdfef").Append("112313");

    arr[0] = "r 100";

    a["array"] = arr;

    std::string s = a.Dump();

    CMap tb = a["1"].GetMap();

    std::string s2 = tb.Dump();

    CMap c = a;
    std::string sc = c.Dump();


    return 0;
}

};
