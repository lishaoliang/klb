///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_extension.h
/// @brief   windows sdl extension, Lua扩展, SDL平台底层接口
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_EXTENSION_H__
#define __WSDL_EXTENSION_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbutil/klb_rect.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_extension_t_ wsdl_extension_t;

int kluaex_register_wsdl(klua_env_t* p_env);

wsdl_extension_t* kluaex_get_wsdl(klua_env_t* p_env);
wsdl_extension_t* kluaex_get_wsdl_by_L(lua_State* L);


int kluaex_wsdl_open_wnd(wsdl_extension_t* p_ex, const char* p_font_path, int w, int h, const char* p_title);
int kluaex_wsdl_close_wnd(wsdl_extension_t* p_ex);

int kluaex_wsdl_push_media(wsdl_extension_t* p_ex, int chnn, int sidx, klb_buf_t* p_media);

int kluaex_wsdl_set_video_pos(wsdl_extension_t* p_ex, int idx, const klb_rect_t* p_dst_rect);


#ifdef __cplusplus
}
#endif

#endif // __WSDL_EXTENSION_H__
//end
