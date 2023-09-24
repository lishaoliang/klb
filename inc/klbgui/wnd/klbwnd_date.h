///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_date.h
/// @brief   klb window date
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DATE_H__
#define __KLBWND_DATE_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbgui/shwnd/klbshw_calendar.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_date_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_date_css_t;


typedef struct klbwnd_date_t_
{
    klbwnd_date_css_t*      p_css;          ///< 样式

    int                     index;          ///< 序号
    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    struct
    {
        int                 year;           ///< 年
        int                 month;          ///< 月
        int                 day;            ///< 日
    };

    klb_wnd_t*              p_calendar;     ///< 弹出日历菜单; klbui_shwnd_get_calendar
}klbwnd_date_t;


/// @brief init/quit/create
KLB_API void klbwnd_date_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_date_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_date_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_date_css_init(klbwnd_date_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_date_css_quit(klbwnd_date_css_t* p_css);
KLB_API void klbwnd_date_css_copy(klbwnd_date_css_t* p_dst, klbwnd_date_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_date_set_css(klb_wnd_t* p_wnd, klbwnd_date_css_t* p_css);


/// @brief 设置/获取序号
KLB_API void klbwnd_date_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int  klbwnd_date_get_index(klb_wnd_t* p_wnd);


/// @brief set/get title
KLB_API void klbwnd_date_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_date_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_date_set_value(klb_wnd_t* p_wnd, int year, int month, int day);
KLB_API void klbwnd_date_get_value(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_DATE_H__
//end
