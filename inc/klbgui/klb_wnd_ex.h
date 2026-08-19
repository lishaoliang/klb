// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_wnd_ex.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   窗口绘图扩充 API
///            1. 扩充支持到 arm-linux
///            2. 扩充支持到 windows
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WND_EX_H__
#define __KLB_WND_EX_H__


#include "klb_type.h"
#include "klbutil/klb_rect.h"
#include "klbgui/klb_wnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// GUI FB 透明色, 视频控件常用
#define KLB_CANVAS_COLOR_ZERO                 0x0     ///< 透明色, 用于在 FB 界面"打个洞", 透视出底层图像


#define KLB_CANVAS_PATH_TMPIMAGE              "~/tmpimage"    ///< 临时图片文件路径


//////////////////////////////////////////////////////////////////////////
// draw_opt (arm-linux / 嵌入式扩充区间: 100 ~ 512)

#define KLB_CANVAS_DRAW_OPT_LINE              100     ///< 绘制线条, 包含斜线
#define KLB_CANVAS_DRAW_OPT_LINES             101     ///< 绘制(多条)线条, 包含折线

#define KLB_CANVAS_DRAW_OPT_RECT              110     ///< 绘制矩形区域边框
#define KLB_CANVAS_DRAW_OPT_RECTS             111     ///< 绘制(多个)矩形区域边框

#define KLB_CANVAS_DRAW_OPT_FILL_RECTS        120     ///< 填充(多个)矩形区域

#define KLB_CANVAS_DRAW_OPT_IMAGE_RESIZE      130     ///< 绘制图片, 缩放图片到目标矩形区域内
#define KLB_CANVAS_DRAW_OPT_IMAGE_COLOR_KEY   131     ///< 绘制图片, 带关键色(透明色)
#define KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9              132     ///< 绘制图片, 九宫格居中拉伸
#define KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9_COLOR_KEY    133     ///< 绘制图片, 九宫格居中拉伸, 带关键色(透明色)

#define KLB_CANVAS_DRAW_OPT_TEXT              150     ///< 绘制文本
#define KLB_CANVAS_DRAW_OPT_TEXT_LINES        151     ///< 绘制文本，带自动换行; 支持"\n"换行


#define KLB_CANVAS_DRAW_LINES_MAX             200     ///< 绘制(多条)线条, 一次最多的数目
#define KLB_CANVAS_DRAW_RECTS_MAX             100     ///< 绘制(多个)矩形区域边框, 一次最多的数目
#define KLB_CANVAS_DRAW_FILL_RECTS_MAX        100     ///< 填充(多个)矩形区域, 一次最多的数目


//////////////////////////////////////////////////////////////////////////
// ioctrl_opt (字库/图集加载见 klb_canvas_vtable_t.load_font/load_image 等)

#define KLB_CANVAS_IOCTRL_OPT_IMAGE           20      ///< 图片信息
#define KLB_CANVAS_IOCTRL_OPT_TMPIMAGE        21      ///< 临时图片信息


//////////////////////////////////////////////////////////////////////////
// 线条

/// @brief 绘制线条, 包含斜线
/// @param [in] *p_wnd     窗口指针
/// @param [in] x1         起点 X
/// @param [in] y1         起点 Y
/// @param [in] x2         终点 X
/// @param [in] y2         终点 Y
/// @param [in] color      颜色 (ARGB8888)
/// @param [in] thick      线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_LINE; klb_wnd_draw_opt6
KLB_API int klb_wndex_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color, int thick);

/// @brief 绘制(多条)线条, 包含折线; 每条线可指定颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_pt1     线段起点数组
/// @param [in] *p_pt2     线段终点数组
/// @param [in] *p_color   每条线段颜色数组
/// @param [in] count      线段数目
/// @param [in] thick      线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_LINES; klb_wnd_draw_opt5
KLB_API int klb_wndex_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_pt1, const klb_point_t* p_pt2, const uint32_t* p_color, int count, int thick);

/// @brief 绘制(多条)线条; 所有线段使用同一颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_pt1     线段起点数组
/// @param [in] *p_pt2     线段终点数组
/// @param [in] color      颜色 (ARGB8888)
/// @param [in] count      线段数目
/// @param [in] thick      线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_LINES; klb_wnd_draw_opt5
KLB_API int klb_wndex_draw_lines2(klb_wnd_t* p_wnd, const klb_point_t* p_pt1, const klb_point_t* p_pt2, uint32_t color, int count, int thick);


//////////////////////////////////////////////////////////////////////////
// 矩形边框

/// @brief 绘制矩形区域边框
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_rect    矩形区域
/// @param [in] color      颜色 (ARGB8888)
/// @param [in] thick      边框线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_RECT; klb_wnd_draw_opt3
KLB_API int klb_wndex_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color, int thick);

