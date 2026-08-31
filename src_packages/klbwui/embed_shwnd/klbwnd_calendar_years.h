// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_calendar_years.h
/// @brief   klb window calendar years
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CALENDAR_YEARS_H__
#define __KLBWND_CALENDAR_YEARS_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/embed_shwnd/klbwnd_calendar.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_CAL_YEARS_row            4       ///< 行数
#define KLBWND_CAL_YEARS_column         4       ///< 列数


typedef struct klbwnd_calendar_years_t_
{
    klbwnd_calendar_css_t*      p_css;          ///< 样式

    klb_wnd_t*                  p_btns[KLBWND_CAL_YEARS_row][KLBWND_CAL_YEARS_column];
}klbwnd_calendar_years_t;


/// @brief init/deinit/create
void klbwnd_calendar_years_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_calendar_years_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_calendar_years_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief set css
void klbwnd_calendar_years_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css);


/// @brief 重新布局
void klbwnd_calendar_years_relayout(klb_wnd_t* p_wnd);


/// @brief 设置显示年份
void klbwnd_calendar_years_set_date_page(klb_wnd_t* p_wnd, klbwnd_calendar_ymd_t* p_ymd);


/// @brief 获取年份范围
void klbwnd_calendar_years_get_year_ranges(klb_wnd_t* p_wnd, int* p_year_min, int* p_year_max);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_CALENDAR_YEARS_H__

// end
