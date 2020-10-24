///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_color.h
/// @brief   颜色相关
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_COLOR_H__
#define __KLB_COLOR_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_color_fmt_e
/// @brief 像素颜色格式
///  与"./klb/src/kimage/kcolor/color.go"保持一致
typedef enum klb_color_fmt_e_
{
    KLB_COLOR_FMT_NULL      = 0,
    KLB_COLOR_FMT_ARGB1555  = 1,
    KLB_COLOR_FMT_ARGB4444  = 2,
    KLB_COLOR_FMT_ARGB8888  = 3,

    KLB_COLOR_FMT_RGBA1555  = 11,
    KLB_COLOR_FMT_RGBA4444  = 12,
    KLB_COLOR_FMT_RGBA8888  = 13,
}klb_color_fmt_e;


/// @def   KLB_ARGB8888
/// @brief 颜色宏
#ifndef KLB_ARGB8888
#define KLB_ARGB8888(a_, r_, g_, b_)                    \
                ((((uint32_t)(a_) & 0xFF) << 24) |      \
                (((uint32_t)(r_) & 0xFF) << 16) |       \
                (((uint32_t)(g_) & 0xFF) << 8) |        \
                ((uint32_t)(b_) & 0xFF))
#endif


#ifndef KLB_COLOR_TO
#define KLB_COLOR_TO(color_, color_fmt_)    klb_color_argb8888_to((color_), (color_fmt_))
#endif


/// @brief 获取颜色BPP
/// @param [in] color_fmt       颜色格式
/// @return int 1, 2, 3, 4
int klb_color_bpp(int color_fmt);


uint32_t klb_color_argb8888_to(uint32_t argb8888, int color_fmt);


uint32_t klb_color_rgba8888_to(uint32_t rgba8888, int color_fmt);


#ifdef __cplusplus
}
#endif

#endif // __KLB_COLOR_H__
//end
