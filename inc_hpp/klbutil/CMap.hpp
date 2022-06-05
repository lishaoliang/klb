///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CMap.hpp
/// @brief   klb_hlist_t的CPP封装与扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __CMAP_HPP__
#define __CMAP_HPP__

#include "klb_type.h"
#include "klbutil/klb_vector.h"
#include "klbutil/klb_hlist.h"
#include <string>

namespace klb {

    class CMapObj;
    class CArray;
    class CMap;

    KLB_EXTERN class KLB_API_CPP CMapIter
    {
    public:
        CMapIter();
        CMapIter(klb_hlist_iter_t* p_iter);
        CMapIter(const CMapIter& t);
        ~CMapIter();

        CMapIter& operator=(const CMapIter& t);

        CMapIter Next();
        bool IsEnd();
        CMapObj& Data();

    private:
        void CopyTo(const CMapIter& t);

        klb_hlist_iter_t*    m_iter;
    };

    typedef enum CMapObjType_e_
    {
        CMAP_None,          // none
        CMAP_CArray,        // CArray
        CMAP_CMap,          // CMap
        CMAP_Func,          // function
        CMAP_Ptr,           // 指针
        CMAP_Buf,           // 数据缓存
        CMAP_String,        // 字符串 std::string, char*
        CMAP_Bool,          // bool
        CMAP_Double,        // double
        CMAP_Int64,         // int64_t
    }CMapObjType;

    KLB_EXTERN class KLB_API_CPP CMapObj
    {
        friend class CArray;
        friend class CMap;
    public:
        CMapObj();
        CMapObj(const CMapObj& t);
        CMapObj(const std::string& key);
        CMapObj(const std::string& key, const CArray& v);
        CMapObj(const std::string& key, const CMap& v);
        CMapObj(const std::string& key, const void* func, const void* o);
        CMapObj(const std::string& key, const void* v);
        CMapObj(const std::string& key, const char* v);
        CMapObj(const std::string& key, const char* data, int size);
        CMapObj(const std::string& key, const std::string& v);
        CMapObj(const std::string& key, bool v);
        CMapObj(const std::string& key, double v);
        CMapObj(const std::string& key, int64_t v);
        ~CMapObj();

        // =
        CMapObj& operator=(const CArray& v);
        CMapObj& operator=(const CMap& v);
        CMapObj& operator=(const void* v);
        CMapObj& operator=(const char* v);
        CMapObj& operator=(const std::string& v);
        CMapObj& operator=(const bool& v);
        CMapObj& operator=(const double& v);
        CMapObj& operator=(const int64_t& v);

        // ==
        bool operator==(const void* v);
        bool operator==(const char* v);
        bool operator==(const std::string& v);
        bool operator==(const bool& v);
        bool operator==(const int64_t& v);

        // 获取值
        bool Get(CArray& v);
        bool Get(CMap& v);
        bool Get(void** func, void** o);
        bool Get(void** v);
        bool Get(char** data, int* size);
        bool Get(std::string& v);
        bool Get(bool& v);
        bool Get(double& v);
        bool Get(int64_t& v);

        // 类型
        CMapObjType Type();
        bool IsType(CMapObjType t);

        CArray GetArray();
        CMap GetMap();
        void* GetFunc(void** o, const void* p_default = NULL);
        void* GetPtr(const void* p_default = NULL);
        char* GetBuf(int* size, const void* p_default = NULL);
        std::string GetString(const std::string v_default = "");
        bool GetBool(const bool v_default = false);
        double GetDouble(const double v_default = 0.0);
        int64_t GetInt64(const int64_t v_default = 0);

        // 设置值
        CMapObj& Set(const CArray& v);
        CMapObj& Set(const CMap& v);
        CMapObj& Set(const void* func, const void* o);
        CMapObj& Set(const void* v);
        CMapObj& Set(const char* v);
        CMapObj& Set(const char* data, int size);
        CMapObj& Set(const std::string& v);
        CMapObj& Set(bool v);
        CMapObj& Set(double v);
        CMapObj& Set(int64_t v);

        // 获取Key
        std::string GetKey();

        // dump
        std::string Dump();

    protected:
        void Init();
        void Quit();
        void Reset();
        void SetArray(CArray* p_array, int idx);
        void SetMap(CMap* p_map);
        void SetKey(const std::string& key);

        void ClearValue();

    protected:
        void CopyTo(const CMapObj& t);

        CArray*         m_array;
        int             m_array_idx;

        CMap*           m_map;
        std::string     m_key;

        CMapObjType     m_type;
        void*           m_value;
        union
        {
            int64_t     m_value_int64;
            bool        m_value_bool;
            double      m_value_double;
            void*       m_obj;
        };
    };

