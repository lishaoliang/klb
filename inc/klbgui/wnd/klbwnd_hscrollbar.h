///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_hscrollbar.h
/// @brief   klb window horizontal scrollbar 水平滚动条
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_HSCROLLBAR_H__
#define __KLBWND_HSCROLLBAR_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/wnd/klbwnd_btnex.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_hscrollbar_css_t_
{
    klbuicss_margin_t       margin;             ///< 外边距
    klbuicss_padding_t      padding;            ///< 内边距

    klbuicssex_attributes_t normal;             ///< normal 常规状态参数
    klbuicssex_attributes_t focus;              ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;            ///< disable 不使能状态参数

    klbwnd_btnex_css_t      btnex;              ///< 按钮css
}klbwnd_hscrollbar_css_t;


typedef struct klbwnd_hscrollbar_t_
{
    klbwnd_hscrollbar_css_t*    p_css;          ///< 样式

    klb_wnd_t*                  p_left;         ///< 向左 : klbwnd_btnex_create
    klb_wnd_t*                  p_right;        ///< 向右 : klbwnd_btnex_create
    klb_wnd_t*                  p_middle;       ///< 中间滑块 : klbwnd_btnex_create

	// 值/范围
	struct
	{
		int                     min;
		int                     max;
		int						step;
		int                     value;
	};
}klbwnd_hscrollbar_t;


/// @brief init/quit/create
KLB_API void klbwnd_hscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_hscrollbar_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_hscrollbar_css_init(klbwnd_hscrollbar_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_hscrollbar_css_quit(klbwnd_hscrollbar_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_hscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_hscrollbar_css_t* p_css);


/// @brief set/get value
KLB_API void klbwnd_hscrollbar_set_value(klb_wnd_t* p_wnd, int value);
KLB_API int klbwnd_hscrollbar_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
KLB_API void klbwnd_hscrollbar_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_HSCROLLBAR_H__
//end
