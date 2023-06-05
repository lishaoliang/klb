///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_tab.h
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
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLBWND_TAB_max          8           ///< Tab 页面最大值


typedef struct klbwnd_tab_btn_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_tab_btn_css_t;


typedef struct klbwnd_tab_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数

    int                     btn_height;     ///< 按钮(tab按钮条)高度
    klbwnd_tab_btn_css_t    btn_css;
}klbwnd_tab_css_t;


typedef struct klbwnd_tab_t_
{
    klbwnd_tab_css_t*       p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    klb_wnd_t*              p_btns[KLBWND_TAB_max]; ///< klbwnd_tab_btn_create
    int                     btn_count;              ///< Tab 按钮数(页面数目)
}klbwnd_tab_t;


/// @brief init/quit/create
KLB_API void klbwnd_tab_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_tab_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_tab_css_init(klbwnd_tab_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_tab_css_quit(klbwnd_tab_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_tab_set_css(klb_wnd_t* p_wnd, klbwnd_tab_css_t* p_css);


/// @brief set/get title
KLB_API void klbwnd_tab_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_tab_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_tab_set_value(klb_wnd_t* p_wnd, const char* p_value);
KLB_API const sds klbwnd_tab_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_TAB_H__
//end
