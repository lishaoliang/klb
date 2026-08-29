///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_css.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   GUI CSS相关定义
///   参考 CSS3 : https://www.w3school.com.cn/css/index.asp
///   命名规则,含义,用法等尽可能参考 CSS3
/// @version 0.1
/// @history 修改历史
///   \n [2023-4] 使用宏来区分是否支持高级CSS3: __KLB_GUI_CSS3__
///               默认情况下, 使用基础CSS
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CSS_H__
#define __KLBUI_CSS_H__


#include "klb_type.h"
#include "klbthird/sds.h"
#include "klbutil/klb_map.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;
typedef struct klb_wnd_t_ klb_wnd_t;


/// @brief 边框模型
///  参考: https://www.w3school.com.cn/css/css_boxmodel.asp
///  element(元素/window)实际宽 = 外边距(margin) + 边框(border) + 内边距(padding) + 元素宽(width)
///  element(元素/window)实际高 = 外边距(margin) + 边框(border) + 内边距(padding) + 元素宽(height)
///  参考模型图:
///  *---------------- margin(外边距) ------------------*
///  |  *------------- border(边框) -----------------*  |
///  |  |  *---------- padding(内边距) -----------*  |  |
///  |  |  |  *--------------------------------*  |  |  |
///  |  |  |  |                                |  |  |  |
///  |  |  |  |        element(元素/window)    |  |  |  |
///  |  |  |  |                                |  |  |  |
///  |  |  |  *--------------------------------*  |  |  |
///  |  |  *--------------------------------------*  |  |
///  |  *--------------------------------------------*  |
///  *--------------------------------------------------*
///  
///  klb_wnd_t.pos.rect_in_parent = 基于父窗口的 实际区域
///  klb_wnd_t.pos.rect_in_canvas = 基于画布的 实际区域
///
///  注意: 控件窗口需要负责绘制包含 margin/border/padding/element 在内的全部
///  一般情况下的控件, 会将 margin = 0


/// 显示/隐藏
/// klb_wnd_t.state | KLB_WND_STATUS_HIDE   ///< ["visibility"]
#define KLBUICSS_visibility_visible  0      ///< ["visibility"] = "visible" [默认值]元素是可见的
#define KLBUICSS_visibility_hidden   1      ///< ["visibility"] = "hidden" 元素是不可见的


/// @struct klbuicss_margin_t
/// @brief  外边距
///   参考: https://www.w3school.com.cn/css/css_margin.asp
///   合并写法: ["margin"] = {25, 50, 75, 100}
///   单位像素
typedef struct klbuicss_margin_t_
{
    int     top;            ///< "margin-top" 上外边距
    int     right;          ///< "margin-right" 右外边距
    int     bottom;         ///< "margin-bottom" 下外边距
    int     left;           ///< "margin-left" 左外边距
}klbuicss_margin_t;


/// @struct klbuicss_padding_t
/// @brief  内边距
///   参考: https://www.w3school.com.cn/css/css_margin.asp
///   合并写法: ["padding"] = {25, 50, 75, 100}
///   单位像素
typedef struct klbuicss_padding_t_
{
    int     top;            ///< "padding-top" 上内边距
    int     right;          ///< "padding-right" 右内边距
    int     bottom;         ///< "padding-bottom" 下内边距
    int     left;           ///< "padding-left" 左内边距
}klbuicss_padding_t;


/// @struct klbuicss_box_t
/// @brief  基础盒模型属性
typedef struct klbuicss_box_t_
{
    int     max_height;         ///< ["max-height"] 最大高度
    int     max_width;          ///< ["max-width"] 最大宽度
    int     min_height;         ///< ["min-height"] 最小高度
    int     min_width;          ///< ["min-width"] 最大宽度
}klbuicss_box_t;


/// @struct klbuicss_laylout_t
/// @brief  布局属性
typedef struct klbuicss_laylout_t_
{
    int     z_index;            ///< ["z-index"] 堆叠顺序
}klbuicss_laylout_t;


