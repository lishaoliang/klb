///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_canvas.h
/// @brief   画布
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CANVAS_H__
#define __KLB_CANVAS_H__

#include "klb_type.h"
#include "image/klb_rect.h"
#include "image/klb_color.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_canvas_t_ klb_canvas_t;


#define KLB_CANVAS_LOCK(CANVAS_)            \
{                                           \
    if ((CANVAS_)->vtable.lock)             \
    {                                       \
        (CANVAS_)->vtable.lock(CANVAS_);    \
    }                                       \
}


#define KLB_CANVAS_LOCK_OK(CANVAS_)                                         \
(                                                                           \
    (NULL == (CANVAS_)->vtable.lock) ||                                     \
    ((CANVAS_)->vtable.lock && (0 == (CANVAS_)->vtable.lock(CANVAS_)))      \
)


#define KLB_CANVAS_UNLOCK(CANVAS_)          \
{                                           \
    if ((CANVAS_)->vtable.unlock)           \
    {                                       \
        (CANVAS_)->vtable.unlock(CANVAS_);  \
    }                                       \
}

/// @struct klb_canvas_vtable_t
/// @brief  canvas画布操作虚表
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

    /// @brief 开始绘制
    /// @param [in] *p_canvas   画布对象
    /// @return int 0.成功; 非0.失败
    ///  \n 表示开始使用draw_*系列函数
    int(*draw_begin)(klb_canvas_t* p_canvas);

    /// @brief 结束绘制
    /// @param [in] *p_canvas   画布对象
    /// @return int 0.成功; 非0.失败
    ///  \n 表示结束使用draw_*系列函数
    int(*draw_end)(klb_canvas_t* p_canvas);

    /// @brief 使用单色填充绘制
    /// @param [in] *p_canvas   画布对象
    /// @param [in] x           起始X坐标
    /// @param [in] y           起始Y坐标
    /// @param [in] w           宽
    /// @param [in] h           高
    /// @param [in] color       颜色
    /// @return int 0.成功; 非0.失败
    int(*draw_fill)(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color);

    /// @brief 拷贝绘制画布
    /// @param [in] *p_canvas       画布对象
    /// @param [in] x               起始X坐标
    /// @param [in] y               起始Y坐标
    /// @param [in] *p_src_canvas   被拷贝的原画布
    /// @param [in] src_x           原画布起始X坐标
    /// @param [in] src_y           原画布起始Y坐标
    /// @param [in] src_w           原画布宽
    /// @param [in] src_h           原画布高
    /// @return int 0.成功; 非0.失败
    ///  \n 源画布/目标画布像素格式必须一致
    ///  \n 源区域大于目标区域时,裁剪
    int(*draw_canvas)(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, int src_x, int src_y, int src_w, int src_h);

    /// @brief 刷新画布到显存(屏幕)
    /// @param [in] *p_canvas   画布对象
    /// @param [in] x           起始X坐标
    /// @param [in] y           起始Y坐标
    /// @param [in] w           宽
    /// @param [in] h           高
    /// @return int 0.成功; 非0.失败
    int(*refresh)(klb_canvas_t* p_canvas, int x, int y, int w, int h);

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
    int64_t             stride;                 ///< 行跨距 = w * bpp(1,2,3,4)
    int                 color_fmt;              ///< 画布像素格式
}klb_canvas_t;


/// @brief 创建画布
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color_fmt   颜色格式
/// @return klb_canvas_t* 画布对象
///  \n 软实现
klb_canvas_t* klb_canvas_create(int w, int h, int color_fmt);


/// @brief 销毁画布
/// @param [in] *p_canvas   画布对象
/// @return 无
void klb_canvas_destroy(klb_canvas_t* p_canvas);


/// @brief 清除颜色
/// @param [in] *p_canvas   画布对象
/// @param [in] x           坐标X
/// @param [in] y           坐标Y
/// @param [in] w           宽
/// @param [in] h           高
/// @return int 0
int klb_canvas_draw_clear(klb_canvas_t* p_canvas, int x, int y, int w, int h);


/// @brief 绘制点
/// @param [in] *p_canvas   画布对象
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color       颜色
/// @return int 0
int klb_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y, uint32_t color);


/// @brief 绘制线段
/// @param [in] *p_canvas   画布对象
/// @param [in] x1          点1坐标X
/// @param [in] y1          点1坐标Y
/// @param [in] x2          点2坐标X
/// @param [in] y2          点2坐标Y
/// @param [in] color       颜色
/// @param [in] width       线宽
/// @return int 0
int klb_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2, uint32_t color, int width);


/// @brief 填充颜色
/// @param [in] *p_canvas   画布对象
/// @param [in] x           坐标X
/// @param [in] y           坐标Y
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color       颜色
/// @return int 0
int klb_canvas_draw_fill(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color);


/// @brief 绘制空心矩形
/// @param [in] *p_canvas   画布对象
/// @param [in] x           坐标X
/// @param [in] y           坐标Y
/// @param [in] w           宽
/// @param [in] h           高
/// @param [in] color       颜色
/// @param [in] depth       线宽
/// @return int 0
int klb_canvas_draw_rect(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color, int depth);


/// @brief 拷贝画布
/// @param [in] *p_canvas       画布对象
/// @param [in] x               坐标X
/// @param [in] y               坐标Y
/// @param [in] *p_src_canvas   被拷贝的原画布
/// @param [in] src_x           原画布坐标X
/// @param [in] src_y           原画布坐标Y
/// @param [in] src_w           原画布宽
/// @param [in] src_h           原画布高
/// @return int 0
int klb_canvas_draw_canvas(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, int src_x, int src_y, int src_w, int src_h);


/// @brief 绘制文字
/// @param [in] *p_canvas       画布对象
/// @return int 0
int klb_canvas_draw_text(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color, int font, const char* p_utf8, int utf8_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_CANVAS_H__
//end
