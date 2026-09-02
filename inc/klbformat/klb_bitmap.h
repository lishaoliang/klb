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


/// @brief 读取 BMP 为画布
/// @param [in]  *p_filename    文件路径
/// @param [in]  color_fmt      目标色格式
/// @return klb_canvas_t* 成功非 NULL; 失败 NULL
/// @note Windows BMP v3/v4/v5: BI_RGB; 32bpp 另支持标准 BGRA 掩码的 BI_BITFIELDS
///  \n 8/16/24/32 bpp; 自下而上或顶向下; 行 4 字节对齐
///  \n 16bpp 按 RGB555; 32bpp 文件序 BGRA; BI_RGB 时 a==0 视为不透明, BITFIELDS 含 A 掩码时保留透明
///  \n 不支持 1/4 bpp, RLE, 非标准掩码, OS/2 CORE
KLB_API klb_canvas_t* klb_bitmap_read(const char* p_filename, int color_fmt);


/// @brief 将画布写为 BMP
/// @param [in]  *p_filename    文件路径
/// @param [in]  *p_canvas      画布
/// @return 0.成功; 非0.失败
/// @note 主路径: 32bpp 无压缩自下而上; 画布须为 ARGB8888
///  \n 按 rect.w*4 写像素行, 不用 pitch 作为 BMP 行宽
KLB_API int klb_bitmap_write(const char* p_filename, const klb_canvas_t* p_canvas);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BITMAP_H__

// end
