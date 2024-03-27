// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CCss.hpp"
#include "klbgui/CGui.hpp"
#include <assert.h>


using namespace klb;


namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构

CCss::CCss()
{
    m_normal = NULL;
    m_focus = NULL;
    m_disable = NULL;
}

CCss::~CCss()
{

}

//////////////////////////////////////////////////////////////////////////
//

klbuicss_margin_t* CCss::Margin()
{
    return &m_margin;
}

klbuicss_padding_t* CCss::Padding()
{
    return &m_padding;
}

klbuicssex_attributes_t* CCss::Normal()
{
    return m_normal;
}

klbuicssex_attributes_t* CCss::Focus()
{
    return m_focus;
}

klbuicssex_attributes_t* CCss::Disable()
{
    return m_disable;
}

//////////////////////////////////////////////////////////////////////////
// normal 状态 css 接口

///////////////////////////////////////
// klbuicss_margin_t 外边距

void CCss::SetMargin(int w)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->top = w;
    p_margin->right = w;
    p_margin->bottom = w;
    p_margin->left = w;
}

void CCss::SetMargin(int top, int right, int bottom, int left)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->top = top;
    p_margin->right = right;
    p_margin->bottom = bottom;
    p_margin->left = left;
}

void CCss::SetPadding(int w)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->top = w;
    p_padding->right = w;
    p_padding->bottom = w;
    p_padding->left = w;
}

void CCss::SetPadding(int top, int right, int bottom, int left)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->top = top;
    p_padding->right = right;
    p_padding->bottom = bottom;
    p_padding->left = left;
}

///////////////////////////////////////
// klbuicss_text_t 文本属性

void CCss::SetColor(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->text.color = color;
}

uint32_t CCss::GetColor()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->text.color;
}

void CCss::SetTextAlign(int align)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->text.align = align;
}

///////////////////////////////////////
// klbuicss_font_t (文本)字体

void CCss::SetFontSize(int font_size)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->font.size = font_size;
}

///////////////////////////////////////
// klbuicss_background_t 背景属性

void CCss::SetBackgroundColor(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->background.color = color;
}

void CCss::SetBackgroundImage(const char* p_image_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
}

void CCss::SetBackgroundImage(const std::string& image_path)
{
    SetBackgroundImage(image_path.c_str());
}

///////////////////////////////////////
// klbuicss_border_t 边框属性

void CCss::SetBorderWidth(int w)
{
    klbuicssex_attributes_t* p_attr = Normal();

    p_attr->border.width.top = w;
    p_attr->border.width.right = w;
    p_attr->border.width.bottom = w;
    p_attr->border.width.left = w;
}

void CCss::SetBorderWidth(int top, int right, int bottom, int left)
{
    klbuicssex_attributes_t* p_attr = Normal();

    p_attr->border.width.top = top;
    p_attr->border.width.right = right;
    p_attr->border.width.bottom = bottom;
    p_attr->border.width.left = left;
}

void CCss::SetBorderColor(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Normal();

    p_attr->border.color.top = color;
    p_attr->border.color.right = color;
    p_attr->border.color.bottom = color;
    p_attr->border.color.left = color;
}

void CCss::SetBorderColor(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left)
{
    klbuicssex_attributes_t* p_attr = Normal();

    p_attr->border.color.top = color_top;
    p_attr->border.color.right = color_right;
    p_attr->border.color.bottom = color_bottom;
    p_attr->border.color.left = color_left;
}

//////////////////////////////////////////////////////////////////////////
// 

/// @brief 销毁内存
void CCss::OnDelete()
{

}

} // namespace klbui
