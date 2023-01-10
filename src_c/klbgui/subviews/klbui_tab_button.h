///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_tab_button.h
/// @brief   "ktab"的私有子控件
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_TAB_BUTTON_H__
#define __KLBUI_TAB_BUTTON_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"


#if defined(__cplusplus)
extern "C" {
#endif

/// @struct klbui_tab_botton_t
/// @brief  ktab私有控件
typedef struct klbui_tab_botton_t_
{
    sds                     title;          ///< 标题

    // normal
    klbuicss_margin_t       margin;         ///< 外边框
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t check;          ///< check 选中状态参数

    // 非CSS属性
    struct
    {
        int                 index;          ///< 序号
        klb_wnd_t*          p_bind_wnd;     ///< 绑定的窗口
        bool                is_check;       ///< 是否选中
    };
}klbui_tab_botton_t;


klb_wnd_t* klbui_tab_botton_create(klb_gui_t* p_gui, int x, int y, int w, int h);

void klbui_tab_botton_set_index(klb_wnd_t* p_wnd, int idx);
int klbui_tab_botton_get_index(klb_wnd_t* p_wnd);

void klbui_tab_botton_bind_wnd(klb_wnd_t* p_wnd, klb_wnd_t* p_dst);
klb_wnd_t* klbui_tab_botton_get_bind_wnd(klb_wnd_t* p_wnd);

void klbui_tab_botton_check(klb_wnd_t* p_wnd, bool check);

void klbui_tab_botton_update_title(klb_wnd_t* p_wnd);


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_TAB_BUTTON_H__
//end
