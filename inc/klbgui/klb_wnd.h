///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wnd.h
/// @brief   窗口定义
/// @version 0.1
/// @history 修改历史
///   \n [2023-4] 提供 klb_wnd_push_child 函数, 许可在扩展控件开发中 自行构建窗口树
///   \n [2023-5] 添加 klb_wnd_on_paint_cb 定义, 许可控件开发者替换绘图函数
///   \n [2024-1] 添加 klb_wnd_draw_opt* 系列函数, 许可控件开发者 扩展基础绘图函数
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WND_H__
#define __KLB_WND_H__

#include "klb_type.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_canvas.h"
#include "klbgui/klb_msg.h"
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
    KLB_WND_STYLE_TOP                   = 0x0001,   ///< 顶层窗口
    KLB_WND_STYLE_PEEK_EVENT            = 0x0002,   ///< (在消息冒泡中)读取消息事件: 若需要响应部分子窗口事件, 可使用此标记
    KLB_WND_STYLE_NOFOCUS               = 0x0004,   ///< 无聚焦状态
    KLB_WND_STYLE_NOCOMMAND             = 0x0008,   ///< 无on_command命令响应: klb_wnd_bind_command 函数不生效
    KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW  = 0x0010,   ///< 有聚焦行为, 但聚焦时不会触发控件重绘
    KLB_WND_STYLE_FOCUS_CONTINUE        = 0x0020,   ///< 继续寻找焦点窗口

    KLB_WND_STYLE_LAYER_TIP             = 0x8000,   ///< TIP 图层
}klb_wnd_style_e;


/// @enum  klb_wnd_status_e
/// @brief 窗口状态标记
typedef enum klb_wnd_status_e_
{
    KLB_WND_STATUS_HIDE                 = 0x0001,   ///< 隐藏状态
    KLB_WND_STATUS_INPUT                = 0x0002,   ///< 输入状态
    KLB_WND_STATUS_CHECK                = 0x0004,   ///< 选中状态
    KLB_WND_STATUS_DISABLE              = 0x0008,   ///< 不使能
    KLB_WND_STATUS_TOPMOST              = 0x0100,   ///< 激活中的最顶层窗口: 所有 "modal"/"popup"/"messagebox"中处于最顶层
    KLB_WND_STATUS_FOCUS                = 0x1000,   ///< 鼠标聚焦
    KLB_WND_STATUS_RESIZE               = 0x4000,   ///< 重置了窗口大小, 需要控件处理布局问题
    KLB_WND_STATUS_CANVAS_RECT          = 0x8000,   ///< 需要重新计算窗口基于屏幕的位置
}klb_wnd_status_e;


/// @struct klb_wnd_state_t
/// @brief  窗口状态参数
typedef struct klb_wnd_state_t_
{
    uint32_t    style;                      ///< 典型窗口样式: klb_wnd_style_e
    uint32_t    status;                     ///< 窗口状态: klb_wnd_status_e
}klb_wnd_state_t;


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


/// @brief 窗口绘制函数
/// @param [in] *p_wnd      窗体对象
/// @return int
typedef int(*klb_wnd_on_paint_cb)(klb_wnd_t* p_wnd);


