///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_canvas.h
/// @brief   画布
/// @version 0.1
/// @history 修改历史
///   \n [2023-8] 添加支持多图层合并刷新
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


/// @def   KLB_CANVAS_LAYER_max
/// @brief 最大图层数
#define KLB_CANVAS_LAYER_max            8


typedef struct klb_canvas_t_ klb_canvas_t;


/// @enum  klb_canvas_layer_type_e
/// @brief 画布图层类型
/// @note  待探讨: 可以否将GUI画布 与 视频 画布 做 融合 啥的???
typedef enum klb_canvas_layer_type_e_
{
    KLB_CANVAS_LAYER_main   = 0,        ///< 主画布: (GUI)用于显示 model/popup/messagebox
    KLB_CANVAS_LAYER_any,               ///< 任意(可选): 未明确用途
    KLB_CANVAS_LAYER_tip,               ///< TIP画布图层(可选): (GUI)用户显示 tip
}klb_canvas_layer_type_e;


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
    int(*load_image)(klb_canvas_t* p_canvas, const char* p_key, const char* p_path, int* p_w, int* p_h);

    /// @brief 获取图片大小
    /// @return int 0.成功; 非0.失败
    int(*image_size)(klb_canvas_t* p_canvas, const char* p_key, int* p_out_w, int* p_out_h);

    /// @brief 清空所有图片资源
    /// @return int 0.成功; 非0.失败
    int(*clear_image)(klb_canvas_t* p_canvas);

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

    /// @brief 以当前字体大小, 绘制utf8文本所需要的宽高
    int(*text_size)(klb_canvas_t* p_canvas, const char* p_utf8, int utf8_len, int* p_out_w, int* p_out_h);

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
    int(*draw_copy)(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, const klb_rect_t* p_src_rect);

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

    /// @brief 刷新画布到显存(屏幕), 支持多图层合并刷新
    int(*refresh)(klb_canvas_t* p_canvas,                               // 主画布
                const klb_rect_t dst[KLB_CANVAS_LAYER_max],             // 目标主显存对应区域
                klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],       // 待刷新的源画布
                const klb_rect_t src[KLB_CANVAS_LAYER_max],             // 源区域
                int layer_count);                                       // 图层数

    /// @brief 申请画布
    /// @param [in] w           宽
    /// @param [in] h           高
    /// @param [in] layer_type  图层类型: eg. KLB_CANVAS_LAYER_tip
    /// @return klb_canvas_t* 画布对象
    ///  \n 可申请使用硬件的画布
    ///  \n 仅主画布可以使用此函数, 若与主画布相关, 则新申请的画布bpp(位宽)与主画布一致
    klb_canvas_t* (*malloc)(klb_canvas_t* p_canvas, int w, int h, int layer_type);

    /// @brief 释放画布
    /// @param [in] *p_canvas   画布对象
    /// @return 无 
    void(*free)(klb_canvas_t* p_canvas);


    /// @brief 用户自定义绘图函数
    /// @param [in] *p_canvas       画布对象
    /// @param [in] opt             绘制功能参数: 可自行定义
    /// @param [in] ptr1            绘制参数1
    /// @return int 0.成功; 非0.失败
    /// @note 基础绘图函数, 若无法满足私有控件绘图要求, 可以扩展此函数
    ///     这里为尽可能满足要求, 定义多种参数方式, 实际使用时 选用其中部分即可
    int(*draw_opt1)(klb_canvas_t* p_canvas, int opt, const void* ptr1);
    int(*draw_opt2)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2);
    int(*draw_opt3)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3);
    int(*draw_opt4)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4);
    int(*draw_opt5)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5);
    int(*draw_opt6)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6);
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

    int                 bpp;                    ///< 像素位宽
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
KLB_API int klb_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y);
KLB_API int klb_canvas_draw_points(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);


/// @brief 绘制线段
/// @param [in] *p_canvas   画布对象
/// @param [in] x1          点1坐标X
/// @param [in] y1          点1坐标Y
/// @param [in] x2          点2坐标X
/// @param [in] y2          点2坐标Y
/// @param [in] color       颜色
/// @param [in] width       线宽
/// @return int 0
KLB_API int klb_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2);
KLB_API int klb_canvas_draw_lines(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count);

KLB_API int klb_canvas_draw_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
KLB_API int klb_canvas_draw_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);


KLB_API int klb_canvas_draw_fill_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
KLB_API int klb_canvas_draw_fill_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

/// @brief 绘制文字
/// @param [in] *p_canvas       画布对象
/// @return int 0
KLB_API int klb_canvas_draw_text(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len);

/// @brief 获取以当前字体大小, 绘制utf8文本所需要的宽高
KLB_API int klb_canvas_text_size(klb_canvas_t* p_canvas, const char* p_utf8, int utf8_len, int* p_out_w, int* p_out_h);

/// @brief 获取图片尺寸
KLB_API int klb_canvas_image_size(klb_canvas_t* p_canvas, const char* p_path, int* p_out_w, int* p_out_h);

/// @brief 绘制图片
KLB_API int klb_canvas_draw_image(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path);

/// @brief 刷新
KLB_API int klb_canvas_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect);
KLB_API int klb_canvas_refresh_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count);

KLB_API int klb_canvas_refresh(klb_canvas_t* p_canvas,                              // 主画布
                                const klb_rect_t dst[KLB_CANVAS_LAYER_max],         // 目标主显存对应区域
                                klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],   // 待刷新的源画布
                                const klb_rect_t src[KLB_CANVAS_LAYER_max],         // 源区域
                                int layer_count);

/// @brief 申请新画布
KLB_API klb_canvas_t* klb_canvas_malloc(klb_canvas_t* p_canvas, int w, int h, int layer_type);


/// @brief 用户自定义绘图函数
/// @param [in] *p_canvas       画布对象
/// @param [in] opt             绘制功能参数: 可自行定义
/// @param [in] ptr1            绘制参数1
/// @return int 0.成功; 非0.失败
/// @note 基础绘图函数, 若无法满足私有控件绘图要求, 可以扩展此函数
KLB_API int klb_canvas_draw_opt1(klb_canvas_t* p_canvas, int opt, const void* ptr1);
KLB_API int klb_canvas_draw_opt2(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2);
KLB_API int klb_canvas_draw_opt3(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3);
KLB_API int klb_canvas_draw_opt4(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4);
KLB_API int klb_canvas_draw_opt5(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5);
KLB_API int klb_canvas_draw_opt6(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6);


#ifdef __cplusplus
}
#endif

#endif // __KLB_CANVAS_H__
//end
