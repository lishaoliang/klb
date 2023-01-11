///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_time.h
/// @brief   处理时间部分的扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_TIME_H__
#define __KLBUIEX_TIME_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbuiex_time_t_ klbuiex_time_t;


int klbuiex_register_time(klb_gui_t* p_gui);
klbuiex_time_t* klbuiex_get_time(klb_gui_t* p_gui);



#ifdef __cplusplus
}
#endif

#endif // __KLBUIEX_TIME_H__
//end
