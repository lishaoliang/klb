// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_messagebox.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   标准消息弹出框 共享窗口
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_MESSAGEBOX_H__
#define __KLBSHW_MESSAGEBOX_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_shwnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_messagebox               "/klbui/messagebox"


#define KLBSHW_messagebox_close         0x0001      ///< 关闭; 与 KLBWND_MESSAGEBOX_close 一致
#define KLBSHW_messagebox_ok            0x0002      ///< 确认; 与 KLBWND_MESSAGEBOX_ok 一致
#define KLBSHW_messagebox_cancel        0x0004      ///< 取消; 与 KLBWND_MESSAGEBOX_cancel 一致


/// @brief 获取 messagebox 共享窗口
/// @param [in] *p_gui      GUI 对象
/// @return klb_wnd_t* 共享窗口; 首次调用时创建并 push
klb_wnd_t* klbui_shwnd_get_messagebox(klb_gui_t* p_gui);


/// @brief 获取消息框默认宽高
/// @param [in] *p_gui          GUI 对象; 可 NULL
/// @param [out] *p_out_w       宽; 可 NULL
/// @param [out] *p_out_h       高; 可 NULL
/// @return int 0.成功
int klbshw_messagebox_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h);


/// @brief 设置标题栏文本
/// @param [in] *p_wnd          消息框窗口
/// @param [in] *p_title        标题; 可 NULL 表示清空
/// @return 无
void klbshw_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title);


/// @brief 设置正文提示
/// @param [in] *p_wnd          消息框窗口
/// @param [in] *p_text         正文; 可 NULL 表示清空
/// @return 无
void klbshw_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text);


/// @brief 获取当前值 (close / ok / cancel)
/// @param [in] *p_wnd          消息框窗口
/// @return int close=0x0001; ok=0x0002; cancel=0x0004
int klbshw_messagebox_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBSHW_MESSAGEBOX_H__

// end
