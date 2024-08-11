///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_util.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   其他和UI有相关性的定义或函数
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_UTIL_H__
#define __KLBUI_UTIL_H__

#include "klb_type.h"
#include "klbutil/klb_rect.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"

#if defined(__cplusplus)
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// define

//utf8 -> unicode
#define KLBUI_utf8_to_unicode(UTF8_, UNICODE_, LEN_) \
{ \
    int nnnn = 0; \
    uint8_t uft8_ch = UTF8_[nnnn++]; \
    if((uft8_ch & 0x80) == 0) \
    { \
        UNICODE_ = uft8_ch; \
        LEN_ = 1; \
    } \
    else if((uft8_ch & 0xe0) == 0xe0) \
    { \
        UNICODE_ = (uft8_ch & 0x1F) << 12; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 3; \
    } \
    else \
    { \
        UNICODE_ = (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 2; \
    } \
}

//////////////////////////////////////////////////////////////////////////
// 一些函数

/// @brief 矩形, 移除 外边距/内边距/边框
KLB_API void klbuiutil_remove_margin(klb_rect_t* p_rect, klbuicss_margin_t* p_margin);
KLB_API void klbuiutil_remove_padding(klb_rect_t* p_rect, klbuicss_padding_t* p_padding);
KLB_API void klbuiutil_remove_border(klb_rect_t* p_rect, klbuicss_border_t* p_border);


//////////////////////////////////////////////////////////////////////////
// 常用绘制图形函数

//////////////////////////////////////
// 矩形

/// @brief 绘制实体矩形
KLB_API void klbuiutil_draw_rectangle(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);


//////////////////////////////////////
// 三角形

/// @brief 绘制向左实体三角形
KLB_API void klbuiutil_draw_triangle_left(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制向右实体三角形
KLB_API void klbuiutil_draw_triangle_right(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制向上实体三角形
KLB_API void klbuiutil_draw_triangle_up(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制向下实体三角形
KLB_API void klbuiutil_draw_triangle_down(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);


//////////////////////////////////////////////////////////////////////////
// 实线

/// @brief 绘制实线符号 "+"
KLB_API void klbuiutil_draw_line_plus(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 "-"
KLB_API void klbuiutil_draw_line_minus(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 "X"
KLB_API void klbuiutil_draw_line_x(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 ">"
KLB_API void klbuiutil_draw_line_1(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 ">|"
KLB_API void klbuiutil_draw_line_2(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 "<"
KLB_API void klbuiutil_draw_line_3(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);

/// @brief 绘制实线符号 "|<"
KLB_API void klbuiutil_draw_line_4(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_UTIL_H__
//end
