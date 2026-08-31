// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_decimal_menu_ip.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   IP 用 10进制输入菜单运行基础 (embed_shwnd 内部; 不注册 Lua type)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DECIMAL_MENU_IP_H__
#define __KLBWND_DECIMAL_MENU_IP_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/embed_wnd/klbwnd_button.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu_ip.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_DECMENU_IP_row              5
#define KLBWND_DECMENU_IP_column           4

#define KLBWND_DECMENU_IP_KEY_none         0
#define KLBWND_DECMENU_IP_KEY_normal       1
#define KLBWND_DECMENU_IP_KEY_backspace    2
#define KLBWND_DECMENU_IP_KEY_clear        3
#define KLBWND_DECMENU_IP_KEY_enter        4
#define KLBWND_DECMENU_IP_KEY_moveleft     5
#define KLBWND_DECMENU_IP_KEY_moveright    6
#define KLBWND_DECMENU_IP_KEY_point        7


typedef struct klbwnd_decimal_menu_ip_layout_t_
{
    klb_rect_t      rect;           ///< 位置区域
    const char*     p_num;          ///< 10 进制数字或标题
    int             type;           ///< 按键类型 eg. KLBWND_DECMENU_IP_KEY_normal
} klbwnd_decimal_menu_ip_layout_t;


typedef struct klbwnd_decimal_menu_ip_fg_t_
{
    sds             move_left_image;
    sds             move_right_image;
    sds             backspace_image;
} klbwnd_decimal_menu_ip_fg_t;


typedef struct klbwnd_decimal_menu_ip_btn_attr_t_
{
    sds                         normal_stretch_image;
    sds                         focus_stretch_image;
    sds                         disable_stretch_image;

    sds                         check_normal_stretch_image;
    sds                         check_focus_stretch_image;
    sds                         check_disable_stretch_image;

    klbwnd_decimal_menu_ip_fg_t    normal_foreground;
    klbwnd_decimal_menu_ip_fg_t    focus_foreground;
    klbwnd_decimal_menu_ip_fg_t    disable_foreground;
} klbwnd_decimal_menu_ip_btn_attr_t;


typedef struct klbwnd_decimal_menu_ip_css_t_
{
    klbuicss_margin_t               margin;
    klbuicss_padding_t              padding;

    klbuicssex_attributes_t         normal;

    klbuicss_background_t           string_background;
    int                             string_height;
    int                             string_offset;

    klbwnd_button_css_t             btn_css;
    klbwnd_decimal_menu_ip_btn_attr_t  btn_attr;
} klbwnd_decimal_menu_ip_css_t;


typedef struct klbwnd_decimal_menu_ip_t_
{
    sds                             title;
    sds                             value;
    int                             max_len;

    int                             cursor_pos;
    uint32_t                        cursor_color;
    bool                            show_cursor;

    klbwnd_decimal_menu_ip_layout_t    layout[KLBWND_DECMENU_IP_row][KLBWND_DECMENU_IP_column];
    klb_wnd_t*                      p_wnds[KLBWND_DECMENU_IP_row][KLBWND_DECMENU_IP_column];

    klbwnd_decimal_menu_ip_css_t       css;

    klbshw_decimal_menu_ip_cb          cb;
    klbshw_decimal_menu_ip_cb          cb_click_point;
    klbshw_decimal_menu_ip_cb          cb_click_number;
    klbshw_decimal_menu_ip_cb          cb_click_moveleft;
    klbshw_decimal_menu_ip_cb          cb_click_moveright;
    klbshw_decimal_menu_ip_cb          cb_click_backspace;
    klbshw_decimal_menu_ip_cb          cb_click_clear;
    klbshw_decimal_menu_ip_cb          cb_get_cursor_pos;
    klbshw_decimal_menu_ip_cb          cb_set_show_cursor;
    void*                           ptr;
} klbwnd_decimal_menu_ip_t;


void klbwnd_decimal_menu_ip_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_decimal_menu_ip_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_decimal_menu_ip_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_decimal_menu_ip_css_init(klbwnd_decimal_menu_ip_t* p_menu, klb_gui_t* p_gui);
void klbwnd_decimal_menu_ip_css_deinit(klbwnd_decimal_menu_ip_t* p_menu);


int klbwnd_decimal_menu_ip_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


void klbwnd_decimal_menu_ip_set_css(klb_wnd_t* p_wnd, klbwnd_decimal_menu_ip_t* p_menu);


void klbwnd_decimal_menu_ip_set_ranges(klb_wnd_t* p_wnd, int max_len);


void klbwnd_decimal_menu_ip_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_decimal_menu_ip_get_value(klb_wnd_t* p_wnd);


int klbwnd_decimal_menu_ip_bind(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb_result, void* ptr);

int klbwnd_decimal_menu_ip_bind_click_point(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_click_number(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_click_moveleft(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_click_moveright(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_click_backspace(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_click_clear(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_get_cursor_pos(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);
int klbwnd_decimal_menu_ip_bind_set_show_cursor(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);




int klbwnd_decimal_menu_ip_layout(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_DECIMAL_MENU_IP_H__

// end
