// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_edit_menu.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   编辑框输入菜单运行基础 (embed_shwnd 内部; 不注册 Lua type)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_EDIT_MENU_H__
#define __KLBWND_EDIT_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/embed_wnd/klbwnd_button.h"
#include "klbwui/embed_shwnd/klbshw_edit_menu.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_EDITMENU_row             5
#define KLBWND_EDITMENU_column          10

#define KLBWND_EDITMENU_KEY_none        0
#define KLBWND_EDITMENU_KEY_normal      1
#define KLBWND_EDITMENU_KEY_backspace   2
#define KLBWND_EDITMENU_KEY_capslock    3
#define KLBWND_EDITMENU_KEY_symbol      4
#define KLBWND_EDITMENU_KEY_space       5
#define KLBWND_EDITMENU_KEY_enter       6
#define KLBWND_EDITMENU_KEY_moveleft    7
#define KLBWND_EDITMENU_KEY_moveright   8
#define KLBWND_EDITMENU_KEY_clear       9
#define KLBWND_EDITMENU_KEY_global      10


typedef struct klbwnd_edit_menu_layout_t_
{
    klb_rect_t      rect;           ///< 位置区域
    const char*     p_lowercase;    ///< 小写
    const char*     p_uppercase;    ///< 大写
    const char*     p_symbol;       ///< 符号
    int             type;           ///< 按键类型 eg. KLBWND_EDITMENU_KEY_normal
} klbwnd_edit_menu_layout_t;


typedef struct klbwnd_edit_menu_fg_t_
{
    sds             move_left_image;
    sds             move_right_image;
    sds             backspace_image;
    sds             global_image;
} klbwnd_edit_menu_fg_t;


typedef struct klbwnd_edit_menu_btn_attr_t_
{
    sds                     normal_stretch_image;
    sds                     focus_stretch_image;
    sds                     disable_stretch_image;

    sds                     check_normal_stretch_image;
    sds                     check_focus_stretch_image;
    sds                     check_disable_stretch_image;

    klbwnd_edit_menu_fg_t   normal_foreground;
    klbwnd_edit_menu_fg_t   focus_foreground;
    klbwnd_edit_menu_fg_t   disable_foreground;
} klbwnd_edit_menu_btn_attr_t;


typedef struct klbwnd_edit_menu_css_t_
{
    klbuicss_margin_t           margin;
    klbuicss_padding_t          padding;

    klbuicssex_attributes_t     normal;

    klbuicss_background_t       string_background;
    int                         string_height;
    int                         string_offset;

    klbwnd_button_css_t         btn_css;
    klbwnd_edit_menu_btn_attr_t btn_attr;
} klbwnd_edit_menu_css_t;


typedef struct klbwnd_edit_menu_t_
{
    sds                         title;
    sds                         value;

    bool                        is_hide_letter;
    sds                         hide_title;

    int                         cursor_pos;
    uint32_t                    cursor_color;
    bool                        show_cursor;

    sds                         temp_str;

    int                         page;

    klbwnd_edit_menu_layout_t   layout[KLBWND_EDITMENU_row][KLBWND_EDITMENU_column];
    klb_wnd_t*                  p_wnds[KLBWND_EDITMENU_row][KLBWND_EDITMENU_column];

    klbwnd_edit_menu_css_t      css;

    klbshw_edit_menu_cb         cb;
    void*                       ptr;
} klbwnd_edit_menu_t;


void klbwnd_edit_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_edit_menu_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_edit_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_edit_menu_css_init(klbwnd_edit_menu_t* p_menu, klb_gui_t* p_gui);
void klbwnd_edit_menu_css_deinit(klbwnd_edit_menu_t* p_menu);


int klbwnd_edit_menu_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


void klbwnd_edit_menu_set_css(klb_wnd_t* p_wnd, klbwnd_edit_menu_t* p_menu);


void klbwnd_edit_menu_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_edit_menu_get_value(klb_wnd_t* p_wnd);


int klbwnd_edit_menu_bind(klb_wnd_t* p_wnd, klbshw_edit_menu_cb cb_result, void* ptr);


int klbwnd_edit_menu_layout(klb_wnd_t* p_wnd);


int klbwnd_edit_menu_set_hide_letter(klb_wnd_t* p_wnd, bool is_hide_letter);


int klbwnd_edit_menu_set_page_letter(klb_wnd_t* p_wnd, int page);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_EDIT_MENU_H__

// end
