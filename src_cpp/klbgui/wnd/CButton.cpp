// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CButton.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CButtonCss

CButtonCss::CButtonCss(CGui* p_gui) : CCss(p_gui)
{

}

CButtonCss::~CButtonCss()
{

}

////////////////////////////////////////
// 继承重写 函数

void CButtonCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CButton

CButton::CButton(CGui* p_gui, int x, int y, int w, int h) : CWnd(p_gui, x, y, w, h)
{
    // step1 : set type / tid
    SetType(KLBUI_cbutton);
    SetTID(KLBUI_cbutton_tid);

    // step2. CSS属性
    CButtonCss* p_css = new CButtonCss(p_gui);
    SetCss(p_css);

    // step3. 样式
    SetStyle(0);

    // step4. 其他初始值
}

CButton::~CButton()
{

}

////////////////////////////////////////
// 禁止拷贝

CButton& CButton::operator=(const CButton& ths)
{
    return *this;
}

////////////////////////////////////////
// 对外提供的接口

CButtonCss* CButton::ToButtonCss()
{
    return dynamic_cast<CButtonCss*>(GetCss());
}

klb::CString& CButton::GetTitle()
{
    return m_title;
}

void CButton::GetTitle(std::string& title)
{
    title = m_title.c_str();
}

void CButton::GetTitle(klb::CString& title)
{
    title = m_title;
}

void CButton::SetTitle(const std::string& title)
{
    m_title = title;
}

void CButton::SetTitle(const char* p_title)
{
    m_title = p_title;
}

void CButton::SetTitle(const klb::CString& title)
{
    m_title = title;
}

klb::CString& CButton::GetValue()
{
    return m_value;
}

void CButton::SetValue(const std::string& value)
{
    m_value = value;
}

void CButton::SetValue(const char* p_value)
{
    m_value = p_value;
}

void CButton::SetValue(const klb::CString& value)
{
    m_value = value;
}

////////////////////////////////////////
// 内部事件响应等函数

void CButton::OnPaintNormal(CButtonCss* p_css, klb_rect_t* p_rect)
{
    // 背景
    DrawFillRect2(p_rect, p_css->GetBackgroundColor());

    // 边框
    uint32_t color_border = 0;
    p_css->GetBorderColor(&color_border, NULL, NULL, NULL);
    DrawRect2(p_rect, color_border);

    // 标题文本
    DrawText2(p_rect, m_title, p_css->GetColor(), p_css->GetFontSize());
}

void CButton::OnPaintFocus(CButtonCss* p_css, klb_rect_t* p_rect)
{
    // 背景
    DrawFillRect2(p_rect, p_css->GetBackgroundColorFocus());

    // 边框
    uint32_t color_border = 0;
    p_css->GetBorderColorFocus(&color_border, NULL, NULL, NULL);
    DrawRect2(p_rect, color_border);

    // 标题文本
    DrawText2(p_rect, m_title, p_css->GetColorFocus(), p_css->GetFontSizeFocus());
}

void CButton::OnPaintDisable(CButtonCss* p_css, klb_rect_t* p_rect)
{
    // 背景
    DrawFillRect2(p_rect, p_css->GetBackgroundColorDisable());

    // 边框
    uint32_t color_border = 0;
    p_css->GetBorderColorDisable(&color_border, NULL, NULL, NULL);
    DrawRect2(p_rect, color_border);

    // 标题文本
    DrawText2(p_rect, m_title, p_css->GetColorDisable(), p_css->GetFontSizeDisable());
}

////////////////////////////////////////
// 继承重写 函数

// 删除(销毁)
void CButton::OnDelete()
{
    CButtonCss* p_css = ToButtonCss();
    p_css->OnDelete();

    delete this;
}

// 绘制
int CButton::OnPaint()
{
    CButtonCss* p_css = ToButtonCss();
    klb_rect_t* p_rect = GetCanvasRect();

    if (IsFocus())
    {
        OnPaintFocus(p_css, p_rect); // 聚焦状态
    }
    else if(IsDisable())
    {
        OnPaintDisable(p_css, p_rect); // 不使能
    }
    else
    {
        OnPaintNormal(p_css, p_rect); // 常规状态
    }

    return 0;
}

// 本控件 事件响应
int CButton::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}

////////////////////////////////////////
// CSS 函数

void CButton::OnCssTitle(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CButton* p_this = dynamic_cast<CButton*>(p_cwnd);
    CButtonCss* p_css = p_this->ToButtonCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTitle();
    }
    else
    {
        int start = 1;

        klb::CString title;
        if (p_this->CheckCssString(p_in, start, title))
        {
            p_this->SetTitle(title);

            p_this->Update();
        }
    }
}

void CButton::OnCssValue(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CButton* p_this = dynamic_cast<CButton*>(p_cwnd);
    CButtonCss* p_css = p_this->ToButtonCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetValue();
    }
    else
    {
        int start = 1;

        klb::CString value;
        if (p_this->CheckCssString(p_in, start, value))
        {
            p_this->SetValue(value);

            p_this->Update();
        }
    }
}

bool CButton::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first; // 不是第一次, 直接退出
    }

    // 绑定响应函数

    // 私有函数
    BindCssFunction("title", CButton::OnCssTitle); // 标题
    BindCssFunction("value", CButton::OnCssValue); // 值

    return is_first;
}

////////////////////////////////////////
// 创建/注册

static klb_wnd_t* klbuicpp_create_cbutton(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);

    // step2 : new
    CButton* p_button = new CButton(p_cppgui, x, y, w, h);

    // step3 : 初始化CSS 支持的方法
    p_button->InitCssFunctionMap(KLBUI_cbutton);

    return p_button->GetWnd();
}

// 注册类型: "cbutton" (class button, CButton)
int klbui_register_cbutton(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_cbutton, klbuicpp_create_cbutton);
}

} // namespace klbui
//end
