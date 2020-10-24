///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_gui_in.h
/// @brief   GUI内部定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GUI_IN_H__
#define __KLB_GUI_IN_H__

#include "klb_type.h"
#include "gui/klb_gui.h"
#include "gui/klb_wnd.h"
#include "list/klb_list.h"
#include "hash/klb_hlist.h"
#include "thread/klb_mutex.h"
#include "thread/klb_thread.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_msg_t_
{
    int         msg;
    klb_point_t pt1;
    klb_point_t pt2;
    int         lparam;
    int         wparam;
}klb_msg_t;


#define KLB_GUI_POPUP_WND_MAX           16


typedef struct klb_gui_t_
{
    // 画布
    struct
    {
        /// @brief 绑定的主显存画布
        klb_canvas_t*   p_canvas;
    };

    // 窗口管理
    struct
    {
        klb_list_t*     p_top_list;         ///< 顶层对话框列表

        /// @brief 窗口类型查找表
        ///  \n 通过"klb_gui_register"函数注册
        ///  \n 查找指定类型的创建函数
        klb_hlist_t* p_wnd_type_hlist;

        /// @brief 窗口查找表
        ///  \n 存储通过"klb_gui_append"函数加入进来的窗口
        ///  \n 在需要的时候通过路径名提取
        klb_hlist_t* p_wnd_hlist;

        /// @brief 当前显示的弹出窗口栈
        ///  \n 绘制UI,系统消息等
        klb_wnd_t* p_wnd_popup[KLB_GUI_POPUP_WND_MAX];

        /// @brief 当前显示的弹出窗口数
        int wnd_popup_num;
    };

    // 聚焦等
    struct
    {
        klb_wnd_t*      p_focus;
    };

    // 刷新
    struct
    {
        bool            redraw;
        bool            refresh;
    };

    // 消息
    struct
    {
        klb_list_t*     p_msg_list;
        klb_mutex_t*    p_msg_mutex;
    };

    // 线程
    struct
    {
        klb_thread_t*   p_thread1;
    };
}klb_gui_t;


int klb_gui_pop_message(klb_gui_t* p_gui, klb_msg_t** p_msg);

int klb_gui_dispatch_message(klb_gui_t* p_gui, klb_msg_t* p_msg);

int klb_gui_process_message_once(klb_gui_t* p_gui);

int klb_gui_redraw(klb_gui_t* p_gui);

int klb_gui_update_rect(klb_gui_t* p_gui, const klb_rect_t* p_rect);
int klb_gui_refresh(klb_gui_t* p_gui);


//////////////////////////////////////////////////////////////////////////

int klb_gui_register_ktype(klb_gui_t* p_gui);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_IN_H__
//end
