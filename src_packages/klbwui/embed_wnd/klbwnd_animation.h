// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_animation.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window animation, 序列帧/图动画 (ticker 刷新)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_ANIMATION_H__
#define __KLBWND_ANIMATION_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_animation_attributes_t_
{
    klbuicss_text_t         text;           ///< color, text-align
    klbuicss_font_t         font;           ///< font-size
    klbuicss_background_t   background;     ///< background-color / background-image
    klbuicss_border_t       border;         ///< border-width, border-color
} klbwnd_animation_attributes_t;


typedef struct klbwnd_animation_css_t_
{
    klbuicss_margin_t               margin;     ///< 外边距
    klbuicss_padding_t              padding;    ///< 内边距

    klbwnd_animation_attributes_t   normal;     ///< normal 常规状态参数
} klbwnd_animation_css_t;


typedef struct klbwnd_animation_t_
{
    klbwnd_animation_css_t* p_css;          ///< 样式

    sds                     title;          ///< 标题 (NULL 表示未设置)
    sds                     value;          ///< 值 (NULL 表示未设置)

    int                     index;          ///< 帧序号
    bool                    enable;         ///< 是否启用定时刷新
    int64_t                 tick_count;     ///< 上次触发滴答 (毫秒)
    int64_t                 tc_interval;    ///< 间隔 (毫秒)
} klbwnd_animation_t;


void klbwnd_animation_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_animation_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_animation_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_animation_css_init(klbwnd_animation_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_animation_css_deinit(klbwnd_animation_css_t* p_css);
void klbwnd_animation_css_copy(klbwnd_animation_css_t* p_dst, klbwnd_animation_css_t* p_src);


void klbwnd_animation_set_css(klb_wnd_t* p_wnd, klbwnd_animation_css_t* p_css);


void klbwnd_animation_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_animation_get_title(klb_wnd_t* p_wnd);


void klbwnd_animation_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_animation_get_value(klb_wnd_t* p_wnd);


void klbwnd_animation_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_animation_get_index(klb_wnd_t* p_wnd);


void klbwnd_animation_set_enable(klb_wnd_t* p_wnd, bool is_enable);
bool klbwnd_animation_get_enable(klb_wnd_t* p_wnd);


void klbwnd_animation_set_interval(klb_wnd_t* p_wnd, int64_t interval);
int64_t klbwnd_animation_get_interval(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_ANIMATION_H__

// end
