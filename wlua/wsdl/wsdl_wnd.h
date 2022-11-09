///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_wnd.h
/// @brief   windows sdl window, 窗口(对话框)部分
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_WND_H__
#define __WSDL_WND_H__


#include "klb_type.h"
#include "SDL.h"
#include "klbutil/klb_canvas.h"
#include "klbgui/klb_gui.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_wnd_t_
{
    bool                open;
    
    SDL_Window*         p_window;
    SDL_Renderer*       p_render;

    SDL_Surface*        p_surface;
    SDL_Texture*        p_texture;

    SDL_Texture*        p_tex_text;
}wsdl_wnd_t;


wsdl_wnd_t* wsdl_wnd_create();
void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd);


int wsdl_wnd_open(wsdl_wnd_t* p_wnd, klb_gui_t* p_gui, int w, int h, const char* p_title);
void wsdl_wnd_close(wsdl_wnd_t* p_wnd);



#ifdef __cplusplus
}
#endif


#endif // __WSDL_WND_H__
//end
