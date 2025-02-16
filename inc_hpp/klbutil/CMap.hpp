///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CMap.hpp
/// @brief   klb_map_t的cpp封装与扩展
///   \n 1. 含有 key/value 和 array 两种类型方式
/// @version 0.1
/// @history 修改历史
/// @warning 警告
///   \n 1. 需防止cpp隐式转换匹配到的多态函数, 并非是调用者期望的函数, 这种隐晦Bug
///   \n 2. 托管内存为将指针对应的内存交给CMap管理
///   \n 3. CMap内部的数据节点可能发生变更, 采用临时使用方式
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CMAP_HPP__
#define __KLB_CMAP_HPP__

#include "klb_type.h"
#include "klbthird/sds.h"
#include "klbutil/klb_adt.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_map.h"
#include "klbutil/CString.hpp"
#include <string>

namespace klb {

class CMap;

KLB_EXTERN class KLB_API_CPP CMapItem
{
    friend class CMap;
    friend class CMapIter;
public:
    CMapItem();
    CMapItem(const CMapItem& t);
    ~CMapItem();

    CMapItem& operator=(const CMapItem& t);
    CMapItem& operator=(const bool b);
    CMapItem& operator=(const int64_t i64);
    CMapItem& operator=(const uint64_t u64);
    CMapItem& operator=(const double d);
    CMapItem& operator=(const std::string& str);
    CMapItem& operator=(const char* p_str);
    CMapItem& operator=(const CString& str);
    CMapItem& operator=(const CMap& m);

    bool operator==(const bool b);
    bool operator==(const int64_t i64);
    bool operator==(const uint64_t u64);
    bool operator==(const std::string& str);
    bool operator==(const char* p_str);
    bool operator==(const CString& str);

    CMapItem& Set(const bool b);
    CMapItem& Set(const int64_t i64);
    CMapItem& Set(const uint64_t u64);
    CMapItem& Set(const double d);
    CMapItem& Set(const std::string& str);
    CMapItem& Set(const char* p_str);
    CMapItem& Set(const char* p_str, int str_len);
    CMapItem& Set(const CString& str);
    CMapItem& Set(const void* ptr1, const void* ptr2);
    CMapItem& Set(const CMap& m);

    klb_adt_type_e Type();

    CMapItem& Get(bool& b);
    CMapItem& Get(int64_t& i64);
    CMapItem& Get(uint64_t& u64);
    CMapItem& Get(double& d);
    CMapItem& Get(std::string& str);
    CMapItem& Get(const char** p_out_str, int* p_out_len);
    CMapItem& Get(CString& str);
    CMapItem& Get(const void** p_out_ptr1, const void** p_out_ptr2);
    CMapItem& Get(CMap& m);

    bool        ToBool();
    int64_t     ToInt64();
    uint64_t    ToUint64();
    double      ToDouble();
    std::string ToString();
    const char* ToString(int* p_out_len);
    const void* ToPtr(const void** p_out_ptr2);

    std::string Print();

protected:
    void SetMapAdt(CMap* p_map, const std::string& key, klb_adt_t* p_adt);
    void SetMapAdt(CMap* p_map, const CString& key, klb_adt_t* p_adt);
    void SetMapAdt(CMap* p_map, int idx, klb_adt_t* p_adt);
    void SetMapAdt(klb_adt_t* p_adt);

private:
    void InitAdt();
    void QuitAdt();
    void CopyTo(const CMapItem& t);

private:
    CMap*           m_ref_map;  ///< CMap
    klb_adt_t*      m_ref_adt;  ///< CMap 节点数据的引用

    int             m_type;
#define KLB_CMAP_ITEM_keyvalue  1
#define KLB_CMAP_ITEM_array     2

    int             m_idx;
    sds             m_key;
};

KLB_EXTERN class KLB_API_CPP CMapIter
{
    friend class CMap;
public:
    CMapIter();
    CMapIter(const CMapIter& t);
    CMapIter(CMap* p_map, klb_map_iter_t* p_iter);
    ~CMapIter();

    CMapIter& operator=(const CMapIter& t);
    CMapIter& operator++(int i);
    CMapIter& operator--(int d);

    bool IsNull();

    CMapIter& Next();
    CMapIter& Prev();

    CMapItem Value();
    std::string Key();

protected:
    CMap*           m_map;      ///< CMap
    klb_map_iter_t* m_iter;     ///< klb_map_t / klb_map_iter_t
};

KLB_EXTERN class KLB_API_CPP CMap
{
    friend class CMapItem;
    friend class CMapIter;
public:
    CMap();
    CMap(klb_map_t* p_map, bool share); // 共享/控制
    CMap(klb_map_t* p_map); // 托管内存
    CMap(const CMap& t);
    ~CMap();

    /////////////////////////////////////
    // 
    const klb_map_t* GetMap();

    void Attach(klb_map_t* p_map); // 托管内存
    void Clear();
    int  Size();

    /////////////////////////////////////
    // 操作符

    CMap&     operator=(const CMap& t);
    CMap&     operator=(const klb_map_t* p_map);

    CMapItem& operator[](const std::string& key);
    CMapItem& operator[](const char* p_key);
    CMapItem& operator[](const CString& key);
    CMapItem& operator[](int idx);

    /////////////////////////////////////
    // Set 设置(key/value)

    CMap& SetNull(const std::string& key);
    CMap& Set(const std::string& key, const bool b);
    CMap& Set(const std::string& key, const int64_t i64);
    CMap& Set(const std::string& key, const uint64_t u64);
    CMap& Set(const std::string& key, const double d);
    CMap& Set(const std::string& key, const std::string& str);
    CMap& Set(const std::string& key, const char* p_str);
    CMap& Set(const std::string& key, const char* p_str, int str_len);
    CMap& Set(const std::string& key, const CString& str);
    CMap& Set(const std::string& key, const void* ptr1, const void* ptr2);
    CMap& Set(const std::string& key, const CMap& m);

    CMap& SetNull(const char* p_key);
    CMap& Set(const char* p_key, bool b);
    CMap& Set(const char* p_key, int64_t i64);
    CMap& Set(const char* p_key, uint64_t u64);
    CMap& Set(const char* p_key, double d);
    CMap& Set(const char* p_key, const std::string& str);
    CMap& Set(const char* p_key, const char* p_str);
    CMap& Set(const char* p_key, const char* p_str, int str_len);
    CMap& Set(const char* p_key, const CString& str);
    CMap& Set(const char* p_key, const void* ptr1, const void* ptr2);
    CMap& Set(const char* p_key, const CMap& m);

    /////////////////////////////////////
    // Get 获取(key/value)

    CMap& Get(const std::string& key, bool& b);
    CMap& Get(const std::string& key, int64_t& i64);
    CMap& Get(const std::string& key, uint64_t& u64);
    CMap& Get(const std::string& key, double& d);
    CMap& Get(const std::string& key, std::string& str);
    CMap& Get(const std::string& key, const char** p_out_str, int* p_out_len);
    CMap& Get(const std::string& key, CString& str);
    CMap& Get(const std::string& key, const void** p_out_ptr1, const void** p_out_ptr2);
    CMap& Get(const std::string& key, CMap& m);

    CMap& Get(const char* p_key, bool& b);
    CMap& Get(const char* p_key, int64_t& i64);
    CMap& Get(const char* p_key, uint64_t& u64);
    CMap& Get(const char* p_key, double& d);
    CMap& Get(const char* p_key, std::string& str);
    CMap& Get(const char* p_key, const char** p_out_str, int* p_out_len);
    CMap& Get(const char* p_key, CString& str);
    CMap& Get(const char* p_key, const void** p_out_ptr1, const void** p_out_ptr2);
    CMap& Get(const char* p_key, CMap& m);
 
    klb_adt_type_e Type(const std::string& key);
    klb_adt_type_e Type(const char* p_key);
    klb_adt_type_e Type(const CString& key);

    int  KeyValueSize();
    bool Remove(const std::string& key);
    bool Remove(const char* p_key);
    bool Remove(const CString& key);
    bool Remove(const CMapIter& iter);

    CMapIter Begin();
    CMapIter End();

    /////////////////////////////////////
    // 设置 (array)

    CMap& AppendNull();
    CMap& Append(const bool b);
    CMap& Append(const int64_t i64);
    CMap& Append(const uint64_t u64);
    CMap& Append(const double d);
    CMap& Append(const std::string& str);
    CMap& Append(const char* p_str);
    CMap& Append(const char* p_str, int str_len);
    CMap& Append(const CString& str);
    CMap& Append(const void* ptr1, const void* ptr2);
    CMap& Append(const CMap& m);

    CMap& SetNull(const int idx);
    CMap& Set(const int idx, const bool b);
    CMap& Set(const int idx, const int64_t i64);
    CMap& Set(const int idx, const uint64_t u64);
    CMap& Set(const int idx, const double d);
    CMap& Set(const int idx, const std::string& str);
    CMap& Set(const int idx, const char* p_str);
    CMap& Set(const int idx, const char* p_str, int str_len);
    CMap& Set(const int idx, const CString& str);
    CMap& Set(const int idx, const void* ptr1, const void* ptr2);
    CMap& Set(const int idx, const CMap& m);  

    /////////////////////////////////////
    // 获取 (array)

    CMap& Get(const int idx, bool& b);
    CMap& Get(const int idx, int64_t& i64);
    CMap& Get(const int idx, uint64_t& u64);
    CMap& Get(const int idx, double& d);
    CMap& Get(const int idx, std::string& str);
    CMap& Get(const int idx, const char** p_out_str, int* p_out_len);
    CMap& Get(const int idx, CString& str);
    CMap& Get(const int idx, const void** p_out_ptr1, const void** p_out_ptr2);
    CMap& Get(const int idx, CMap& m);

    klb_adt_type_e Type(const int idx);

    int  ArraySize();
    bool Remove(int idx);
    bool RemoveTail();

    typedef int(*cmap_sort_cb)(CMapItem& data1, CMapItem& data2);

    int Sort(cmap_sort_cb cb_sort);
    int Sort(klb_map_array_sort_cb cb_sort, void* ptr1);

    /////////////////////////////////////
    // serialize 二进制
    klb_buf_t* Pack();
    void Unpack(const char* p_data, int data_len);

    /////////////////////////////////////
    // json
    std::string PrintJson();

private:
    static int CbSortMapArray(const klb_adt_t* p_adt1, const klb_adt_t* p_adt2, void* ptr1);

protected:
    klb_adt_t* GetNoneAdt();

private:
    void InitKlbMap();
    void QuitKlbMap();

protected:
    // 共享C指针方式, 容许 将 m_map 定位于外部 内存 做 C++ 操作
    // 但不可变更原始map指针
    bool        m_share;        ///< 是否共享控制
    klb_map_t   m_map;          ///< map

    klb_adt_t   m_none_adt;     ///< adt
    CMapItem    m_none_item;    ///< none

public:
    static int Test();
};

}; // namespace klb

#endif // __KLB_CMAP_HPP__
