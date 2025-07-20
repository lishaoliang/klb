///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_tip.h
/// @brief   "tip"扩展: 处理tip相关
/// @version 0.3
/// @history 修改历史
///   \n [2025-7] TIP模块机制重写
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


/// @struct klbuiex_tip_t
/// @brief  tip扩展
typedef struct klbuiex_tip_t_ klbuiex_tip_t;


/// @brief TIP扩展 模块
int klbuiex_register_tip(klb_gui_t* p_gui);
klbuiex_tip_t* klbuiex_get_tip(klb_gui_t* p_gui);


/// @brief 尝试 申请 并 附加 TIP画布
///  流程中 尝试 调用主画布的 klb_canvas_malloc(..., KLB_CANVAS_LAYER_tip) 函数申请TIP画布
void klbuiex_tip_try_attach_canvas(klbuiex_tip_t* p_ex, const klb_canvas_t* p_main_canvas);


/// @brief 是否有TIP画布
bool klbuiex_tip_has_canvas(klbuiex_tip_t* p_ex);

/// @brief 获取tip画布
klb_canvas_t* klbuiex_tip_get_canvas(klbuiex_tip_t* p_ex);

/// @brief 获取是否 显示
bool klbuiex_tip_is_show(klbuiex_tip_t* p_ex);

/// @brief 设置 是否显示
void klbuiex_tip_show(klbuiex_tip_t* p_ex, bool show);

/// @brief 设置标题
void klbuiex_tip_set_title(klbuiex_tip_t* p_ex, const char* p_title);

/// @brief 移动tip位置
void klbuiex_tip_move(klbuiex_tip_t* p_ex, int x, int y);

/// @brief 设置重绘
void klbuiex_tip_set_redraw(klbuiex_tip_t* p_ex);

/// @brief 重新绘制
bool klbuiex_tip_redraw(klbuiex_tip_t* p_ex);


/// @brief 设置/获取 脏矩形区域
void klbuiex_tip_set_dirty(klbuiex_tip_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect);
bool klbuiex_tip_get_dirty(klbuiex_tip_t* p_ex, klb_rect_t* p_out_diry_rect);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_TIP_H__
//end
