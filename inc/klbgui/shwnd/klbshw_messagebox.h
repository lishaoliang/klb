///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_messagebox.h
/// @brief   klb share window messagebox, 标准消息弹出框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_MESSAGEBOX_H__
#define __KLBSHW_MESSAGEBOX_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_messagebox            "/klbui/messagebox"


/// @brief 获取"messagebox"弹出对话框
KLB_API klb_wnd_t* klbui_shwnd_get_messagebox(klb_gui_t* p_gui);


/// @brief 获取消息框所需要的宽高
KLB_API int klbshw_messagebox_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h);


/// @brief 设置消息框的标题栏
KLB_API void klbshw_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title);


/// @brief 设置消息框的问题提示
KLB_API void klbshw_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_MESSAGEBOX_H__
//end
