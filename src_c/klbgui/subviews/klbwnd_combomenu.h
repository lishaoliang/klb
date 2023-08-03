///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_combomenu.h
/// @brief   klb window combo menu; "kcombo"控件专用菜单
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_COMBOMENU_H__
#define __KLBWND_COMBOMENU_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_sds.h"
#include "klbgui/subviews/klbwnd_combomenu_item.h"
#include "klbgui/wnd/klbwnd_vscrollbar.h"
#include "klbgui/shwnd/klbshw_combomenu.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define KLBWND_COMBOMENU_item_max       9       ///< 一页最大的选项数目

typedef struct klbwnd_combomenu_css_t_
{
    klbuicss_margin_t           margin;         ///< 外边距
    klbuicss_padding_t          padding;        ///< 内边距

    klbuicssex_attributes_t     normal;         ///< normal 常规状态参数

    klbwnd_combomenu_item_css_t css_item;       ///< 选项的 css
    klbwnd_vscrollbar_css_t     css_vscrollbar; ///< 垂直滚动条的 css
}klbwnd_combomenu_css_t;


typedef struct klbwnd_combomenu_t_
{
    klbwnd_combomenu_css_t* p_css;          ///< 样式

    // 子窗口
    int                     item_count;                             ///< 窗口能容纳的item数目
    klb_wnd_t*              p_items[KLBWND_COMBOMENU_item_max];     ///< 选项按钮: klbwnd_combomenu_item_create
    klb_wnd_t*              p_vscrollbar;                           ///< 垂直滚动条: klbwnd_vscrollbar_create

    struct
    {
        int                 array_size;             ///< 数组大小
        klb_map_t*          p_ref_data_array;       ///< 绑定的数据引用
        int                 need_w;                 ///< 预估需要的宽度  
        int                 need_h;                 ///< 预估需要的高度
    };

    // 回调
    struct
    {
        klbshw_combomenu_result_cb  cb_result;      ///< 结束之后的回调函数
        void*                       ptr;            ///< 绑定的指针
    };
}klbwnd_combomenu_t;


/// @brief init/quit/create
void klbwnd_combomenu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_combomenu_quit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_combomenu_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/quit
void klbwnd_combomenu_css_init(klbwnd_combomenu_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_combomenu_css_quit(klbwnd_combomenu_css_t* p_css);


/// @brief set css
void klbwnd_combomenu_set_css(klb_wnd_t* p_wnd, klbwnd_combomenu_css_t* p_css);


/// @brief 绑定数据等
int klbwnd_combomenu_bind(klb_wnd_t* p_wnd, klbshw_combomenu_result_cb cb_result, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h);



#if defined(__cplusplus)
}
#endif

#endif // __KLBWND_COMBOMENU_H__
//end
