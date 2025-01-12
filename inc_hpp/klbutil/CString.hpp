///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CString.hpp
/// @brief   string
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CSTRING_H__
#define __KLB_CSTRING_H__

#include "klb_type.h"
#include "klbthird/sds.h"
#include <string>


namespace klb {


/// @class CString
/// @brief string
KLB_EXTERN class KLB_API_CPP CString
{
public:
    /////////////////////////////////////////////////
    // 构造 / 析构
    CString();
    CString(const CString& s);
    CString(const CString* p_str);
    CString(const char* p_str);
    CString(const sds s);
    CString(const std::string& s);
    CString(const std::string* p_str);
    ~CString();


    /////////////////////////////////////////////////
    // 操作符 重载
    CString& operator=(const CString& s); 
    CString& operator=(const CString* p_str);
    CString& operator=(const char* p_str);
    CString& operator=(const sds s);
    CString& operator=(const std::string& s);
    CString& operator=(const std::string* p_str);


    /////////////////////////////////////////////////
    // 获取长度
    size_t Len();
    size_t Length();


    /////////////////////////////////////////////////
    // 按长度 拷贝追加
    CString& CatCopylen(const void* s, size_t len);


    /////////////////////////////////////////////////
    /// @brief 按最大长度 追加字符串
    /// @return *this
    CString& Catlen(const CString& s, size_t max_len);
    CString& Catlen(const CString* p_str, size_t max_len);
    CString& Catlen(const char* p_str, size_t max_len);
    CString& Catlen(const sds s, size_t max_len);
    CString& Catlen(const std::string& s, size_t max_len);
    CString& Catlen(const std::string* p_str, size_t max_len);


    /////////////////////////////////////////////////
    // 追加
    CString& Cat(const CString& s);
    CString& Cat(const CString* p_str);
    CString& Cat(const char* p_str);
    CString& Cat(const sds s);
    CString& Cat(const std::string& s);
    CString& Cat(const std::string* p_str);


    /////////////////////////////////////////////////
    // 拷贝 "copy" => "cp"
    CString& Copy(const CString& s);
    CString& Copy(const CString* p_str);
    CString& Copy(const char* p_str);
    CString& Copy(const sds s);
    CString& Copy(const std::string& s);
    CString& Copy(const std::string* p_str);


    /////////////////////////////////////////////////
    /// @brief 清空字符串, 结果为""
    /// @return *this
    CString& Clear();


    /////////////////////////////////////////////////
    // 获取字符串

    const sds   c_sds();
    const char* c_str();


protected:
    sds             m_sds;              ///< sds 字符串


public:
    // 测试
    static int Test();

};


}


#endif // __KLB_CSTRING_H__
//end