/// @brief 绘制(多个)矩形区域边框; 每个矩形可指定颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_rects   矩形数组
/// @param [in] *p_color   每个矩形颜色数组
/// @param [in] count      矩形数目
/// @param [in] thick      边框线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_RECTS; klb_wnd_draw_opt4
KLB_API int klb_wndex_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, const uint32_t* p_color, int count, int thick);

/// @brief 绘制(多个)矩形区域边框; 所有矩形使用同一颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_rects   矩形数组
/// @param [in] color      颜色 (ARGB8888)
/// @param [in] count      矩形数目
/// @param [in] thick      边框线宽
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_RECTS; klb_wnd_draw_opt4
KLB_API int klb_wndex_draw_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, uint32_t color, int count, int thick);


//////////////////////////////////////////////////////////////////////////
// 矩形填充

/// @brief 填充(多个)矩形区域; 每个矩形可指定颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_rects   矩形数组
/// @param [in] *p_color   每个矩形颜色数组
/// @param [in] count      矩形数目
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_FILL_RECTS; klb_wnd_draw_opt3
KLB_API int klb_wndex_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, const uint32_t* p_color, int count);

/// @brief 填充(多个)矩形区域; 所有矩形使用同一颜色
/// @param [in] *p_wnd     窗口指针
/// @param [in] *p_rects   矩形数组
/// @param [in] color      颜色 (ARGB8888)
/// @param [in] count      矩形数目
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_FILL_RECTS; klb_wnd_draw_opt3
KLB_API int klb_wndex_draw_fill_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, uint32_t color, int count);


//////////////////////////////////////////////////////////////////////////
// 图片

/// @brief 绘制图片, 缩放图片到目标矩形区域内
/// @param [in] *p_wnd         窗口指针
/// @param [in] *p_dst_rect    目标矩形区域
/// @param [in] *p_image_path  图片路径或 key
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_IMAGE_RESIZE; klb_wnd_draw_opt2
KLB_API int klb_wndex_draw_image_resize(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_image_path);

/// @brief 绘制图片, 带关键色(透明色)
/// @param [in] *p_wnd         窗口指针
/// @param [in] *p_rect        目标矩形区域
/// @param [in] *p_image_path  图片路径或 key
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_IMAGE_COLOR_KEY; klb_wnd_draw_opt2
KLB_API int klb_wndex_draw_image_color_key(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path);

/// @brief 绘制图片, 九宫格居中拉伸
/// @param [in] *p_wnd         窗口指针
/// @param [in] *p_rect        目标矩形区域
/// @param [in] *p_image_path  图片路径或 key
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9; klb_wnd_draw_opt2
KLB_API int klb_wndex_draw_image_scale9(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path);

/// @brief 绘制图片, 九宫格居中拉伸, 带关键色(透明色)
/// @param [in] *p_wnd         窗口指针
/// @param [in] *p_rect        目标矩形区域
/// @param [in] *p_image_path  图片路径或 key
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9_COLOR_KEY; klb_wnd_draw_opt2
KLB_API int klb_wndex_draw_image_scale9_color_key(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path);


//////////////////////////////////////////////////////////////////////////
// 文本

/// @brief 绘制文本
/// @param [in]  *p_wnd      窗口指针
/// @param [in]  *p_rect     绘制文本区域
/// @param [in]  *p_utf8     UTF8 字符串, eg. "123456"
/// @param [in]  utf8_len    UTF8 字符串长度; 0 表示由后端处理
/// @param [in]  color       文本颜色 (ARGB8888), eg. 0xFFA0A0A0
/// @param [in]  font_h      字体高度, eg. 20
/// @param [out] *p_out_w    (输出)已绘制的宽度; 可 NULL
/// @param [out] *p_out_h    (输出)已绘制的高度; 可 NULL
/// @param [out] *p_out_all  (输出)是否完整绘制; 可 NULL
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_TEXT; klb_wnd_draw_opt8
KLB_API int klb_wndex_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int* p_out_w, int* p_out_h, bool* p_out_all);

/// @brief 绘制文本, 带自动换行; 支持 "\n" 换行
/// @param [in] *p_wnd         窗口指针
/// @param [in] *p_rect        绘制文本区域
/// @param [in] *p_utf8        UTF8 字符串
/// @param [in] utf8_len       UTF8 字符串长度
/// @param [in] color          文本颜色 (ARGB8888)
/// @param [in] font_h         字体高度
/// @param [in] line_spacing   行间距
/// @return int 0.成功; 非0.失败
/// @note opt = KLB_CANVAS_DRAW_OPT_TEXT_LINES; klb_wnd_draw_opt6
KLB_API int klb_wndex_draw_text_lines(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int line_spacing);


#ifdef __cplusplus
}
#endif

#endif // __KLB_WND_EX_H__
//end