/// @struct klbuicss_text_t
/// @brief  文本
///   参考: https://www.w3school.com.cn/css/css_text.asp
typedef struct klbuicss_text_t_
{
    uint32_t    color;          ///< ["color"] 文本颜色
    int         align;          ///< ["text-align"] 文本对齐
#define KLBUICSS_text1_left   0 ///< ["text-align"] = "left" 左对齐
#define KLBUICSS_text1_center 1 ///< ["text-align"] = "center" 中心对齐
#define KLBUICSS_text1_right  2 ///< ["text-align"] = "right" 右对齐

#if defined(__KLB_GUI_CSS3__)
    int         transform;      ///< ["text-transform"] 文本转换

    int         indent;         ///< ["text-indent"] 属性用于指定文本第一行的缩进
    int         spacing;        ///< ["letter-spacing"] 属性用于指定文本中字符之间的间距
    int         line_height;    ///< ["line-height"] 属性用于指定行之间的间距
    int         word_spacing;   ///< ["word-spacing"] 属性用于指定文本中单词之间的间距
    int         white_space;    ///< ["white-space"] 属性指定元素内部空白的处理方式
#endif
}klbuicss_text_t;


/// @struct klbuicss_text_t
/// @brief  字体
///   参考: https://www.w3school.com.cn/css/css_font.asp
typedef struct klbuicss_font_t_
{
#if defined(__KLB_GUI_CSS3__)
    int         style;             ///< ["font-style"] 属性主要用于指定斜体文本
#define KLBUICSS_font1_normal   0  ///< ["font-style"] = "normal" 文字正常显示
#define KLBUICSS_font1_italic   1  ///< ["font-style"] = "italic" 文本以斜体显示
#define KLBUICSS_font1_oblique  2  ///< ["font-style"] = "oblique" 文本为"倾斜"(倾斜与斜体非常相似,但支持较少)

    int         weight;             ///< ["font-weight"] 属性指定字体的粗细
#define KLBUICSS_font2_normal   0   ///< ["font-weight"] = "normal"
#define KLBUICSS_font2_bold     1   ///< ["font-weight"] = "bold"
#endif

    int         size;               ///< ["font-size"] 属性设置文本的大小
}klbuicss_font_t;


/// @struct klbuicss_background_t_
/// @brief  背景
///   参考: https://www.w3school.com.cn/css/css_background.asp
typedef struct klbuicss_background_t_
{
    uint32_t    color;      ///< "background-color" 属性指定元素的背景色
    sds         image;      ///< "background-image" 属性指定用作元素背景的图像

    uint8_t     image_mode;
#define KLBUICSS_bgimg_mode_default     0   ///< 默认: klb_wnd_draw_image
#define KLBUICSS_bgimg_mode_scale9      1   ///< 九宫格 -> IMAGE_SCALE9

    uint8_t     image_flags;
#define KLBUICSS_bgimg_flag_none        0   ///< 无 flag
#define KLBUICSS_bgimg_flag_color_key   0x1 ///< 关键色透明   

#if defined(__KLB_GUI_CSS3__)
    int         repeat;     ///< "background-repeat" 属性指定重复图像
    int         position;   ///< "background-position" 属性用于指定背景图像的位置
    int         attachment; ///< "background-attachment" 属性指定背景图像是应该滚动还是固定的(不会随页面的其余部分一起滚动)
#endif
}klbuicss_background_t;


