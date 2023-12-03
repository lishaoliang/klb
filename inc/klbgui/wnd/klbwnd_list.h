///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_list.h
/// @brief   klb window list, 列表框控件
///          简易列表框: 
///          1. 提供列表文本显示, 不支持添加内插控件
///          2. 单行选中
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_LIST_H__
#define __KLBWND_LIST_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/wnd/klbwnd_vscrollbar.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_LIST_column_max      12      ///< 列最大值
#define KLBWND_LIST_row_max         64      ///< 行最大值


typedef struct klbwnd_list_row_t_ klbwnd_list_row_t;


typedef struct klbwnd_list_head_column_t_
{
    bool        enable;
    int         width;
    sds         title;
}klbwnd_list_head_column_t;


typedef struct klbwnd_list_head_t_
{
    klbwnd_list_head_column_t   column[KLBWND_LIST_column_max];
    int                         column_count;
}klbwnd_list_head_t;


typedef struct klbwnd_list_row_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    uint32_t    normal_background_color2;   ///< normal 第二行背景颜色
    uint32_t    focus_background_color2;    ///< focus 第二行背景颜色
    uint32_t    disable_background_color2;  ///< disable 第二行背景颜色

    uint32_t    check_background_color;     ///< 选中的背景色
}klbwnd_list_row_css_t;


typedef struct klbwnd_list_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    uint32_t    title_background_color;     ///< 标题背景颜色
    uint32_t    title_line_color;           ///< 标题线条颜色

    klbwnd_list_row_css_t   css_list_row;   ///< 行控件CSS
    klbwnd_vscrollbar_css_t css_vscrollbar; ///< 垂直滚动条CSS
}klbwnd_list_css_t;


typedef struct klbwnd_list_t_
{
    klbwnd_list_css_t*      p_css;          ///< 样式
    sds                     tmp;            ///< 临时使用参数

    // 题头
    klbwnd_list_head_t      head;

    // 子控件: klbwnd_list_row_t
    klb_wnd_t*              p_list_row[KLBWND_LIST_row_max]; ///< 
    int                     list_row_count; ///< 

    // 右侧垂直滚动条
    struct
    {
        klb_wnd_t*          p_vscrollbar;   ///< 右侧垂直滚动条

        int                 vsc_min;
        int                 vsc_max;
        int                 vsc_value;
    };

    int                     sel;            ///< 当前选中的行

    // 数据
    klb_map_t               data_map;       ///< 数据
}klbwnd_list_t;


/// @brief init/quit/create
KLB_API void klbwnd_list_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
KLB_API void klbwnd_list_quit(klb_wnd_t* p_wnd);
KLB_API klb_wnd_t* klbwnd_list_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
KLB_API void klbwnd_list_css_init(klbwnd_list_css_t* p_css, klb_gui_t* p_gui);
KLB_API void klbwnd_list_css_quit(klbwnd_list_css_t* p_css);
KLB_API void klbwnd_list_css_copy(klbwnd_list_css_t* p_dst, klbwnd_list_css_t* p_src);


/// @brief set css
KLB_API void klbwnd_list_set_css(klb_wnd_t* p_wnd, klbwnd_list_css_t* p_css);


/// @brief 添加列
KLB_API int klbwnd_list_append_column(klb_wnd_t* p_wnd, int w_column, const char* p_title);

/// @brief 更新列
KLB_API int klbwnd_list_update_column(klb_wnd_t* p_wnd, int idx, int w_column, const char* p_title);

/// @brief 数据列表
KLB_API klb_map_t* klbwnd_list_get_data_map(klb_wnd_t* p_wnd);

/// @brief 重新布局
KLB_API void klbwnd_list_relayout(klb_wnd_t* p_wnd);

/// @brief 清空数据
KLB_API void klbwnd_list_clear_data(klb_wnd_t* p_wnd);

/// @brief 清空所有
KLB_API void klbwnd_list_clear(klb_wnd_t* p_wnd);

/// @brief 设置/获取选中的数据
KLB_API void klbwnd_list_set_sel(klb_wnd_t* p_wnd, int sel);
KLB_API klb_map_t* klbwnd_list_get_sel(klb_wnd_t* p_wnd, int* p_sel);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_LIST_H__
//end
