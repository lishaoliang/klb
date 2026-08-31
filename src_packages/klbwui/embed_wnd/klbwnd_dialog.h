// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_dialog.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window dialog, 带标题栏对话框容器
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DIALOG_H__
#define __KLBWND_DIALOG_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_DIALOG_title_h           32      ///< 默认标题栏高度


typedef struct klbwnd_dialog_attributes_t_
{
    klbuicss_text_t         text;           ///< 标题 color, text-align
    klbuicss_font_t         font;           ///< 标题 font-size
    klbuicss_background_t   background;     ///< 客户区背景
    klbuicss_border_t       border;         ///< 边框
    klbuicss_background_t   titlebar;       ///< 标题栏背景
} klbwnd_dialog_attributes_t;


typedef struct klbwnd_dialog_css_t_
{
    klbuicss_margin_t           margin;     ///< 外边距
    klbuicss_padding_t          padding;    ///< 内边距

    klbwnd_dialog_attributes_t  normal;     ///< normal 常规状态参数
} klbwnd_dialog_css_t;


typedef struct klbwnd_dialog_t_
{
    klbwnd_dialog_css_t*    p_css;          ///< 样式

    sds                     title;          ///< 标题栏文本
    sds                     value;          ///< 值
    int                     title_h;        ///< 标题栏高度
} klbwnd_dialog_t;


void klbwnd_dialog_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_dialog_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_dialog_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_dialog_css_init(klbwnd_dialog_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_dialog_css_deinit(klbwnd_dialog_css_t* p_css);
void klbwnd_dialog_css_copy(klbwnd_dialog_css_t* p_dst, klbwnd_dialog_css_t* p_src);


void klbwnd_dialog_set_css(klb_wnd_t* p_wnd, klbwnd_dialog_css_t* p_css);


void klbwnd_dialog_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_dialog_get_title(klb_wnd_t* p_wnd);


void klbwnd_dialog_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_dialog_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_DIALOG_H__

// end
