// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_num.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window num, 数值编辑框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_NUM_H__
#define __KLBWND_NUM_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_num_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_num_attributes_t;


typedef struct klbwnd_num_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_num_attributes_t     normal;         ///< normal 常规状态参数
    klbwnd_num_attributes_t     focus;          ///< focus 聚焦状态参数
    klbwnd_num_attributes_t     disable;        ///< disable 不使能状态参数
} klbwnd_num_css_t;


typedef struct klbwnd_num_t_
{
    klbwnd_num_css_t*           p_css;          ///< 样式

    int                         value;          ///< 当前值
    int                         min;            ///< 最小值
    int                         max;            ///< 最大值

    klb_wnd_t*                  p_decimal;      ///< 10 进制输入菜单: klbui_shwnd_get_decimal_menu
} klbwnd_num_t;


/// @brief init/deinit/create
void klbwnd_num_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_num_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_num_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_num_css_init(klbwnd_num_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_num_css_deinit(klbwnd_num_css_t* p_css);
void klbwnd_num_css_copy(klbwnd_num_css_t* p_dst, klbwnd_num_css_t* p_src);


/// @brief set css
void klbwnd_num_set_css(klb_wnd_t* p_wnd, klbwnd_num_css_t* p_css);


/// @brief 设置/获取值
void klbwnd_num_set_value(klb_wnd_t* p_wnd, int value);
int klbwnd_num_get_value(klb_wnd_t* p_wnd);


/// @brief 设置范围
void klbwnd_num_set_ranges(klb_wnd_t* p_wnd, int min, int max);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_NUM_H__

// end
