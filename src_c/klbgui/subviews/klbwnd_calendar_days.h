///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_calendar_days.h
/// @brief   klb window calendar days
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CALENDAR_DAYS_H__
#define __KLBWND_CALENDAR_DAYS_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/wnd/klbwnd_calendar.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_CAL_DAYS_row         7           ///< 行数
#define KLBWND_CAL_DAYS_column      7           ///< 列数

typedef struct klbwnd_calendar_days_t_
{
    klbwnd_calendar_css_t*      p_css;          ///< 样式

    klb_wnd_t*                  p_statics[KLBWND_CAL_DAYS_column];
    klb_wnd_t*                  p_btns[(KLBWND_CAL_DAYS_row - 1) * KLBWND_CAL_DAYS_column];
}klbwnd_calendar_days_t;


/// @brief init/quit/create
void klbwnd_calendar_days_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_calendar_days_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_calendar_days_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief set css
void klbwnd_calendar_days_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css);


/// @brief 重新布局
void klbwnd_calendar_days_relayout(klb_wnd_t* p_wnd);


/// @brief 设置显示年月
void klbwnd_calendar_days_set_date_page(klb_wnd_t* p_wnd, klbwnd_calendar_ymd_t* p_ymd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_CALENDAR_DAYS_H__
//end
