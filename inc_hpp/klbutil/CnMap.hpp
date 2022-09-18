///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CnMap.hpp
/// @brief   map : 存放的指针,调用者需自行释放
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CNMAP_HPP__
#define __KLB_CNMAP_HPP__

#include "klb_type.h"
#include "klbutil/klb_hlist.h"
#include <string>

namespace klb {


class CnMap;

/// @class CnMapIter
/// @brief map iter
KLB_EXTERN class KLB_API_CPP CnMapIter
{
    friend class CnMap;
public:
    CnMapIter();
    CnMapIter(klb_hlist_iter_t* p_iter);
    CnMapIter(const CnMapIter& t);
    ~CnMapIter();

    CnMapIter& operator=(const CnMapIter& t);
    CnMapIter& operator++();
    CnMapIter& operator--();
    bool operator==(const void* ptr);
    void* operator*();

    CnMapIter& Next();
    CnMapIter& Prev();

    bool IsNull();
    void* Data();

    void* Key(int* p_out_key_len = NULL);

protected:
    void SetIter(klb_hlist_iter_t* p_iter);

private:
    void CopyTo(const CnMapIter& t);

protected:
    klb_hlist_iter_t*   m_iter;
};


/// @class CnMap
/// @brief map : 存放的指针,调用者需自行释放
KLB_EXTERN class KLB_API_CPP CnMap
{
protected:
    CnMap& operator=(const CnMap& t); // 禁止拷贝

public:
    CnMap();
    ~CnMap();
 
    CnMapIter& operator[](const std::string& key);
    CnMapIter& operator[](const char* p_key);

    klb_hlist_t* GetHlist();

    void Clean(klb_hlist_clean_cb cb_clean, void* p_obj);
    int Size(); 

    CnMapIter& PushHead(const void* p_key, int key_len, void* p_data);
    CnMapIter& PushHead(const std::string& key, void* p_data);
    CnMapIter& PushHead(const char* p_key, void* p_data);

    CnMapIter& PushTail(const void* p_key, int key_len, void* p_data);
    CnMapIter& PushTail(const std::string& key, void* p_data);
    CnMapIter& PushTail(const char* p_key, void* p_data);

    void* PopHead();
    void* PopTail();

    void* Head();
    void* Tail();

    void* Update(const void* p_key, int key_len, void* p_data);
    void* Update(const std::string& key, void* p_data);
    void* Update(const char* p_key, void* p_data);

    void* Find(const void* p_key, int key_len);
    void* Find(const std::string& key);
    void* Find(const char* p_key);

    CnMapIter& FindIter(const void* p_key, int key_len);
    CnMapIter& FindIter(const std::string& key);
    CnMapIter& FindIter(const char* p_key);

    void* Remove(const CnMapIter& iter);

    void* RemoveByKey(const void* p_key, int key_len);
    void* RemoveByKey(const std::string& key);
    void* RemoveByKey(const char* p_key);

    CnMapIter& Begin();
    CnMapIter& End();

private:
    klb_hlist_t*        m_hlist;
    CnMapIter           m_tmp_iter;

public:
    static int Test();
};

} // namespace klb

#endif // __KLB_CNMAP_HPP__
