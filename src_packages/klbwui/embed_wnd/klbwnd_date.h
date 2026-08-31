// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_date.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window date, 日期编辑框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DATE_H__
#define __KLBWND_DATE_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_date_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_date_attributes_t;


typedef struct klbwnd_date_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_date_attributes_t    normal;         ///< normal 常规状态参数
    klbwnd_date_attributes_t    focus;          ///< focus 聚焦状态参数
    klbwnd_date_attributes_t    disable;        ///< disable 不使能状态参数
} klbwnd_date_css_t;


typedef struct klbwnd_date_t_
{
    klbwnd_date_css_t*          p_css;          ///< 样式

    sds                         title;          ///< 显示文本

    int                         year;           ///< 年
    int                         month;          ///< 月
    int                         day;            ///< 日

    klb_wnd_t*                  p_calendar;     ///< 日历菜单: klbui_shwnd_get_calendar_menu
} klbwnd_date_t;


/// @brief init/deinit/create
void klbwnd_date_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_date_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_date_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_date_css_init(klbwnd_date_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_date_css_deinit(klbwnd_date_css_t* p_css);
void klbwnd_date_css_copy(klbwnd_date_css_t* p_dst, klbwnd_date_css_t* p_src);


/// @brief set css
void klbwnd_date_set_css(klb_wnd_t* p_wnd, klbwnd_date_css_t* p_css);


/// @brief 设置/获取标题
void klbwnd_date_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_date_get_title(klb_wnd_t* p_wnd);


/// @brief 设置/获取日期
void klbwnd_date_set_value(klb_wnd_t* p_wnd, int year, int month, int day);
void klbwnd_date_get_value(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_DATE_H__

// end
