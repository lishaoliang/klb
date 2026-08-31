// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_tab_btn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   Tab页面中的按钮
///          klbwnd_tab_t 专用
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_TAB_BTN_H__
#define __KLBWND_TAB_BTN_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"
#include "klbwui/embed_wnd/klbwnd_tab.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klbwnd_tab_btn_t_
{
    klbwnd_tab_btn_css_t*   p_css;          ///< 样式

    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    // 非CSS属性
    struct
    {
        int                 index;          ///< 序号
        klb_wnd_t*          p_bind_wnd;     ///< 绑定的窗口
        bool                is_check;       ///< 是否选中
    };
}klbwnd_tab_btn_t;


/// @brief init/deinit/create
void klbwnd_tab_btn_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_tab_btn_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_tab_btn_create(klb_gui_t* p_gui, int x, int y, int w, int h);

/// @brief css init/deinit
void klbwnd_tab_btn_css_init(klbwnd_tab_btn_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_tab_btn_css_deinit(klbwnd_tab_btn_css_t* p_css);
void klbwnd_tab_btn_css_copy(klbwnd_tab_btn_css_t* p_dst, klbwnd_tab_btn_css_t* p_src);

/// @brief set css
void klbwnd_tab_btn_set_css(klb_wnd_t* p_wnd, klbwnd_tab_btn_css_t* p_css);

/// @brief set/get title
void klbwnd_tab_btn_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_tab_btn_get_title(klb_wnd_t* p_wnd);

/// @brief set/get value
void klbwnd_tab_btn_set_value(klb_wnd_t* p_wnd, const char* p_value);
const sds klbwnd_tab_btn_get_value(klb_wnd_t* p_wnd);

/// @brief 设置/获取序号
void klbwnd_tab_btn_set_index(klb_wnd_t* p_wnd, int idx);
int klbwnd_tab_btn_get_index(klb_wnd_t* p_wnd);

/// @brief 设置/获取 绑定的窗口
void klbwnd_tab_btn_set_bind_wnd(klb_wnd_t* p_wnd, klb_wnd_t* p_dst);
klb_wnd_t* klbwnd_tab_btn_get_bind_wnd(klb_wnd_t* p_wnd);

/// @brief 设置选中
void klbwnd_tab_btn_check(klb_wnd_t* p_wnd, bool check);

/// @brief 依绑定窗口, 更新标题
void klbwnd_tab_btn_update_title(klb_wnd_t* p_wnd);

/// @brief 获取标题需要的尺寸
int  klbwnd_tab_btn_title_size(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h);


#ifdef __cplusplus
}
#endif

#endif // __KLBWND_TAB_BTN_H__

// end
