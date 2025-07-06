///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_canvas.h
/// @brief   UI画布
/// @version 0.3
/// @history 修改历史
///   \n [2023-8] 添加支持多图层合并刷新
///   \n [2025-6] 添加多图层
///        1.调整画布函数表 klb_canvas_vtable_t.malloc() 的参数含义
///        2.添加 klb_canvas_vtable_t.move() 函数, 支持 移动画布操作
///        3.添加 klb_canvas_vtable_t.resize() 函数, 支持 重新设置画布宽高
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
#define KLB_CANVAS_LAYER_max            16


/// @struct klb_canvas_t
/// @brief  UI使用的画布
typedef struct klb_canvas_t_ klb_canvas_t;


/// @enum  klb_canvas_layer_type_e
/// @brief 画布图层类型
///   单图层方案(main + tip)
///       main 图层: 显示 model/popup/messagebox
///       tip 图层: 现实 tip
///
///   多图层方案(main + popup + msgbox + tip)
///       main 图层: 显示 model
///       popup 图层: 显示 popup
///       msgbox 图层: 显示 messagebox
///       tip 图层: 现实 tip
typedef enum klb_canvas_layer_type_e_
{
    KLB_CANVAS_LAYER_main   = 0,        ///< 主画布层[最多1](必须)
    KLB_CANVAS_LAYER_popup  = 1,        ///< popup画布层[最多4](可选)
    KLB_CANVAS_LAYER_msgbox = 2,        ///< msgbox画布层[最多1][(可选)
    KLB_CANVAS_LAYER_udata  = 3,        ///< 用户自定义图层[最多4](可选)

    KLB_CANVAS_LAYER_wait   = 8,        ///< 等待圈圈图层[最多1](可选)
    KLB_CANVAS_LAYER_tip    = 9,        ///< TIP画布图层[最多1](可选): (GUI)用户显示 tip
}klb_canvas_layer_type_e;


/// @enum  klb_canvas_refresh_opt_e
/// @brief 刷新方式
typedef enum klb_canvas_refresh_opt_e_
{
    KLB_CANVAS_REFRESH_copy         = 0,    ///< 依次拷贝
    KLB_CANVAS_REFRESH_copy_bubble  = 1,    ///< 冒泡拷贝: 拷贝当前画布数据后, 若后续有画布与当前区域有重叠, 则也需要拷贝
}klb_canvas_refresh_opt_e;


/// @struct klb_canvas_layer_t
/// @brief  画布图层
typedef struct klb_canvas_layer_t_
{
    int                 layer_type;     ///< 图层类型(klb_canvas_layer_type_e); eg. KLB_CANVAS_LAYER_main

    bool                is_used;        ///< 当前图层是否使用
    bool                is_redraw;      ///< 是否需要重绘
    klb_rect_t          redraw_rect;    ///< 重绘区域
    klb_canvas_t*       p_canvas;       ///< 画布
}klb_canvas_layer_t;


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

    /// @brief 刷新画布到显存(屏幕), 支持多画布合并刷新
    int(*refresh)(klb_canvas_t* p_canvas,                               // 主画布
                const klb_rect_t dst[KLB_CANVAS_LAYER_max],             // 目标主显存对应区域
                klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],       // 待刷新的源画布
                const klb_rect_t src[KLB_CANVAS_LAYER_max],             // 源区域
                int layer_count);                                       // 图层数

    /// @brief 刷新画布到显存(屏幕), 支持多个图层合并刷新
    /// @param [in] refresh_opt 刷新方式(klb_canvas_refresh_opt_e); eg. KLB_CANVAS_REFRESH_copy
    /// @param [in] layers      待刷新的图层画布
    /// @param [in] layer_count 图层数
    /// @return int 0.成功; 非0.失败
    /// @note 注意:
    ///     KLB_CANVAS_REFRESH_copy         依次拷贝; 当UI需要完全重新刷新时
    ///     KLB_CANVAS_REFRESH_copy_bubble  冒泡拷贝; 当UI需要局部刷新时
    int(*refresh_layer)(klb_canvas_t* p_canvas, int refresh_opt, const klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count);

    /// @brief 移动画布位置
    /// @param [in] x           x 坐标
    /// @param [in] y           y 坐标
    /// @note 注意:
    ///  \n [2025-6] 添加支持
    int(*move)(klb_canvas_t* p_canvas, int x, int y);

    /// @brief 重新设置画布宽高
    /// @note 注意:
    ///  \n [2025-6] 添加支持
    ///  \n    1. 在 总内存长度 不变的 基础上, 调整 画布宽高
    ///  \n    2. 即满足: (bpp * w + padding) * h <= mem_len
    int(*resize)(klb_canvas_t* p_canvas, int w, int h);

    /// @brief 申请画布
    /// @param [in] idx         图层序号: eg. 0
    /// @param [in] rsv         保留: 默认.0
    /// @param [in] layer_type  图层类型(klb_canvas_layer_type_e): eg. KLB_CANVAS_LAYER_tip
    /// @return klb_canvas_t* 画布对象
    /// @note 注意:
    ///  \n [2025-6] 调整函数参数含义
    ///  \n 可申请使用硬件的画布
    ///  \n 仅主画布可以使用此函数, 若与主画布相关, 则新申请的画布bpp(位宽)与主画布一致
    ///  \n 由图形适配 提供具体画布大小; 若UI使用的宽高不符时, 会 resize 重新设置画布大小
    klb_canvas_t* (*malloc)(klb_canvas_t* p_canvas, int idx, int rsv, int layer_type);

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
    int(*draw_opt7)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7);
    int(*draw_opt8)(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8);


    /// @brief 用户自定义 直接操控画布设备函数
    /// @return int 0.成功; 非0.失败
    /// @note 若有需要与画布设备 通信, 扩展此函数
    int(*ioctrl_opt8)(klb_canvas_t* p_canvas, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);
}klb_canvas_vtable_t;


