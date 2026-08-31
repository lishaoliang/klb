// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_decimal_menu.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   10进制输入菜单 共享窗口
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_DECIMAL_MENU_H__
#define __KLBSHW_DECIMAL_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_shwnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_decimal_menu             "/klbui/decimal-menu"


/// @brief 获取 10 进制输入菜单
/// @param [in] *p_gui      GUI 对象
/// @return klb_wnd_t* 共享窗口; 首次调用时创建并 push
klb_wnd_t* klbui_shwnd_get_decimal_menu(klb_gui_t* p_gui);


/// @brief Popup 弹窗结束之前的回调
/// @param [in] *ptr            绑定的指针
/// @param [in] *p_wnd_menu     菜单窗口
/// @param [in] ok              是否 ok
/// @param [in] value           当前值
/// @return int
typedef int(*klbshw_decimal_menu_cb)(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value);


/// @brief 绑定 Popup 弹窗结束之前的回调函数
/// @param [in] *p_wnd      菜单窗口
/// @param [in] cb          回调
/// @param [in] *ptr        绑定指针
/// @return int 0.成功
int klbshw_decimal_menu_bind(klb_wnd_t* p_wnd, klbshw_decimal_menu_cb cb, void* ptr);


/// @brief 获取所需要的宽高
/// @param [in] *p_wnd      菜单窗口; 可 NULL
/// @param [out] *p_out_w   宽; 可 NULL
/// @param [out] *p_out_h   高; 可 NULL
/// @return int 0.成功
int klbshw_decimal_menu_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


/// @brief 设置最大位数
void klbshw_decimal_menu_set_ranges(klb_wnd_t* p_wnd, int max_len);


/// @brief 设置值
void klbshw_decimal_menu_set_value(klb_wnd_t* p_wnd, int value);

/// @brief 获取值
int klbshw_decimal_menu_get_value(klb_wnd_t* p_wnd);


/// @brief 重新布局
void klbshw_decimal_menu_layout(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_DECIMAL_MENU_H__

// end
