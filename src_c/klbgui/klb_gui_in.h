///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_gui_in.h
/// @brief   GUI内部定义
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GUI_IN_H__
#define __KLB_GUI_IN_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_wnd.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_hlist.h"
#include "klbplatform/klb_mutex.h"
#include "klbutil/klb_map.h"
#include "klbthird/sds.h"
#include "klbgui/extensions/klbuiex_extensions.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_gui_extension_activated_t
/// @brief  激活的扩展
typedef struct klb_gui_extension_activate_t_
{
    void*                   ptr;        ///< 扩展的指针
    sds                     name;       ///< 扩展的名称
    klb_gui_extension_t     ex;         ///< 扩展的接口函数
}klb_gui_extension_activated_t;


typedef struct klb_msg_t_
{
    int         msg;
    klb_point_t pt1;
    klb_point_t pt2;
    int         lparam;
    int         wparam;
}klb_msg_t;


#define KLBUI_MODAL_WND_MAX           16
#define KLBUI_POPUP_WND_MAX           4


typedef struct klb_gui_t_
{
    // 画布
    struct
    {
        /// @brief 绑定的主显存画布
        klb_canvas_t*   p_canvas;
    };

    // 扩展: 将gui部分功能分散处理
    struct
    {
        klb_hlist_t*    p_extension_hlist;              ///< 注册的扩展; klb_gui_extension_t*
        klb_hlist_t*    p_extension_activated_hlist;    ///< 激活的扩展; klb_gui_extension_activated_t*
    };

    // 常用的扩展
    struct
    {
        klbuiex_wndhash_t*  p_wndhash;                  ///< 窗口创建,查找等
        klbuiex_redraw_t*   p_redraw;                   ///< 重绘记录
        klbuiex_util_t*     p_util;                     ///< util
    };

    // 当前绘制窗口, 窗口显示次序
    struct
    {
        /// @brief modal 方式显示的窗口栈
        int             modal_num;
        klb_wnd_t*      p_modal_wnd[KLBUI_MODAL_WND_MAX];

        /// @brief popup 方式显示的窗口栈 
        int             popup_num;
        klb_wnd_t*      p_popup_wnd[KLBUI_POPUP_WND_MAX];

        klb_wnd_t*      p_msg_box;      ///< message box
        klb_wnd_t*      p_tip;          ///< tip
    };

    // 聚焦等
    struct
    {
        klb_wnd_t*      p_focus_top;
        klb_wnd_t*      p_focus;
    };

    // 消息
    struct
    {
        klb_nlist_t*    p_msg_list;
        klb_mutex_t*    p_msg_mutex;
    };

    // lua env
    struct
    {
        klua_env_t*     p_klua_env;
    };
}klb_gui_t;


// 更新窗口
int klb_gui_update_wnd(klb_gui_t* p_gui, klb_wnd_t* p_wnd);

// 重绘
int klb_gui_redraw(klb_gui_t* p_gui);

// 刷新
int klb_gui_refresh(klb_gui_t* p_gui);


//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_IN_H__
//end
