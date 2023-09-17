///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_static.h
/// @brief   klb window static,  静态文本框控件
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_STATIC_H__
#define __KLBWND_STATIC_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_static_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
}klbwnd_static_css_t;


typedef struct klbwnd_static_t_
{
    klbwnd_static_css_t*    p_css;          ///< 样式

    int                     index;          ///< 序号
    sds                     title;          ///< 标题
}klbwnd_static_t;


/// @brief init/quit/create
KLB_API void klbwnd_static_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_static_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_static_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_static_css_init(klbwnd_static_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_static_css_quit(klbwnd_static_css_t* p_css);
KLB_API void klbwnd_static_css_copy(klbwnd_static_css_t* p_dst, klbwnd_static_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_static_set_css(klb_wnd_t* p_wnd, klbwnd_static_css_t* p_css);


/// @brief 设置/获取 序号
KLB_API void klbwnd_static_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int klbwnd_static_get_index(klb_wnd_t* p_wnd);


/// @brief 设置/获取 标题
KLB_API void klbwnd_static_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_static_get_title(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_STATIC_H__
//end
