///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_gui.h
/// @brief   GUI扩展
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_GUI_H__
#define __KLUA_EX_GUI_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "gui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLUA_EX_GUI_NAME       "_KLUA_EX_GUI_"

typedef struct klua_ex_gui_t_ klua_ex_gui_t;

int klua_ex_register_gui(klua_env_t* p_env);

klua_ex_gui_t* klua_ex_get_gui(klua_env_t* p_env);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_GUI_H__
//end
