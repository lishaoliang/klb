// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_picture.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window picture, 简易图片显示
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
///   \n [2026] 背景改走 klbuicss_background_t / klbwuicss_draw_background
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_PICTURE_H__
#define __KLBWND_PICTURE_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_picture_attributes_t_
{
    klbuicss_background_t   background;    ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t       border;        ///< border-width, border-color
} klbwnd_picture_attributes_t;


typedef struct klbwnd_picture_css_t_
{
    klbuicss_margin_t           margin;     ///< 外边距
    klbuicss_padding_t          padding;    ///< 内边距

    klbwnd_picture_attributes_t normal;     ///< normal 常规状态参数
} klbwnd_picture_css_t;


typedef struct klbwnd_picture_t_
{
    klbwnd_picture_css_t*   p_css;          ///< 样式

    sds                     title;          ///< 标题 (NULL 表示未设置)
    sds                     value;          ///< 值 (NULL 表示未设置)
} klbwnd_picture_t;


/// @brief init/deinit/create
void klbwnd_picture_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_picture_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_picture_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_picture_css_init(klbwnd_picture_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_picture_css_deinit(klbwnd_picture_css_t* p_css);
void klbwnd_picture_css_copy(klbwnd_picture_css_t* p_dst, klbwnd_picture_css_t* p_src);


/// @brief set css
void klbwnd_picture_set_css(klb_wnd_t* p_wnd, klbwnd_picture_css_t* p_css);


void klbwnd_picture_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_picture_get_title(klb_wnd_t* p_wnd);


void klbwnd_picture_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_picture_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_PICTURE_H__
//end
