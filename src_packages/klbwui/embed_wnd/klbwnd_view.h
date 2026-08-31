// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_view.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window view, 基础视图页面
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
///   \n [2026] 背景改走 klbuicss_background_t / klbwuicss_draw_background
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_VIEW_H__
#define __KLBWND_VIEW_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_view_attributes_t_
{
    klbuicss_background_t   background;    ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t       border;        ///< border-width, border-color
} klbwnd_view_attributes_t;


typedef struct klbwnd_view_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_view_attributes_t    normal;         ///< normal 常规状态参数
} klbwnd_view_css_t;


typedef struct klbwnd_view_t_
{
    klbwnd_view_css_t*      p_css;          ///< 样式

    int                     index;          ///< 序号
    sds                     title;          ///< 标题
} klbwnd_view_t;


/// @brief init/deinit/create
void klbwnd_view_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_view_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_view_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_view_css_init(klbwnd_view_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_view_css_deinit(klbwnd_view_css_t* p_css);
void klbwnd_view_css_copy(klbwnd_view_css_t* p_dst, klbwnd_view_css_t* p_src);


/// @brief set css
void klbwnd_view_set_css(klb_wnd_t* p_wnd, klbwnd_view_css_t* p_css);


/// @brief 设置/获取序号
void klbwnd_view_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_view_get_index(klb_wnd_t* p_wnd);


void klbwnd_view_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_view_get_title(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_VIEW_H__
// end