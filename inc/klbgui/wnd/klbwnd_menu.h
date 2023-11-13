///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_menu.h
/// @brief   klb window menu: 菜单, 最多支持到2级
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_MENU_H__
#define __KLBWND_MENU_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_MENU_ITEM_max        32      ///< 菜单项最大数目

#define KLBWND_MENU_WIDTH_min       124     ///< 菜单宽度最小值
#define KLBWND_MENU_WIDTH_max       512     ///< 菜单宽度最大值


typedef struct klbwnd_menu_item_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_menu_item_css_t;

typedef struct klbwnd_menu_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数

    klbwnd_menu_item_css_t  css_item;       ///< 子项css
}klbwnd_menu_css_t;


typedef struct klbwnd_menu_t_
{
    klbwnd_menu_css_t*      p_css;          ///< 样式

    sds                     value;          ///< 值

    klb_map_t               data;           ///< 菜单数据
    klb_map_t               data_2rd;       ///< 第二级菜单数据

    klb_wnd_t*              p_items[KLBWND_MENU_ITEM_max];  ///< 子窗口项: klbwnd_menu_item_create

    klb_wnd_t*              p_menu_2rd;     ///< 第二级菜单: klbwnd_menu_2rd_create

    struct
    {
        bool                is_popup_2rd;   ///< 是否弹出二级菜单
        sds                 key_2rd;        ///< 二级菜单的关键值
    };
}klbwnd_menu_t;


/// @brief init/quit/create
KLB_API void klbwnd_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_menu_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_menu_css_init(klbwnd_menu_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_menu_css_quit(klbwnd_menu_css_t* p_css);
KLB_API void klbwnd_menu_css_copy(klbwnd_menu_css_t* p_dst, klbwnd_menu_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_menu_set_css(klb_wnd_t* p_wnd, klbwnd_menu_css_t* p_css);


/// @brief set/get value
KLB_API void klbwnd_menu_set_value(klb_wnd_t* p_wnd, const char* p_value);
KLB_API const sds klbwnd_menu_get_value(klb_wnd_t* p_wnd);


/// @brief 添加菜单项
KLB_API void klbwnd_menu_append(klb_wnd_t* p_wnd, klb_map_t* p_array);

/// @brief 更新菜单项属性
KLB_API void klbwnd_menu_item_update(klb_wnd_t* p_wnd, klb_map_t* p_array);


/// @brief 添加子菜单项
KLB_API void klbwnd_menu_append_2rd(klb_wnd_t* p_wnd, klb_map_t* p_2rd);

/// @brief 更新子菜单项属性
KLB_API void klbwnd_menu_item_2rd_update(klb_wnd_t* p_wnd, klb_map_t* p_2rd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_MENU_H__
//end
