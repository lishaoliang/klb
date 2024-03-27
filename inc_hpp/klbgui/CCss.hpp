///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CCss.hpp
/// @brief   klbui css 的 cpp 封装
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CCSS_HPP__
#define __KLB_CCSS_HPP__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include <string>

namespace klbui {

KLB_EXTERN class KLB_API_CPP CCss
{
public:
    CCss();
    virtual ~CCss();

public:
    //////////////////////////////////////////////////////////////////////////
    // 各种状态 基础结构

    klbuicss_margin_t*  Margin();
    klbuicss_padding_t* Padding();

    klbuicssex_attributes_t* Normal();
    klbuicssex_attributes_t* Focus();
    klbuicssex_attributes_t* Disable();

public:
    //////////////////////////////////////////////////////////////////////////
    // normal 状态 css 接口

    ///////////////////////////////////////
    // klbuicss_margin_t 外边距

    /// @brief [normal] 外边距 "margin"
    void SetMargin(int w);
    void SetMargin(int top, int right, int bottom, int left);

    /// @brief [normal] 外边距 "padding"
    void SetPadding(int w);
    void SetPadding(int top, int right, int bottom, int left);

    ///////////////////////////////////////
    // klbuicss_text_t 文本属性

    /// @brief [normal]文本颜色 "color"
    void SetColor(uint32_t color);
    uint32_t GetColor();

    /// @brief [normal]文本对齐, "text-align"
    /// eg. KLBUICSS_text1_left
    void SetTextAlign(int align);

    ///////////////////////////////////////
    // klbuicss_font_t 字体属性

    /// @brief [normal]文本大小, "font-size"
    void SetFontSize(int font_size);

    ///////////////////////////////////////
    // klbuicss_background_t 背景属性

    /// @brief [normal]背景色, "background-color"
    void SetBackgroundColor(uint32_t color);

    /// @brief [normal]背景图片, "background-image"
    void SetBackgroundImage(const char* p_image_path);
    void SetBackgroundImage(const std::string& image_path);

    ///////////////////////////////////////
    // klbuicss_border_t 边框属性

    /// @brief [normal]边框的宽度, "border-width"
    void SetBorderWidth(int w);
    void SetBorderWidth(int top, int right, int bottom, int left);

    /// @brief [normal]边框的颜色, "border-color"
    void SetBorderColor(uint32_t color);
    void SetBorderColor(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left);

public:
    //////////////////////////////////////////////////////////////////////////
    // 

    /// @brief 销毁内存
    virtual void OnDelete();

private:
    // 常驻 css 项

    klbuicss_margin_t           m_margin;       ///< 外边距
    klbuicss_padding_t          m_padding;      ///< 内边距

private:
    // 可选 css 项目
    // 可选 css 项, 默认情况下没有; 在使用到时动态生成
    klbuicssex_attributes_t*    m_normal;
    klbuicssex_attributes_t*    m_focus;
    klbuicssex_attributes_t*    m_disable;

}; // class CCss

} // namespace klbui

#endif // __KLB_CCSS_HPP__
//end
