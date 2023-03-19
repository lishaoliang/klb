///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_combo_menu.h
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_COMBO_MENU_H__
#define __KLBUI_COMBO_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBUI_COMBO_MENU_item_max       8


typedef struct klbui_combo_menu_item_t_ klbui_combo_menu_item_t;


typedef struct klbui_combo_menu_t_
{
    // normal
    klbuicss_margin_t           margin;         ///< 外边框
    klbuicss_padding_t          padding;        ///< 内边框

    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t     focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t     disable;        ///< disable 不使能状态参数

    klbui_combo_menu_item_t*   p_item[KLBUI_COMBO_MENU_item_max];
}klbui_combo_menu_t;


klb_wnd_t* klbui_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);



#ifdef __cplusplus
}
#endif

#endif // __KLBUI_COMBO_MENU_H__
//end
