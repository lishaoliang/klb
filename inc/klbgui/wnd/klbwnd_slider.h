///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_slider.h
/// @brief   klb window slider
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_SLIDER_H__
#define __KLBWND_SLIDER_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbui_slider_attributes_t
/// @brief  滑动条状态属性
typedef struct klbui_slider_attributes_t_
{
    uint32_t                background_color;   ///< 整个背景色
    uint32_t                foreground_color;   ///< 滑块颜色

    klbuicss_text_t         text;               ///< 左侧值占据的颜色

    uint32_t                pos_color;          ///< 中间指示颜色(非图片时)
    sds                     pos_image;          ///< 中间指示位置图片
}klbui_slider_attributes_t;


/// @brief  滑动条CSS属性
typedef struct klbwnd_slider_css_t_
{
    int         foreground_h;   ///< 滑块高度
    int         pos_w;          ///< 中间指示宽度
    int         pos_h;          ///< 中间指示高度

    klbuicss_margin_t         margin;         ///< 外边距
    klbuicss_padding_t        padding;        ///< 内边距

    klbui_slider_attributes_t normal;         ///< normal 常规状态参数
    klbui_slider_attributes_t focus;          ///< focus 聚焦状态参数
    klbui_slider_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_slider_css_t;


/// @brief  滑动条
typedef struct klbwnd_slider_t_
{
    klbwnd_slider_css_t*    p_css;          ///< 样式

    sds                     title;          ///< 标题

    int                     min;            ///< 最小值
    int                     max;            ///< 最大值
    int                     value;          ///< 当前值
}klbwnd_slider_t;


/// @brief init/quit/create
KLB_API void klbwnd_slider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_slider_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_slider_css_init(klbwnd_slider_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_slider_css_quit(klbwnd_slider_css_t* p_css);
KLB_API void klbwnd_slider_css_copy(klbwnd_slider_css_t* p_dst, klbwnd_slider_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_slider_set_css(klb_wnd_t* p_wnd, klbwnd_slider_css_t* p_css);


/// @brief set/get title
KLB_API void klbwnd_slider_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_slider_get_title(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_SLIDER_H__
//end
