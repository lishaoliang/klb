///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_time.h
/// @brief   klb window time
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TIME_H__
#define __KLBWND_TIME_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/shwnd/klbshw_decimal.h"
#include "klbutil/klb_sds.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_TIME_idx_hour            0
#define KLBWND_TIME_idx_minute          1
#define KLBWND_TIME_idx_second          2


typedef struct klbwnd_time_region_t_
{
    klb_rect_t              rect;
    sds                     title;
}klbwnd_time_region_t;

typedef struct klbwnd_time_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_time_css_t;


typedef struct klbwnd_time_t_
{
    klbwnd_time_css_t*      p_css;          ///< 样式

    klb_wnd_t*              p_decimal;      ///< 10进制软件键盘: klbui_shwnd_get_decimal

    klbwnd_time_region_t    hms[3];         ///< 时分秒
    klbwnd_time_region_t    colon[2];       ///< 冒号:
    klbwnd_time_region_t    apm;            ///< 12小时制: AM/PM

    int                     sel_idx;        ///< 选中序号

    // 值
    struct
    {
        int                 hour;           ///< 小时
        int                 minute;         ///< 分钟
        int                 second;         ///< 秒钟
    };
}klbwnd_time_t;


/// @brief init/quit/create
KLB_API void klbwnd_time_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_time_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_time_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_time_css_init(klbwnd_time_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_time_css_quit(klbwnd_time_css_t* p_css);
KLB_API void klbwnd_time_css_copy(klbwnd_time_css_t* p_dst, klbwnd_time_css_t* p_src);

/// @brief set css
KLB_API void klbwnd_time_set_css(klb_wnd_t* p_wnd, klbwnd_time_css_t* p_css);


/// @brief set/get value
KLB_API void klbwnd_time_set_value(klb_wnd_t* p_wnd, int hour, int minute, int second);
KLB_API void klbwnd_time_get_value(klb_wnd_t* p_wnd, int* p_hour, int* p_minute, int* p_second);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_TIME_H__
//end
