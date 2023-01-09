///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_default.h
/// @brief   gui部分的默认值
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_DEFAULT_H__
#define __KLBUI_DEFAULT_H__


#include "klb_type.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;

/// @struct klbui_default_t
/// @brief  gui部分的默认值
typedef struct klbui_default_t_
{
    // 文本 text
    uint32_t    text_color;                 // 文本颜色


    // 字体 font
    int         font_size;                  // 字体大小


    // 背景 background
    uint32_t    background_color;           // 背景颜色


    // 边框 border
    int         border_width;               // 边框宽度
    uint32_t    border_color;               // 边框颜色
    int         border_radius;              // 边框圆角

}klbui_default_t;


/// @brief 获取标准控件的默认值指针
KLB_API const klbui_default_t* klb_gui_get_std_default(klb_gui_t* p_gui);


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_DEFAULT_H__
//end
