///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_redraw.h
/// @brief   记录重绘窗口. 仅记录, 不负责重绘
///        GUI重绘思路:
///        1. 将需要重绘的窗口指针(klb_wnd_t*)记录在此
///        2. 剔除存在父子关系的窗口, 只保留其父窗口
///        3. 若遇特殊情况(eg. model\popup\messagebox 等), 则放弃记录数据, 直接全部重绘
///        重绘使用流程:
///        1. 使用者调用 klb_wnd_update 函数, 标记窗口需要重绘, 记录在本模块
///        2. 一次 klb_gui_loop_once 中所有消息处理完毕后, 再一次性重绘
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_REDRAW_H__
#define __KLBUIEX_REDRAW_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbuiex_redraw_t_ klbuiex_redraw_t;


int klbuiex_register_redraw(klb_gui_t* p_gui);
klbuiex_redraw_t* klbuiex_get_redraw(klb_gui_t* p_gui);


/// @brief 清空记录 / 清除绘制标记
void klbuiex_redraw_clear(klbuiex_redraw_t* p_re);


/// @brief 放入需要重绘的窗口
/// @param [in]  *p_re      重绘扩展
/// @param [in]  *p_wnd     窗口
/// @return 无
void klbuiex_redraw_push(klbuiex_redraw_t* p_re, klb_wnd_t* p_wnd);


/// @brief 标记重绘所有
void klbuiex_redraw_all(klbuiex_redraw_t* p_re);


/// @brief 查询是否需要重绘
bool klbuiex_redraw_need_repaint(klbuiex_redraw_t* p_re, bool* p_redraw_all);


/// @brief 获取重绘列表
klb_hlist_t* klbuiex_redraw_get_hlist(klbuiex_redraw_t* p_re);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUIEX_REDRAW_H__
//end
