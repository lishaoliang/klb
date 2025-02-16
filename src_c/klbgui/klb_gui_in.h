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
        klbuiex_wndhash_t*      p_wndhash;              ///< 窗口创建,查找等
        klbuiex_wndticker_t*    p_wndticker;            ///< 窗口定时器
        klbuiex_redraw_t*       p_redraw;               ///< 重绘记录
        klbuiex_tip_t*          p_tip;                  ///< tip
        klbuiex_util_t*         p_util;                 ///< util
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
    };

    // 聚焦等
    struct
    {
        klb_wnd_t*      p_focus_top;    ///< 顶层聚焦窗口
        klb_wnd_t*      p_focus;        ///< 当前聚焦窗口

        int64_t         focus_tc;       ///< 聚焦时的时间点(单位毫秒, 系统滴答数)
        int64_t         focusdelay_tc;  ///< 聚焦延时 触发的时间间隔(单位毫秒, 默认600)
        bool            focusdelay;     ///< true. 可能需要触发 KLBUI_focusdelay 事件
    };

    // 消息处理流程
    struct
    {
        int64_t         loop_tc;                    ///< 当前时间
        bool            is_drop_msg_dispatch;       ///< 是否放弃消息"冒泡"
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

    // cpp的 指针支持
    struct
    {
        void*           p_cppgui;     ///< klbui::CGui*
    };

    // Bug. 在Lua里面直接使用 klb_gui_clear函数, 会导致在自身的GUI流程中, 销毁GUI框架本身
    // 这里专门提供异步延时清理机制, klb_gui_clear_async 函数标记清理, GUI框架在合适的时机
    // 执行清理, 清理完成之后, 再使用回调函数通知调用者
    // e. 若有更好方案, 可以修改这里
    struct
    {
        bool                    is_need_clear;      ///< 是否需要清理

        klb_gui_clear_result_cb cb_clear_result;    ///< 清理之后的回调函数
        void*                   p_clear_result;     ///< 附加指针
    };
}klb_gui_t;


// 支持c++ klbui::CGui

// 更新窗口
int klb_gui_update_wnd(klb_gui_t* p_gui, klb_wnd_t* p_wnd);

// 更新tip
int klb_gui_update_tip(klb_gui_t* p_gui, const char* p_tip);


//////////////////////////////////////////

// 设置/获取 放弃消息"冒泡"
// 在有些情况下, 可能需要终止消息分发
void klb_gui_drop_msg_dispatch(klb_gui_t* p_gui, bool drop);
bool klb_gui_is_drop_msg_dispatch(klb_gui_t* p_gui);


//////////////////////////////////////////////////////////////////////////

/// @brief 向css 属性表中添加 标准处理函数
/// @param [in] *p_css_map      css属性表
/// @param [in] *ptr            组件
/// @return 无
void klb_gui_css_map_append_std_function(klb_map_t* p_css_map, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_IN_H__
//end
