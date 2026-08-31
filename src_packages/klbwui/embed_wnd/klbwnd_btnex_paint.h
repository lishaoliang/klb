// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_btnex_paint.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window button extend paint, 扩展按钮内置图形绘制
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_BTNEX_PAINT_H__
#define __KLBWND_BTNEX_PAINT_H__


#include "klb_type.h"
#include "klbwui/embed_wnd/klbwnd_btnex.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klbwnd_btnex_paint_type_e
/// @brief 内部使用的按钮类型
typedef enum klbwnd_btnex_paint_type_e_
{
    // klbwnd_decimal_menu 等 shwnd 专用
    KLBWND_BTNEX_SHW_DECIMAL_clear = 800,           ///< 清除
    KLBWND_BTNEX_SHW_DECIMAL_enter,                 ///< 确认输入
} klbwnd_btnex_paint_type_e;


int klbwnd_btnex_paint_select(klb_wnd_t* p_wnd, int type);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_BTNEX_PAINT_H__

// end
