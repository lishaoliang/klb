///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_map.h
/// @brief   按(K:V)存储, key仅适用字符串
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MAP_H__
#define __KLB_MAP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @struct klb_map_t
/// @brief  map
typedef struct klb_map_t_ klb_map_t;


/// @brief 创建map对象
/// @return klb_map_t*     map对象
KLB_API klb_map_t* klb_map_create();


/// @brief 销毁map对象
/// @param [in] *p_map     map对象
/// @return 无
/// @note 销毁前必须自行清空, 否则内存泄露
KLB_API void klb_map_destroy(klb_map_t* p_map);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MAP_H__
//end
