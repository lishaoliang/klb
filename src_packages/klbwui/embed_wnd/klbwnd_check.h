// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_check.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window check, 选择框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_CHECK_H__
#define __KLBWND_CHECK_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_check_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_check_attributes_t;


typedef struct klbwnd_check_css_t_
{
    klbuicss_margin_t           margin;             ///< 外边距
    klbuicss_padding_t          padding;            ///< 内边距

    klbwnd_check_attributes_t   normal;             ///< normal 常规状态参数
    klbwnd_check_attributes_t   focus;              ///< focus 聚焦状态参数
    klbwnd_check_attributes_t   disable;            ///< disable 不使能状态参数

    klbwnd_check_attributes_t   check_normal;       ///< check 选中常规状态参数
    klbwnd_check_attributes_t   check_focus;        ///< check 选中聚焦状态参数
    klbwnd_check_attributes_t   check_disable;      ///< check 选中不使能状态参数
} klbwnd_check_css_t;


typedef struct klbwnd_check_t_
{
    klbwnd_check_css_t*         p_css;          ///< 样式

    int                         index;          ///< 序号
    sds                         title;          ///< 标题
} klbwnd_check_t;


/// @brief init/deinit/create
void klbwnd_check_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_check_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_check_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_check_css_init(klbwnd_check_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_check_css_deinit(klbwnd_check_css_t* p_css);
void klbwnd_check_css_copy(klbwnd_check_css_t* p_dst, klbwnd_check_css_t* p_src);


/// @brief set css
void klbwnd_check_set_css(klb_wnd_t* p_wnd, klbwnd_check_css_t* p_css);


/// @brief 设置/获取序号
void klbwnd_check_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_check_get_index(klb_wnd_t* p_wnd);


/// @brief 设置/获取标题
void klbwnd_check_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_check_get_title(klb_wnd_t* p_wnd);


// @brief 设置/获取值
void klbwnd_check_set_value(klb_wnd_t* p_wnd, bool value);
bool klbwnd_check_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_CHECK_H__

// end
