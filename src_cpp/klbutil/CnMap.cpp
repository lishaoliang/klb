// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CnMap.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>


namespace klb {

//////////////////////////////////////////////////////////////////////////
// CnMapIter

CnMapIter::CnMapIter()
{
    m_iter = NULL;
}

CnMapIter::CnMapIter(klb_hlist_iter_t* p_iter)
{
    m_iter = p_iter;
}

CnMapIter::CnMapIter(const CnMapIter& t)
{
    m_iter = NULL;
    CopyTo(t);
}

CnMapIter::~CnMapIter()
{

}

void CnMapIter::CopyTo(const CnMapIter& t)
{
    m_iter = t.m_iter;
}

CnMapIter& CnMapIter::operator=(const CnMapIter& t)
{
    CopyTo(t);
    return *this;
}

CnMapIter& CnMapIter::operator++()
{
    return Next();
}

CnMapIter& CnMapIter::operator--()
{
    return Prev();
}

bool CnMapIter::operator==(const void* ptr)
{
    return (m_iter == ptr) ? true : false;
}

void* CnMapIter::operator*()
{
    return Data();
}

CnMapIter& CnMapIter::Next()
{
    if (NULL != m_iter)
    {
        m_iter = klb_hlist_next(m_iter);
    }

    return *this;
}

CnMapIter& CnMapIter::Prev()
{
    if (NULL != m_iter)
    {
        m_iter = klb_hlist_prev(m_iter);
    }

    return *this;
}

bool CnMapIter::IsNull()
{
    return (NULL == m_iter) ? true : false;
}

void* CnMapIter::Data()
{
    if (NULL != m_iter)
    {
        return klb_hlist_data(m_iter);
    }

    return NULL;
}

void* CnMapIter::Key(int* p_out_key_len)
{
    if (NULL != m_iter)
    {
        return klb_hlist_key(m_iter, p_out_key_len);
    }

    if (NULL != p_out_key_len)
    {
        *p_out_key_len = 0;
    }
    return NULL;
}

void CnMapIter::SetIter(klb_hlist_iter_t* p_iter)
{
    m_iter = p_iter;
}


//////////////////////////////////////////////////////////////////////////
// CnMap

// 禁止拷贝
CnMap& CnMap::operator=(const CnMap& t)
{
    assert(false);
    return *this;
}

CnMap::CnMap()
{
    m_hlist = klb_hlist_create(0);
}

CnMap::~CnMap()
{
    KLB_FREE_BY(m_hlist, klb_hlist_destroy);
}

CnMapIter& CnMap::operator[](const std::string& key)
{
    return FindIter(key);
}

CnMapIter& CnMap::operator[](const char* p_key)
{
    return FindIter(p_key);
}

klb_hlist_t* CnMap::GetHlist()
{
    return m_hlist;
}

void CnMap::Clear(klb_hlist_clear_cb cb_clear, void* p_obj)
{
    klb_hlist_clear(m_hlist, cb_clear, p_obj);
}

int CnMap::Size()
{
    return klb_hlist_size(m_hlist);
}

CnMapIter& CnMap::PushHead(const void* p_key, int key_len, void* p_data)
{
    klb_hlist_iter_t* p_iter = klb_hlist_push_head(m_hlist, p_key, key_len, p_data);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::PushHead(const std::string& key, void* p_data)
{
    klb_hlist_iter_t* p_iter = klb_hlist_push_head(m_hlist, key.c_str(), key.length(), p_data);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::PushHead(const char* p_key, void* p_data)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return PushHead(key, p_data);
}

CnMapIter& CnMap::PushTail(const void* p_key, int key_len, void* p_data)
{
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(m_hlist, p_key, key_len, p_data);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::PushTail(const std::string& key, void* p_data)
{
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(m_hlist, key.c_str(), key.length(), p_data);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::PushTail(const char* p_key, void* p_data)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return PushTail(key, p_data);
}

void* CnMap::PopHead()
{
    return klb_hlist_pop_head(m_hlist);
}

void* CnMap::PopTail()
{
    return klb_hlist_pop_tail(m_hlist);
}

void* CnMap::Head()
{
    return klb_hlist_head(m_hlist);
}

void* CnMap::Tail()
{
    return klb_hlist_tail(m_hlist);
}

void* CnMap::Update(const void* p_key, int key_len, void* p_data)
{
    return klb_hlist_update(m_hlist, p_key, key_len, p_data);
}

void* CnMap::Update(const std::string& key, void* p_data)
{
    return klb_hlist_update(m_hlist, key.c_str(), key.length(), p_data);
}

void* CnMap::Update(const char* p_key, void* p_data)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Update(key, p_data);
}

void* CnMap::Find(const void* p_key, int key_len)
{
    return klb_hlist_find(m_hlist, p_key, key_len);
}

void* CnMap::Find(const std::string& key)
{
    return klb_hlist_find(m_hlist, key.c_str(), key.length());
}

void* CnMap::Find(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return Find(key);
}

CnMapIter& CnMap::FindIter(const void* p_key, int key_len)
{
    klb_hlist_iter_t* p_iter = klb_hlist_find_iter(m_hlist, p_key, key_len);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::FindIter(const std::string& key)
{
    klb_hlist_iter_t* p_iter = klb_hlist_find_iter(m_hlist, key.c_str(), key.length());

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::FindIter(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return FindIter(key);
}

void* CnMap::Remove(const CnMapIter& iter)
{
    return klb_hlist_remove(m_hlist, iter.m_iter);
}

void* CnMap::RemoveByKey(const void* p_key, int key_len)
{
    return klb_hlist_remove_bykey(m_hlist, p_key, key_len);
}

void* CnMap::RemoveByKey(const std::string& key)
{
    return klb_hlist_remove_bykey(m_hlist, key.c_str(), key.length());
}

void* CnMap::RemoveByKey(const char* p_key)
{
    std::string key = (NULL != p_key) ? p_key : "";
    return RemoveByKey(key);
}

CnMapIter& CnMap::Begin()
{
    klb_hlist_iter_t* p_iter = klb_hlist_begin(m_hlist);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

CnMapIter& CnMap::End()
{
    klb_hlist_iter_t* p_iter = klb_hlist_end(m_hlist);

    m_tmp_iter.SetIter(p_iter);
    return m_tmp_iter;
}

//////////////////////////////////////////////////////////////////////////

int CnMap::Test()
{
    return 0;
}

} // namespace klb
