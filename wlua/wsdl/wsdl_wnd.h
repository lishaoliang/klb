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
#include "ft_raster.h"
#include "wsdl_images.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_wnd_t_ wsdl_wnd_t;


wsdl_wnd_t* wsdl_wnd_create();
void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd);


int wsdl_wnd_open(wsdl_wnd_t* p_wnd, klb_gui_t* p_gui, int w, int h, const char* p_title);
void wsdl_wnd_close(wsdl_wnd_t* p_wnd);

bool wsdl_wnd_is_open(wsdl_wnd_t* p_wnd);

SDL_Renderer* wsdl_wnd_get_render(wsdl_wnd_t* p_wnd);


// 立即刷新显存
void wsdl_wnd_render_present(wsdl_wnd_t* p_wnd);


// 刷新所有
void wsdl_wnd_refresh(wsdl_wnd_t* p_wnd);


// UI画布接口
int wsdl_wnd_set_draw_color(wsdl_wnd_t* p_wnd, uint32_t color);
uint32_t wsdl_wnd_get_draw_color(wsdl_wnd_t* p_wnd);

int wsdl_wnd_set_font_height(wsdl_wnd_t* p_wnd, int h);
int wsdl_wnd_get_font_height(wsdl_wnd_t* p_wnd);

int wsdl_wnd_draw_clear(wsdl_wnd_t* p_wnd);
int wsdl_wnd_draw_point(wsdl_wnd_t* p_wnd, int x, int y);
int wsdl_wnd_draw_points(wsdl_wnd_t* p_wnd, const klb_point_t* p_points, int count);
int wsdl_wnd_draw_line(wsdl_wnd_t* p_wnd, int x1, int y1, int x2, int y2);
int wsdl_wnd_draw_lines(wsdl_wnd_t* p_wnd, const klb_point_t* p_points, int count);
int wsdl_wnd_draw_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect);
int wsdl_wnd_draw_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count);
int wsdl_wnd_draw_fill_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect);
int wsdl_wnd_draw_fill_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count);
int wsdl_wnd_draw_text(wsdl_wnd_t* p_wnd, ft_raster_t* p_ft, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len);
int wsdl_wnd_draw_image(wsdl_wnd_t* p_wnd, wsdl_images_t* p_images, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);

int wsdl_wnd_refresh_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect);
int wsdl_wnd_refresh_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count);


// 视频接口
int wsdl_wnd_video_update(wsdl_wnd_t* p_wnd, int idx);
int wsdl_wnd_video_set_pos(wsdl_wnd_t* p_wnd, int idx, const klb_rect_t* p_dst_rect, const klb_rect_t* p_src_rect);


#ifdef __cplusplus
}
#endif


#endif // __WSDL_WND_H__
//end
