// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_list_row.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   list 列表框控件(klbwnd_list_t), "每行"子控件
///          klbwnd_list_t 专用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_LIST_ROW_H__
#define __KLBWND_LIST_ROW_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbwnd_list_head_t_ klbwnd_list_head_t;
typedef struct klbwnd_list_row_css_t_ klbwnd_list_row_css_t;


typedef struct klbwnd_list_row_t_
{
    sds                     title;          ///< 显示标题
    int                     idx;            ///< 排到的序号
    bool                    check;          ///< 选中状态

    klbwnd_list_row_css_t*  p_css;          ///< 样式

    klbwnd_list_head_t*     p_head;         ///< 列表头
    klb_map_t*              p_show_data;    ///< 当前行数据
}klbwnd_list_row_t;


/// @brief init/deinit/create
void klbwnd_list_row_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_list_row_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_list_row_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_list_row_css_init(klbwnd_list_row_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_list_row_css_deinit(klbwnd_list_row_css_t* p_css);
void klbwnd_list_row_css_copy(klbwnd_list_row_css_t* p_dst, klbwnd_list_row_css_t* p_src);


/// @brief set css
void klbwnd_list_row_set_css(klb_wnd_t* p_wnd, klbwnd_list_row_css_t* p_css);


/// @brief 设置序号
void klbwnd_list_row_set_index(klb_wnd_t* p_wnd, int idx);

/// @brief 设置/获取选中
void klbwnd_list_row_set_check(klb_wnd_t* p_wnd, bool check);
bool klbwnd_list_row_get_check(klb_wnd_t* p_wnd);


/// @brief 设置待显示的数据
void klbwnd_list_row_set_show_data(klb_wnd_t* p_wnd, klbwnd_list_head_t* p_head, klb_map_t* p_show_data);



#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_LIST_ROW_H__

// end
