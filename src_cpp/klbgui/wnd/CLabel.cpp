// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CLabel.hpp"
#include "klbgui/CUiWidgets.hpp"
#include "klbgui/CDrawDC.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CLabelCss

CLabelCss::CLabelCss(CGui* p_gui) : CCss(p_gui)
{

}

CLabelCss::~CLabelCss()
{

}

/////////////////////////////////////
// 继承重写 函数

void CLabelCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CLabel

CLabel::CLabel(CGui* p_gui, int x, int y, int w, int h) : CWnd(p_gui, x, y, w, h)
{
    // step1 : set type / tid
    SetType(KLBUI_clabel);
    SetTID(KLBUI_clabel_tid);

    // step2. CSS属性
    CLabelCss* p_css = new CLabelCss(p_gui);
    SetCss(p_css);

    // step3. 样式
    SetStyle(KLB_WND_STYLE_NOCOMMAND | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW | KLB_WND_STYLE_FOCUS_DELAY);

    // step4. 其他初始值
}

CLabel::~CLabel()
{

}

////////////////////////////////////////
// 禁止拷贝

CLabel& CLabel::operator=(const CLabel& ths)
{
    return *this;
};

////////////////////////////////////////
// 对外提供的接口

CLabelCss* CLabel::ToLabelCss()
{
    return dynamic_cast<CLabelCss*>(GetCss());
}

klb::CString& CLabel::GetTitle()
{
    return m_title;
}

void CLabel::SetTitle(const klb::CString& title)
{
    m_title = title;

    // 动态tip 需要更新
    DynTip(true);
}

void CLabel::SetTitle(const std::string& title)
{
    m_title = title;

    // 动态tip 需要更新
    DynTip(true);
}

//////////////////////////////////////////////////////////////////////////
// 内部事件响应等函数

// 常规状态绘图
void CLabel::OnPaintNormal(CLabelCss* p_css, klb_rect_t* p_rect)
{
    CDrawDC dc(this);

    // 背景
    dc.DrawFillRect2(p_rect, p_css->GetBackgroundColor());

    // 标题文本
    dc.DrawText2(p_rect, m_title, p_css->GetColor(), p_css->GetFontSize());
}

void CLabel::OnFocusDelay(CLabelCss* p_css, klb_rect_t* p_rect)
{
    // 需要 设置 KLB_WND_STYLE_FOCUS_DELAY 样式, 才会触发本事件

    if (!IsDynTip())
    {
        return; // 无需更新
    }

    // 计算绘图区域 区域是否足够 绘制文本
    // 若不足, 则设置动态tip
    CDrawDC dc(this);

    int txt_w = 0;
    dc.TextSize2(m_title, p_css->GetFontSize(), &txt_w, NULL);

    if (0 < txt_w && p_rect->w < txt_w)
    {
        SetTipDynamic(m_title);
    }
    else
    {
        SetTipDynamic("");
    }

    // 动态tip已经更新
    DynTip(false);
}

//////////////////////////////////////////////////////////////////////////
// 继承重写 函数

// 删除(销毁)
void CLabel::OnDelete()
{
    CLabelCss* p_css = ToLabelCss();
    p_css->OnDelete();

    delete this;
}

// 绘制
int CLabel::OnPaint()
{
    CLabelCss* p_css = ToLabelCss();
    klb_rect_t* p_rect = GetCanvasRect();

    // 只支持 常规状态
    {
        OnPaintNormal(p_css, p_rect); // 常规状态
    }

    return 0;
}

// 本控件 事件响应
int CLabel::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLBUI_focusdelay:
        {
            CLabelCss* p_css = ToLabelCss();
            klb_rect_t* p_rect = GetCanvasRect();

            OnFocusDelay(p_css, p_rect);
        }
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// CSS 函数

void CLabel::OnCssTitle(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CLabel* p_this = dynamic_cast<CLabel*>(p_cwnd);
    CLabelCss* p_css = p_this->ToLabelCss();

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

//////////////////////////////////////////////////////////////////////////
// 初始化 CSS 函数表

bool CLabel::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first; // 不是第一次, 直接退出
    }

    // 绑定响应函数
    // 

    // 私有函数
    BindCssFunction("title", CLabel::OnCssTitle);                // 标题

    return is_first;
}

//////////////////////////////////////////////////////////////////////////
// 创建/注册

static klb_wnd_t* klbuicpp_create_clabel(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);

    // step2 : new
    CLabel* p_ticker = new CLabel(p_cppgui, x, y, w, h);

    // step3 : 初始化CSS 支持的方法
    p_ticker->InitCssFunctionMap(KLBUI_clabel);

    return p_ticker->GetWnd();
}

// 注册类型: "clabel" (class label, CLabel)
int klbui_register_clabel(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_clabel, klbuicpp_create_clabel);
}

} // namespace klbui
//end
