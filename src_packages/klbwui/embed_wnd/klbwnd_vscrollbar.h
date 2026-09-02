// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_vscrollbar.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   垂直滚动条运行基础 (embed_wnd 内部; 不注册 Lua type)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_VSCROLLBAR_H__
#define __KLBWND_VSCROLLBAR_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbwui/embed_wnd/klbwnd_btnex.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_vscrollbar_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_vscrollbar_attributes_t;


typedef struct klbwnd_vscrollbar_css_t_
{
    klbuicss_margin_t                   margin;         ///< 外边距
    klbuicss_padding_t                  padding;        ///< 内边距

    klbwnd_vscrollbar_attributes_t      normal;         ///< normal 常规状态参数
    klbwnd_vscrollbar_attributes_t      focus;          ///< focus 聚焦状态参数
    klbwnd_vscrollbar_attributes_t      disable;        ///< disable 不使能状态参数

    klbwnd_btnex_css_t                  css_btn;        ///< 子按钮样式 (triangle_up/down + rectangle)
} klbwnd_vscrollbar_css_t;


typedef struct klbwnd_vscrollbar_t_
{
    klbwnd_vscrollbar_css_t*    p_css;          ///< 样式

    klb_wnd_t*                  p_up;           ///< 上按钮
    klb_wnd_t*                  p_down;         ///< 下按钮
    klb_wnd_t*                  p_middle;       ///< 滑块

    int                         min;            ///< 最小值
    int                         max;            ///< 最大值
    int                         step;           ///< 步进
    int                         value;          ///< 当前值
    int                         h_middle;       ///< 滑块高度
    bool                        enable_mousewheel;  ///< 是否响应鼠标滚轮
} klbwnd_vscrollbar_t;


/// @brief init/deinit/create
void klbwnd_vscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_vscrollbar_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_vscrollbar_css_init(klbwnd_vscrollbar_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_vscrollbar_css_deinit(klbwnd_vscrollbar_css_t* p_css);
void klbwnd_vscrollbar_css_copy(klbwnd_vscrollbar_css_t* p_dst, klbwnd_vscrollbar_css_t* p_src);


/// @brief set css
void klbwnd_vscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_vscrollbar_css_t* p_css);


/// @brief 设置/获取值
void klbwnd_vscrollbar_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_vscrollbar_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
void klbwnd_vscrollbar_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step);


/// @brief 鼠标滚轮
int klbwnd_vscrollbar_enable_mousewheel(klb_wnd_t* p_wnd, bool b);


/// @brief 步进
int klbwnd_vscrollbar_up(klb_wnd_t* p_wnd, int v);
int klbwnd_vscrollbar_down(klb_wnd_t* p_wnd, int v);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_VSCROLLBAR_H__

// end
