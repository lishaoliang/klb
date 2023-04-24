///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_shwnd.h
/// @brief   处理共享窗口的扩展
///        缘由: 
///         1. 窗口控件中可以共享使用弹出菜单项目: eg. combo控件 弹出菜单, 理论上可以共享使用 
///         2. 自定义弹出菜单等, 由使用创建好对话框后, 寄存在此共享使用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_SHWND_H__
#define __KLBUIEX_SHWND_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klbuiex_shwnd_t_ klbuiex_shwnd_t;


int klbuiex_register_shwnd(klb_gui_t* p_gui);
klbuiex_shwnd_t* klbuiex_get_shwnd(klb_gui_t* p_gui);


int klbuiex_shwnd_push(klbuiex_shwnd_t* p_shwnd, const char* p_path, klb_wnd_t* p_top_wnd);

klb_wnd_t* klbuiex_shwnd_get(klbuiex_shwnd_t* p_shwnd, const char* p_path);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_SHWND_H__
//end
