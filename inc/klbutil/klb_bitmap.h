///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_bitmap.h
/// @brief   bitmap相关
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BITMAP_H__
#define __KLB_BITMAP_H__

#include "klb_type.h"
#include "klbutil/klb_canvas.h"

#if defined(__cplusplus)
extern "C" {
#endif


klb_canvas_t* klb_bitmap_read(const char* p_filename, int color_fmt);


int klb_bitmap_write(const char* p_filename, const klb_canvas_t* p_canvas);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BITMAP_H__
//end
