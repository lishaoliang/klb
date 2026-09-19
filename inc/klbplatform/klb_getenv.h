// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_getenv.h
/// @brief   环境变量
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GETENV_H__
#define __KLB_GETENV_H__


#include "klb_type.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 获取系统环境字符串
/// @param [in]  *p_name        环境字符串; eg. "PATH"
/// @return sds 字符串
KLB_API sds klb_getenv(const char* p_name);


/// @brief 设置系统环境字符串
KLB_API int klb_setenv(const char* p_name, const char* p_value, bool overwrite);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_GETENV_H__

// end
