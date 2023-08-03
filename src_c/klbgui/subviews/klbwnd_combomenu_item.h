///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_combomenu_item.h
/// @brief   klb window combo menu item; "kcombo"控件专用菜单项
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_COMBOMENU_ITEM_H__
#define __KLBWND_COMBOMENU_ITEM_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_combomenu_item_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_combomenu_item_css_t;


typedef struct klbwnd_combomenu_item_t_
{
    klbwnd_combomenu_item_css_t* p_css;     ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值
}klbwnd_combomenu_item_t;


/// @brief init/quit/create
void klbwnd_combomenu_item_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_combomenu_item_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_combomenu_item_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_combomenu_item_css_init(klbwnd_combomenu_item_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_combomenu_item_css_quit(klbwnd_combomenu_item_css_t* p_css);


/// @brief set css
void klbwnd_combomenu_item_set_css(klb_wnd_t* p_wnd, klbwnd_combomenu_item_css_t* p_css);


/// @brief set/get title
void klbwnd_combomenu_item_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_combomenu_item_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
void klbwnd_combomenu_item_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_combomenu_item_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_COMBOMENU_ITEM_H__
//end
