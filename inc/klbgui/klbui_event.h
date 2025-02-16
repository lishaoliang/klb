///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_event.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb ui event 事件
///        1. UI消息/事件只是名称上的叫法, 在UI里面描述的是同一个
///        2. 事件名称定义参考 H5
/// @version 0.1
/// @history 修改历史
///   \n [2023-1]调整自定义message名称为 event 事件
///   \n [2025-1]添加定时器事件
/// @warning 没有警告
/// @note 窗口流程:
///        -> *_create(...) / klb_wnd_push_child(...) / klb_gui_append(...)         创建及加入窗口树
///        -> klb_gui_modal(...) / klb_gui_popup(...) / klb_gui_messagebox(...)     modal/popup/messagebox等方式压入栈待显示
///        -> KLBUI_onload          加载事件: "onload"
///        -> KLBUI_onpredraw       预绘制事件: "onpredraw"
///        -> klb_wnd_draw(...)     绘制动作(框架内部函数)
///        -> KLBUI_onpaint(...)    绘制事件: "onpaint"
///        -> ... click/focus/...   中间用户操作等事件
///        -> klb_gui_modal_end(...)/klb_gui_popup_end(...)/klb_gui_messagebox_end(...)     结束显示
///        -> KLBUI_onunload        卸载事件: "onunload"
///        -> *_destroy(...)        销毁
///        -------------------------------------------------
///        @ *_create(..) / *_destroy(...) 一般只执行一次
///        @ modal/popup/messagebox ~ modal_end/popup_end/messagebox_end(onunload) 一般流程在这些之间
///        @ "onpredraw" 第一次绘制之前, 最后调整窗口的时机; eg. 某些页面需要自动处理流程, 在这里处理
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_EVENT_H__
#define __KLBUI_EVENT_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
// 特殊比特标记位

#define KLBUI_event_bit1        0x40000000
#define KLBUI_event_bit2        0x20000000
#define KLBUI_event_bit3        0x10000000


//////////////////////////////////////////////////////////////////////////
// 公共事件[0x400 ~ 0x3FFF]


/// @def   KLBUI_guievent
/// @brief 框架事件开始
#define KLBUI_guievent          0x400


///////////////////////////////////////
// 键鼠事件定义

/// @def   klbui_mouse_button_e
/// @brief 鼠标按键, 参考SDL2.X定义
typedef enum klbui_mouse_button_e_
{
    KLBUI_MOUSE_left    = 1,        ///< 鼠标左键
    KLBUI_MOUSE_middle  = 2,        ///< 鼠标中键
    KLBUI_MOUSE_right   = 3,        ///< 鼠标右键
    KLBUI_MOUSE_x1      = 4,        ///< 扩展x1
    KLBUI_MOUSE_x2      = 5,        ///< 扩展x2
}klbui_mouse_button_e;


/// @def   KLBUI_click
/// @brief 左键单击事件: 当左键点击并释放鼠标左键时触发
#define KLBUI_click             0x401           // click

/// @def   KLBUI_dblclick
/// @brief 左键双击事件: 当左键双击并释放鼠标左键时触发
#define KLBUI_dblclick          0x402           // dblclick

/// @def   KLBUI_mousedown
/// @brief 鼠标按下事件
/// @note 参数说明
///     pt1     参数表示当前鼠标位置
///     lparam  参数表示按下的鼠标按钮: klbui_mouse_button_e
#define KLBUI_mousedown         0x403           // mousedown

/// @def   KLBUI_mouseenter
/// @brief 
#define KLBUI_mouseenter        0x404           // mouseenter

/// @def   KLBUI_mouseleave
/// @brief 
#define KLBUI_mouseleave        0x405           // mouseleave

/// @def   KLBUI_mousemove
/// @brief  鼠标移动
#define KLBUI_mousemove         0x406           // mousemove

/// @def   KLBUI_mouseout
/// @brief 
#define KLBUI_mouseout          0x407           // mouseout

/// @def   KLBUI_mouseover
/// @brief 
#define KLBUI_mouseover         0x408           // mouseover

/// @def   KLBUI_mouseup
/// @brief 鼠标弹起事件
#define KLBUI_mouseup           0x409           // mouseup


/// @def   klbui_mousedrag_lparam_e
/// @brief 鼠标拖拽事件的 lparam 参数
typedef enum klbui_mousedrag_lparam_e_
{
    KLBUI_MOUSEDRAG_start = 1,      ///< 拖拽事件开始
    KLBUI_MOUSEDRAG_move  = 2,      ///< 拖拽移动中
    KLBUI_MOUSEDRAG_end   = 3,      ///< 拖拽事件结束
}klbui_mousedrag_lparam_e;

/// @def   KLBUI_mousedrag
/// @brief 鼠标拖拽事件
/// @note 参数说明
///     pt1     参数表示当前鼠标位置
///     pt2     参数表示拖拽起始鼠标位置
///     lparam  参数表示啥情况: klbui_mousedrag_lparam_e
///     wparam  参数表示拖拽的鼠标按钮: klbui_mouse_button_e
#define KLBUI_mousedrag         0x410           // mousedrag


