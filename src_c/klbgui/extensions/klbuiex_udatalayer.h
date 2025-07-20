///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_udatalayer.h
/// @brief   user data layer canvas 用户自定义图层
///           1. 类似TIP图层, 在窗口图层之上, TIP层之下
///           2. 许可用户 自行设计 此图层
///           3. 图层的 窗口事件(支持部分) 由本模块处理
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_UDATALAYER_H__
#define __KLBUIEX_UDATALAYER_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_canvas.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbuiex_udatalayer_t
/// @brief  user data layer canvas 用户自定义图层
typedef struct klbuiex_udatalayer_t_ klbuiex_udatalayer_t;


/// @brief 注册/获取
int klbuiex_register_udatalayer(klb_gui_t* p_gui);
klbuiex_udatalayer_t* klbuiex_get_udatalayer(klb_gui_t* p_gui);


/// @brief 尝试 申请 并 附加 用户自定义 画布
///  流程中 尝试 调用主画布的 klb_canvas_malloc(..., KLB_CANVAS_LAYER_udata) 函数申请画布
void klbuiex_udatalayer_try_attach_canvas(klbuiex_udatalayer_t* p_ex, const klb_canvas_t* p_main_canvas);


/// @brief 是否有 用户自定义 画布
bool klbuiex_udatalayer_has_canvas(klbuiex_udatalayer_t* p_ex);

/// @brief 获取 用户自定义 画布
klb_canvas_t* klbuiex_udatalayer_get_canvas(klbuiex_udatalayer_t* p_ex);


/// @brief 设置/获取 计时 时间间隔, 毫秒ms
void klbuiex_udatalayer_set_interval(klbuiex_udatalayer_t* p_ex, int64_t interval);
int64_t klbuiex_udatalayer_get_interval(klbuiex_udatalayer_t* p_ex);


/// @brief 绑定 用户自定义窗口
void klbuiex_udatalayer_bind_wnd(klbuiex_udatalayer_t* p_ex, klb_wnd_t* p_top);
klb_wnd_t* klbuiex_udatalayer_get_wnd(klbuiex_udatalayer_t* p_ex);


/// @brief 获取是否 显示
bool klbuiex_udatalayer_is_show(klbuiex_udatalayer_t* p_ex);

/// @brief 设置 是否显示
void klbuiex_udatalayer_show(klbuiex_udatalayer_t* p_ex, bool show);

/// @brief 移动位置
void klbuiex_udatalayer_move(klbuiex_udatalayer_t* p_ex, int x, int y);

/// @brief 设置需要重新绘制
void klbuiex_udatalayer_set_redraw(klbuiex_udatalayer_t* p_ex);

/// @brief 窗口是否 属于绑定的窗口
bool klbuiex_udatalayer_wnd_in_bind(klbuiex_udatalayer_t* p_ex, klb_wnd_t* p_wnd);


/// @brief 重新绘制
bool klbuiex_udatalayer_redraw(klbuiex_udatalayer_t* p_ex);


/// @brief 设置/获取 脏矩形区域
void klbuiex_udatalayer_set_dirty(klbuiex_udatalayer_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect);
bool klbuiex_udatalayer_get_dirty(klbuiex_udatalayer_t* p_ex, klb_rect_t* p_out_diry_rect);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_UDATALAYER_H__
//end
