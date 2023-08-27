///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_tip.h
/// @brief   "tip"扩展: 处理tip相关
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_TIP_H__
#define __KLBUIEX_TIP_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_canvas.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBUIEX_TIP_WIDTH_max   540
#define KLBUIEX_TIP_HEIGHT_max  64


/// @struct klbuiex_tip_t
/// @brief  tip扩展
typedef struct klbuiex_tip_t_
{
    klb_gui_t*      p_gui;              ///< gui

    klb_canvas_t*   p_canvas;           ///< tip 图层画布
    klb_wnd_t*      p_tip_wnd;          ///< tip 窗口

    klb_rect_t      rect_dst;           ///< 显示屏幕目标位置

    bool            is_redraw;          ///< 是否重绘
    bool            is_show;            ///< 是否显示

    klb_rect_t      rect_old;           ///< 旧区域
    bool            is_old_refresh;     ///< 是否需要刷新旧区域
}klbuiex_tip_t;


int klbuiex_register_tip(klb_gui_t* p_gui);
klbuiex_tip_t* klbuiex_get_tip(klb_gui_t* p_gui);


void klbuiex_tip_attach_canvas(klbuiex_tip_t* p_ex, klb_canvas_t* p_canvas);


/// @brief 是否需要重绘
bool klbuiex_tip_need_repaint(klbuiex_tip_t* p_ex);

/// @brief 更新了, 需要重新绘制
void klbuiex_tip_update(klbuiex_tip_t* p_ex, const char* p_tilte);

/// @brief 重绘
void klbuiex_tip_redraw(klbuiex_tip_t* p_ex);


/// @brief 获取刷新的一些参数
klb_canvas_t* klbuiex_tip_get_refresh(klbuiex_tip_t* p_ex, bool* p_is_show, klb_rect_t* p_dst, klb_rect_t* p_src);

/// @brief 设置tip标题
void klbuiex_tip_set_tilte(klbuiex_tip_t* p_ex, const char* p_title, int* p_out_w, int* p_out_h);

/// @brief 显示tip
void klbuiex_tip_show(klbuiex_tip_t* p_ex, bool show, int x, int y);


void klbuiex_tip_set_old(klbuiex_tip_t* p_ex, klb_rect_t* p_rect_old);
bool klbuiex_tip_get_old(klbuiex_tip_t* p_ex, klb_rect_t* p_rect_old);
void klbuiex_tip_set_old_refresh(klbuiex_tip_t* p_ex, bool refresh);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_TIP_H__
//end
