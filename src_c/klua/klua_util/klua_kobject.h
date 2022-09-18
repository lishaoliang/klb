///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_kobject.h
/// @brief   klua kobject
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_KOBJECT_H__
#define __KLUA_KOBJECT_H__

#include "klb_type.h"
#include "klua/klua.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


void createmeta_kobject_handle(lua_State *L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_KOBJECT_H__
