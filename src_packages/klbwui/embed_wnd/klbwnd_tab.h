// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_tab.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window tab, Tab页面
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TAB_H__
#define __KLBWND_TAB_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klbwnd_tab_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_tab_attributes_t;

typedef struct klbwnd_tab_btn_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_tab_btn_attributes_t;

#define KLBWND_TAB_max          8           ///< Tab 页面最大值


typedef struct klbwnd_tab_btn_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbwnd_tab_btn_attributes_t normal;         ///< normal 常规状态参数
    klbwnd_tab_btn_attributes_t focus;          ///< focus 聚焦状态参数
    klbwnd_tab_btn_attributes_t disable;        ///< disable 不使能状态参数

    int                     check_height;               ///< 选中区, 高度
    uint32_t                check_background_color;     ///< 选中区, 颜色
    uint32_t                uncheck_background_color;   ///< 未选中区, 颜色

    int                     height;         ///< 按钮高度
    int                     width_min;      ///< 宽度最小值
    int                     width_max;      ///< 宽度最大值
}klbwnd_tab_btn_css_t;


typedef struct klbwnd_tab_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbwnd_tab_attributes_t normal;         ///< normal 常规状态参数

    klbwnd_tab_btn_css_t    btn_css;

    uint32_t                head_background;            ///< 头部背景色
    uint32_t                head_line_background_color; ///< 头部下方线条颜色
    int                     head_line_h;                ///< 头部下方线条高度
}klbwnd_tab_css_t;


typedef struct klbwnd_tab_t_
{
    klbwnd_tab_css_t*       p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    klb_wnd_t*              p_btns[KLBWND_TAB_max]; ///< klbwnd_tab_btn_create
    int                     btn_count;              ///< Tab 按钮数(页面数目)
}klbwnd_tab_t;


/// @brief init/deinit/create
void klbwnd_tab_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_tab_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_tab_css_init(klbwnd_tab_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_tab_css_deinit(klbwnd_tab_css_t* p_css);
void klbwnd_tab_css_copy(klbwnd_tab_css_t* p_dst, klbwnd_tab_css_t* p_src);


/// @brief set css
void klbwnd_tab_set_css(klb_wnd_t* p_wnd, klbwnd_tab_css_t* p_css);


/// @brief set/get title
void klbwnd_tab_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_tab_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
void klbwnd_tab_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_tab_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_TAB_H__

// end
