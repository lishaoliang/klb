///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_vslider.h
/// @brief   klb window vertical slider : 垂直滑动条
///          参考: https://learn.microsoft.com/zh-cn/dotnet/api/system.windows.controls.slider?view=windowsdesktop-7.0
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_VSLIDER_H__
#define __KLBWND_VSLIDER_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbui_vslider_attributes_t
/// @brief  垂直滑动条单状态css属性
typedef struct klbui_vslider_attributes_t_
{
    uint32_t    background_color;   ///< 整个背景色
    uint32_t    foreground_color;   ///< 滑块区颜色
    uint32_t    color;              ///< 值区域使用的颜色

    uint32_t    thumb_color;        ///< 滑动块颜色(非图片时)
    sds         thumb_image;        ///< 滑动块图片
}klbui_vslider_attributes_t;


/// @brief  垂直滑动条css属性
typedef struct klbwnd_vslider_css_t_
{
    int                         foreground_width;   ///< 滑块区宽度(其高度依据控件高度而定)
    int                         thumb_width;        ///< 滑动块宽
    int                         thumb_height;       ///< 滑动块高

    klbuicss_margin_t           margin;             ///< 外边距
    klbuicss_padding_t          padding;            ///< 内边距

    klbui_vslider_attributes_t  normal;             ///< normal 常规状态参数
    klbui_vslider_attributes_t  focus;              ///< focus 聚焦状态参数
    klbui_vslider_attributes_t  disable;            ///< disable 不使能状态参数
}klbwnd_vslider_css_t;


/// @brief  垂直滑动条
typedef struct klbwnd_vslider_t_
{
    klbwnd_vslider_css_t*   p_css;      ///< 样式

    int                     min;        ///< 最小值
    int                     max;        ///< 最大值
    int                     step;       ///< 步长
    int                     value;      ///< 当前值
}klbwnd_vslider_t;


/// @brief init/quit/create
KLB_API void klbwnd_vslider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_vslider_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_vslider_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_vslider_css_init(klbwnd_vslider_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_vslider_css_quit(klbwnd_vslider_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_vslider_set_css(klb_wnd_t* p_wnd, klbwnd_vslider_css_t* p_css);


/// @brief set/get value
KLB_API void klbwnd_vslider_set_value(klb_wnd_t* p_wnd, int value);
KLB_API int klbwnd_vslider_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
KLB_API void klbwnd_vslider_set_range(klb_wnd_t* p_wnd, int min, int max, int step);
KLB_API void klbwnd_vslider_set_min(klb_wnd_t* p_wnd, int min);
KLB_API void klbwnd_vslider_set_max(klb_wnd_t* p_wnd, int max);
KLB_API void klbwnd_vslider_set_step(klb_wnd_t* p_wnd, int step);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_VSLIDER_H__
//end
