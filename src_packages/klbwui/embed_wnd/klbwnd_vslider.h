// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_vslider.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window vertical slider, 垂直滑动条
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_VSLIDER_H__
#define __KLBWND_VSLIDER_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbwnd_vslider_attributes_t
/// @brief  垂直滑动条单状态属性 (foreground / thumb)
typedef struct klbwnd_vslider_attributes_t_
{
    uint32_t                        background_color;   ///< 整个背景色
    uint32_t                        foreground_color;   ///< 滑块区颜色
    uint32_t                        color;              ///< 值区域使用的颜色

    uint32_t                        thumb_color;        ///< 滑动块颜色 (非图片时)
    sds                             thumb_image;        ///< 滑动块图片
} klbwnd_vslider_attributes_t;


typedef struct klbwnd_vslider_css_t_
{
    int                             foreground_width;   ///< 滑块区宽度 (高度依据控件高度)
    int                             thumb_width;        ///< 滑动块宽
    int                             thumb_height;       ///< 滑动块高

    klbuicss_margin_t               margin;             ///< 外边距
    klbuicss_padding_t              padding;            ///< 内边距

    klbwnd_vslider_attributes_t     normal;             ///< normal 常规状态参数
    klbwnd_vslider_attributes_t     focus;              ///< focus 聚焦状态参数
    klbwnd_vslider_attributes_t     disable;            ///< disable 不使能状态参数
} klbwnd_vslider_css_t;


typedef struct klbwnd_vslider_t_
{
    klbwnd_vslider_css_t*           p_css;      ///< 样式

    int                             min;        ///< 最小值
    int                             max;        ///< 最大值
    int                             step;       ///< 步长
    int                             value;      ///< 当前值
} klbwnd_vslider_t;


/// @brief init/deinit/create
void klbwnd_vslider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_vslider_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_vslider_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_vslider_css_init(klbwnd_vslider_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_vslider_css_deinit(klbwnd_vslider_css_t* p_css);
void klbwnd_vslider_css_copy(klbwnd_vslider_css_t* p_dst, klbwnd_vslider_css_t* p_src);


/// @brief set css
void klbwnd_vslider_set_css(klb_wnd_t* p_wnd, klbwnd_vslider_css_t* p_css);


/// @brief 设置/获取当前值
void klbwnd_vslider_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_vslider_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
void klbwnd_vslider_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step);
void klbwnd_vslider_set_min(klb_wnd_t* p_wnd, int min);
void klbwnd_vslider_set_max(klb_wnd_t* p_wnd, int max);
void klbwnd_vslider_set_step(klb_wnd_t* p_wnd, int step);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_VSLIDER_H__

// end
