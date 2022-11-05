///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_gui.h
/// @brief   内置简易gui
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_GUI_H__
#define __KLUA_GUI_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API klb_gui_t* klua_gui_get(klua_env_t* p_env);
KLB_API klb_gui_t* klua_gui_get_by_L(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_GUI_H__
//end
