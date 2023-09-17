///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_combo.h
/// @brief   klb window combo
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_COMBO_H__
#define __KLBWND_COMBO_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_combo_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_combo_css_t;


typedef struct klbwnd_combo_t_
{
    klbwnd_combo_css_t*     p_css;          ///< 样式

    int                     index;          ///< 序号
    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    klb_map_t               data;           ///< 数据

    klb_wnd_t*              p_menu;         ///< 菜单: klbui_shwnd_get_combomenu
}klbwnd_combo_t;

/// @brief init/quit/create
KLB_API void klbwnd_combo_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_combo_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_combo_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_combo_css_init(klbwnd_combo_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_combo_css_quit(klbwnd_combo_css_t* p_css);
KLB_API void klbwnd_combo_css_copy(klbwnd_combo_css_t* p_dst, klbwnd_combo_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_combo_set_css(klb_wnd_t* p_wnd, klbwnd_combo_css_t* p_css);


/// @brief 设置/获取序号
KLB_API void klbwnd_combo_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int  klbwnd_combo_get_index(klb_wnd_t* p_wnd);


/// @brief set/get title
KLB_API void klbwnd_combo_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_combo_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_combo_set_value(klb_wnd_t* p_wnd, const char* p_value);
KLB_API const sds klbwnd_combo_get_value(klb_wnd_t* p_wnd);


/// @brief 追加数据/清空数据
KLB_API void klbwnd_combo_append(klb_wnd_t* p_wnd, klb_map_t* p_array);
KLB_API void klbwnd_combo_clear(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_COMBO_H__
//end
