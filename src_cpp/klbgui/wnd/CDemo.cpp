// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CDemo.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CDemoCss

CDemoCss::CDemoCss(CGui* p_gui) : CCss(p_gui)
{

}

CDemoCss::~CDemoCss()
{

}

/////////////////////////////////////
// 继承重写 函数

void CDemoCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CDemo

CDemo::CDemo(CGui* p_gui, int x, int y, int w, int h) : CWnd(p_gui, x, y, w, h)
{
    // step1 : set type / tid
    SetType(KLBUI_cdemo);
    SetTID(KLBUI_cdemo_tid);

    // step2. CSS属性
    CDemoCss* p_css = new CDemoCss(p_gui);
    SetCss(p_css);

    // step3. 样式
    SetStyle(0);

    // step4. 其他初始值
}

CDemo::~CDemo()
{

}

////////////////////////////////////////
// 禁止拷贝

CDemo& CDemo::operator=(const CDemo& ths)
{
    return *this; 
};

////////////////////////////////////////
// 对外提供的接口

CDemoCss* CDemo::ToDemoCss()
{
    return dynamic_cast<CDemoCss*>(GetCss());
}

klb::CString& CDemo::GetTitle()
{
    return m_title;
}

void CDemo::SetTitle(const klb::CString& title)
{
    m_title = title;
}

void CDemo::SetTitle(const std::string& title)
{
    m_title = title;
}

//////////////////////////////////////////////////////////////////////////
// 内部事件响应等函数

// 常规状态绘图
void CDemo::OnPaintNormal(CDemoCss* p_css, klb_rect_t* p_rect)
{

}

// 聚焦状态绘图
void CDemo::OnPaintFocus(CDemoCss* p_css, klb_rect_t* p_rect)
{

}

// 不使能状态绘图
void CDemo::OnPaintDisable(CDemoCss* p_css, klb_rect_t* p_rect)
{

}

//////////////////////////////////////////////////////////////////////////
// 继承重写 函数

// 删除(销毁)
void CDemo::OnDelete()
{
    CDemoCss* p_css = ToDemoCss();
    p_css->OnDelete();

    delete this;
}

// 绘制
int CDemo::OnPaint()
{
    CDemoCss* p_css = ToDemoCss();
    klb_rect_t* p_rect = GetCanvasRect();

    if (IsFocus())
    {
        OnPaintFocus(p_css, p_rect); // 聚焦状态
    }
    else if (IsDisable())
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
int CDemo::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// CSS 函数

void CDemo::OnCssTitle(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CDemo* p_this = dynamic_cast<CDemo*>(p_cwnd);
    CDemoCss* p_css = p_this->ToDemoCss();

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

bool CDemo::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first; // 不是第一次, 直接退出
    }

    // 绑定响应函数
    // 

    // 私有函数
    BindCssFunction("title", CDemo::OnCssTitle);                // 标题

    return is_first;
}

//////////////////////////////////////////////////////////////////////////
// 创建/注册

static klb_wnd_t* klbuicpp_create_cdemo(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);

    // step2 : new
    CDemo* p_ticker = new CDemo(p_cppgui, x, y, w, h);

    // step3 : 初始化CSS 支持的方法
    p_ticker->InitCssFunctionMap(KLBUI_cdemo);

    return p_ticker->GetWnd();
}

// 注册类型: "cdemo" (class demo, CDemo)
int klbui_register_cdemo(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_cdemo, klbuicpp_create_cdemo);
}

} // namespace klbui
//end
