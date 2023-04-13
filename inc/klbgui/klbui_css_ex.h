///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_css_ex.h
/// @brief   UI组件 CSS属性操作帮助函数
///   参考 CSS3 : https://www.w3school.com.cn/css/index.asp
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CSS_EX_H__
#define __KLBUI_CSS_EX_H__


#include "klb_type.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klb_wnd.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbui_default_t_ klbui_default_t;


#define KLBUI_CSSEX_get     0                   ///< get method
#define KLBUI_CSSEX_set     1                   ///< set method


/// @struct klbuicssex_attribute_t
/// @brief  CSS属性集合: "normal", "focus", "disable"
///   UI组件 参考属性集合
typedef struct klbuicssex_attributes_t_
{
    klbuicss_text_t             text;           ///< 文本属性
    klbuicss_font_t             font;           ///< 字体属性
    klbuicss_background_t       background;     ///< 背景属性
    klbuicss_border_t           border;         ///< 边框属性
}klbuicssex_attributes_t;


//////////////////////////////////////////////////////////////////////////
// 默认属性初始化/退出
KLB_API void klbuicssex_attributes_init(klbuicssex_attributes_t* p_attr, const klbuicssex_attributes_t* p_src);
KLB_API void klbuicssex_attributes_quit(klbuicssex_attributes_t* p_attr);


//////////////////////////////////////////////////////////////////////////
// 公共 单属性操作

KLB_API void klbuicssex_attribute_sds(sds* p_sds, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);
KLB_API void klbuicssex_attribute_int(int* p_int, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);
KLB_API void klbuicssex_attribute_color(uint32_t* p_color, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);
KLB_API void klbuicssex_attribute_image(sds* p_image, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// CSS基础

// 显隐 visibility
KLB_API void klbuicssex_visibility(klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 外边距

// 外边距 margin
KLB_API void klbuicssex_margin(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 外边距-上 margin-top
KLB_API void klbuicssex_margin_top(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 外边距-右 margin-right
KLB_API void klbuicssex_margin_right(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 外边距-下 margin-bottom
KLB_API void klbuicssex_margin_bottom(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 外边距-上 margin-left
KLB_API void klbuicssex_margin_left(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 内边距

// 内边距 padding
KLB_API void klbuicssex_padding(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 内边距-上 padding-top
KLB_API void klbuicssex_padding_top(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 内边距-右 padding-right
KLB_API void klbuicssex_padding_right(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 内边距-下 padding-bottom
KLB_API void klbuicssex_padding_bottom(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 内边距-上 padding-left
KLB_API void klbuicssex_padding_left(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 文本

// 文本颜色 color
KLB_API void klbuicssex_text_color(klbuicss_text_t* p_text, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 文本对齐 text-align
KLB_API void klbuicssex_text_align(klbuicss_text_t* p_text, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 字体

// 斜体 font-style
KLB_API void klbuicssex_font_style(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 字体粗细 font-weight
KLB_API void klbuicssex_font_weight(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 字体大小 font-size
KLB_API void klbuicssex_font_size(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 背景

// 背景色 background-color
KLB_API void klbuicssex_background_color(klbuicss_background_t* p_background, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 背景图片 background-image
KLB_API void klbuicssex_background_image(klbuicss_background_t* p_background, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 边框

// 边框类型 border-style
KLB_API void klbuicssex_border_style(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 边框宽度 border-width
KLB_API void klbuicssex_border_width(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 边框颜色 border-color
KLB_API void klbuicssex_border_color(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

// 圆角边框 border-radius
KLB_API void klbuicssex_border_radius(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);



//////////////////////////////////////////////////////////////////////////
// 参考绘制


// 绘制边框
KLB_API void klbuicssex_draw_border(klb_wnd_t* p_wnd, klb_rect_t* p_rect, klbuicss_border_t* p_border);

// 绘制文本
KLB_API void klbuicssex_draw_text(klb_wnd_t* p_wnd, sds txt, klb_rect_t* p_rect, klbuicss_border_t* p_border, klbuicss_padding_t* p_padding, klbuicss_text_t* p_css_text, klbuicss_font_t* p_css_font);


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_CSS_EX_H__
//end
