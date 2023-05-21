///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_vscrollbar.h
/// @brief   klb window vertical scrollbar 垂直滚动条
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_VSCROLLBAR_H__
#define __KLBWND_VSCROLLBAR_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/wnd/klbwnd_btnex.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_vscrollbar_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t     focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t     disable;        ///< disable 不使能状态参数

    klbwnd_btnex_css_t          btnex;          ///< 按钮CSS
}klbwnd_vscrollbar_css_t;


typedef struct klbwnd_vscrollbar_t_
{
    klbwnd_vscrollbar_css_t*    p_css;          ///< 样式

    // 子控件
    struct
    {
        klb_wnd_t*              p_up;           ///< 向上 : klbwnd_btnex_create
        klb_wnd_t*              p_down;         ///< 向下 : klbwnd_btnex_create
        klb_wnd_t*              p_middle;       ///< 中间滑块 : klbwnd_btnex_create
    };

    // 值/范围
    struct
    {
        int                     min;
        int                     max;
        int                     value;
    };
}klbwnd_vscrollbar_t;


/// @brief init/quit/create
KLB_API void klbwnd_vscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_vscrollbar_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_vscrollbar_css_init(klbwnd_vscrollbar_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_vscrollbar_css_quit(klbwnd_vscrollbar_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_vscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_vscrollbar_css_t* p_css);


/// @brief set/get value
KLB_API void klbwnd_vscrollbar_set_value(klb_wnd_t* p_wnd, int value);
KLB_API int klbwnd_vscrollbar_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_VSCROLLBAR_H__
//end
