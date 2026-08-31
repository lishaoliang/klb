// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_ticker.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window ticker, 定时刷新控件 (默认隐藏, 无绘图)
/// @version 0.1
/// @history 修改历史
///   \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TICKER_H__
#define __KLBWND_TICKER_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_ticker_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距
} klbwnd_ticker_css_t;


typedef struct klbwnd_ticker_t_
{
    klbwnd_ticker_css_t*        p_css;          ///< 样式

    int                         index;          ///< 序号

    bool                        enable;         ///< 是否启用
    bool                        enable_default; ///< 是否启用的默认值

    int64_t                     tick_count;     ///< 计时, 系统滴答 (单位: 毫秒)
    int64_t                     tc_interval;    ///< 触发响应的间隔 (单位: 毫秒)
} klbwnd_ticker_t;


/// @brief init/deinit/create
void klbwnd_ticker_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_ticker_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_ticker_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_ticker_css_init(klbwnd_ticker_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_ticker_css_deinit(klbwnd_ticker_css_t* p_css);
void klbwnd_ticker_css_copy(klbwnd_ticker_css_t* p_dst, klbwnd_ticker_css_t* p_src);


/// @brief set css
void klbwnd_ticker_set_css(klb_wnd_t* p_wnd, klbwnd_ticker_css_t* p_css);


/// @brief 设置/获取序号
void klbwnd_ticker_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_ticker_get_index(klb_wnd_t* p_wnd);


/// @brief 设置/获取 是否使能
void klbwnd_ticker_set_enable(klb_wnd_t* p_wnd, bool is_enable);
bool klbwnd_ticker_get_enable(klb_wnd_t* p_wnd);


/// @brief 设置/获取 是否默认使能
void klbwnd_ticker_set_enable_default(klb_wnd_t* p_wnd, bool is_enable);
bool klbwnd_ticker_get_enable_default(klb_wnd_t* p_wnd);


/// @brief 设置/获取 ticker-interval (单位: 毫秒)
void klbwnd_ticker_set_interval(klb_wnd_t* p_wnd, int64_t interval);
int64_t klbwnd_ticker_get_interval(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_TICKER_H__

// end
