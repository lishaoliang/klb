// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/CString.hpp"
#include "klbmem/klb_mem.h"

namespace klb {


//////////////////////////////////////////////////////////////////////////
// 构造 / 析构

CString::CString()
{
    m_sds = sdsempty();
}

CString::CString(const CString& s)
{
    m_sds = sdsnew(s.m_sds);
}

CString::CString(const CString* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdsnew(p_str->m_sds);
    }
    else
    {
        m_sds = sdsempty();
    }
}

CString::CString(const char* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdsnew(p_str);
    }
    else
    {
        m_sds = sdsempty();
    }
}

CString::CString(const sds s)
{
    if (NULL != s)
    {
        m_sds = sdsnew(s);
    }
    else
    {
        m_sds = sdsempty();
    }
}

CString::CString(const std::string& s)
{
    m_sds = sdsnew(s.c_str());
}

CString::CString(const std::string* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdsnew(p_str->c_str());
    }
    else
    {
        m_sds = sdsempty();
    }
}

CString::~CString()
{
    KLB_FREE_BY(m_sds, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// 操作符 重载

CString& CString::operator=(const CString& s)
{
    m_sds = sdscpy(m_sds, s.m_sds);
    return *this;
}

CString& CString::operator=(const CString* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str->m_sds);
    }
    else
    {
        sdsclear(m_sds);
    }
    return *this;
}

CString& CString::operator=(const char* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str);
    }
    else
    {
        sdsclear(m_sds);
    }
    return *this;
}

CString& CString::operator=(const sds s)
{
    if (NULL != s)
    {
        m_sds = sdscpy(m_sds, s);
    }
    else
    {
        sdsclear(m_sds);
    }
    return *this;
}

CString& CString::operator=(const std::string& s)
{
    m_sds = sdscpy(m_sds, s.c_str());
    return *this;
}

CString& CString::operator=(const std::string* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str->c_str());
    }
    else
    {
        sdsclear(m_sds);
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数

/////////////////////////////////////////
// 获取长度

size_t CString::Len()
{
    return sdslen(m_sds);
}

size_t CString::Length()
{
    return sdslen(m_sds);
}

/////////////////////////////////////////
// 按长度 追加

CString& CString::CatCopylen(const void* s, size_t len)
{
    if (NULL != s)
    {
        m_sds = sdscatlen(m_sds, s, len);
    }

    return *this;
}

/////////////////////////////////////////
// 按最大长度 追加

CString& CString::Catlen(const CString& s, size_t max_len)
{
    size_t s_len = sdslen(s.m_sds);
    size_t cp_len = MIN(s_len, max_len);

    if (0 < cp_len)
    {
        m_sds = sdscatlen(m_sds, s.m_sds, cp_len);
    }

    return *this;
}

CString& CString::Catlen(const CString* p_str, size_t max_len)
{
    size_t s_len = (NULL != p_str) ? sdslen(p_str->m_sds) : 0;
    size_t cp_len = MIN(s_len, max_len);

    if (0 < cp_len)
    {
        m_sds = sdscatlen(m_sds, p_str->m_sds, cp_len);
    }

    return *this;
}

CString& CString::Catlen(const char* p_str, size_t max_len)
{
    if (NULL != p_str)
    {
        m_sds = sdscatlen(m_sds, p_str, max_len);
    }
    return *this;
}

CString& CString::Catlen(const sds s, size_t max_len)
{
    size_t s_len = (NULL != s) ? sdslen(s) : 0;
    size_t cp_len = MIN(s_len, max_len);

    if (0 < cp_len)
    {
        m_sds = sdscatlen(m_sds, s, cp_len);
    }

    return *this;
}

CString& CString::Catlen(const std::string& s, size_t max_len)
{
    size_t s_len = s.length();
    size_t cp_len = MIN(s_len, max_len);

    if (0 < cp_len)
    {
        m_sds = sdscatlen(m_sds, s.c_str(), cp_len);
    }

    return *this;
}

CString& CString::Catlen(const std::string* p_str, size_t max_len)
{
    size_t s_len = (NULL != p_str) ? p_str->length() : 0;
    size_t cp_len = MIN(s_len, max_len);

    if (0 < cp_len)
    {
        m_sds = sdscatlen(m_sds, p_str->c_str(), cp_len);
    }

    return *this;
}

/////////////////////////////////////////
// 追加

CString& CString::Cat(const CString& s)
{
    m_sds = sdscat(m_sds, s.m_sds);
    return *this;
}

CString& CString::Cat(const CString* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscat(m_sds, p_str->m_sds);
    }
    return *this;
}

CString& CString::Cat(const char* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscat(m_sds, p_str);
    }

    return *this;
}

CString& CString::Cat(const sds s)
{
    if (NULL != s)
    {
        m_sds = sdscatsds(m_sds, s);
    }
    return *this;
}

CString& CString::Cat(const std::string& s)
{
    m_sds = sdscat(m_sds, s.c_str());
    return *this;
}

CString& CString::Cat(const std::string* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscat(m_sds, p_str->c_str());
    }
    return *this;
}

/////////////////////////////////////////
// 拷贝 "copy" => "cp"

CString& CString::Copy(const CString& s)
{
    m_sds = sdscpy(m_sds, s.m_sds);
    return *this;
}

CString& CString::Copy(const CString* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str->m_sds);
    }
    else
    {
        sdsclear(m_sds); // 清空
    }

    return *this;
}

CString& CString::Copy(const char* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str);
    }
    else
    {
        sdsclear(m_sds); // 清空
    }

    return *this;
}

CString& CString::Copy(const sds s)
{
    if (NULL != s)
    {
        m_sds = sdscpy(m_sds, s);
    }
    else
    {
        sdsclear(m_sds); // 清空
    }

    return *this;
}

CString& CString::Copy(const std::string& s)
{
    m_sds = sdscpy(m_sds, s.c_str());
    return *this;
}

CString& CString::Copy(const std::string* p_str)
{
    if (NULL != p_str)
    {
        m_sds = sdscpy(m_sds, p_str->c_str());
    }
    else
    {
        sdsclear(m_sds); // 清空
    }

    return *this;
}

/////////////////////////////////////////

CString& CString::Clear()
{
    sdsclear(m_sds);
    return *this;
}

/////////////////////////////////////////

const sds CString::c_sds()
{
    return m_sds;
}

const char* CString::c_str()
{
    return m_sds;
}

/////////////////////////////////////////
// 自测试

int CString::Test()
{
    std::string std_s1 = "abc";
    CString s1, s2(std_s1), s3("123");

    s1 = "123456";


    return 0;
}

//////////////////////////////////////////////////////////////////////////
// end
}
