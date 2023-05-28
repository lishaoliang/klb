///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_listex_row.h
/// @brief   listex 扩展列表框控件(klbwnd_listex_t), "每行"子控件
///          klbwnd_listex_t 专用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_LISTEX_ROW_H__
#define __KLBWND_LISTEX_ROW_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"
#include "klbgui/wnd/klbwnd_listex.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_listex_head_t_ klbwnd_listex_head_t;
typedef struct klbwnd_listex_row_css_t_ klbwnd_listex_row_css_t;
typedef struct klbwnd_listex_css_t_ klbwnd_listex_css_t;


#define KLBWND_LISTEX_ROW_child_max          4


typedef struct klbwnd_listex_row_t_
{
    sds                         title;          ///< 显示标题
    int                         idx;            ///< 排到的序号
    bool                        b_select;       ///< 选中状态
    bool                        b_check;        ///< check 状态

    klb_map_t*                  p_show_data;    ///< 当前行数据

    klbwnd_listex_row_css_t*    p_row_css;      ///< 样式
    klbwnd_listex_css_t*        p_css;          ///< 整个扩展列表框CSS

    klbwnd_listex_head_t*       p_head;         ///< 列表头

    // 第0列, 选择框
    struct
    {
        klb_wnd_t*              p_check;        ///< 选择框控件
        int                     w0;             ///< 选择框列宽度
    };

    // 最大列数 = KLBWND_LISTEX_column_max
    // 每列最大子控件个数 = KLBWND_LISTEX_ROW_child_max
    klb_wnd_t*                  p_child[KLBWND_LISTEX_column_max][KLBWND_LISTEX_ROW_child_max];
}klbwnd_listex_row_t;


/// @brief init/quit/create
void klbwnd_listex_row_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_listex_row_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_listex_row_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_listex_row_css_init(klbwnd_listex_row_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_listex_row_css_quit(klbwnd_listex_row_css_t* p_css);

/// @brief set css
void klbwnd_listex_row_set_css(klb_wnd_t* p_wnd, klbwnd_listex_row_css_t* p_row_css, klbwnd_listex_css_t* p_css);


/// @brief 设置序号
void klbwnd_listex_row_set_index(klb_wnd_t* p_wnd, int idx);

/// @brief 设置/获取选中
void klbwnd_listex_row_set_select(klb_wnd_t* p_wnd, bool check);
bool klbwnd_listex_row_get_select(klb_wnd_t* p_wnd);

/// @brief 设置题头
void klbwnd_listex_row_set_head(klb_wnd_t* p_wnd, klbwnd_listex_head_t* p_head);

/// @brief 初始化子窗口
void klbwnd_listex_row_child(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* ptr);


/// @brief 设置待显示的数据
void klbwnd_listex_row_set_show_data(klb_wnd_t* p_wnd, klb_map_t* p_show_data);


/// @brief 获取 check 状态
bool klbwnd_listex_row_get_check(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_LISTEX_ROW_H__
//end
