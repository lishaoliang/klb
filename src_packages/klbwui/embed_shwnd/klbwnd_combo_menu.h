// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_combo_menu.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   combo 弹出菜单运行基础 (embed_shwnd 内部; 不注册 Lua type)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
///   \n 2026 onresize 时 relayout
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_COMBO_MENU_H__
#define __KLBWND_COMBO_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/embed_wnd/klbwnd_button.h"
#include "klbwui/embed_wnd/klbwnd_vscrollbar.h"
#include "klbwui/embed_shwnd/klbshw_combo_menu.h"
#include "klbutil/klb_map.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_COMBO_MENU_row_max        16
#define KLBWND_COMBO_MENU_item_max       (16 * 4)

#define KLBWND_COMBO_MENU_col_1          1
#define KLBWND_COMBO_MENU_col_2          2
#define KLBWND_COMBO_MENU_col_3          3
#define KLBWND_COMBO_MENU_col_4          4


typedef struct klbwnd_combo_menu_css_t_
{
    klbuicss_margin_t           margin;
    klbuicss_padding_t          padding;

    klbuicssex_attributes_t     normal;

    klbwnd_button_css_t         css_item;
    klbwnd_vscrollbar_css_t     css_vscrollbar;

    int                         item_h;
    int                         vscrollbar_w;
    int                         height_max;
} klbwnd_combo_menu_css_t;


typedef struct klbwnd_combo_menu_t_
{
    klbwnd_combo_menu_css_t      css;

    int                         row_count;
    klb_wnd_t*                  p_items[KLBWND_COMBO_MENU_item_max];
    klb_wnd_t*                  p_vscrollbar;

    int                         array_size;
    klb_map_t*                  p_ref_data_array;
    int                         need_w;
    int                         need_h;
    int                         col_count;

    klbshw_combo_menu_cb         cb;
    void*                       ptr;
} klbwnd_combo_menu_t;


void klbwnd_combo_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_combo_menu_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_combo_menu_css_init(klbwnd_combo_menu_t* p_menu, klb_gui_t* p_gui);
void klbwnd_combo_menu_css_deinit(klbwnd_combo_menu_t* p_menu);


void klbwnd_combo_menu_set_css(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu);


int klbwnd_combo_menu_bind(klb_wnd_t* p_wnd, klbshw_combo_menu_cb cb, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h, bool is_more_col, int menu_min_w);


int klbwnd_combo_menu_layout(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_COMBO_MENU_H__

// end
