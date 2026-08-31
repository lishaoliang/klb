// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_group.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window group, 分组框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_GROUP_H__
#define __KLBWND_GROUP_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_group_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_group_attributes_t;


typedef struct klbwnd_group_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_group_attributes_t   normal;         ///< normal 常规状态参数
    klbwnd_group_attributes_t   focus;          ///< focus 聚焦状态参数
    klbwnd_group_attributes_t   disable;        ///< disable 不使能状态参数
} klbwnd_group_css_t;


typedef struct klbwnd_group_t_
{
    klbwnd_group_css_t*         p_css;          ///< 样式

    sds                         title;          ///< 标题
    sds                         value;          ///< 值
} klbwnd_group_t;


/// @brief init/deinit/create
void klbwnd_group_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_group_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_group_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_group_css_init(klbwnd_group_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_group_css_deinit(klbwnd_group_css_t* p_css);
void klbwnd_group_css_copy(klbwnd_group_css_t* p_dst, klbwnd_group_css_t* p_src);


/// @brief set css
void klbwnd_group_set_css(klb_wnd_t* p_wnd, klbwnd_group_css_t* p_css);


/// @brief 设置/获取标题
void klbwnd_group_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_group_get_title(klb_wnd_t* p_wnd);


/// @brief 设置/获取值
void klbwnd_group_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_group_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_GROUP_H__

// end
