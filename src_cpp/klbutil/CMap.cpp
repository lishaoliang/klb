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

CMapObj& CMapObj::operator=(const CMap& v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const void* v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const char* v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const std::string& v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const bool& v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const double& v)
{
    ClearValue();
    Set(v);

    return (*this);
}

CMapObj& CMapObj::operator=(const int64_t& v)
{
    ClearValue();
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

CMapObj& CMapObj::Set(const CMap& v)
{
    ClearValue();

    m_value = new CMap(v);
    m_type = CMAP_CMap;

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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

    if (NULL != m_map)
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
    case CMAP_CMap:
        {
            CMap* ptr = (CMap*)m_value;
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
    return m_size;
}

CMapObj& CArray::Get(int idx)
{
    return m_none;
}

CMapObj& CArray::operator[](int idx)
{
    return Get(idx);
}

bool CArray::Get(int idx, CMap& v)
{
    return false;
}

bool CArray::Get(int idx, void** func, void** o)
{
    return false;
}

bool CArray::Get(int idx, void** v)
{
    return false;
}

bool CArray::Get(int idx, std::string& v)
{
    return false;
}

bool CArray::Get(int idx, bool& v)
{
    return false;
}

bool CArray::Get(int idx, double& v)
{
    return false;
}

bool CArray::Get(int idx, int64_t& v)
{
    return false;
}

CMapObjType CArray::Type(int idx)
{
    return CMAP_None;
}

bool CArray::IsType(int idx, CMapObjType t)
{
    return false;
}

CMap CArray::GetMap(int idx)
{
    return CMap();
}

void* CArray::GetFunc(int idx, void** o, const void* p_default)
{
    return (void*)p_default;
}

void* CArray::GetPtr(int idx, const void* p_default)
{
    return (void*)p_default;
}

std::string CArray::GetString(int idx, const std::string v_default)
{
    return v_default;
}

bool CArray::GetBool(int idx, const bool v_default)
{
    return v_default;
}

double CArray::GetDouble(int idx, const double v_default)
{
    return v_default;
}

int64_t CArray::GetInt64(int idx, const int64_t v_default)
{
    return v_default;
}

CArray& CArray::Set(int idx, const CMap& v)
{
    return (*this);
}

CArray& CArray::Set(int idx, const void* func, const void* o)
{
    return (*this);
}

CArray& CArray::Set(int idx, const void* v)
{
    return (*this);
}

CArray& CArray::Set(int idx, const char* v)
{
    return (*this);
}

CArray& CArray::Set(int idx, const std::string& v)
{
    return (*this);
}

CArray& CArray::Set(int idx, bool v)
{
    return (*this);
}

CArray& CArray::Set(int idx, double v)
{
    return (*this);
}

CArray& CArray::Set(int idx, int64_t v)
{
    return (*this);
}

// dump
std::string CArray::Dump()
{
    return "";
}

void CArray::Init()
{
    m_max = 0;
    m_size = 0;
    m_items = NULL;
}

void CArray::Quit()
{
    if (NULL != m_items)
    {
        for (int i = 0; i < m_size; i++)
        {
            if (NULL != m_items[i])
            {
                delete m_items[i];
            }
        }

        delete [] m_items;
        m_items = NULL;
    }

    m_max = 0;
    m_size = 0;
}

void CArray::ReSize(int n)
{
    if (m_max < n)
    {
        int num = n + 512;
        
        CMapObj** ptr = new CMapObj*[num];
        memset(ptr, 0, sizeof(CMapObj*) * num);

        if (0 < m_size)
        {
            memcpy(ptr, m_items, m_size);
        }

        m_max = num;

        CMapObj** p_old = m_items;
        m_items = ptr;

        if (NULL != p_old)
        {
            delete[] p_old;
        }
    }
}

void CArray::CopyTo(const CArray& t)
{

}


//////////////////////////////////////////////////////////////////////////
// CMap

CMap::CMap()
{
    Init();
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

CMap& CMap::operator=(const CMap& t)
{
    Quit();
    Init();
    CopyTo(t);
    return (*this);
}

//////////////////////////////////
// 

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

bool CMap::Get(const std::string& key, CMap& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, void** func, void** o)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(func, o);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, void** v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, char** data, int* size)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(data, size);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, std::string& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, bool& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, double& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
    }

    return false;
}

bool CMap::Get(const std::string& key, int64_t& v)
{
    CMapObj* ptr = Find(key);
    if (NULL != ptr)
    {
        ptr->Get(v);
        return true;
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

char* CMap::GetBuf(const std::string& key, int* size, const void* p_default)
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

    std::string s = a.Dump();


    CMap tb = a["1"].GetMap();

    std::string s2 = tb.Dump();

    CMap c = a;
    std::string sc = c.Dump();


    return 0;
}

};
