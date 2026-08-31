// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_menu_2rd.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window menu 2rd: 第二级菜单
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_MENU_2RD_H__
#define __KLBWND_MENU_2RD_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"
#include "klbwui/embed_wnd/klbwnd_menu.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_menu_2rd_t_
{
    klbwnd_menu_css_t*      p_css;          ///< 样式

    sds                     value;          ///< 值

    klb_map_t*              p_data_ref;     ///< 数据引用

    klb_wnd_t*              p_items[KLBWND_MENU_ITEM_max];  ///< 子窗口项: klbwnd_menu_item_create
}klbwnd_menu_2rd_t;


/// @brief init/deinit/create
void klbwnd_menu_2rd_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_menu_2rd_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_menu_2rd_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief set css
void klbwnd_menu_2rd_set_css(klb_wnd_t* p_wnd, klbwnd_menu_css_t* p_css);


/// @brief set/get value
void klbwnd_menu_2rd_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_menu_2rd_get_value(klb_wnd_t* p_wnd);


/// @brief 设置数据引用
void klbwnd_menu_set_data_ref(klb_wnd_t* p_wnd, klb_map_t* p_data_ref);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_MENU_2RD_H__

// end