    KLB_EXTERN class KLB_API_CPP CArray
    {
        friend class CMap;
    public:
        CArray();
        CArray(const CArray& t);
        ~CArray();

        CArray& operator=(const CArray& t);

        // 大小
        int Size();

        // 获取obj
        CMapObj& Get(int idx);
        CMapObj& operator[](int idx);

        // 获取值
        bool Get(int idx, CArray& v);
        bool Get(int idx, CMap& v);
        bool Get(int idx, void** func, void** o);
        bool Get(int idx, void** v);
        bool Get(int idx, char** data, int* size);
        bool Get(int idx, std::string& v);
        bool Get(int idx, bool& v);
        bool Get(int idx, double& v);
        bool Get(int idx, int64_t& v);

        // 按类型获取值
        CMapObjType Type(int idx);
        bool IsType(int idx, CMapObjType t);

        CArray GetArray(int idx);
        CMap GetMap(int idx);
        void* GetFunc(int idx, void** o, const void* p_default = NULL);
        void* GetPtr(int idx, const void* p_default = NULL);
        char* GetBuf(int idx, int* size, const char* p_default = NULL);
        std::string GetString(int idx, const std::string v_default = "");
        bool GetBool(int idx, const bool v_default = false);
        double GetDouble(int idx, const double v_default = 0.0);
        int64_t GetInt64(int idx, const int64_t v_default = 0);

        // 设置值
        CArray& Set(int idx, const CArray& v);
        CArray& Set(int idx, const CMap& v);
        CArray& Set(int idx, const void* func, const void* o);
        CArray& Set(int idx, const void* v);
        CArray& Set(int idx, const char* v);
        CArray& Set(int idx, const char* data, int size);
        CArray& Set(int idx, const std::string& v);
        CArray& Set(int idx, bool v);
        CArray& Set(int idx, double v);
        CArray& Set(int idx, int64_t v);

        // 追加值: 在末尾追加一个值
        CArray& Append(const CArray& v);
        CArray& Append(const CMap& v);
        CArray& Append(const void* func, const void* o);
        CArray& Append(const void* v);
        CArray& Append(const char* v);
        CArray& Append(const char* data, int size);
        CArray& Append(const std::string& v);
        CArray& Append(bool v);
        CArray& Append(double v);
        CArray& Append(int64_t v);

        // dump
        std::string Dump();

    protected:
        CArray& SetValue(int idx, CMapObj* p_v);
        CArray& AppendValue(CMapObj* p_v);

    private:
        void Init();
        void Quit();

        CMapObj* Find(int idx);
        void CopyTo(const CArray& t); // 拷贝

        klb_vector_t*   m_vector;
        CMapObj         m_none;
    };

    KLB_EXTERN class KLB_API_CPP CMap
    {
    public:
        CMap();
        CMap(bool is_array);
        CMap(const CArray& t);
        CMap(const CMap& t);
        ~CMap();

        CMap& operator=(const CArray& t);
        CMap& operator=(const CMap& t);

        // 是否为数组
        bool IsArray();
        CArray& Array();

        // 大小
        int Size();

        // 删除
        CMap& Delete(const std::string& key);

        // 获取obj
        CMapObj& Get(const std::string& key);
        CMapObj& operator[](const std::string& key);

        // 获取值
        bool Get(const std::string& key, CArray& v);
        bool Get(const std::string& key, CMap& v);
        bool Get(const std::string& key, void** func, void** o);
        bool Get(const std::string& key, void** v);
        bool Get(const std::string& key, char** data, int* size);
        bool Get(const std::string& key, std::string& v);
        bool Get(const std::string& key, bool& v);
        bool Get(const std::string& key, double& v);
        bool Get(const std::string& key, int64_t& v);

        // 按类型获取值
        CMapObjType Type(const std::string& key);
        bool IsType(const std::string& key, CMapObjType t);

        CArray GetArray(const std::string& key);
        CMap GetMap(const std::string& key);
        void* GetFunc(const std::string& key, void** obj, const void* p_default = NULL);
        void* GetPtr(const std::string& key, const void* p_default = NULL);
        char* GetBuf(const std::string& key, int* size, const char* p_default = NULL);
        std::string GetString(const std::string& key, const std::string v_default = "");
        bool GetBool(const std::string& key, const bool v_default = false);
        double GetDouble(const std::string& key, const double v_default = 0.0);
        int64_t GetInt64(const std::string& key, const int64_t v_default = 0);

        // 设置值
        CMap& Set(const std::string& key, const CArray& v);
        CMap& Set(const std::string& key, const CMap& v);
        CMap& Set(const std::string& key, const void* func, const void* o);
        CMap& Set(const std::string& key, const void* v);
        CMap& Set(const std::string& key, const char* v);
        CMap& Set(const std::string& key, const char* data, int size);
        CMap& Set(const std::string& key, const std::string& v);
        CMap& Set(const std::string& key, bool v);
        CMap& Set(const std::string& key, double v);
        CMap& Set(const std::string& key, int64_t v);

        // 遍历
        CMapIter Begin();

        // dump
        std::string Dump();

    protected:
        CMap& SetValue(const std::string& key, CMapObj* p_v);

    private:
        void Init();
        void Quit();

        CMapObj* Find(const std::string& key);

        void CopyTo(const CMap& t);     // 拷贝
        void CopyTo(const CArray& t);   // 拷贝

        bool            m_is_array;
        CArray          m_array;

        klb_hlist_t*    m_hlist;
        CMapObj         m_none;

    public:
        static int Test();
    };
};

#endif
