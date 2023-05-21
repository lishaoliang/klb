///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_calendar.h
/// @brief   klb window calendar, 日历基础组件, 提供C接口
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CALENDAR_H__
#define __KLBWND_CALENDAR_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbthird/sds.h"
#include "klbgui/wnd/klbwnd_static.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_CAL_YEAR_min             2020
#define KLBWND_CAL_YEAR_max             2051


#define KLBWND_CAL_PAGE_days            0
#define KLBWND_CAL_PAGE_months          1
#define KLBWND_CAL_PAGE_years           2


typedef struct klbwnd_calendar_ymd_t_
{
    int         year;
    int         month;
    int         day;
}klbwnd_calendar_ymd_t;


typedef struct klbwnd_calendar_btn_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t     focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t     disable;        ///< disable 不使能状态参数
}klbwnd_calendar_btn_css_t;


typedef struct klbwnd_calendar_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t     focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t     disable;        ///< disable 不使能状态参数

    klbwnd_static_css_t         sta;            ///< 静态文本
    klbwnd_calendar_btn_css_t   btn;            ///< 按钮
}klbwnd_calendar_css_t;


typedef struct klbwnd_calendar_t_
{
    klbwnd_calendar_css_t*      p_css;          ///< 样式

    // 子控件
    struct
    {
        klb_wnd_t*              p_btn_ymd;      ///< 切换年/月/日按钮
        klb_wnd_t*              p_btn_prev;     ///< 向前一个年/月
        klb_wnd_t*              p_btn_next;     ///< 向后一个年/月

        klb_wnd_t*              p_years;        ///< 年页面
        klb_wnd_t*              p_months;       ///< 月页面
        klb_wnd_t*              p_days;         ///< 日页面
    };

    // 数据/记录等
    struct
    {
        int                     cur_page;       ///< 当前页面: KLBWND_CAL_PAGE_days
    };

    klbwnd_calendar_ymd_t       date_page;      ///< 页面显示年份

    klbwnd_calendar_ymd_t       date;           ///< 当前日期
}klbwnd_calendar_t;


/// @brief init/quit/create
KLB_API void klbwnd_calendar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_calendar_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_calendar_css_init(klbwnd_calendar_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_calendar_css_quit(klbwnd_calendar_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_calendar_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css);


/// @brief set 限制日期范围
KLB_API void klbwnd_calendar_limit(int* p_year, int* p_month, int* p_day);


/// @brief set 设置/获取当前日期
KLB_API void klbwnd_calendar_set_date(klb_wnd_t* p_wnd, int year, int month, int day);
KLB_API void klbwnd_calendar_get_date(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day);


/// @brief set 设置显示日期页面
KLB_API void klbwnd_calendar_set_date_page(klb_wnd_t* p_wnd, int year, int month, int day);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_CALENDAR_H__
//end
