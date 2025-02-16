// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CCss.hpp"
#include "klbgui/CGui.hpp"
#include "klbmem/klb_mem.h"
#include "klbthird/sds.h"
#include "klbgui/klbui_css_ex.h"
#include <assert.h>


using namespace klb;


namespace klbui {


//////////////////////////////////////////////////////////////////////////
// 构造/析构

CCss::CCss(CGui* p_gui)
{
    const klbui_default_t* p_default = p_gui->GetStdDefault();

    m_gui = p_gui;

    m_margin = p_default->margin;
    m_padding = p_default->padding;

    m_normal = NULL;
    m_focus = NULL;
    m_disable = NULL;
}

CCss::~CCss()
{
    KLB_FREE_BY(m_normal, FreeAttributes);
    KLB_FREE_BY(m_focus, FreeAttributes);
    KLB_FREE_BY(m_disable, FreeAttributes);

    m_gui = NULL;
}

/// @brief 销毁内存
void CCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
//

klbuicssex_attributes_t* CCss::MallocAttributes()
{
    klbuicssex_attributes_t* p_attr = KLB_MALLOCZ(klbuicssex_attributes_t, 1, 0);

    p_attr->background.image = sdsempty();

    return p_attr;
}

void CCss::FreeAttributes(klbuicssex_attributes_t* p_attr)
{
    if (NULL != p_attr)
    {
        KLB_FREE_BY(p_attr->background.image, sdsfree);
        KLB_FREE(p_attr);
    }
}

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
    if (NULL == m_normal)
    {
        m_normal = MallocAttributes();

        // 初始化
        const klbui_default_t* p_default = m_gui->GetStdDefault();
        klbuicssex_attributes_copy(m_normal, &p_default->normal);
    }

    return m_normal;
}

klbuicssex_attributes_t* CCss::Focus()
{
    if (NULL == m_focus)
    {
        m_focus = MallocAttributes();

        // 初始化
        const klbui_default_t* p_default = m_gui->GetStdDefault();
        klbuicssex_attributes_copy(m_focus, &p_default->focus);
    }

    return m_focus;
}

klbuicssex_attributes_t* CCss::Disable()
{
    if (NULL == m_disable)
    {
        m_disable = MallocAttributes();

        // 初始化
        const klbui_default_t* p_default = m_gui->GetStdDefault();
        klbuicssex_attributes_copy(m_disable, &p_default->disable);
    }

    return m_disable;
}

//////////////////////////////////////////////////////////////////////////
// css 接口

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

void CCss::SetMarginTop(int top)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->top = top;
}

void CCss::SetMarginRight(int right)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->right = right;
}

void CCss::SetMarginBottom(int bottom)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->bottom = bottom;
}

void CCss::SetMarginLeft(int left)
{
    klbuicss_margin_t* p_margin = Margin();

    p_margin->left = left;
}

void CCss::GetMargin(int* p_top, int* p_right, int* p_bottom, int* p_left)
{
    klbuicss_margin_t* p_margin = Margin();

    if (NULL != p_top) { *p_top = p_margin->top; }
    if (NULL != p_right) { *p_right = p_margin->right; }
    if (NULL != p_bottom) { *p_bottom = p_margin->bottom; }
    if (NULL != p_left) { *p_left = p_margin->left; }
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

void CCss::SetPaddingTop(int top)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->top = top;
}

void CCss::SetPaddingRight(int right)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->right = right;
}

void CCss::SetPaddingBottom(int bottom)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->bottom = bottom;
}

void CCss::SetPaddingLeft(int left)
{
    klbuicss_padding_t* p_padding = Padding();

    p_padding->left = left;
}

void CCss::GetPadding(int* p_top, int* p_right, int* p_bottom, int* p_left)
{
    klbuicss_padding_t* p_padding = Padding();

    if (NULL != p_top) { *p_top = p_padding->top; }
    if (NULL != p_right) { *p_right = p_padding->right; }
    if (NULL != p_bottom) { *p_bottom = p_padding->bottom; }
    if (NULL != p_left) { *p_left = p_padding->left; }
}

///////////////////////////////////////
// klbuicss_text_t 文本属性

void CCss::SetColor(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->text.color = color;
}

void CCss::SetColorFocus(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->text.color = color;
}

void CCss::SetColorDisable(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->text.color = color;
}

uint32_t CCss::GetColor()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->text.color;
}

uint32_t CCss::GetColorFocus()
{
    klbuicssex_attributes_t* p_attr = Focus();
    return p_attr->text.color;
}

uint32_t CCss::GetColorDisable()
{
    klbuicssex_attributes_t* p_attr = Disable();
    return p_attr->text.color;
}

void CCss::SetTextAlign(int align)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->text.align = align;
}

void CCss::SetTextAlignFocus(int align)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->text.align = align;
}

void CCss::SetTextAlignDisable(int align)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->text.align = align;
}

