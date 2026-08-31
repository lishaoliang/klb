// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_messagebox.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window messagebox, 标准消息弹出框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_MESSAGEBOX_H__
#define __KLBWND_MESSAGEBOX_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbwui/embed_wnd/klbwnd_static.h"
#include "klbwui/embed_wnd/klbwnd_button.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_MESSAGEBOX_close         0x0001      ///< 关闭
#define KLBWND_MESSAGEBOX_ok            0x0002      ///< 确认
#define KLBWND_MESSAGEBOX_cancel        0x0004      ///< 取消

#define KLBWND_MESSAGEBOX_title_h       32          ///< 默认标题栏高度


typedef struct klbwnd_messagebox_attributes_t_
{
    klbuicss_text_t         text;           ///< 标题 color, text-align
    klbuicss_font_t         font;           ///< 标题 font-size
    klbuicss_background_t   background;     ///< 客户区背景
    klbuicss_border_t       border;         ///< 边框
    klbuicss_background_t   titlebar;       ///< 标题栏背景
} klbwnd_messagebox_attributes_t;


typedef struct klbwnd_messagebox_css_t_
{
    klbuicss_margin_t                   margin;     ///< 外边距
    klbuicss_padding_t                  padding;    ///< 内边距

    klbwnd_messagebox_attributes_t      normal;     ///< normal 常规状态参数

    klbwnd_static_css_t                 css_sta;    ///< 正文静态框 css
    klbwnd_button_css_t                 css_btn;    ///< 按钮 css
} klbwnd_messagebox_css_t;


typedef struct klbwnd_messagebox_t_
{
    klbwnd_messagebox_css_t*    p_css;          ///< 样式

    sds                         title;          ///< 标题栏文本
    int                         value;          ///< 关闭 / 确认 / 取消
    int                         title_h;        ///< 标题栏高度

    klb_wnd_t*                  p_sta_txt;      ///< 正文: klbwnd_static_create
    klb_wnd_t*                  p_btn_ok;       ///< 确认: klbwnd_button_create
    klb_wnd_t*                  p_btn_cancel;   ///< 取消: klbwnd_button_create
} klbwnd_messagebox_t;


void klbwnd_messagebox_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_messagebox_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_messagebox_css_init(klbwnd_messagebox_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_messagebox_css_deinit(klbwnd_messagebox_css_t* p_css);
void klbwnd_messagebox_css_copy(klbwnd_messagebox_css_t* p_dst, klbwnd_messagebox_css_t* p_src);


void klbwnd_messagebox_set_css(klb_wnd_t* p_wnd, klbwnd_messagebox_css_t* p_css);


void klbwnd_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_messagebox_get_title(klb_wnd_t* p_wnd);


void klbwnd_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text);


int klbwnd_messagebox_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_MESSAGEBOX_H__

// end
