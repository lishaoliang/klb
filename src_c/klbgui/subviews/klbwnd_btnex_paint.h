///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_btnex_paint.h
/// @brief   klb window button extend paint, 扩展按钮专用的内置 图形按钮
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_BTNEX_PAINT_H__
#define __KLBWND_BTNEX_PAINT_H__

#include "klb_type.h"
#include "klbgui/wnd/klbwnd_btnex.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klbwnd_btnex_paint_type_e
/// @brief 内部使用的按钮类型
typedef enum klbwnd_btnex_paint_type_e_
{
    // klbshw_decimal.c 专用
    KLBWND_BTNEX_SHW_DECIMAL_clear = 800,           ///< 清除
    KLBWND_BTNEX_SHW_DECIMAL_enter,                 ///< 确认输入

}klbwnd_btnex_paint_type_e;


int klbwnd_btnex_paint_select(klb_wnd_t* p_wnd, int type);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_BTNEX_PAINT_H__
//end
