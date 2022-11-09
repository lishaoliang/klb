///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_images.h
/// @brief   windows sdl images, 图片部分
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_IMAGES_H__
#define __WSDL_IMAGES_H__


#include "klb_type.h"
#include "SDL.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_images_t_ wsdl_images_t;


wsdl_images_t* wsdl_images_create();
void wsdl_images_destroy(wsdl_images_t* p_imgs);

int wsdl_images_load(wsdl_images_t* p_imgs, SDL_Renderer* p_render, const char* p_key, const char* p_path);


SDL_Texture* wsdl_images_find(wsdl_images_t* p_imgs, SDL_Renderer* p_render, const char* p_path);


#ifdef __cplusplus
}
#endif


#endif // __WSDL_IMAGES_H__
//end
