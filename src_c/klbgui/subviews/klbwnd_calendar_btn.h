///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_calendar_btn.h
/// @brief   klb window calendar button; 日历控件专用按钮
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CALENDAR_BTN_H__
#define __KLBWND_CALENDAR_BTN_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbgui/wnd/klbwnd_calendar.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_calendar_btn_css_t_ klbwnd_calendar_btn_css_t;



#define KLBWND_CAL_BTN_normal       0       ///< 常规普通按钮

#define KLBWND_CAL_BTN_prev         10      ///< 向前(时间减小)
#define KLBWND_CAL_BTN_next         11      ///< 向后(时间增大)


typedef struct klbwnd_calendar_btn_t_
{
    klbwnd_calendar_btn_css_t*  p_css;      ///< 样式

    int         type;       ///< 类型: KLBWND_CAL_BTN_normal

    sds         title;      ///< 标题
    sds         value;      ///< 值

    struct
    {
        int     year;       ///< 年
        int     month;      ///< 月
        int     day;        ///< 日
    };
}klbwnd_calendar_btn_t;


/// @brief init/quit/create
void klbwnd_calendar_btn_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_calendar_btn_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_calendar_btn_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_calendar_btn_css_init(klbwnd_calendar_btn_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_calendar_btn_css_quit(klbwnd_calendar_btn_css_t* p_css);
void klbwnd_calendar_btn_css_copy(klbwnd_calendar_btn_css_t* p_dst, klbwnd_calendar_btn_css_t* p_src);


/// @brief set css
void klbwnd_calendar_btn_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_btn_css_t* p_css);


/// @brief 设置绘图类型
void klbwnd_calendar_btn_set_type(klb_wnd_t* p_wnd, int type);


/// @brief set/get title
void klbwnd_calendar_btn_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_calendar_btn_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
void klbwnd_calendar_btn_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_calendar_btn_get_value(klb_wnd_t* p_wnd);


/// @brief 设置年/月/日 值
void klbwnd_calendar_btn_set_ymd(klb_wnd_t* p_wnd, int year, int month, int day);
void klbwnd_calendar_btn_get_ymd(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_CALENDAR_BTN_H__
//end
