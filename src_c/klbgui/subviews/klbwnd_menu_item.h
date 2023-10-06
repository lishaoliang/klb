///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_menu_item.h
/// @brief   klb window menu item
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_MENU_ITEM_H__
#define __KLBWND_MENU_ITEM_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbgui/wnd/klbwnd_menu.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_menu_item_t_
{
    klbwnd_menu_item_css_t* p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    bool                    has_2rd;        ///< 是否拥有二级子菜单
}klbwnd_menu_item_t;


/// @brief init/quit/create
void klbwnd_menu_item_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_menu_item_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_menu_item_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_menu_item_css_init(klbwnd_menu_item_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_menu_item_css_quit(klbwnd_menu_item_css_t* p_css);
void klbwnd_menu_item_css_copy(klbwnd_menu_item_css_t* p_dst, klbwnd_menu_item_css_t* p_src);


/// @brief set css
void klbwnd_menu_item_set_css(klb_wnd_t* p_wnd, klbwnd_menu_item_css_t* p_css);


/// @brief set/get title
void klbwnd_menu_item_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_menu_item_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
void klbwnd_menu_item_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_menu_item_get_value(klb_wnd_t* p_wnd);


/// @brief 设置/获取 是否拥有二级子菜单
void klbwnd_menu_item_set_has_2rd(klb_wnd_t* p_wnd, bool has_2rd);
bool klbwnd_menu_item_has_2rd(klb_wnd_t* p_wnd);



#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_MENU_ITEM_H__
//end