int CCss::GetTextAlign()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->text.align;
}

int CCss::GetTextAlignFocus()
{
    klbuicssex_attributes_t* p_attr = Focus();
    return p_attr->text.align;
}

int CCss::GetTextAlignDisable()
{
    klbuicssex_attributes_t* p_attr = Disable();
    return p_attr->text.align;
}

///////////////////////////////////////
// klbuicss_font_t (文本)字体

void CCss::SetFontSize(int font_size)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->font.size = font_size;
}

void CCss::SetFontSizeFocus(int font_size)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->font.size = font_size;
}

void CCss::SetFontSizeDisable(int font_size)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->font.size = font_size;
}

int CCss::GetFontSize()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->font.size;
}

int CCss::GetFontSizeFocus()
{
    klbuicssex_attributes_t* p_attr = Focus();
    return p_attr->font.size;
}

int CCss::GetFontSizeDisable()
{
    klbuicssex_attributes_t* p_attr = Disable();
    return p_attr->font.size;
}

///////////////////////////////////////
// klbuicss_background_t 背景属性

void CCss::SetBackgroundColor(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->background.color = color;
}

void CCss::SetBackgroundColorFocus(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->background.color = color;
}

void CCss::SetBackgroundColorDisable(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->background.color = color;
}

uint32_t CCss::GetBackgroundColor()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->background.color;
}

uint32_t CCss::GetBackgroundColorFocus()
{
    klbuicssex_attributes_t* p_attr = Focus();
    return p_attr->background.color;
}

uint32_t CCss::GetBackgroundColorDisable()
{
    klbuicssex_attributes_t* p_attr = Disable();
    return p_attr->background.color;
}

void CCss::SetBackgroundImage(const char* p_image_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->background.image = sdscpy(p_attr->background.image, p_image_path);
}

