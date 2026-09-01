// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_bitmap.h
/// @brief   BMP 读写
/// @version 0.1
/// @history 修改历史
///  \n [2020] 创建文件
///  \n [2026] 自 klbutil 迁入 klbformat
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BITMAP_H__
#define __KLB_BITMAP_H__

#include "klb_type.h"
#include "klbutil/klb_canvas.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API klb_canvas_t* klb_bitmap_read(const char* p_filename, int color_fmt);


KLB_API int klb_bitmap_write(const char* p_filename, const klb_canvas_t* p_canvas);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BITMAP_H__

// end
