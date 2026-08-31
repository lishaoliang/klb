// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_demo.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window demo, 学习用最小控件 (固定显示 "kdemo")
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_DEMO_H__
#define __KLBWND_DEMO_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_demo_css_t_
{
    klbuicss_margin_t           margin;
    klbuicss_padding_t          padding;

    klbuicssex_attributes_t     normal;
} klbwnd_demo_css_t;


typedef struct klbwnd_demo_t_
{
    klbwnd_demo_css_t*          p_css;
} klbwnd_demo_t;


void klbwnd_demo_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_demo_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_demo_create(klb_gui_t* p_gui, int x, int y, int w, int h);


void klbwnd_demo_css_init(klbwnd_demo_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_demo_css_deinit(klbwnd_demo_css_t* p_css);
void klbwnd_demo_css_copy(klbwnd_demo_css_t* p_dst, klbwnd_demo_css_t* p_src);


void klbwnd_demo_set_css(klb_wnd_t* p_wnd, klbwnd_demo_css_t* p_css);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_DEMO_H__
// end