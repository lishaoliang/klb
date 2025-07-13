///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_tip.h
/// @brief   klb share window tip, 内部tip图层的tip提示框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_TIP_H__
#define __KLBSHW_TIP_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_tip            "/klbui/tip"


/// @brief 获取"tip"对话框
klb_wnd_t* klbui_shwnd_get_tip(klb_gui_t* p_gui);


/// @brief 设置/获取 标题
void klbshw_tip_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbshw_tip_get_title(klb_wnd_t* p_wnd);


/// @brief 对TIP窗口进行 重新布局
/// @note 重新布局会调整 tip 窗大小
void klbshw_tip_layout(klb_wnd_t* p_wnd, int max_w, int max_h, int* p_out_w, int* p_out_h);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_TIP_H__
//end
