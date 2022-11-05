///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_canvas.h
/// @brief   画布
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CANVAS_H__
#define __KLB_CANVAS_H__

#include "klb_type.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_color.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_canvas_t_ klb_canvas_t;


/// @struct klb_canvas_vtable_t
/// @brief  canvas画布操作
typedef struct klb_canvas_vtable_t_
{
    /// @brief 锁定
    /// @param [in] *p_canvas   画布对象
    /// @return 0.成功; 非0.失败
    ///  \n 表示开始操作画布内存
    int(*lock)(klb_canvas_t* p_canvas);

    /// @brief 取消锁定
    /// @param [in] *p_canvas   画布对象
    /// @return 0
    ///  \n 表示结束操作画布内存
    int(*unlock)(klb_canvas_t* p_canvas);

    /// @brief 设置绘制颜色
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*set_draw_color)(klb_canvas_t* p_canvas, uint32_t color);

    /// @brief 获取绘制颜色
    /// @return uint32_t ARGB8888颜色值
    uint32_t(*get_draw_color)(klb_canvas_t* p_canvas);

    /// @brief 设置字体高度
    int(*set_font_height)(klb_canvas_t* p_canvas, int h);

    /// @brief 获取字体高度
    int(*get_font_height)(klb_canvas_t* p_canvas);

    /// @brief 加载图片
    /// @return int 0.成功; 非0.失败
    int(*load_image)(klb_canvas_t* p_canvas, const char* p_path, int* p_w, int* p_h);

    /// @brief 使用单色清空屏幕
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_clear)(klb_canvas_t* p_canvas);

    /// @brief 绘制点
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_point)(klb_canvas_t* p_canvas, int x, int y);

    /// @brief 绘制多个点
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_points)(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);

    /// @brief 绘制线段
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_line)(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2);

    /// @brief 绘制多个线段
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_lines)(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);

    /// @brief 绘制空心矩形
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_rect)(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);

    /// @brief 绘制多个空心矩形
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_rects)(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

    /// @brief 使用单色填充绘制单个区域
    /// @param [in] *p_canvas   画布对象
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_fill_rect)(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);

    /// @brief 使用单色填充多个区域
    /// @param [in] *p_canvas   画布对象
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_fill_rects)(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

    /// @brief 绘制utf8文本
    int(*draw_text)(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len);

    /// @brief 绘制图片
    int(*draw_image)(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);

    /// @brief 拷贝绘制画布
    /// @param [in] *p_canvas       画布对象
    /// @param [in] x               起始X坐标
    /// @param [in] y               起始Y坐标
    /// @param [in] *p_src_canvas   被拷贝的原画布
    /// @return int 0.成功; 非0.失败
    ///  \n 源画布/目标画布像素格式必须一致
    ///  \n 源区域大于目标区域时,裁剪
    //int(*draw_copy)(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, const klb_rect_t* p_src_rect);

    /// @brief 刷新画布到显存(屏幕)
    /// @param [in] *p_canvas   画布对象
    /// @param [in] x           起始X坐标
    /// @param [in] y           起始Y坐标
    /// @param [in] w           宽
    /// @param [in] h           高
    /// @return int 0.成功; 非0.失败
    int(*refresh_rect)(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);

    /// @brief 刷新画布到显存(屏幕)
    int(*refresh_rects)(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

    /// @brief 申请画布
    /// @param [in] w           宽
    /// @param [in] h           高
    /// @param [in] color_fmt   颜色格式
    /// @return klb_canvas_t* 画布对象
    ///  \n 可申请使用硬件的画布
    klb_canvas_t* (*malloc)(int w, int h, int color_fmt);

    /// @brief 释放画布
    /// @param [in] *p_canvas   画布对象
    /// @return 无 
    void(*free)(klb_canvas_t* p_canvas);
}klb_canvas_vtable_t;


/// @struct klb_canvas_t
/// @brief  画布
///  \n 和"./klb/src/kimage/kcanvas/canvas.go"保持一致
typedef struct klb_canvas_t_
{
    klb_canvas_vtable_t vtable;                 ///< 操作画布的虚表
    void*               p_obj;                  ///< vtable附加对象

    klb_rect_t          rect;                   ///< 画布区域

    uint8_t*            p_addr;                 ///< 画布虚拟地址
    uintptr_t           phy_addr;               ///< 画布物理地址

    int64_t             mem_len;                ///< 内存长度
    int64_t             pitch;                  ///< 行跨距 = w * bpp(1,2,3,4) + padding

    uint32_t            draw_color;             ///< 设置绘制颜色
    uint32_t            real_draw_color;        ///< 真实的绘制颜色

    int                 color_fmt;              ///< 画布像素格式
    int                 font_h;                 ///< 字体高度
}klb_canvas_t;


/// @brief 创建画布
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color_fmt   颜色格式
/// @return klb_canvas_t* 画布对象
///  \n 软实现
KLB_API klb_canvas_t* klb_canvas_create(int w, int h, int color_fmt);

/// @brief 销毁画布
/// @param [in] *p_canvas   画布对象
/// @return 无
KLB_API void klb_canvas_destroy(klb_canvas_t* p_canvas);

/// @brief 设置绘制颜色
KLB_API int klb_canvas_set_draw_color(klb_canvas_t* p_canvas, uint32_t color);

/// @brief 获取绘制颜色
KLB_API uint32_t klb_canvas_get_draw_color(klb_canvas_t* p_canvas);


KLB_API int klb_canvas_set_font_height(klb_canvas_t* p_canvas, int h);
KLB_API int klb_canvas_get_font_height(klb_canvas_t* p_canvas);


/// @brief 使用颜色清屏幕
/// @param [in] *p_canvas   画布对象
/// @return int 0
KLB_API int klb_canvas_draw_clear(klb_canvas_t* p_canvas);


/// @brief 绘制点
/// @param [in] *p_canvas   画布对象
/// @param [in] w           宽
/// @param [in] h           高
/// @return int 0
int klb_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y);
int klb_canvas_draw_points(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);


/// @brief 绘制线段
/// @param [in] *p_canvas   画布对象
/// @param [in] x1          点1坐标X
/// @param [in] y1          点1坐标Y
/// @param [in] x2          点2坐标X
/// @param [in] y2          点2坐标Y
/// @param [in] color       颜色
/// @param [in] width       线宽
/// @return int 0
int klb_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2);
int klb_canvas_draw_lines(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);

int klb_canvas_draw_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
int klb_canvas_draw_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);


int klb_canvas_draw_fill_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
int klb_canvas_draw_fill_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

/// @brief 绘制文字
/// @param [in] *p_canvas       画布对象
/// @return int 0
int klb_canvas_draw_text(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len);

/// @brief 绘制图片
int klb_canvas_draw_image(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path);

int klb_canvas_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
int klb_canvas_refresh_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);


#ifdef __cplusplus
}
#endif

#endif // __KLB_CANVAS_H__
//end
