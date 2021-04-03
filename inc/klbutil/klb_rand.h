///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/04/03
//
/// @file    klb_rand.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/04/03 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_RAND_H__
#define __KLB_RAND_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 伪随机字符串
/// @param [in,out] *p_dest
/// @param [in]     str_len
/// @return char* 返回p_dest
/// @note  set_end = true: 缓存长度需要 str_len + 1
KLB_API char* klb_rand_string(char* p_dest, int str_len, bool set_end);


/// @brief 伪随机首位不为0的正整数(10进制)
/// @param [in,out] *p_dest
/// @param [in]     str_len
/// @return char* 返回p_dest
/// @note set_end = true: 缓存长度需要 str_len + 1
KLB_API char* klb_rand_integer(char* p_dest, int str_len, bool set_end);


#ifdef __cplusplus
}
#endif

#endif // __KLB_RAND_H__
//end
