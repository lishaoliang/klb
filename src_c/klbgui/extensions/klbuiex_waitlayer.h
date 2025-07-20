///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_waitlayer.h
/// @brief   wait layer canvas 等待图层
///           1. 类似TIP图层, 在窗口图层之上, TIP层之下
///           2. 当需要 阻塞UI 显示 UI处于等待 状态 时, 显示的图层
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_WAITLAYER_H__
#define __KLBUIEX_WAITLAYER_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_canvas.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbuiex_waitlayer_t
/// @brief  wait layer canvas 等待图层
typedef struct klbuiex_waitlayer_t_ klbuiex_waitlayer_t;


/// @brief 注册/获取
int klbuiex_register_waitlayer(klb_gui_t* p_gui);
klbuiex_waitlayer_t* klbuiex_get_waitlayer(klb_gui_t* p_gui);


/// @brief 尝试 申请 并 附加 等待 画布
///  流程中 尝试 调用主画布的 klb_canvas_malloc(..., KLB_CANVAS_LAYER_wait) 函数申请画布
void klbuiex_waitlayer_try_attach_canvas(klbuiex_waitlayer_t* p_ex, const klb_canvas_t* p_main_canvas);


/// @brief 是否有 等待 画布
bool klbuiex_waitlayer_has_canvas(klbuiex_waitlayer_t* p_ex);

/// @brief 获取 等待 画布
klb_canvas_t* klbuiex_waitlayer_get_canvas(klbuiex_waitlayer_t* p_ex);


/// @brief 设置/获取 计时 时间间隔, 毫秒ms
void klbuiex_waitlayer_set_interval(klbuiex_waitlayer_t* p_ex, int64_t interval);
int64_t klbuiex_waitlayer_get_interval(klbuiex_waitlayer_t* p_ex);

/// @brief 绑定 等待 窗口
///  窗口 由用户 自行定义设计; 不是内置的
void klbuiex_waitlayer_bind_wnd(klbuiex_waitlayer_t* p_ex, klb_wnd_t* p_top);
klb_wnd_t* klbuiex_waitlayer_get_wnd(klbuiex_waitlayer_t* p_ex);


/// @brief 获取是否 显示
bool klbuiex_waitlayer_is_show(klbuiex_waitlayer_t* p_ex);


/// @brief 设置 是否显示
void klbuiex_waitlayer_show(klbuiex_waitlayer_t* p_ex, bool show);


/// @brief 移动位置
void klbuiex_waitlayer_move(klbuiex_waitlayer_t* p_ex, int x, int y);

/// @brief 设置需要重新绘制
void klbuiex_waitlayer_set_redraw(klbuiex_waitlayer_t* p_ex);

/// @brief 窗口是否 属于绑定的窗口
bool klbuiex_waitlayer_wnd_in_bind(klbuiex_waitlayer_t* p_ex, klb_wnd_t* p_wnd);

/// @brief 重新绘制
bool klbuiex_waitlayer_redraw(klbuiex_waitlayer_t* p_ex);


/// @brief 设置/获取 脏矩形区域
void klbuiex_waitlayer_set_dirty(klbuiex_waitlayer_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect);
bool klbuiex_waitlayer_get_dirty(klbuiex_waitlayer_t* p_ex, klb_rect_t* p_out_diry_rect);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_WAITLAYER_H__
//end
