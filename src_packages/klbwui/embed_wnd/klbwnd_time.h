// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_time.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window time, 时间编辑框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TIME_H__
#define __KLBWND_TIME_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_TIME_idx_hour            0
#define KLBWND_TIME_idx_minute          1
#define KLBWND_TIME_idx_second          2


typedef struct klbwnd_time_region_t_
{
    klb_rect_t                  rect;           ///< 绘制 / 点击区域
    sds                         title;          ///< 显示文本
} klbwnd_time_region_t;


typedef struct klbwnd_time_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_time_attributes_t;


typedef struct klbwnd_time_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbwnd_time_attributes_t    normal;         ///< normal 常规状态参数
    klbwnd_time_attributes_t    focus;          ///< focus 聚焦状态参数
    klbwnd_time_attributes_t    disable;        ///< disable 不使能状态参数
} klbwnd_time_css_t;


typedef struct klbwnd_time_t_
{
    klbwnd_time_css_t*          p_css;          ///< 样式

    klb_wnd_t*                  p_decimal;      ///< 10 进制输入菜单: klbui_shwnd_get_decimal_menu

    klbwnd_time_region_t        hms[3];         ///< 时分秒
    klbwnd_time_region_t        colon[2];       ///< 冒号
    klbwnd_time_region_t        apm;            ///< 12 小时制 AM/PM

    int                         sel_idx;        ///< 选中序号

    int                         hour;           ///< 小时 0-23
    int                         minute;         ///< 分钟 0-59
    int                         second;         ///< 秒钟 0-59
} klbwnd_time_t;


/// @brief init/deinit/create
void klbwnd_time_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_time_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_time_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_time_css_init(klbwnd_time_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_time_css_deinit(klbwnd_time_css_t* p_css);
void klbwnd_time_css_copy(klbwnd_time_css_t* p_dst, klbwnd_time_css_t* p_src);


/// @brief set css
void klbwnd_time_set_css(klb_wnd_t* p_wnd, klbwnd_time_css_t* p_css);


/// @brief 设置/获取时分秒
void klbwnd_time_set_value(klb_wnd_t* p_wnd, int hour, int minute, int second);
void klbwnd_time_get_value(klb_wnd_t* p_wnd, int* p_hour, int* p_minute, int* p_second);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_TIME_H__

// end
