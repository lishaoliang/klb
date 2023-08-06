///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_gui.h
/// @brief   GUI扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_GUI_H__
#define __KLUA_EX_GUI_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLUA_EX_GUI_NAME       "_KLUA_EX_GUI_"

typedef struct klua_ex_gui_t_ klua_ex_gui_t;

int klua_ex_register_gui(klua_env_t* p_env);

klua_ex_gui_t* klua_ex_get_gui(klua_env_t* p_env);
klua_ex_gui_t* klua_ex_get_gui_by_L(lua_State* L);


/// @brief 从扩展中获取gui指针
klb_gui_t* klua_ex_gui_get(klua_ex_gui_t* p_ex);


/// @brief 绑定lua脚本函数作为command响应
int klua_ex_gui_bind_command(klua_ex_gui_t* p_ex, const char* p_path_name, int idx);


/// @brief 清空gui
int klua_ex_gui_clear(klua_ex_gui_t* p_ex);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_GUI_H__
//end
