///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_event.h
/// @brief   klb ui event 事件
/// @version 0.1
/// @history 修改历史
///   \n [2023-1]调整message名称为 event 事件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_EVENT_H__
#define __KLBUI_EVENT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLBUI_PAINT
/// @brief 绘制事件
#define KLBUI_PAINT             0x501       // paint


/// @def   KLBUI_LOAD
/// @brief 加载事件: 对话框即将载入
#define KLBUI_LOAD              0x601       // load


/// @def   KLBUI_UNLOAD
/// @brief 卸载事件: 对话框即将关闭
#define KLBUI_UNLOAD            0x602       // unload


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_EVENT_H__
//end
