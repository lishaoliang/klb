///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_group.h
/// @brief   klb window group
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_GROUP_H__
#define __KLBWND_GROUP_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_group_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_group_css_t;


typedef struct klbwnd_group_t_
{
    klbwnd_group_css_t*     p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值
}klbwnd_group_t;


/// @brief init/quit/create
KLB_API void klbwnd_group_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_group_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_group_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_group_css_init(klbwnd_group_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_group_css_quit(klbwnd_group_css_t* p_css);
KLB_API void klbwnd_group_css_copy(klbwnd_group_css_t* p_dst, klbwnd_group_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_group_set_css(klb_wnd_t* p_wnd, klbwnd_group_css_t* p_css);


/// @brief set/get title
KLB_API void klbwnd_group_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_group_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_group_set_value(klb_wnd_t* p_wnd, const char* p_value);
KLB_API const sds klbwnd_group_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_GROUP_H__
//end
