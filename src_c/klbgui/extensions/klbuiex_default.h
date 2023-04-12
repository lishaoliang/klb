///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_default.h
/// @brief   处理gui默认值
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_DEFAULT_H__
#define __KLBUIEX_DEFAULT_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_default.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbuiex_default_t_ klbuiex_default_t;


int klbuiex_register_default(klb_gui_t* p_gui);
klbuiex_default_t* klbuiex_get_default(klb_gui_t* p_gui);


klbui_default_t* klbuiex_default_get_value(klbuiex_default_t* p_default);


// 通过 Lua 获取, 设置默认CSS属性
int klbuiex_default_css_set(klbuiex_default_t* p_default, const klb_map_t* p_map);
klb_map_t* klbuiex_default_css_get(klbuiex_default_t* p_default, const klb_map_t* p_map);


#ifdef __cplusplus
}
#endif

#endif // __KLBUIEX_DEFAULT_H__
//end
