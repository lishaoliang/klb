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
#include "klbutil/klb_map.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBUI_COMBO_MENU_item_max       8


typedef struct klbui_combo_menu_item_t_ klbui_combo_menu_item_t;


typedef int(*klbui_combo_menu_cb)(klb_wnd_t* p_combo, sds value, sds title);


typedef struct klbui_combo_menu_css_t_
{
    // normal
    klbuicss_padding_t          padding;        ///< 内边距
    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数

    // item
    klbuicssex_attributes_t     item_normal;    ///< normal 常规状态参数
    klbuicssex_attributes_t     item_focus;     ///< focus 聚焦状态参数
}klbui_combo_menu_css_t;

typedef struct klbui_combo_menu_t_
{
    klbui_combo_menu_item_t*        p_item[KLBUI_COMBO_MENU_item_max];

    struct
    {
        klbui_combo_menu_css_t*     p_ref_css;      ///< CSS样式: 采用引用combo组件的样式定义
        klb_map_t*                  p_ref_array;    ///< combo组件中存储的数据

        klb_wnd_t*                  p_combo;        ///< combo组件
        klbui_combo_menu_cb         cb_combo;       ///< 选中后的回调函数
    };
}klbui_combo_menu_t;


klb_wnd_t* klbui_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);

void klbui_combo_menu_css_init(const klbui_default_t* p_default, klbui_combo_menu_css_t* p_menu_css);
void klbui_combo_menu_css_quit(klbui_combo_menu_css_t* p_menu_css);

int klbui_combo_menu_bind(klb_wnd_t* p_wnd, klbui_combo_menu_css_t* p_css, klb_map_t* p_data_array, klbui_combo_menu_cb cb, klb_wnd_t* p_combo, int* p_out_w, int* p_out_h);


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_COMBO_MENU_H__
//end
