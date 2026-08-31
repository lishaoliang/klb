// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_calendar_menu.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   日历菜单 共享窗口
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_CALENDAR_MENU_H__
#define __KLBSHW_CALENDAR_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_shwnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_calendar_menu            "/klbui/calendar-menu"


/// @brief 获取日历菜单
/// @param [in] *p_gui      GUI 对象
/// @return klb_wnd_t* 共享窗口; 首次调用时创建并 push
klb_wnd_t* klbui_shwnd_get_calendar_menu(klb_gui_t* p_gui);


/// @brief Popup 弹窗结束之前的回调
/// @param [in] *ptr            绑定的指针
/// @param [in] *p_wnd_cal      日历菜单
/// @param [in] ok              是否 ok
/// @param [in] year            年
/// @param [in] month           月
/// @param [in] day             日
/// @return int
typedef int(*klbshw_calendar_menu_cb)(void* ptr, klb_wnd_t* p_wnd_cal, bool ok, int year, int month, int day);


/// @brief 绑定 Popup 弹窗结束之前的回调函数
int klbshw_calendar_menu_bind(klb_wnd_t* p_wnd, klbshw_calendar_menu_cb cb, void* ptr);


/// @brief 设置日期
void klbshw_calendar_menu_set_date(klb_wnd_t* p_wnd, int year, int month, int day);


/// @brief 获取所需要的宽高
int klbshw_calendar_menu_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_CALENDAR_MENU_H__

// end
