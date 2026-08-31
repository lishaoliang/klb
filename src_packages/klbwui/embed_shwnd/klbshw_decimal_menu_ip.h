// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_decimal_menu_ip.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   IP 用 10 进制输入菜单 共享窗口
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_DECIMAL_MENU_IP_H__
#define __KLBSHW_DECIMAL_MENU_IP_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_shwnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_decimal_menu_ip          "/klbui/decimal-menu-ip"


/// @brief 获取 IP 用 10 进制输入菜单
/// @param [in] *p_gui      GUI 对象
/// @return klb_wnd_t* 共享窗口; 首次调用时创建并 push
klb_wnd_t* klbui_shwnd_get_decimal_menu_ip(klb_gui_t* p_gui);


/// @brief Popup / 按键回调
/// @param [in] *ptr            绑定的指针
/// @param [in] *p_wnd_menu     菜单窗口
/// @param [in] ok              是否 ok
/// @param [in] value           当前值
/// @return int 业务码; get_cursor_pos 时为光标位置
typedef int(*klbshw_decimal_menu_ip_cb)(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value);


/// @brief 绑定 Popup 弹窗结束之前的回调函数
int klbshw_decimal_menu_ip_bind(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击小数点以后的回调; 未绑定时菜单默认追加 '.'
int klbshw_decimal_menu_ip_bind_click_point(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击数字以后的回调; 菜单仍先写入数字
int klbshw_decimal_menu_ip_bind_click_number(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击左移以后的回调; 未绑定时菜单默认移动光标
int klbshw_decimal_menu_ip_bind_click_moveleft(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击右移以后的回调; 未绑定时菜单默认移动光标
int klbshw_decimal_menu_ip_bind_click_moveright(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击退格以后的回调; 未绑定时菜单默认退格
int klbshw_decimal_menu_ip_bind_click_backspace(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定点击清除以后的回调; 未绑定时菜单默认清零
int klbshw_decimal_menu_ip_bind_click_clear(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定获取光标位置的回调; 返回值作为 cursor_pos
int klbshw_decimal_menu_ip_bind_get_cursor_pos(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);

/// @brief 绑定设置是否显示光标的回调; value 非 0 显示
int klbshw_decimal_menu_ip_bind_set_show_cursor(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr);


/// @brief 获取所需要的宽高
int klbshw_decimal_menu_ip_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


void klbshw_decimal_menu_ip_set_ranges(klb_wnd_t* p_wnd, int max_len);


void klbshw_decimal_menu_ip_set_value(klb_wnd_t* p_wnd, int value);
int klbshw_decimal_menu_ip_get_value(klb_wnd_t* p_wnd);


void klbshw_decimal_menu_ip_layout(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_DECIMAL_MENU_IP_H__

// end