/// @struct klbuicss_border_t
/// @brief  边框
///   参考: https://www.w3school.com.cn/css/css_border.asp
typedef struct klbuicss_border_t_
{
#if defined(__KLB_GUI_CSS3__)
    struct
    {
        int     top;
        int     right;
        int     bottom;
        int     left;
    }style;                         ///< "border-style" 属性指定要显示的边框类型,属性可以设置一到四个值(用于上边框,右边框,下边框和左边框)
#define KLBUICSS_border1_none   0   ///< ["border-style"] = "none" 定义无边框
#define KLBUICSS_border1_dotted 1   ///< ["border-style"] = "dotted" 定义无边框
#define KLBUICSS_border1_dashed 2   ///< ["border-style"] = "dashed" 定义虚线边框
#define KLBUICSS_border1_solid  3   ///< ["border-style"] = "solid" 定义实线边框
#define KLBUICSS_border1_double 4   ///< ["border-style"] = "double" 定义双边框
#define KLBUICSS_border1_hidden 5   ///< ["border-style"] = "hidden" 定义隐藏边框
#define KLBUICSS_border1_inset  6   ///< ["border-style"] = "inset" 定义3D inset边框
#define KLBUICSS_border1_outset 7   ///< ["border-style"] = "outset" 定义3D outset边框

    int         radius;             ///< "border-radius" 属性用于向元素添加圆角边框
#endif

    struct
    {
        int     top;
        int     right;
        int     bottom;
        int     left;
    }width;                         ///< "border-width" 属性指定四个边框的宽度

    struct
    {
        uint32_t top;
        uint32_t right;
        uint32_t bottom;
        uint32_t left;
    }color;                         ///< "border-color" 属性用于设置四个边框的颜色
}klbuicss_border_t;


/// @struct klbuicss_outline_t
/// @brief  轮廓
///   参考: https://www.w3school.com.cn/css/css_outline.asp
typedef struct klbuicss_outline_t
{
    int      style;                     ///< ["outline-style"] 属性指定轮廓的样式
#define KLBUICSS_outline1_none       0  ///< ["outline-style"] = "none" 定义无轮廓
#define KLBUICSS_outline1_dotted     1  ///< ["outline-style"] = "dotted"定义点状的轮廓
#define KLBUICSS_outline1_dashed     2  ///< ["outline-style"] = "dashed"定义虚线的轮廓
#define KLBUICSS_outline1_solid      3  ///< ["outline-style"] = "solid"定义实线的轮廓
#define KLBUICSS_outline1_double     4  ///< ["outline-style"] = "double"定义双线的轮廓
#define KLBUICSS_outline1_groove     5  ///< ["outline-style"] = "groove"定义 3D 凹槽轮廓
#define KLBUICSS_outline1_ridge      6  ///< ["outline-style"] = "ridge"定义 3D 凸槽轮廓
#define KLBUICSS_outline1_inset      7  ///< ["outline-style"] = "inset"定义 3D 凹边轮廓
#define KLBUICSS_outline1_outset     8  ///< ["outline-style"] = "outset"定义 3D 凸边轮廓
#define KLBUICSS_outline1_hidden     9  ///< ["outline-style"] = "hidden"定义隐藏的轮廓

    int      width;                      ///< ["outline-width"] 属性指定轮廓的宽度
#define KLBUICSS_outline2_thin       1   ///< ["outline-width"] = "thin", 单位像素
#define KLBUICSS_outline2_medium     3   ///< ["outline-width"] = "medium", 单位像素
#define KLBUICSS_outline2_thick      5   ///< ["outline-width"] = "thick", 单位像素

    uint32_t color;                     ///< ["outline-color"] 属性用于设置轮廓的颜色
    int      offset;                    ///< ["outline-offset"] 轮廓偏移
}klbuicss_outline_t;


/// @struct klbuicss_util_t
/// @brief  其他属性
typedef struct klbuicss_util_t_
{
    int     cursor;                     ///< ["cursor"] = "" 指定光标形状
}klbuicss_util_t;


//////////////////////////////////////////////////////////////////////////
// CSS辅助函数

/// @brief 检查参数是否为颜色
KLB_API bool klb_gui_check_color(klb_gui_t* p_gui, const klb_map_t* p_map, int start, uint32_t* p_out_color);



//////////////////////////////////////////////////////////////////////////
// 同类型控件, 私有CSS属性 函数处理表