/// @struct klb_wnd_vtable_t
/// @brief  ui窗口虚表
typedef struct klb_wnd_vtable_t_
{
    /// @brief 销毁
    /// @param [in] *p_wnd      窗体对象
    /// @return 无
    klb_wnd_destroy_cb      destroy;

    /// @brief 消息控制函数: 一般由控件内部定义函数完成
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] msg         消息事件
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int 0
    ///  \n 不使用完整"消息冒泡"机制
    ///  \n 简化为 : 聚焦窗口和其最顶层窗口响应
    ///  \n 若控件需要处理子窗口消息: 额外添加标记 KLB_WND_STYLE_PEEK_EVENT
    klb_wnd_on_control_cb   on_control;

    /// @brief 消息响应函数: 一般由使用者绑定外部函数
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] msg         消息事件
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @param [in] lparam      附加参数1
    /// @param [in] wparam      附加参数2
    /// @return int
    /// @note
    ///  \n 不使用完整"消息冒泡"机制
    ///  \n 简化为 : 聚焦窗口和其最顶层窗口响应
    ///  \n 若控件需要处理子窗口消息: 额外添加标记 KLB_WND_STYLE_PEEK_EVENT
    klb_wnd_on_command_cb   on_command;

    /// @brief 自定义绘图
    /// @param [in] *p_wnd      窗体对象
    /// @return int
    /// @note
    ///  \n 若有自定义绘图, 则优先使用自定义绘图来完成重绘
    klb_wnd_on_paint_cb     on_paint;

    /// @brief 向控件设置数据: 样式\显示\状态等等
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] **p_map     map数据集合
    /// @return int 0.成功; 非0.失败(错误码)
    klb_wnd_on_set_cb       on_set;

    /// @brief 向控件获取数据: 样式\显示\状态等等
    /// @param [in] *p_wnd      窗体对象
    /// @param [in] **p_map     map数据集合
    /// @return map* map数据集合
    klb_wnd_on_get_cb       on_get;
}klb_wnd_vtable_t;


/// @struct klb_wnd_t
/// @brief  窗口基础结构
///  基础窗口结构, 不携带CSS属性, 尽量减小体积 
typedef struct klb_wnd_t_
{
    // 抽象函数
    klb_wnd_vtable_t    vtable;     ///< 窗口函数表

    // 窗口关系
    // 用于记录树形结构的窗口结构
    klb_wnd_t*          p_parent;   ///< 父节点, 树形结构
    klb_wnd_t*          p_child;    ///< 子节点

    klb_wnd_t*          p_prev;     ///< 前一个兄弟节点
    klb_wnd_t*          p_next;     ///< 后一个兄弟节点

    // gui
    klb_gui_t*          p_gui;      ///< 窗口所属的gui

    // 窗口位置/状态等
    klb_wnd_pos_t       pos;        ///< 窗口位置
    klb_wnd_state_t     state;      ///< 窗口状态的参数

    // tip
    sds                 tip;        ///< 聚焦之后的tip数据

    // 用户数据
    void*               p_udata;    ///< public user data, [绑定响应函数的附加指针]

    // 组件(控件)数据
    char                ctrl[];     ///< 控件私有数据域, (控件数据)
}klb_wnd_t;


/// @def   KLB_WIDGETS_PTR
/// @brief 从窗口指针获取组件指针
#define KLB_WIDGETS_PTR(WND_, ST_)  ((ST_*)((WND_)->ctrl))


/// @def   KLB_WND_PTR
/// @brief 从组件指针获取窗口指针
#define KLB_WND_PTR(WIDGETS_)       ((klb_wnd_t*)((char*)(WIDGETS_) - sizeof(klb_wnd_t)))


/// @def   KLB_FREE_WND
/// @brief 释放单个窗口
#define KLB_FREE_WND(WND_) { \
    if(NULL!=(WND_)){ \
        klb_wnd_destroy_cb destroy=(WND_)->vtable.destroy; \
        KLB_FREE_BY(((WND_)->tip), sdsfree); \
        if(destroy) { destroy(WND_); } \
        (WND_)=NULL; \
    } \
}

//////////////////////////////////////////////////////////////////////////
// 通用

/// @brief 销毁窗口, 及其所有子窗口
KLB_API void klb_wnd_destroy_tree(klb_wnd_t* p_wnd);


/// @brief 获取gui
KLB_API klb_gui_t* klb_wnd_get_gui(klb_wnd_t* p_wnd);


/// @brief 获取画布
/// @param [in] *p_wnd      窗口对象
/// @return klb_canvas_t* 画布指针
/// @note 不要缓存窗口画布指针; 需要使用时, 临时获取
///       框架可能会依据当前状态情况, 变更窗口画布
KLB_API klb_canvas_t* klb_wnd_get_canvas(klb_wnd_t* p_wnd);