void CCss::SetBackgroundImage(const std::string& image_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

void CCss::SetBackgroundImage(const klb::CString& image_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

void CCss::SetBackgroundImageFocus(const char* p_image_path)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->background.image = sdscpy(p_attr->background.image, p_image_path);
}

void CCss::SetBackgroundImageFocus(const std::string& image_path)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

void CCss::SetBackgroundImageFocus(const klb::CString& image_path)
{
    klbuicssex_attributes_t* p_attr = Focus();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

void CCss::SetBackgroundImageDisable(const char* p_image_path)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->background.image = sdscpy(p_attr->background.image, p_image_path);
}

void CCss::SetBackgroundImageDisable(const std::string& image_path)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

void CCss::SetBackgroundImageDisable(const klb::CString& image_path)
{
    klbuicssex_attributes_t* p_attr = Disable();
    p_attr->background.image = sdscpy(p_attr->background.image, image_path.c_str());
}

const sds CCss::GetBackgroundImage()
{
    klbuicssex_attributes_t* p_attr = Normal();
    return p_attr->background.image;
}

const sds CCss::GetBackgroundImageFocus()
{
    klbuicssex_attributes_t* p_attr = Focus();
    return p_attr->background.image;
}

const sds CCss::GetBackgroundImageDisable()
{
    klbuicssex_attributes_t* p_attr = Disable();
    return p_attr->background.image;
}

void CCss::GetBackgroundImage(std::string& out_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
    out_path = p_attr->background.image;
}

void CCss::GetBackgroundImage(klb::CString& out_path)
{
    klbuicssex_attributes_t* p_attr = Normal();
    out_path = p_attr->background.image;
}

void CCss::GetBackgroundImageFocus(std::string& out_path)
{
    klbuicssex_attributes_t* p_attr = Focus();
    out_path = p_attr->background.image;
}

void CCss::GetBackgroundImageFocus(klb::CString& out_path)
{
    klbuicssex_attributes_t* p_attr = Focus();
    out_path = p_attr->background.image;
}

void CCss::GetBackgroundImageDisable(std::string& out_path)
{
    klbuicssex_attributes_t* p_attr = Disable();
    out_path = p_attr->background.image;
}

void CCss::GetBackgroundImageDisable(klb::CString& out_path)
{
    klbuicssex_attributes_t* p_attr = Disable();
    out_path = p_attr->background.image;
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

void CCss::SetBorderWidthFocus(int w)
{
    klbuicssex_attributes_t* p_attr = Focus();

    p_attr->border.width.top = w;
    p_attr->border.width.right = w;
    p_attr->border.width.bottom = w;
    p_attr->border.width.left = w;
}

void CCss::SetBorderWidthFocus(int top, int right, int bottom, int left)
{
    klbuicssex_attributes_t* p_attr = Focus();

    p_attr->border.width.top = top;
    p_attr->border.width.right = right;
    p_attr->border.width.bottom = bottom;
    p_attr->border.width.left = left;
}

void CCss::SetBorderWidthDisable(int w)
{
    klbuicssex_attributes_t* p_attr = Disable();

    p_attr->border.width.top = w;
    p_attr->border.width.right = w;
    p_attr->border.width.bottom = w;
    p_attr->border.width.left = w;
}

void CCss::SetBorderWidthDisable(int top, int right, int bottom, int left)
{
    klbuicssex_attributes_t* p_attr = Disable();

    p_attr->border.width.top = top;
    p_attr->border.width.right = right;
    p_attr->border.width.bottom = bottom;
    p_attr->border.width.left = left;
}

void CCss::GetBorderWidth(int* p_top, int* p_right, int* p_bottom, int* p_left)
{
    klbuicssex_attributes_t* p_attr = Normal();

    if (NULL != p_top) { *p_top = p_attr->border.width.top; }
    if (NULL != p_right) { *p_right = p_attr->border.width.right; }
    if (NULL != p_bottom) { *p_bottom = p_attr->border.width.bottom; }
    if (NULL != p_left) { *p_left = p_attr->border.width.left; }
}

void CCss::GetBorderWidthFocus(int* p_top, int* p_right, int* p_bottom, int* p_left)
{
    klbuicssex_attributes_t* p_attr = Focus();

    if (NULL != p_top) { *p_top = p_attr->border.width.top; }
    if (NULL != p_right) { *p_right = p_attr->border.width.right; }
    if (NULL != p_bottom) { *p_bottom = p_attr->border.width.bottom; }
    if (NULL != p_left) { *p_left = p_attr->border.width.left; }
}

void CCss::GetBorderWidthDisable(int* p_top, int* p_right, int* p_bottom, int* p_left)
{
    klbuicssex_attributes_t* p_attr = Disable();

    if (NULL != p_top) { *p_top = p_attr->border.width.top; }
    if (NULL != p_right) { *p_right = p_attr->border.width.right; }
    if (NULL != p_bottom) { *p_bottom = p_attr->border.width.bottom; }
    if (NULL != p_left) { *p_left = p_attr->border.width.left; }
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

void CCss::SetBorderColorFocus(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Focus();

    p_attr->border.color.top = color;
    p_attr->border.color.right = color;
    p_attr->border.color.bottom = color;
    p_attr->border.color.left = color;
}

void CCss::SetBorderColorFocus(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left)
{
    klbuicssex_attributes_t* p_attr = Focus();

    p_attr->border.color.top = color_top;
    p_attr->border.color.right = color_right;
    p_attr->border.color.bottom = color_bottom;
    p_attr->border.color.left = color_left;
}

void CCss::SetBorderColorDisable(uint32_t color)
{
    klbuicssex_attributes_t* p_attr = Disable();

    p_attr->border.color.top = color;
    p_attr->border.color.right = color;
    p_attr->border.color.bottom = color;
    p_attr->border.color.left = color;
}

void CCss::SetBorderColorDisable(uint32_t color_top, uint32_t color_right, uint32_t color_bottom, uint32_t color_left)
{
    klbuicssex_attributes_t* p_attr = Disable();

    p_attr->border.color.top = color_top;
    p_attr->border.color.right = color_right;
    p_attr->border.color.bottom = color_bottom;
    p_attr->border.color.left = color_left;
}

void CCss::GetBorderColor(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left)
{
    klbuicssex_attributes_t* p_attr = Normal();

    if (NULL != p_color_top) { *p_color_top = p_attr->border.color.top; }
    if (NULL != p_color_right) { *p_color_right = p_attr->border.color.right; }
    if (NULL != p_color_bottom) { *p_color_bottom = p_attr->border.color.bottom; }
    if (NULL != p_color_left) { *p_color_left = p_attr->border.color.left; }
}

void CCss::GetBorderColorFocus(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left)
{
    klbuicssex_attributes_t* p_attr = Focus();

    if (NULL != p_color_top) { *p_color_top = p_attr->border.color.top; }
    if (NULL != p_color_right) { *p_color_right = p_attr->border.color.right; }
    if (NULL != p_color_bottom) { *p_color_bottom = p_attr->border.color.bottom; }
    if (NULL != p_color_left) { *p_color_left = p_attr->border.color.left; }
}

void CCss::GetBorderColorDisable(uint32_t* p_color_top, uint32_t* p_color_right, uint32_t* p_color_bottom, uint32_t* p_color_left)
{
    klbuicssex_attributes_t* p_attr = Disable();

    if (NULL != p_color_top) { *p_color_top = p_attr->border.color.top; }
    if (NULL != p_color_right) { *p_color_right = p_attr->border.color.right; }
    if (NULL != p_color_bottom) { *p_color_bottom = p_attr->border.color.bottom; }
    if (NULL != p_color_left) { *p_color_left = p_attr->border.color.left; }
}

//////////////////////////////////////////////////////////////////////////
} // namespace klbui
//end
