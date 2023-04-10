///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_event.h
/// @brief   klb ui event 事件
///        1. UI消息/事件只是名称上的叫法, 在UI里面描述的是同一个
///        2. 事件名称定义参考 H5
/// @version 0.1
/// @history 修改历史
///   \n [2023-1]调整自定义message名称为 event 事件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_EVENT_H__
#define __KLBUI_EVENT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
// 公共事件[0x400 ~ 0x7FF]


/// @def   KLBUI_guievent
/// @brief 框架事件开始
#define KLBUI_guievent          0x400


///////////////////////////////////////
// 键鼠事件定义


/// @def   KLBUI_click
/// @brief 左键单击事件: 当左键点击并释放鼠标左键时触发
#define KLBUI_click             0x401           // click

/// @def   KLBUI_dblclick
/// @brief 左键双击事件: 当左键双击并释放鼠标左键时触发
#define KLBUI_dblclick          0x402           // dblclick

/// @def   KLBUI_mousedown
/// @brief 
#define KLBUI_mousedown         0x403           // mousedown

/// @def   KLBUI_mouseenter
/// @brief 
#define KLBUI_mouseenter        0x404           // mouseenter

/// @def   KLBUI_mouseleave
/// @brief 
#define KLBUI_mouseleave        0x405           // mouseleave

/// @def   KLBUI_mousemove
/// @brief 
#define KLBUI_mousemove         0x406           // mousemove

/// @def   KLBUI_mouseout
/// @brief 
#define KLBUI_mouseout          0x407           // mouseout

/// @def   KLBUI_mouseover
/// @brief 
#define KLBUI_mouseover         0x408           // mouseover

/// @def   KLBUI_mouseup
/// @brief 
#define KLBUI_mouseup           0x409           // mouseup


///////////////////////////////////////
// 窗口系统事件定义

/// @def   KLBUI_onabort
/// @brief 窗口加载崩溃事件: 当窗口加载失败时触发; 完全崩溃, 无法使用
#define KLBUI_onabort           0x500           // onabort


/// @def   KLBUI_onerror
/// @brief 窗口加载发生错误事件: 当窗口加载发生错误时触发; 有错误发生, 但不影响使用
#define KLBUI_onerror           0x501           // onerror


/// @def   KLBUI_onpaint
/// @brief 绘制事件: 当需要窗口组件绘制时触发
#define KLBUI_onpaint           0x520           // onpaint



/// @def   KLBUI_onload
/// @brief 加载事件: 对话框/文档/资源 加载完成时触发
#define KLBUI_onload            0x601           // onload


/// @def   KLBUI_onunload
/// @brief 卸载事件: 对话框/文档/资源 卸载时触发
#define KLBUI_onunload          0x602           // onunload



/// @def   KLBUI_onresize
/// @brief 窗口尺寸变化事件: 当窗口尺寸变化时触发
#define KLBUI_onresize          0x603           // onresize



/// @def   KLBUI_focusin
/// @brief 即将获得焦点事件: 窗口组件即将获得焦点时触发
#define KLBUI_focusin           0x700           // focusin


/// @def   KLBUI_focus
/// @brief 聚焦事件: 窗口组件获得焦点时触发
#define KLBUI_focus             0x701           // focus


/// @def   KLBUI_focusout
/// @brief 即将失去聚焦事件: 窗口组件即将失去焦点时触发
#define KLBUI_focusout          0x702           // focusout


/// @def   KLBUI_blur
/// @brief 失去聚焦事件: 窗口组件失去焦点时触发
#define KLBUI_blur              0x703           // blur



//////////////////////////////////////////////////////////////////////////
// 组件私有事件 [0x800 ~ 0x9FF]



//////////////////////////////////////////////////////////////////////////
// 自定义事件 [0x1000 ~ -]



#ifdef __cplusplus
}
#endif

#endif // __KLBUI_EVENT_H__
//end
