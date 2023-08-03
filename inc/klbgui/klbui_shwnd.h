///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_shwnd.h
/// @brief   klb gui share window 共享使用部分界面
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_SHWND_H__
#define __KLBUI_SHWND_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_gui_t_ klb_gui_t;


/// @brief 放入共享使用界面
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path         路径(关键字)
/// @return int 0.成功; 非0.失败(错误码)
/// @note 注意:
///       1. push之后， 由ui框架管理生命周期
///       2. 共享界面是唯一的
///       3. path冲突时, push失败
///       4. "/klbui"路径, 由ui框架保留使用
KLB_API int klb_gui_push_shwnd(klb_gui_t* p_gui, const char* p_path, klb_wnd_t* p_top_wnd);


/// @brief 获取共享使用界面
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path         路径(关键字)
/// @return int 0.成功; 非0.失败(错误码)
KLB_API klb_wnd_t* klb_gui_get_shwnd(klb_gui_t* p_gui, const char* p_path);


/// @brief 设置共享窗口的CSS值
KLB_API int klb_gui_shwnd_css_set(klb_gui_t* p_gui, const char* p_path, const klb_map_t* p_map);


/// @brief 获取共享窗口的CSS值
KLB_API klb_map_t* klb_gui_shwnd_css_get(klb_gui_t* p_gui, const char* p_path, const klb_map_t* p_map);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_SHWND_H__
//end
