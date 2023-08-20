///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_tip.h
/// @brief   klb window button
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TIP_H__
#define __KLBWND_TIP_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_tip_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_tip_css_t;


typedef struct klbwnd_tip_t_
{
    klbwnd_tip_css_t*       p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值
}klbwnd_tip_t;


/// @brief init/quit/create
void klbwnd_tip_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_tip_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_tip_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_tip_css_init(klbwnd_tip_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_tip_css_quit(klbwnd_tip_css_t* p_css);


/// @brief set css
void klbwnd_tip_set_css(klb_wnd_t* p_wnd, klbwnd_tip_css_t* p_css);


/// @brief set/get title
void klbwnd_tip_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_tip_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
void klbwnd_tip_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_tip_get_value(klb_wnd_t* p_wnd);


void klbwnd_tip_layout(klb_wnd_t* p_wnd, int max_w, int max_h, int* p_out_w, int* p_out_h);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_TIP_H__
//end
