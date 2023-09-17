///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_btnex.h
/// @brief   klb window button extend, 扩展按钮
///          含基本按钮功能, 外加:
///          1. 一些固定种类图标[1, 999]
///          2. 可扩展种类, 及对应绘图
///          3. 可附加多种值类型, 默认为string
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_BTNEX_H__
#define __KLBWND_BTNEX_H__

#if defined(__cplusplus)
extern "C" {
#endif


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbthird/sds.h"
#include "klbutil/klb_map.h"


/// @brief 扩展按钮类型 [0, 999] 为内置定义范围
#define KLBWND_BTNEX_normal         0       ///< 常规
#define KLBWND_BTNEX_klbuimax       999     ///< 内置最大值


/// @enum  klbwnd_btnex_type_e
/// @brief 公共使用的按钮类型
typedef enum klbwnd_btnex_type_e_
{
    KLBWND_BTNEX_rectangle          = 20,   ///< 实体矩形
    KLBWND_BTNEX_triangle_left,             ///< 实体向左三角(直角,边角45度)
    KLBWND_BTNEX_triangle_up,               ///< 实体向上三角(直角,边角45度)
    KLBWND_BTNEX_triangle_right,            ///< 实体向右三角(直角,边角45度)
    KLBWND_BTNEX_triangle_down,             ///< 实体向下三角(直角,边角45度)

    KLBWND_BTNEX_line_plus          = 30,   ///< 实线符号 "+"
    KLBWND_BTNEX_line_minus,                ///< 实线符号 "-"
    KLBWND_BTNEX_line_x,                    ///< 实线符号 "X"
    KLBWND_BTNEX_line_1,                    ///< 实线符号 ">"
    KLBWND_BTNEX_line_2,                    ///< 实线符号 ">|"
    KLBWND_BTNEX_line_3,                    ///< 实线符号 "<"
    KLBWND_BTNEX_line_4,                    ///< 实线符号 "|<"
}klbwnd_btnex_type_e;


typedef struct klbwnd_btnex_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbwnd_btnex_css_t;


typedef struct klbwnd_btnex_t_
{
    klbwnd_btnex_css_t*     p_css;          ///< 样式

    int                     type;           ///< 类型: KLBWND_BTNEX_normal

    int                     index;          ///< 序号
    sds                     title;          ///< 标题

    sds                     value;          ///< 值
    int                     i_value;        ///< 整形值
    bool                    b_value;        ///< 布尔值

    klb_map_t*              p_map;          ///< map; 复杂数据存放
}klbwnd_btnex_t;


/// @brief init/quit/create
KLB_API void klbwnd_btnex_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_btnex_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_btnex_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_btnex_css_init(klbwnd_btnex_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_btnex_css_quit(klbwnd_btnex_css_t* p_css);
KLB_API void klbwnd_btnex_css_copy(klbwnd_btnex_css_t* p_dst, klbwnd_btnex_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_btnex_set_css(klb_wnd_t* p_wnd, klbwnd_btnex_css_t* p_css);


/// @brief 设置/获取类型
KLB_API void klbwnd_btnex_set_type(klb_wnd_t* p_wnd, int type);
KLB_API int klbwnd_btnex_get_type(klb_wnd_t* p_wnd);


/// @brief 设置/获取类型字符串
KLB_API void klbwnd_btnex_set_type_str(klb_wnd_t* p_wnd, const char* p_type);
KLB_API const char* klbwnd_btnex_get_type_str(klb_wnd_t* p_wnd);


/// @brief 设置/获取序号
KLB_API void klbwnd_btnex_set_index(klb_wnd_t* p_wnd, int index);
KLB_API int klbwnd_btnex_get_index(klb_wnd_t* p_wnd);


/// @brief set/get title string
KLB_API void klbwnd_btnex_set_title(klb_wnd_t* p_wnd, const char* p_title);
KLB_API const sds klbwnd_btnex_get_title(klb_wnd_t* p_wnd);


/// @brief set/get value
KLB_API void klbwnd_btnex_set_value(klb_wnd_t* p_wnd, const char* p_value);
KLB_API const sds klbwnd_btnex_get_value(klb_wnd_t* p_wnd);


/// @brief set/get value int
KLB_API void klbwnd_btnex_set_value_int(klb_wnd_t* p_wnd, int value);
KLB_API int klbwnd_btnex_get_value_int(klb_wnd_t* p_wnd);


/// @brief set/get value bool
KLB_API void klbwnd_btnex_set_value_bool(klb_wnd_t* p_wnd, bool value);
KLB_API bool klbwnd_btnex_get_value_bool(klb_wnd_t* p_wnd);


/// @brief get map; 基本类型解决不了, 用map吧
KLB_API klb_map_t* klbwnd_btnex_get_map(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_BTNEX_H__
//end