/// @brief 通过组件类型获取 css 属性函数处理 map
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @return klb_map_t* 函数处理表
///  \n 标准控件类型名命名规则为 "k*", eg."kbutton"
///  \n 相同组件, 使用同一套函数处理表, 目的是为了 精简组件体积
KLB_API klb_map_t* klb_gui_css_map(klb_gui_t* p_gui, const char* p_type);


/// @brief 新建 css 属性函数处理 map
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @return klb_map_t* 函数处理表
KLB_API klb_map_t* klb_gui_new_css_map(klb_gui_t* p_gui, const char* p_type);



//////////////////////////////////////////////////////////////////////////
// 同类型控件, 全局公共CSS属性 函数处理表
// 原因: 1. 同一套UI中设计中, 80%以上同类型控件会采用一致的CSS属性设置
//       2. CSS属性数据一般比窗口属性数据 大 很多倍, 若每个控件都单独持有CSS属性, 窗口数据量大(例如十万级以上), 会消耗非常多内存
//       3. 本GUI框架主要针对 嵌入式/资源受限 环境, 在内存与CPU消耗上, 需要谨慎对待!
// 方案: 在基于 节省内存, 不降低使用体验的情况下, 采用如下思路:
//       A. 默认情况下的控件, 直接使用全局公共CSS属性
//       B. 若控件的"set"(设置CSS)方法被调用, 则新建立私有CSS属性, 转为使用私有CSS属性
//       C. 提供操作全局公共CSS属性的一些接口
// 其他: 在节省内存方面, 还有待考量思路:
//       1).每个控件尽可能精简CSS属性. 缺点: 没有标准处理方法, 需要逐个去对待
//       2).使用CSS-MINI, 精简每个属性占用的字节数: eg. 将 klbuicss_padding_t.top 属性 由"int" 修改为 "int8_t"


/// @brief 通过组件类型获取 全局公共CSS处理表
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @return klb_map_t* 函数处理表
KLB_API klb_map_t* klb_gui_globalcss_map(klb_gui_t* p_gui, const char* p_type);


/// @brief 新建 全局公共CSS处理表
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @return klb_map_t* 函数处理表
KLB_API klb_map_t* klb_gui_new_globalcss_map(klb_gui_t* p_gui, const char* p_type);


/// @brief 销毁 全局公共CSS属性
/// @param [in] *p_css      窗体对象
/// @return 无
typedef void(*klb_gui_globalcss_destroy_cb)(void* p_css);


/// @brief 设置 全局公共CSS属性 指针
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @param [in] *p_css          全局公共CSS属性; 设置后, 由GUI框架托管
/// @param [in] cb_destroy      *p_css的销毁函数
/// @return 无
/// @note 若重复设置, 以最后设置的为准;
///       注意: 不要轻易覆盖, 可能造成 缓存的指针失效!
KLB_API void klb_gui_globalcss_set_ptr(klb_gui_t* p_gui, const char* p_type, void* p_css, klb_gui_globalcss_destroy_cb cb_destroy);


/// @brief 获取 全局公共CSS属性 指针
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         组件类型名
/// @return void* 全局公共CSS属性 指针
/// @note 控件自定义其CSS属性 结构体
KLB_API void* klb_gui_globalcss_get_ptr(klb_gui_t* p_gui, const char* p_type);


/// @brief 全局公共CSS属性 设置/获取 函数
/// @param [in] *ptr            CSS指针
/// @param [in] method          设置/获取:  KLBUI_CSSEX_get, KLBUI_CSSEX_set
/// @return 无
typedef void(*klb_gui_globalcss_cb)(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out);


/// @brief 设置 全局公共CSS属性
/// @note 参考 klb_wnd_set
KLB_API int klb_gui_globalcss_set(klb_gui_t* p_gui, const char* p_type, const klb_map_t* p_map);


/// @brief 获取 全局公共CSS属性
/// @note 参考 klb_wnd_get
KLB_API klb_map_t* klb_gui_globalcss_get(klb_gui_t* p_gui, const char* p_type, const klb_map_t* p_map);


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_CSS_H__
//end
