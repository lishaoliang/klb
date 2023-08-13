///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_messagebox.h
/// @brief   klb window messagebox : 标准消息弹出框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_MESSAGEBOX_H__
#define __KLBWND_MESSAGEBOX_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbgui/wnd/klbwnd_button.h"
#include "klbgui/wnd/klbwnd_static.h"
#include "klbgui/wnd/klbwnd_picture.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_MESSAGEBOX_ok            0
#define KLBWND_MESSAGEBOX_cancel        1


typedef struct klbwnd_messagebox_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    klbwnd_picture_css_t    css_pic;        ///< 图片框css
    klbwnd_static_css_t     css_sta;        ///< 静态框css
    klbwnd_button_css_t     css_btn;        ///< 按钮的css
}klbwnd_messagebox_css_t;


typedef struct klbwnd_messagebox_t_
{
    klbwnd_messagebox_css_t* p_css;         ///< 样式

    sds                      title;         ///< 标题
    int                      value;         ///< 值

    klb_wnd_t*               p_pic_image;   ///< 图片框: klbwnd_picture_create
    klb_wnd_t*               p_sta_txt;     ///< 文本框: klbwnd_static_create

    klb_wnd_t*               p_btn_ok;      ///< 确认按钮: klbwnd_button_create
    klb_wnd_t*               p_btn_cancel;  ///< 取消按钮:  klbwnd_button_create
}klbwnd_messagebox_t;


/// @brief init/quit/create
void klbwnd_messagebox_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_messagebox_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_messagebox_css_init(klbwnd_messagebox_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_messagebox_css_quit(klbwnd_messagebox_css_t* p_css);


/// @brief set css
void klbwnd_messagebox_set_css(klb_wnd_t* p_wnd, klbwnd_messagebox_css_t* p_css);


/// @brief set/get title
void klbwnd_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_messagebox_get_title(klb_wnd_t* p_wnd);


/// @brief set body 图片
void klbwnd_messagebox_set_body_image(klb_wnd_t* p_wnd, const char* p_image);


/// @brief set body text
void klbwnd_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text);


/// @brief get value
int klbwnd_messagebox_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_MESSAGEBOX_H__
//end
