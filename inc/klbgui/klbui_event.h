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


//////////////////////////////////////////////////////////////////////////
// 公共事件[0x500 ~ 0x7FF]


/// @def   KLBUI_PAINT
/// @brief 绘制事件
#define KLBUI_PAINT             0x501       // paint


/// @def   KLBUI_LOAD
/// @brief 加载事件: 对话框即将载入
#define KLBUI_LOAD              0x601       // load


/// @def   KLBUI_UNLOAD
/// @brief 卸载事件: 对话框即将关闭
#define KLBUI_UNLOAD            0x602       // unload


/// @def   KLBUI_FOCUS
/// @brief 聚焦事件: 控件得到焦点
#define KLBUI_FOCUS             0x701       // focus


/// @def   KLBUI_UNFOCUS
/// @brief 失去聚焦事件
#define KLBUI_UNFOCUS           0x702       // unfocus


//////////////////////////////////////////////////////////////////////////
// 组件私有事件 [0x800 ~ 0x9FF]



//////////////////////////////////////////////////////////////////////////
// 自定义事件 [0x1000 ~ -]



#ifdef __cplusplus
}
#endif

#endif // __KLBUI_EVENT_H__
//end