/// @brief 鼠标滚轮事件, lparam 表示的滚轮方向
#define KLBUI_MOUSEWHEEL_is_up(Y_)      ((Y_) > 0)      // 向上滚动
#define KLBUI_MOUSEWHEEL_is_down(Y_)    ((Y_) < 0)      // 向下滚动
#define KLBUI_MOUSEWHEEL_value(Y_)      (ABS(Y_))       // 滚动的值


/// @def   KLBUI_mousewheel
/// @brief 鼠标滚轮事件
/// @note 参数说明
///     pt1     参数表示当前鼠标位置
///     lparam  参数表示滚轮滚动情况, 负数/正数均有效
#define KLBUI_mousewheel        0x411           // mousewheel


/// @def   KLBUI_outwindow
/// @brief 在popup窗口外点击事件
#define KLBUI_outwindow         0x450           // outwindow


///////////////////////////////////////
// 窗口系统事件定义

/// @def   KLBUI_onabort
/// @brief 窗口加载崩溃事件: 当窗口加载失败时触发; 完全崩溃, 无法使用
#define KLBUI_onabort           0x500           // onabort


/// @def   KLBUI_onerror
/// @brief 窗口加载发生错误事件: 当窗口加载发生错误时触发; 有错误发生, 但不影响使用
#define KLBUI_onerror           0x501           // onerror


/// @def   KLBUI_onpredraw
/// @brief 预绘制事件: 在第一次窗口绘制之前(on pre-draw)
#define KLBUI_onpredraw         0x518           // onpredraw


/// @def   KLBUI_onpaint
/// @brief 绘制事件: 当需要窗口组件绘制时触发
#define KLBUI_onpaint           0x520           // onpaint


///////////////////////////////////////
// 定时器事件定义


/// @def   KLBUI_ontimer
/// @brief 一次性定时器
#define KLBUI_ontimer           0x570           // ontimer


/// @def   KLBUI_onticker
/// @brief 长期性定时器
#define KLBUI_onticker          0x571           // onticker


///////////////////////////////////////
// Lua解析事件定义


/// @def   KLBUI_onparsewindow
/// @brief on parse window: Lua解析窗口(控件), 完成后触发; 包含所有子窗口解析完成
///   部分控件或窗口, 需要在所有 CSS 属性设置完成之后, 再做调整
///   此时传统C/C流程是无此流程的, 这里特加入 KLBUI_onparsewindow/KLBUI_onparsedialog 事件来扩充
#define KLBUI_onparsewindow     0x580           // onparsewindow


/// @def   KLBUI_onparsedialog
/// @brief on parse dialog: Lua解析对话框, 完成后触发
#define KLBUI_onparsedialog     0x581           // onparsedialog


///////////////////////////////////////
// 控件系统事件定义

/// @def   KLBUI_onload
/// @brief 加载事件: 对话框/文档/资源 加载完成时触发
#define KLBUI_onload            0x601           // onload


/// @def   KLBUI_onunload
/// @brief 卸载事件: 对话框/文档/资源 卸载时触发
#define KLBUI_onunload          0x602           // onunload



/// @def   KLBUI_onresize
/// @brief 窗口尺寸变化事件: 当窗口尺寸变化时触发
#define KLBUI_onresize          0x603           // onresize


/// @def   KLBUI_onchange
/// @brief 内容变更事件: 当窗口组件内容变更时触发
#define KLBUI_onchange          0x604           // onchange



///////////////////////////////////////
// 焦点相关


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


/// @def   KLBUI_focusdelay
/// @brief 聚焦延时触发事件: 窗口组件获得焦点, 且延时一段时间后触发
/// @note eg. 聚焦一段时间后, 再弹出二级菜单
#define KLBUI_focusdelay        0x710           // focusdelay


///////////////////////////////////////
// 

/// @def   KLBUI_layout
/// @brief (控件)重新布局事件
/// @note  组合复杂控件时, 高概率需要对控件内部做重新布局操作
///        这里加入 "layout" 事件 和 标准方法, 来处理此类情况
#define KLBUI_layout            0x861           // layout


//////////////////////////////////////////////////////////////////////////
// 其他 


/// @def   KLBUI_suggestw
/// @brief 控件建议宽
/// @note 需要依据(内容)等, 来设置窗口大小时使用
#define KLBUI_suggestw          0x881           // suggestw


/// @def   KLBUI_suggesth
/// @brief 控件建议高
#define KLBUI_suggesth          0x882           // suggesth


//////////////////////////////////////////////////////////////////////////
// 其他 [0x8A0 ~ 0x8FF]


/// @def   KLBUI_meminfo
/// @brief 窗口组件消耗的内存统计
#define KLBUI_meminfo           0x8A1           // meminfo




//////////////////////////////////////////////////////////////////////////
// 组件事件 [0x3A00 ~ 0x3FFF]

/// @def   KLBUI_event_ctrl
/// @brief 窗口组件私有事件
#define KLBUI_event_ctrl        0x3A00          // 组件私有事件开始


//////////////////////////////////////////////////////////////////////////
// 自定义私有事件 [0x4000 ~ - 0xFFFF]


/// @def   KLBUI_event_user
/// @brief 自定义私有事件
#define KLBUI_event_user        0x4000          // 自定义事件开始


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_EVENT_H__
//end