//////////////////////////////////////////////////////////////////////////
// 样式

/// @brief 获取样式
/// @param [in] *p_wnd      窗口对象
/// @return uint32_t 样式
KLB_API uint32_t klb_wnd_get_style(klb_wnd_t* p_wnd);


/// @brief 设置样式
/// @param [in] *p_wnd      窗口对象
/// @param [in] style       样式
/// @return 无
KLB_API void klb_wnd_set_style(klb_wnd_t* p_wnd, uint32_t style);

//////////////////////////////////////////////////////////////////////////
// 状态

/// @brief 设置是否显示: 附带标记更新 窗口
KLB_API void klb_wnd_show(klb_wnd_t* p_wnd, bool show);

/// @brief 获取是否显示
KLB_API bool klb_wnd_is_show(klb_wnd_t* p_wnd);

/// @brief 设置是否隐藏: 只设置状态
/// @note  多数时候, 可能只是需要设置显隐标记, 并不需要触发更新标记
///        这里使用 hide / show 函数来区分
KLB_API void klb_wnd_hide(klb_wnd_t* p_wnd, bool hide);

/// @brief 获取是否隐藏
KLB_API bool klb_wnd_is_hide(klb_wnd_t* p_wnd);

/// @brief 设置输入状态
KLB_API void klb_wnd_input(klb_wnd_t* p_wnd, bool input);

/// @brief 获取是否是输入状态
KLB_API bool klb_wnd_is_input(klb_wnd_t* p_wnd);

/// @brief 设置选中状态
KLB_API void klb_wnd_check(klb_wnd_t* p_wnd, bool check);

/// @brief 获取是否是选中状态
KLB_API bool klb_wnd_is_check(klb_wnd_t* p_wnd);

/// @brief 设置不使能状态
KLB_API void klb_wnd_disable(klb_wnd_t* p_wnd, bool disable);

/// @brief 获取是否是 不使能
KLB_API bool klb_wnd_is_disable(klb_wnd_t* p_wnd);

/// @brief 设置使能状态
KLB_API void klb_wnd_enable(klb_wnd_t* p_wnd, bool enable);

/// @brief 获取是否是 使能
KLB_API bool klb_wnd_is_enable(klb_wnd_t* p_wnd);

