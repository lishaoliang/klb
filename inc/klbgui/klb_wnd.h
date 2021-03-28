///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wnd.h
/// @brief   窗口定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WND_H__
#define __KLB_WND_H__

#include "klb_type.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_canvas.h"
#include "klbgui/klb_msg.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;
typedef struct klb_wnd_t_ klb_wnd_t;


/// @struct klb_wnd_pos_t
/// @brief  窗口位置
typedef struct klb_wnd_pos_t_
{
    klb_rect_t  rect_in_canvas;             ///< 基于画布的坐标(屏幕/顶层窗口)
    klb_rect_t  rect_in_parent;             ///< 基于父窗口的坐标
}klb_wnd_pos_t;


/// @enum  klb_ui_style_e
/// @brief 窗口样式标记
typedef enum klb_wnd_style_e_
{
    KLB_WND_STYLE_TOP            = 0x0001,   ///< 顶层窗口
}klb_wnd_style_e;


/// @enum  klb_wnd_status_e
/// @brief 窗口状态标记
typedef enum klb_wnd_status_e_
{
    KLB_WND_STATUS_HIDE          = 0x0001,   ///< 隐藏
    KLB_WND_STATUS_FOCUS         = 0x1000,   ///< 鼠标聚焦
    KLB_WND_STATUS_RE_CALCULATE  = 0x8000,   ///< 顶层窗口才具有的属性: 需要重新计算窗口基于屏幕的位置
}klb_wnd_status_e;


/// @struct klb_wnd_state_t
/// @brief  窗口状态参数
typedef struct klb_wnd_state_t_
{
    uint32_t    style;                      ///< 典型窗口样式: klb_ui_style_e
    uint32_t    status;                     ///< 窗口状态: klb_ui_status_e
}klb_wnd_state_t;


/// @struct klb_wnd_env_t
/// @brief  窗口环境
typedef struct klb_wnd_env_t_
{
    klb_gui_t*  p_gui;
}klb_wnd_env_t;


/// @brief 销毁
/// @param [in] *p_wnd      窗体对象
/// @return 无
typedef void(*klb_wnd_destroy_cb)(klb_wnd_t* p_wnd);


/// @brief 消息控制函数
/// @param [in] *p_wnd      窗体对象
/// @param [in] msg         消息命令
/// @param [in] *p_p1       点1
/// @param [in] *p_p2       点2
/// @return int 0
typedef int(*klb_wnd_on_control_cb)(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @brief 消息响应函数
/// @param [in] *p_wnd      窗体对象
/// @param [in] msg         消息命令
/// @param [in] *p_p1       点1
/// @param [in] *p_p2       点2
/// @return int
///  \n 0. 消息终止,不再"冒泡"
///  \n msg. 任然以msg"冒泡"
///  \n 非0. 转换为其他消息"冒泡"
typedef int(*klb_wnd_on_command_cb)(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @brief 向控件设置数据: 样式\显示\状态等等
/// @param [in] *p_wnd      窗体对象
/// @param [in] *p_json     JSON格式数据
/// @return int 0.成功; 非0.失败(错误码)
typedef int(*klb_wnd_on_set_cb)(klb_wnd_t* p_wnd, const char* p_json);


/// @brief 向控件获取数据: 样式\显示\状态等等
/// @param [in] *p_wnd      窗体对象
/// @param [in] *p_json     JSON格式数据
/// @return char* JSON串或NULL
typedef char*(*klb_wnd_on_get_cb)(klb_wnd_t* p_wnd, const char* p_json);


/// @struct klb_wnd_vtable_t
/// @brief  ui窗口虚表
typedef struct klb_wnd_vtable_t_
{
    /// @brief 销毁
    /// @param [in] *p_wnd      窗体对象
    /// @return 无
    klb_wnd_destroy_cb      destroy;

    /// @brief 消息控制函数
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] msg         消息命令
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int 0
    klb_wnd_on_control_cb   on_control;

    /// @brief 消息响应函数
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] msg         消息命令
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int
    ///  \n 0. 消息终止,不再"冒泡"
    ///  \n msg. 任然以msg"冒泡"
    ///  \n 非0. 转换为其他消息"冒泡"
    klb_wnd_on_command_cb   on_command;

    /// @brief 向控件设置数据: 样式\显示\状态等等
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] *p_json     JSON格式数据
    /// @return int 0.成功; 非0.失败(错误码)
    klb_wnd_on_set_cb       on_set;

    /// @brief 向控件获取数据: 样式\显示\状态等等
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] *p_json     JSON格式数据
    /// @return char* JSON串或NULL
    klb_wnd_on_get_cb       on_get;
}klb_wnd_vtable_t;


/// @struct klb_wnd_t
/// @brief  窗口
typedef struct klb_wnd_t_
{
    klb_wnd_vtable_t vtable;    ///< 窗口虚表

    klb_wnd_t*      p_parent;   ///< 父节点, 二叉树
    klb_wnd_t*      p_child;    ///< 子节点

    klb_wnd_t*      p_prev;     ///< 前一个兄弟节点
    klb_wnd_t*      p_next;     ///< 后一个兄弟节点

    klb_wnd_pos_t   pos;        ///< 窗口位置
    klb_wnd_state_t state;      ///< 窗口状态的参数

    klb_wnd_env_t   env;        ///< 窗口运行环境

    void*           p_name;     ///< 窗口名称
    void*           p_type;     ///< 窗口类型
    uint16_t        name_len;   ///< 窗口名称长度
    uint16_t        type_len;   ///< 窗口类型长度

    void*           p_udata;    ///< publi user data, [公共用户数据]

    char            ctrl[];     ///< 控件私有数据域, (控件数据)
}klb_wnd_t;


/// @def   KLB_FREE_WND
/// @brief 释放单个窗口
#define KLB_FREE_WND(WND_) { \
    if(NULL!=(WND_)){ \
        klb_wnd_destroy_cb destroy=(WND_)->vtable.destroy; \
        destroy(WND_); \
        (WND_)=NULL; \
    } \
}

/// @brief 获取画布
/// @param [in] *p_wnd      窗口对象
/// @return klb_canvas_t* 画布指针
KLB_API klb_canvas_t* klb_wnd_get_canvas(klb_wnd_t* p_wnd);


/// @brief 获取样式
/// @param [in] *p_wnd      窗口对象
/// @return uint32_t 样式
KLB_API uint32_t klb_wnd_get_style(klb_wnd_t* p_wnd);


/// @brief 设置样式
/// @param [in] *p_wnd      窗口对象
/// @param [in] style       样式
/// @return 无
KLB_API void klb_wnd_set_style(klb_wnd_t* p_wnd, uint32_t style);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WND_H__
//end
