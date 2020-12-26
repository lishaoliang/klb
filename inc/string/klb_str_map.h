///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_str_map.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_STR_MAP_H__
#define __KLB_STR_MAP_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_str_map_t_ klb_str_map_t;


klb_str_map_t* klb_str_map_create();
void klb_str_map_destroy(klb_str_map_t* p_map);

uint32_t klb_str_map_size(klb_str_map_t* p_map);

void klb_str_map_set(klb_str_map_t* p_map, const char* p_key, const char* p_value);
const char* klb_str_map_get(klb_str_map_t* p_map, const char* p_key);

bool klb_str_map_remove(klb_str_map_t* p_map, const char* p_key);
void klb_str_map_remove_all(klb_str_map_t* p_map);




#ifdef __cplusplus
}
#endif

#endif // __KLB_STR_MAP_H__
//end
