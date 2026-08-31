// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_qrcode.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window qrcode, 二维码
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_QRCODE_H__
#define __KLBWND_QRCODE_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_qrcode_attributes_t_
{
    klbuicss_background_t   background;     ///< background-color / background-image
    klbuicss_border_t       border;         ///< border-width, border-color
} klbwnd_qrcode_attributes_t;


typedef struct klbwnd_qrcode_css_t_
{
    klbuicss_margin_t           margin;     ///< 外边距
    klbuicss_padding_t          padding;    ///< 内边距

    klbwnd_qrcode_attributes_t  normal;     ///< normal 常规状态参数
} klbwnd_qrcode_css_t;


typedef struct klbwnd_qrcode_t_
{
    klbwnd_qrcode_css_t*    p_css;          ///< 样式

    sds                     title;          ///< 标题 (NULL 表示未设置)
    sds                     value;          ///< 编码文本 (NULL 表示未设置)
    void*                   p_qrcode;       ///< QRcode*; 无 qrencode 时为 NULL
} klbwnd_qrcode_t;


void klbwnd_qrcode_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_qrcode_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_qrcode_css_init(klbwnd_qrcode_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_qrcode_css_deinit(klbwnd_qrcode_css_t* p_css);
void klbwnd_qrcode_css_copy(klbwnd_qrcode_css_t* p_dst, klbwnd_qrcode_css_t* p_src);


void klbwnd_qrcode_set_css(klb_wnd_t* p_wnd, klbwnd_qrcode_css_t* p_css);


void klbwnd_qrcode_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_qrcode_get_title(klb_wnd_t* p_wnd);


void klbwnd_qrcode_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_qrcode_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_QRCODE_H__

// end
