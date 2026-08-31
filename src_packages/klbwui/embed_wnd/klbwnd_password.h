// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwnd_password.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb window password, 密码输入框
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWND_PASSWORD_H__
#define __KLBWND_PASSWORD_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbwui/core/klbwui_css.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWND_PASSWORD_btn_symbol      0   ///< 显隐密码 (使用 * 替代)
#define KLBWND_PASSWORD_btn_clear       1   ///< 清除密码


typedef struct klbwnd_password_attributes_t_
{
    klbuicss_text_t             text;               ///< color, text-align
    klbuicss_font_t             font;               ///< font-size
    klbuicss_background_t       background;         ///< background-color / background-image / image_mode / image_flags
    klbuicss_border_t           border;             ///< border-width, border-color

    sds                         stretch_image;      ///< 拉伸圆角背景图
    sds                         btn_symbol_image;   ///< 显隐密码按钮图
    sds                         btn_clear_image;    ///< 清除按钮图
} klbwnd_password_attributes_t;


typedef struct klbwnd_password_css_t_
{
    klbuicss_margin_t               margin;         ///< 外边距
    klbuicss_padding_t              padding;        ///< 内边距

    klbwnd_password_attributes_t    normal;         ///< normal 常规状态参数
    klbwnd_password_attributes_t    focus;          ///< focus 聚焦状态参数
    klbwnd_password_attributes_t    disable;        ///< disable 不使能状态参数

    int                             btn_interval;   ///< 按钮间距
    int                             btn_w;          ///< 按钮图片宽
    int                             btn_h;          ///< 按钮图片高
    bool                            btn_visibility; ///< 是否显示按钮图片
} klbwnd_password_css_t;


typedef struct klbwnd_password_t_
{
    klbwnd_password_css_t*      p_css;          ///< 样式

    klb_rect_t                  rect_image[2];  ///< 按钮点击区域

    int                         index;          ///< 序号
    sds                         title;          ///< 标题 / 当前值

    bool                        is_hide_letter; ///< 是否用 * 隐藏显示
    sds                         hide_title;     ///< 隐藏标题串 "*"

    klb_wnd_t*                  p_menu;         ///< 输入面板
} klbwnd_password_t;


/// @brief init/deinit/create
void klbwnd_password_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h);
void klbwnd_password_deinit(klb_wnd_t* p_wnd);
klb_wnd_t* klbwnd_password_create(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief css init/deinit
void klbwnd_password_css_init(klbwnd_password_css_t* p_css, klb_gui_t* p_gui);
void klbwnd_password_css_deinit(klbwnd_password_css_t* p_css);
void klbwnd_password_css_copy(klbwnd_password_css_t* p_dst, klbwnd_password_css_t* p_src);


/// @brief set css
void klbwnd_password_set_css(klb_wnd_t* p_wnd, klbwnd_password_css_t* p_css);


/// @brief 设置/获取序号
void klbwnd_password_set_index(klb_wnd_t* p_wnd, int index);
int  klbwnd_password_get_index(klb_wnd_t* p_wnd);


/// @brief 设置/获取标题
void klbwnd_password_set_title(klb_wnd_t* p_wnd, const char* p_title);
const sds klbwnd_password_get_title(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWND_PASSWORD_H__

// end