/// @brief 获取是否是 所有激活窗口中的最顶层
/// @note  此状态的设置函数, 只能由框架内部决定
KLB_API bool klb_wnd_is_topmost(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// tip

/// @brief 设置,获取 tip
KLB_API void klb_wnd_set_tip(klb_wnd_t* p_wnd, const char* p_tip);
KLB_API const sds klb_wnd_get_tip(klb_wnd_t* p_wnd);

/// @brief 标记刷新 tip
/// @note 仅标记, 由框架决定合适的刷新时机
///       仅 设置了tip 且处于显示的情况下 生效
KLB_API void klb_wnd_tip_update(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 窗口位置/大小

/// @brief 基于父窗口移动到指定的相对坐标
KLB_API void klb_wnd_move(klb_wnd_t* p_wnd, int x, int y);


/// @brief 重新设置控件大小
KLB_API void klb_wnd_resize(klb_wnd_t* p_wnd, int w, int h);

//////////////////////////////////////////////////////////////////////////
// 刷新

/// @brief 标记窗口需要刷新
/// @note 仅标记, 由框架决定合适的刷新时机
KLB_API void klb_wnd_update(klb_wnd_t* p_wnd);


/// @brief 标记窗口需要更新 基于画布的坐标(屏幕/顶层窗口)
/// @note 仅标记, 由框架决定合适的更新时机
KLB_API void klb_wnd_update_canvas_rect(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 绑定外部回调函数


/// @brief 绑定响应函数
KLB_API int klb_wnd_bind_command(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* p_obj);


/// @brief 绑定自定义绘图
KLB_API int klb_wnd_bind_paint(klb_wnd_t* p_wnd, klb_wnd_on_paint_cb on_paint);


//////////////////////////////////////////////////////////////////////////
// 调用函数


/// @brief 调用on_control函数
KLB_API int klb_wnd_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @brief 调用on_command函数
KLB_API int klb_wnd_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @brief 1.调用on_control函数; 2.调用on_command函数
KLB_API int klb_wnd_on_control_and_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


//////////////////////////////////////////////////////////////////////////
// 控件建议宽/高

/// @brief 获取建议宽
KLB_API int klb_wnd_suggestw(klb_wnd_t* p_wnd);


/// @brief 获取建议高
KLB_API int klb_wnd_suggesth(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 窗口关系 

KLB_API int klb_wnd_push_child(klb_wnd_t* p_parent, klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 设置 / 获取

/// @brief 参数设置 / 获取
KLB_API int klb_wnd_set(klb_wnd_t* p_wnd, const klb_map_t* p_map);
KLB_API klb_map_t* klb_wnd_get(klb_wnd_t* p_wnd, const klb_map_t* p_map);


//////////////////////////////////////////////////////////////////////////
// 绘图

/// @brief 基础绘图接口
KLB_API int klb_wnd_set_draw_color(klb_wnd_t* p_wnd, uint32_t color);
KLB_API uint32_t klb_wnd_get_draw_color(klb_wnd_t* p_wnd);
KLB_API int klb_wnd_set_font_height(klb_wnd_t* p_wnd, int h);
KLB_API int klb_wnd_get_font_height(klb_wnd_t* p_wnd);

KLB_API int klb_wnd_draw_clear(klb_wnd_t* p_wnd, uint32_t* p_color);
KLB_API int klb_wnd_draw_point(klb_wnd_t* p_wnd, int x, int y, uint32_t* p_color);
KLB_API int klb_wnd_draw_points(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t* p_color);
KLB_API int klb_wnd_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t* p_color);
KLB_API int klb_wnd_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t* p_color);
KLB_API int klb_wnd_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t* p_color);
KLB_API int klb_wnd_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t* p_color);
KLB_API int klb_wnd_draw_fill_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t* p_color);
KLB_API int klb_wnd_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t* p_color);
KLB_API int klb_wnd_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h);
KLB_API int klb_wnd_text_size(klb_wnd_t* p_wnd, const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h);
KLB_API int klb_wnd_draw_image(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);
KLB_API int klb_wnd_image_size(klb_wnd_t* p_wnd, const char* p_path, int* p_out_w, int* p_out_h);

KLB_API int klb_wnd_draw_clear2(klb_wnd_t* p_wnd, uint32_t color);
KLB_API int klb_wnd_draw_point2(klb_wnd_t* p_wnd, int x, int y, uint32_t color);
KLB_API int klb_wnd_draw_points2(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color);
KLB_API int klb_wnd_draw_line2(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color);
KLB_API int klb_wnd_draw_lines2(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color);
KLB_API int klb_wnd_draw_rect2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color);
KLB_API int klb_wnd_draw_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color);
KLB_API int klb_wnd_draw_fill_rect2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color);
KLB_API int klb_wnd_draw_fill_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color);
KLB_API int klb_wnd_text_size2(klb_wnd_t* p_wnd, const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h);
KLB_API int klb_wnd_draw_text2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h);


/// @brief 可扩展绘图接口
KLB_API int klb_wnd_draw_opt1(klb_wnd_t* p_wnd, int opt, const void* ptr1);
KLB_API int klb_wnd_draw_opt2(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2);
KLB_API int klb_wnd_draw_opt3(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3);
KLB_API int klb_wnd_draw_opt4(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4);
KLB_API int klb_wnd_draw_opt5(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5);
KLB_API int klb_wnd_draw_opt6(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6);
KLB_API int klb_wnd_draw_opt7(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7);
KLB_API int klb_wnd_draw_opt8(klb_wnd_t* p_wnd, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WND_H__
//end
