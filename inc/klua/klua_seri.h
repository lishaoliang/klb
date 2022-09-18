///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_seri.h
/// @brief   lua 序列化 (serialize)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_SERI_H__
#define __KLUA_SERI_H__


#include "klb_type.h"
#include "klua/klua.h"
#include "klbmem/klb_buf.h"
#include "klbthird/cJSON.h"
#include "klbutil/klb_map.h"


#if defined(__cplusplus)
extern "C" {
#endif


KLB_API cJSON* klua_seri_json_pack(lua_State* L, int base_idx);
KLB_API int klua_seri_json_unpack(lua_State* L, int base_idx, const cJSON* p_json);


KLB_API klb_map_t* klua_seri_map_pack(lua_State* L, int base_idx);
KLB_API int klua_seri_map_unpack(lua_State* L, int base_idx, const klb_map_t* p_map);


KLB_API klb_buf_t* klua_seri_map_binary_pack(lua_State* L, int base_idx);
KLB_API int klua_seri_map_binary_unpack(lua_State* L, int base_idx, const char* p_data, int data_len);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_SERI_H__
//end
