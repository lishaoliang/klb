// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_div.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window div, 仅用于排版布局, 无绘图
/// @version 0.1
/// @history 修改历史
///   \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DIV_H__
#define __KLBWND_DIV_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_div_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距
} klbwnd_div_css_t;


typedef struct klbwnd_div_t_
{
    klbwnd_div_css_t*           p_css;          ///< 样式

    int                         index;          ///< 序号
    sds                         title;          ///< 标题
} klbwnd_div_t;


/// @brief init/deinit/create
void klbwnd_div_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_div_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_div_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_div_css_init(klbwnd_div_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_div_css_deinit(klbwnd_div_css_t* p_css);
void klbwnd_div_css_copy(klbwnd_div_css_t* p_dst, klbwnd_div_css_t* p_src);


/// @brief set css
void klbwnd_div_set_css(klb_wnd_t* p_wnd, klbwnd_div_css_t* p_css);


/// @brief 设置/获取序号
void klbwnd_div_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_div_get_index(klb_wnd_t* p_wnd);


/// @brief set/get title
void klbwnd_div_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_div_get_title(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_DIV_H__

// end
