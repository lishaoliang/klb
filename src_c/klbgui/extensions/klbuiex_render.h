///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_render.h
/// @brief   UI图形渲染模块
/// @version 0.1
/// @history 修改历史
///   \n [2025-6] 调整UI图形渲染功能到专门的内部渲染扩展模块中
///   \n [2025-6] 添加多图层绘制方案
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_RENDER_H__
#define __KLBUIEX_RENDER_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/extensions/klbuiex_redraw.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief GUI图形渲染模块
typedef struct klbuiex_render_t_ klbuiex_render_t;


/// @brief 注册/获取
int klbuiex_register_render(klb_gui_t* p_gui);
klbuiex_render_t* klbuiex_get_render(klb_gui_t* p_gui);


/// @brief 尝试 申请 并 附加 popup/msgbox 等图层
///  1. 流程中 尝试 调用主画布的 klb_canvas_malloc(..., KLB_CANVAS_LAYER_popup) 函数申请 popup 画布
///  2. 流程中 尝试 调用主画布的 klb_canvas_malloc(..., KLB_CANVAS_LAYER_msgbox) 函数申请 msgbox 画布
void klbuiex_render_try_attach_canvas(klbuiex_render_t* p_render, const klb_canvas_t* p_main_canvas);


/// @brief popup 窗口重绘与刷新
int klbuiex_render_redraw_and_refresh(klbuiex_render_t* p_render);


/// @brief 是否为多图层模式
bool klbuiex_render_is_multi_layer(klbuiex_render_t* p_render);

/// @brief 获取窗口对应的 画布
klb_canvas_t* klbuiex_render_get_popup_canvas(klbuiex_render_t* p_render, klb_wnd_t* p_top);
klb_canvas_t* klbuiex_render_get_msgbox_canvas(klbuiex_render_t* p_render);


/// @brief 弹出popu/msgbox窗口时, 若为多画布, 则需要更新画布位置
int klbuiex_render_popup_wnd(klbuiex_render_t* p_render, int idx, klb_wnd_t* p_top);
int klbuiex_render_msgbox_wnd(klbuiex_render_t* p_render, klb_wnd_t* p_top);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_RENDER_H__
//end
