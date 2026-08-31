// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_progress.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window progress, 进度条
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_PROGRESS_H__
#define __KLBWND_PROGRESS_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_progress_attributes_t_
{
    klbuicss_text_t                 text;           ///< color, text-align; color 亦作进度条前景
    klbuicss_font_t                 font;           ///< font-size
    klbuicss_background_t           background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t               border;         ///< border-width, border-color
} klbwnd_progress_attributes_t;


typedef struct klbwnd_progress_css_t_
{
    klbuicss_margin_t               margin;         ///< 外边距
    klbuicss_padding_t              padding;        ///< 内边距

    klbwnd_progress_attributes_t    normal;         ///< normal 常规状态参数
    klbwnd_progress_attributes_t    focus;          ///< focus 聚焦状态参数
    klbwnd_progress_attributes_t    disable;        ///< disable 不使能状态参数
} klbwnd_progress_css_t;


typedef struct klbwnd_progress_t_
{
    klbwnd_progress_css_t*          p_css;          ///< 样式

    sds                             title;          ///< 标题
    int                             value;          ///< 进度值, 0 ~ 100
} klbwnd_progress_t;


/// @brief init/deinit/create
void klbwnd_progress_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_progress_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_progress_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_progress_css_init(klbwnd_progress_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_progress_css_deinit(klbwnd_progress_css_t* p_css);
void klbwnd_progress_css_copy(klbwnd_progress_css_t* p_dst, klbwnd_progress_css_t* p_src);


/// @brief set css
void klbwnd_progress_set_css(klb_wnd_t* p_wnd, klbwnd_progress_css_t* p_css);


/// @brief 设置/获取标题
void klbwnd_progress_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_progress_get_title(klb_wnd_t* p_wnd);


/// @brief 设置/获取进度值
void klbwnd_progress_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_progress_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_PROGRESS_H__

// end
