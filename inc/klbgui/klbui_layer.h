///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_layer.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb gui layer 扩展图层相关接口
/// @version 0.1
/// @history 修改历史
///   \n [2025-7] 添加用户自定义图层/等待图层接口
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_LAYER_H__
#define __KLBUI_LAYER_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_gui_t_ klb_gui_t;


//////////////////////////////////////////////////////////////////////////
// 用户自定义图层 接口


/// @brief 给用户图层 绑定窗口
/// @param [in]  *p_path_name       顶层窗口路径; eg. "/aaa"
/// @return int  0.成功; 非0. 错误码
KLB_API int klb_gui_udatalayer_bind(klb_gui_t* p_gui, const char* p_path_name);

/// @brief 给用户图层 绑定窗口
/// @param [in]  *p_top             顶层窗口指针; p_top = NULL 表示, 取消绑定
/// @return int  0.成功; 非0. 错误码
KLB_API int klb_gui_udatalayer_bind_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top);

/// @brief 移动 用户图层
KLB_API void klb_gui_udatalayer_move(klb_gui_t* p_gui, int x, int y);

/// @brief 是否显示 用户图层 
KLB_API void klb_gui_udatalayer_show(klb_gui_t* p_gui, bool show);


//////////////////////////////////////////////////////////////////////////
// 等待 图层 接口


/// @brief 给等待图层 绑定窗口
/// @param [in]  *p_path_name       顶层窗口路径; eg. "/aaa"
/// @return int  0.成功; 非0. 错误码
KLB_API int klb_gui_waitlayer_bind(klb_gui_t* p_gui, const char* p_path_name);

/// @brief 给等待图层 绑定窗口
/// @param [in]  *p_top             顶层窗口指针; p_top = NULL 表示, 取消绑定
/// @return int  0.成功; 非0. 错误码
KLB_API int klb_gui_waitlayer_bind_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top);

/// @brief 移动 等待图层 及窗口
KLB_API void klb_gui_waitlayer_move(klb_gui_t* p_gui, int x, int y);

/// @brief GUI 等待
/// @note  启用等待后, GUI会丢弃所有消息
///        若设置了, 等待图层窗口, 则使用 等待图层窗口 定时绘图
KLB_API void klb_gui_wait(klb_gui_t* p_gui, bool wait);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_LAYER_H__
//end
