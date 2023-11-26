///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_sds.h
/// @brief   sds扩展函数
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SDS_H__
#define __KLB_SDS_H__

#include "klb_type.h"
#include "klbthird/sds.h"
#include "klbutil/klb_adt.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 拷贝sds; 目标s为NULL, 且来源(p_str)不为空, 则新建立一个
/// @param [in]  s      目标sds
/// @return sds  s  返回NULL 或 非NULL的 sds, 可能与原s地址不一致
KLB_API sds klb_sdscpy(sds s, const char* p_str);


/// @brief 拷贝sds; 目标s为NULL, 且来源(p_adt)不为空, 则新建立一个
/// @param [in]  s      目标sds
/// @return sds  s  返回NULL 或 非NULL的 sds, 可能与原s地址不一致
KLB_API sds klb_sdscpy_adt(sds s, const klb_adt_t* p_adt);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SDS_H__
//end
