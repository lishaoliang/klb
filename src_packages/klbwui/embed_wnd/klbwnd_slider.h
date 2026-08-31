// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_slider.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window slider, 水平滑动条
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_SLIDER_H__
#define __KLBWND_SLIDER_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbwnd_slider_attributes_t
/// @brief  水平滑动条单状态属性 (foreground / pos)
typedef struct klbwnd_slider_attributes_t_
{
    uint32_t                    background_color;   ///< 整个背景色
    uint32_t                    foreground_color;   ///< 滑块横杆颜色

    klbuicss_text_t             text;               ///< 左侧已取值区域颜色 (text.color)

    uint32_t                    pos_color;          ///< 中间指示颜色 (非图片时)
    sds                         pos_image;          ///< 中间指示位置图片
} klbwnd_slider_attributes_t;


typedef struct klbwnd_slider_css_t_
{
    int                         foreground_h;   ///< 滑块高度
    int                         pos_w;          ///< 中间指示宽度
    int                         pos_h;          ///< 中间指示高度

    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_slider_attributes_t  normal;         ///< normal 常规状态参数
    klbwnd_slider_attributes_t  focus;          ///< focus 聚焦状态参数
    klbwnd_slider_attributes_t  disable;        ///< disable 不使能状态参数
} klbwnd_slider_css_t;


typedef struct klbwnd_slider_t_
{
    klbwnd_slider_css_t*        p_css;          ///< 样式

    sds                         title;          ///< 标题

    int                         min;            ///< 最小值
    int                         max;            ///< 最大值
    int                         value;          ///< 当前值
} klbwnd_slider_t;


/// @brief init/deinit/create
void klbwnd_slider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_slider_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_slider_css_init(klbwnd_slider_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_slider_css_deinit(klbwnd_slider_css_t* p_css);
void klbwnd_slider_css_copy(klbwnd_slider_css_t* p_dst, klbwnd_slider_css_t* p_src);


/// @brief set css
void klbwnd_slider_set_css(klb_wnd_t* p_wnd, klbwnd_slider_css_t* p_css);


/// @brief 设置/获取标题
void klbwnd_slider_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_slider_get_title(klb_wnd_t* p_wnd);


/// @brief 设置/获取当前值
void klbwnd_slider_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_slider_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
void klbwnd_slider_set_min(klb_wnd_t* p_wnd, int min);
void klbwnd_slider_set_max(klb_wnd_t* p_wnd, int max);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_SLIDER_H__

// end
