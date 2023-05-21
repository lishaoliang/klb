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


/// @brief 拷贝sds; 若源s为NULL, 则新建立一个
/// @param [in]  s      目标sds
/// @return sds     s
KLB_API sds klb_sdscpy(sds s, const char* p_str);


/// @brief 拷贝sds; 若源s为NULL, 则新建立一个
/// @param [in]  s      目标sds
/// @return sds     s
KLB_API sds klb_sdscpy_adt(sds s, const klb_adt_t* p_adt);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SDS_H__
//end
