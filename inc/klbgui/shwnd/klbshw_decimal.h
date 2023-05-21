///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_decimal.h
/// @brief   klb share window decimal keyboard, 10进制输入软键盘
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_DECIMAL_H__
#define __KLBSHW_DECIMAL_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLBSHW_decimal          "/klbui/decimal"


/// @brief 获取10进制输入软键盘
KLB_API klb_wnd_t* klbui_shwnd_get_decimal(klb_gui_t* p_gui);


/// @brief 获取10进制输入软键盘所需要的宽高
KLB_API int klbshw_decimal_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h);


/// @brief Popup弹窗结束之前的回调
/// @param [in] *ptr        绑定的指针
/// @param [in] *p_wnd_dec  10进制输入软键盘      
/// @param [in] ok          是否ok
/// @return int
typedef int(*klbshw_decimal_cb)(void* ptr, klb_wnd_t* p_wnd_dec, bool ok, int value);


/// @brief 绑定Popup弹窗结束之前的回调函数
KLB_API int klbshw_decimal_bind(klb_wnd_t* p_wnd, klbshw_decimal_cb cb, void* ptr);


/// @brief 设置范围
KLB_API void klbshw_decimal_set_max_len(klb_wnd_t* p_wnd, int max_len);


/// @brief 设置/获取值
KLB_API void klbshw_decimal_set_value(klb_wnd_t* p_wnd, int value);
KLB_API int  klbshw_decimal_get_value(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_DECIMAL_H__
//end
