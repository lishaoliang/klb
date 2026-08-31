// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_listexex.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window listex extend; 扩展列表框
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_LISTEX_H__
#define __KLBWND_LISTEX_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"
#include "klbwui/embed_wnd/klbwnd_check.h"
#include "klbwui/embed_wnd/klbwnd_vscrollbar.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_listex_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_listex_attributes_t;

typedef struct klbwnd_listex_row_attributes_t_
{
    klbuicss_text_t             text;           ///< color, text-align
    klbuicss_font_t             font;           ///< font-size
    klbuicss_background_t       background;     ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;         ///< border-width, border-color
} klbwnd_listex_row_attributes_t;

#define KLBWND_LISTEX_column_max      12      ///< 列最大值
#define KLBWND_LISTEX_row_max         64      ///< 行最大值


typedef struct klbwnd_listex_row_t_ klbwnd_listex_row_t;


typedef struct klbwnd_listex_head_column_t_
{
    bool        enable;
    int         width;
    sds         title;

    klb_map_t*  p_child;                    ///< 子窗口属性; 若有, 则表示含有子窗口
}klbwnd_listex_head_column_t;

typedef struct klbwnd_listex_head_t_
{
    klbwnd_listex_head_column_t column[KLBWND_LISTEX_column_max];
    int                         column_count;
}klbwnd_listex_head_t;


typedef struct klbwnd_listex_row_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbwnd_listex_row_attributes_t normal;         ///< normal 常规状态参数
    klbwnd_listex_row_attributes_t focus;          ///< focus 聚焦状态参数
    klbwnd_listex_row_attributes_t disable;        ///< disable 不使能状态参数

    uint32_t    normal_background_color2;   ///< normal 第二行背景颜色
    uint32_t    focus_background_color2;    ///< focus 第二行背景颜色
    uint32_t    disable_background_color2;  ///< disable 第二行背景颜色
}klbwnd_listex_row_css_t;


typedef struct klbwnd_listex_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbwnd_listex_attributes_t normal;         ///< normal 常规状态参数
    klbwnd_listex_attributes_t focus;          ///< focus 聚焦状态参数
    klbwnd_listex_attributes_t disable;        ///< disable 不使能状态参数

    uint32_t    title_background_color;     ///< 标题背景颜色
    uint32_t    title_line_color;           ///< 标题线条颜色

    klbwnd_listex_row_css_t css_listex_row;     ///< 行控件CSS
    klbwnd_check_css_t      css_check;          ///< check控件CSS
    klbwnd_vscrollbar_css_t css_vscrollbar;     ///< 垂直滚动条CSS
}klbwnd_listex_css_t;


typedef struct klbwnd_listex_t_
{
    klbwnd_listex_css_t*    p_css;          ///< 样式
    sds                     tmp;            ///< 临时使用参数

    // 题头
    klbwnd_listex_head_t    head;

    // 子控件: klbwnd_listex_row_t
    klb_wnd_t*              p_rows[KLBWND_LISTEX_row_max]; ///< 
    int                     rows_count;     ///< 

    // 右侧垂直滚动条
    struct
    {
        klb_wnd_t*          p_vscrollbar;   ///< 右侧垂直滚动条

        int                 vsc_min;
        int                 vsc_max;
        int                 vsc_value;
    };

    // 第0列, 选择框
    struct
    {
        klb_wnd_t*          p_check;        ///< 全选选中框(左上角)
        int                 w0;
    };

    // 事件控件
    struct
    {
        klb_wnd_t*          p_event_wnd;
    };

    int                     sel;            ///< 当前选中的行
    bool                    enable_sel;     ///< 是能选中单行

    // 数据
    klb_map_t               data_map;       ///< 数据
}klbwnd_listex_t;


/// @brief init/deinit/create
void klbwnd_listex_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_listex_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_listex_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_listex_css_init(klbwnd_listex_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_listex_css_deinit(klbwnd_listex_css_t* p_css);
void klbwnd_listex_css_copy(klbwnd_listex_css_t* p_dst, klbwnd_listex_css_t* p_src);


/// @brief set css
void klbwnd_listex_set_css(klb_wnd_t* p_wnd, klbwnd_listex_css_t* p_css);


/// @brief 添加列
int klbwnd_listex_append_column(klb_wnd_t* p_wnd, int w_column, const char* p_title, const klb_map_t* p_child);

/// @brief 数据列表
klb_map_t* klbwnd_listex_get_data_map(klb_wnd_t* p_wnd);

/// @brief 重新布局
void klbwnd_listex_relayout(klb_wnd_t* p_wnd);

/// @brief 清空
void klbwnd_listex_clear(klb_wnd_t* p_wnd);

/// @brief 清空数据
void klbwnd_listex_clear_data(klb_wnd_t* p_wnd);

/// @brief 设置/获取选中的数据
void klbwnd_listex_set_sel(klb_wnd_t* p_wnd, int sel);
klb_map_t* klbwnd_listex_get_sel(klb_wnd_t* p_wnd, int* p_sel);

/// @brief 获取当前事件窗口
klb_wnd_t* klbwnd_listex_get_event_wnd(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_LISTEX_H__

// end
