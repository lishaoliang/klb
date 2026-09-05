//////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_extensions.h
/// @brief   GUI 扩展
///     1. 将GUI内部不同模块分离单独处理
///     2. 容许使用者可以参与GUI内部流程处理
/// @version 0.3
///   \n [2025-6] 添加渲染扩展模块
///   \n [2025-7] 添加 用户自定义图层(udatalayer) 扩展模块
///   \n [2025-7] 添加 等待图层(waitlayer) 扩展模块
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_EXTENSIONS_H__
#define __KLBUIEX_EXTENSIONS_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/extensions/klbuiex_wndhash.h"
#include "klbgui/extensions/klbuiex_render.h"
#include "klbgui/extensions/klbuiex_wndticker.h"
#include "klbgui/extensions/klbuiex_redraw.h"
#include "klbgui/extensions/klbuiex_default.h"
#include "klbgui/extensions/klbuiex_shwnd.h"
#include "klbgui/extensions/klbuiex_udatalayer.h"
#include "klbgui/extensions/klbuiex_waitlayer.h"
#include "klbgui/extensions/klbuiex_tip.h"
#include "klbgui/extensions/klbuiex_util.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLBUIEX_register_extensions_std
/// @brief 注册内部标准扩展
#define KLBUIEX_register_extensions_std(GUI_) \
{ \
    klbuiex_register_wndhash((GUI_));   \
    klbuiex_register_render((GUI_));    \
    klbuiex_register_redraw((GUI_));    \
    klbuiex_register_wndticker((GUI_)); \
    klbuiex_register_default((GUI_));   \
    klbuiex_register_shwnd((GUI_));     \
    klbuiex_register_udatalayer((GUI_));\
    klbuiex_register_waitlayer((GUI_)); \
    klbuiex_register_tip((GUI_));       \
    klbuiex_register_util((GUI_));      \
}


#ifdef __cplusplus
}
#endif

#endif // __KLBUIEX_EXTENSIONS_H__
//end
