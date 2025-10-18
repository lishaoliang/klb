///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_string.h
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_STRING_H__
#define __KLB_STRING_H__

#include "klb_type.h"
#include "klbthird/sds.h"
#include "klbutil/klb_nlist.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 在源字符串 中 查找 指定的字符串; 不需要'\0'结束
/// @return char* 寻找到的字符点
KLB_API char* klb_strnstr_memcmp(const char* p_src, int src_len, const char* p_dst, int dst_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_STRING_H__
//end
