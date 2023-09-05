///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_num.h
/// @brief   klb window num: 数值编辑框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_NUM_H__
#define __KLBWND_NUM_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbthird/sds.h"
#include "klbgui/shwnd/klbshw_decimal.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_num_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_num_css_t;


typedef struct klbwnd_num_t_
{
    klbwnd_num_css_t*       p_css;          ///< 样式

    int                     index;          ///< 序号
    sds                     title;          ///< 标题

    int                     value;          ///< 值
    int                     min;
    int                     max;

    klb_wnd_t*              p_decimal;      ///< 10进制输入软键盘
}klbwnd_num_t;


/// @brief init/quit/create
KLB_API void klbwnd_num_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_num_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_num_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_num_css_init(klbwnd_num_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_num_css_quit(klbwnd_num_css_t* p_css);


/// @brief set css
KLB_API void klbwnd_num_set_css(klb_wnd_t* p_wnd, klbwnd_num_css_t* p_css);


/// @brief 设置/获取序号
KLB_API void klbwnd_num_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int  klbwnd_num_get_index(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_num_set_value(klb_wnd_t* p_wnd, int value);
KLB_API int klbwnd_num_get_value(klb_wnd_t* p_wnd);


/// @brief 设置值范围
KLB_API void klbwnd_num_set_ranges(klb_wnd_t* p_wnd, int min, int max);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_NUM_H__
//end
