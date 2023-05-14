///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_calendar.h
/// @brief   klb share window calendar, 共享日历顶层对话框(菜单)
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_CALENDAR_H__
#define __KLBSHW_CALENDAR_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_calendar          "/klbui/calendar"


/// @brief 获取日历顶层对话框(菜单)
klb_wnd_t* klbui_shwnd_get_calendar(klb_gui_t* p_gui);


/// @brief 获取日历所需要的宽高
int klbshw_calendar_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h);


/// @brief Popup弹窗结束之前的回调
/// @param [in] *ptr        绑定的指针
/// @param [in] *p_calendar 日历对话框         
/// @param [in] ok          是否ok
/// @param [in] value       值
/// @return int
typedef int(*klbshw_calendar_cb)(void* ptr, klb_wnd_t* p_calendar, bool ok, int value);


/// @brief 绑定Popup弹窗结束之前的回调函数
int klbshw_calendar_bind(klb_wnd_t* p_wnd, klbshw_calendar_cb cb, void* ptr);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_CALENDAR_H__
//end
