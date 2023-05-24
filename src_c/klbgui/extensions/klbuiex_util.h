///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_util.h
/// @brief   "util"扩展: 一些没法归类 或 简单的 就放这里吧
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_UTIL_H__
#define __KLBUIEX_UTIL_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbuiex_util_t_
{
    klb_gui_t*      p_gui;

    // 日期时间格式
    struct
    {
        int         datefmt;    ///< 时间格式: KLBUI_DATE_YMD1
        int         timefmt;    ///< 日期格式: KLBUI_TIME_24H
    };

    // 当前鼠标位置
    // 用于重新获取当前焦点等
    klb_point_t     mouse_pt;

    // CSS
    klb_map_t       css_map;    ///< css属性函数表
}klbuiex_util_t;


int klbuiex_register_util(klb_gui_t* p_gui);
klbuiex_util_t* klbuiex_get_util(klb_gui_t* p_gui);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_UTIL_H__
//end
