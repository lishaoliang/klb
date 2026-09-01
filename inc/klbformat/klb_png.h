// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_png.h
/// @brief   PNG 读写
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_PNG_H__
#define __KLB_PNG_H__

#include "klb_type.h"
#include "klbutil/klb_canvas.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 读取 PNG 为画布
/// @param [in]  *p_filename    文件路径
/// @param [in]  color_fmt      目标色格式
/// @return klb_canvas_t* 成功非 NULL; 失败 NULL
/// @note 主路径: 8bit 非交错 (灰/RGB/索引/灰A/RGBA); 依赖 zlib 解 IDAT
///  \n no-zlib 或非法文件时返回 NULL
KLB_API klb_canvas_t* klb_png_read(const char* p_filename, int color_fmt);


/// @brief 将画布写为 PNG
/// @param [in]  *p_filename    文件路径
/// @param [in]  *p_canvas      画布
/// @return 0.成功; 非0.失败
/// @note 桩: 固定返回 1
KLB_API int klb_png_write(const char* p_filename, const klb_canvas_t* p_canvas);


#ifdef __cplusplus
}
#endif

#endif // __KLB_PNG_H__

// end
