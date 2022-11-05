///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    ft_raster.h
/// @brief   freetype2 文本绘制
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __FT_RASTER_H__
#define __FT_RASTER_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct ft_raster_t_ ft_raster_t;


ft_raster_t* ft_raster_create(const char* p_font_path);
void ft_raster_destroy(ft_raster_t* p_ft);


typedef struct ft_raster_pixels_t_
{
    uint8_t*    p_pixels;
    int64_t     pitch;
    int         w;
    int         h;
    int         color_fmt;
}ft_raster_pixels_t;

int ft_raster_text(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h);


#ifdef __cplusplus
}
#endif

#endif // __FT_RASTER_H__
//end
