///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_seri_map.h
/// @brief   Lua serialize 序列化 map(klb_map_t)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_SERI_MAP_H__
#define __KLUA_SERI_MAP_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


klb_map_t* luaseri_map_pack(lua_State* L, int base_idx);
int luaseri_map_unpack(lua_State* L, int base_idx, const klb_map_t* p_map);


klb_buf_t* luaseri_map_binary_pack(lua_State* L, int base_idx);
int luaseri_map_binary_unpack(lua_State* L, int base_idx, const char* p_data, int data_len);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_SERI_MAP_H__