/// @struct klb_canvas_t
/// @brief  画布
///  \n 和"./klb/src/kimage/kcanvas/canvas.go"保持一致
typedef struct klb_canvas_t_
{
    klb_canvas_vtable_t vtable;                 ///< 操作画布的函数表
    void*               p_obj;                  ///< 附加对象

    klb_rect_t          rect;                   ///< 画布区域

    uint8_t*            p_addr;                 ///< 画布虚拟地址
    uintptr_t           phy_addr;               ///< 画布物理地址

    int                 bpp;                    ///< 像素位宽: 取值[2, 4]
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

/// @brief 刷新画布到显存(屏幕), 支持多个图层合并刷新
/// @param [in] refresh_opt 刷新方式(klb_canvas_refresh_opt_e); eg. KLB_CANVAS_REFRESH_copy
/// @param [in] layers      待刷新的图层画布
/// @param [in] layer_count 图层数
/// @return int 0.成功; 非0.失败
/// @note 注意:
///     KLB_CANVAS_REFRESH_copy         依次拷贝; 当UI需要完全重新刷新时
///     KLB_CANVAS_REFRESH_copy_bubble  冒泡拷贝; 当UI需要局部刷新时
KLB_API int klb_canvas_refresh_layer(klb_canvas_t* p_canvas, int refresh_opt, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count);


/// @brief 移动画布位置
/// @param [in] x           x 坐标
/// @param [in] y           y 坐标
/// @note 注意:
///  \n [2025-6] 添加支持
KLB_API int klb_canvas_move(klb_canvas_t* p_canvas, int x, int y);


/// @brief 重新设置画布宽高
/// @note 注意:
///  \n [2025-6] 添加支持
///  \n    1. 在 总内存长度 不变的 基础上, 调整 画布宽高
///  \n    2. 即满足: (bpp * w + padding) * h <= mem_len
KLB_API int klb_canvas_resize(klb_canvas_t* p_canvas, int w, int h);


/// @brief 申请画布
/// @param [in] idx         图层序号: eg. 0
/// @param [in] rsv         保留: 默认.0
/// @param [in] layer_type  图层类型(klb_canvas_layer_type_e): eg. KLB_CANVAS_LAYER_tip
/// @return klb_canvas_t* 画布对象
/// @note 注意:
///  \n [2025-6] 调整函数参数含义
///  \n 可申请使用硬件的画布
///  \n 仅主画布可以使用此函数, 若与主画布相关, 则新申请的画布bpp(位宽)与主画布一致
///  \n 由图形适配 提供具体画布大小; 若UI使用的宽高不符时, 会 resize 重新设置画布大小
KLB_API klb_canvas_t* klb_canvas_malloc(klb_canvas_t* p_canvas, int idx, int rsv, int layer_type);


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
KLB_API int klb_canvas_draw_opt7(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7);
KLB_API int klb_canvas_draw_opt8(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8);


/// @brief 用户自定义 直接操控画布设备函数
/// @return int 0.成功; 非0.失败
/// @note 若有需要与画布设备 通信, 扩展此函数
KLB_API int klb_canvas_ioctrl_opt8(klb_canvas_t* p_canvas, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);


#ifdef __cplusplus
}
#endif

#endif // __KLB_CANVAS_H__
//end
