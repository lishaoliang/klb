///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CCss.hpp
/// @brief   klbui css 的 cpp 封装
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CCSS_HPP__
#define __KLBUI_CCSS_HPP__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbthird/sds.h"
#include "klbutil/CString.hpp"
#include <string>

namespace klbui {


class CGui;


/// @class CCss
/// @brief CSS基类
KLB_EXTERN class KLB_API_CPP CCss
{
public:
    CCss(CGui* p_gui);
    virtual ~CCss();

    /// @brief (继承重写)销毁内存
    virtual void OnDelete();

public:
    //////////////////////////////////////////////////////////////////////////
    // 各种状态 基础结构

    static klbuicssex_attributes_t* MallocAttributes();
    static void FreeAttributes(klbuicssex_attributes_t* p_attr);


    klbuicss_margin_t*  Margin();
    klbuicss_padding_t* Padding();

    klbuicssex_attributes_t* Normal();
    klbuicssex_attributes_t* Focus();
    klbuicssex_attributes_t* Disable();

public:
    ///////////////////////////////////////
    // klbuicss_margin_t 外边距

    /// @brief 外边距 "margin"
    void SetMargin(int w);
    void SetMargin(int top, int right, int bottom, int left);
    void SetMarginTop(int top);
    void SetMarginRight(int right);
    void SetMarginBottom(int bottom);
    void SetMarginLeft(int left);

    void GetMargin(int* p_top, int* p_right, int* p_bottom, int* p_left);

    /// @brief 内边距 "padding"
    void SetPadding(int w);
    void SetPadding(int top, int right, int bottom, int left);
    void SetPaddingTop(int top);
    void SetPaddingRight(int right);
    void SetPaddingBottom(int bottom);
    void SetPaddingLeft(int left);

    void GetPadding(int* p_top, int* p_right, int* p_bottom, int* p_left);

    ///////////////////////////////////////
    // klbuicss_text_t 文本属性

    /// @brief 文本颜色 "color"
    void SetColor(uint32_t color);
    void SetColorFocus(uint32_t color);
    void SetColorDisable(uint32_t color);

    uint32_t GetColor();
    uint32_t GetColorFocus();
    uint32_t GetColorDisable();

    /// @brief 文本对齐, "text-align"
    /// eg. KLBUICSS_text1_left
    void SetTextAlign(int align);
    void SetTextAlignFocus(int align);
    void SetTextAlignDisable(int align);

    int GetTextAlign();
    int GetTextAlignFocus();
    int GetTextAlignDisable();

    ///////////////////////////////////////
    // klbuicss_font_t 字体属性

    /// @brief [normal]文本大小, "font-size"
    void SetFontSize(int font_size);
    void SetFontSizeFocus(int font_size);
    void SetFontSizeDisable(int font_size);

    int GetFontSize();
    int GetFontSizeFocus();
    int GetFontSizeDisable();

    ///////////////////////////////////////
    // klbuicss_background_t 背景属性

    /// @brief 背景色, "background-color"
    void SetBackgroundColor(uint32_t color);
    void SetBackgroundColorFocus(uint32_t color);
    void SetBackgroundColorDisable(uint32_t color);

    uint32_t GetBackgroundColor();
    uint32_t GetBackgroundColorFocus();
    uint32_t GetBackgroundColorDisable();

    /// @brief 背景图片, "background-image"
    void SetBackgroundImage(const char* p_image_path);
    void SetBackgroundImage(const std::string& image_path);
    void SetBackgroundImage(const klb::CString& image_path);
    void SetBackgroundImageFocus(const char* p_image_path);
    void SetBackgroundImageFocus(const std::string& image_path);
    void SetBackgroundImageFocus(const klb::CString& image_path);
    void SetBackgroundImageDisable(const char* p_image_path);
    void SetBackgroundImageDisable(const std::string& image_path);
    void SetBackgroundImageDisable(const klb::CString& image_path);

    const sds GetBackgroundImage();
    const sds GetBackgroundImageFocus();
    const sds GetBackgroundImageDisable();
    void GetBackgroundImage(std::string& out_path);
    void GetBackgroundImage(klb::CString& out_path);
    void GetBackgroundImageFocus(std::string& out_path);
    void GetBackgroundImageFocus(klb::CString& out_path);
    void GetBackgroundImageDisable(std::string& out_path);
    void GetBackgroundImageDisable(klb::CString& out_path);

    ///////////////////////////////////////
    // klbuicss_border_t 边框属性

    /// @brief 边框的宽度, "border-width"
    void SetBorderWidth(int w);
    void SetBorderWidth(int top, int right, int bottom, int left);
    void SetBorderWidthFocus(int w);
    void SetBorderWidthFocus(int top, int right, int bottom, int left);
    void SetBorderWidthDisable(int w);
    void SetBorderWidthDisable(int top, int right, int bottom, int left);

    void GetBorderWidth(int* p_top, int* p_right, int* p_bottom, int* p_left);
    void GetBorderWidthFocus(int* p_top, int* p_right, int* p_bottom, int* p_left);
    void GetBorderWidthDisable(int* p_top, int* p_right, int* p_bottom, int* p_left);

    /// @brief 边框的颜色, "border-color"
    void SetBorderColor(uint32_t color);
    void SetBorderColor(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left);
    void SetBorderColorFocus(uint32_t color);
    void SetBorderColorFocus(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left);
    void SetBorderColorDisable(uint32_t color);
    void SetBorderColorDisable(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left);

    void GetBorderColor(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left);
    void GetBorderColorFocus(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left);
    void GetBorderColorDisable(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left);


private:
    CGui*                       m_gui;          ///< gui

    // 常驻 css 项

    klbuicss_margin_t           m_margin;       ///< 外边距
    klbuicss_padding_t          m_padding;      ///< 内边距

private:
    // 可选 css 项目
    // 可选 css 项, 默认情况下没有; 在使用到时动态生成
    klbuicssex_attributes_t*    m_normal;       ///< 常规状态
    klbuicssex_attributes_t*    m_focus;        ///< 聚焦状态
    klbuicssex_attributes_t*    m_disable;      ///< 不使能状态

}; // class CCss

} // namespace klbui

#endif // __KLBUI_CCSS_HPP__
//end
