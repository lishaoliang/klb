///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_check.h
/// @brief   klb window check: 选择框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CHECK_H__
#define __KLBWND_CHECK_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_check_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t on_normal;      ///< 选中状态, 常规
    klbuicssex_attributes_t on_focus;       ///< 选中状态, 聚焦
    klbuicssex_attributes_t on_disable;     ///< 选中状态, 不使能

    klbuicssex_attributes_t off_normal;     ///< 未选中, 常规
    klbuicssex_attributes_t off_focus;      ///< 未选中, 聚焦
    klbuicssex_attributes_t off_disable;    ///< 未选中, 不使能
}klbwnd_check_css_t;


typedef struct klbwnd_check_t_
{
    klbwnd_check_css_t*     p_css;          ///< 样式

    int                     index;          ///< 序号
}klbwnd_check_t;


/// @brief init/quit/create
KLB_API void klbwnd_check_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_check_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_check_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_check_css_init(klbwnd_check_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_check_css_quit(klbwnd_check_css_t* p_css);
KLB_API void klbwnd_check_css_copy(klbwnd_check_css_t* p_dst, klbwnd_check_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_check_set_css(klb_wnd_t* p_wnd, klbwnd_check_css_t* p_css);


/// @brief 设置/获取序号
KLB_API void klbwnd_check_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int  klbwnd_check_get_index(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_CHECK_H__
//end
