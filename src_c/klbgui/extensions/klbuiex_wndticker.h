///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_wndticker.h
/// @brief   控件定时器 扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_WNDTICKER_H__
#define __KLBUIEX_WNDTICKER_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbuiex_wndticker_t_ klbuiex_wndticker_t;


/// @brief 注册/获取
int klbuiex_register_wndticker(klb_gui_t* p_gui);
klbuiex_wndticker_t* klbuiex_get_wndticker(klb_gui_t* p_gui);


/// @brief 设置/获取 计时 时间间隔, 毫秒ms
void klbuiex_wndticker_set_interval(klbuiex_wndticker_t* p_wndticker, int64_t interval);
int64_t klbuiex_wndticker_get_interval(klbuiex_wndticker_t* p_wndticker);


/// @brief 设置窗口计数
void klbuiex_wndticker_set_modal_num(klbuiex_wndticker_t* p_wndticker, int num);
void klbuiex_wndticker_set_popup_num(klbuiex_wndticker_t* p_wndticker, int num);
void klbuiex_wndticker_set_msgbox_num(klbuiex_wndticker_t* p_wndticker, int num);

/// @brief modal 窗口流程处理
int klbuiex_wndticker_modal(klbuiex_wndticker_t* p_wndticker, int idx, klb_wnd_t* p_top_wnd);
int klbuiex_wndticker_modal_end(klbuiex_wndticker_t* p_wndticker, int idx);

/// @brief popup 窗口流程处理
int klbuiex_wndticker_popup(klbuiex_wndticker_t* p_wndticker, int idx, klb_wnd_t* p_top_wnd);
int klbuiex_wndticker_popup_end(klbuiex_wndticker_t* p_wndticker, int idx);

/// @brief msgbox 窗口流程处理
int klbuiex_wndticker_msgbox(klbuiex_wndticker_t* p_wndticker, klb_wnd_t* p_top_wnd);
int klbuiex_wndticker_msgbox_end(klbuiex_wndticker_t* p_wndticker);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_WNDTICKER_H__
//end
