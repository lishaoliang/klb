///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl.h
/// @brief   windows sdl
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_H__
#define __WSDL_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif

KLB_API int luaopen_wsdl(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __WSDL_H__
//end
