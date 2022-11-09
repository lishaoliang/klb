///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_wnd.h
/// @brief   windows sdl window, 窗口部分
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_WND_H__
#define __WSDL_WND_H__


#include "klb_type.h"
#include "SDL.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_wnd_t_ wsdl_wnd_t;


wsdl_wnd_t* wsdl_wnd_create();
void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd);


#ifdef __cplusplus
}
#endif


#endif // __WSDL_WND_H__
//end
