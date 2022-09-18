///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_seri_map.h
/// @brief   Lua serialize 序列化 map(klb_map_t)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_SERI_JSON_H__
#define __KLUA_SERI_JSON_H__


#include "klb_type.h"
#include "klua/klua.h"
#include "klbthird/cJSON.h"


#if defined(__cplusplus)
extern "C" {
#endif

cJSON* luaseri_json_pack(lua_State *L, int base_idx);
int luaseri_json_unpack(lua_State *L, int base_idx, const cJSON* p_json);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_SERI_JSON_H__
