///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wnd.h
/// @brief   窗口定义
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
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_map.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;
typedef struct klb_wnd_t_ klb_wnd_t;


/// @struct klb_wnd_pos_t
/// @brief  窗口位置
///   采用坐标系: 左上角点为原点
typedef struct klb_wnd_pos_t_
{
    klb_rect_t  rect_in_canvas;             ///< 基于画布的坐标(屏幕/顶层窗口)
    klb_rect_t  rect_in_parent;             ///< 基于父窗口的坐标
}klb_wnd_pos_t;


/// @enum  klb_wnd_style_e
/// @brief 窗口样式标记
typedef enum klb_wnd_style_e_
{
    KLB_WND_STYLE_TOP            = 0x0001,   ///< 顶层窗口
    KLB_WND_STYLE_BORDERLESS     = 0x0002,   ///< 无边框/标题栏
    KLB_WND_STYLE_NOFOCUS        = 0x0004,   ///< 无聚焦状态
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
    uint32_t    style;                      ///< 典型窗口样式: klb_wnd_style_e
    uint32_t    status;                     ///< 窗口状态: klb_wnd_status_e
}klb_wnd_state_t;


/// @struct klb_wnd_env_t
/// @brief  窗口环境
typedef struct klb_wnd_env_t_
{
    klb_gui_t*  p_gui;                      ///< 窗口所属的gui
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
/// @param [in] *p_map      map数据集合
/// @return int 0.成功; 非0.失败(错误码)
/// @note map 具体数据格式由控件定义
typedef int(*klb_wnd_on_set_cb)(klb_wnd_t* p_wnd, const klb_map_t* p_map);


/// @brief 向控件获取数据: 样式\显示\状态等等
/// @param [in] *p_wnd      窗体对象
/// @param [in] *p_map      map数据集合
/// @return klb_map_t* map数据集合
/// @note map 具体数据格式由控件定义
typedef klb_map_t* (*klb_wnd_on_get_cb)(klb_wnd_t* p_wnd, const klb_map_t* p_map);


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
    klb_wnd_vtable_t vtable;    ///< 窗口函数表

    klb_wnd_t*      p_parent;   ///< 父节点, 树形结构
    klb_wnd_t*      p_child;    ///< 子节点

    klb_wnd_t*      p_prev;     ///< 前一个兄弟节点
    klb_wnd_t*      p_next;     ///< 后一个兄弟节点

    klb_gui_t*      p_gui;      ///< 窗口所属的gui

    klb_wnd_pos_t   pos;        ///< 窗口位置
    klb_wnd_state_t state;      ///< 窗口状态的参数

    sds             name;       ///< 窗口名称
    sds             type;       ///< 窗口类型
    klb_hlist_t*    p_hlist;    ///< 仅顶层窗口拥有的子窗口查找表

    void*           p_udata;    ///< public user data, [公共用户数据]

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


/// @brief 获取gui
KLB_API klb_gui_t* klb_wnd_get_gui(klb_wnd_t* p_wnd);


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

/// @brief 设置是否显示
KLB_API void klb_wnd_show(klb_wnd_t* p_wnd, bool show);

/// @brief 基于父窗口移动(相对坐标)
KLB_API void klb_wnd_move(klb_wnd_t* p_wnd, int x, int y);

/// @brief 重置控件大小
KLB_API void klb_wnd_resize(klb_wnd_t* p_wnd, int w, int h);


/// @brief 需要刷新
KLB_API void klb_wnd_update(klb_wnd_t* p_wnd);


/// @brief 绑定响应函数
KLB_API int klb_wnd_bind_command(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* p_obj);


/// @brief 参数设置 / 获取
KLB_API int klb_wnd_set(klb_wnd_t* p_wnd, const klb_map_t* p_map);
KLB_API klb_map_t* klb_wnd_get(klb_wnd_t* p_wnd, const klb_map_t* p_map);


/// @brief 绘图
KLB_API int klb_wnd_draw_clear(klb_wnd_t* p_wnd, uint32_t color);
KLB_API int klb_wnd_draw_point(klb_wnd_t* p_wnd, int x, int y, uint32_t color);
KLB_API int klb_wnd_draw_points(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color);
KLB_API int klb_wnd_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color);
KLB_API int klb_wnd_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color);
KLB_API int klb_wnd_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color);
KLB_API int klb_wnd_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color);
KLB_API int klb_wnd_draw_fill_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color);
KLB_API int klb_wnd_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color);
KLB_API int klb_wnd_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h);
KLB_API int klb_wnd_draw_image(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WND_H__
//end
