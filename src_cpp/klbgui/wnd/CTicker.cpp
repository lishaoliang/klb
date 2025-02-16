// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CTicker.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CTickerCss

CTickerCss::CTickerCss(CGui* p_gui) : CCss(p_gui)
{

}

CTickerCss::~CTickerCss()
{

}

/////////////////////////////////////
// 继承重写 函数

void CTickerCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CTicker

CTicker::CTicker(CGui* p_gui, int x, int y, int w, int h) : CWnd(p_gui, x, y, w, h)
{
    // step1 : set type / tid
    SetType(KLBUI_cticker);
    SetTID(KLBUI_cticker_tid);

    // step2. CSS属性
    CTickerCss* p_css = new CTickerCss(p_gui);
    SetCss(p_css);

    // step3. 样式
    SetStyle(KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_TICKER);

    // step4. 其他初始值
    m_enable = true;
    m_enable_default = true;
    m_tick_count = p_gui->GetTickCount();
    m_tc_interval = 1000; // 毫秒

    // 隐藏
    Hide(true);
}

CTicker::~CTicker()
{

}

////////////////////////////////////////
// 禁止拷贝

CTicker& CTicker::operator=(const CTicker& ths)
{
    return *this; 
};

////////////////////////////////////////
// 对外提供的接口

CTickerCss* CTicker::ToTickerCss()
{
    return dynamic_cast<CTickerCss*>(GetCss());
}

bool CTicker::GetTicker()
{
    return m_enable;
}

void CTicker::SetTicker(bool enable)
{
    // 是否启用 定时器
    m_enable = enable;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

bool CTicker::GetTickerDefault()
{
    return m_enable_default;
}

void CTicker::SetTickerDefault(bool enable)
{
    m_enable_default = enable;
}

int64_t CTicker::GetTimeInterval()
{
    return m_tc_interval;
}

void CTicker::SetTimeInterval(int64_t interval)
{
    // 更新
    m_tc_interval = interval;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

//////////////////////////////////////////////////////////////////////////
// 内部事件响应等函数

void CTicker::OnLoad()
{
    // 页面加载之后, 使用默认值更新
    m_enable = m_enable_default;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

void CTicker::OnTicker()
{
    // KLBUI_onticker 事件在 窗口卸载后, 不会产生

    int64_t tc = GetCGui()->GetTickCount();

    if (!m_enable)
    {
        // 不可用, 直接更新时间
        m_tick_count = tc;
        return;
    }

    if (ABS_SUB(tc, m_tick_count) < m_tc_interval)
    {
        return;
    }

    // 定时器到了, 更新
    m_tick_count = tc;

    // 触发 KLBUI_onticker 事件
    // 对于 KLBUI_onticker 事件: 存在以下场景
    // 1. 标准的 定时器控件, 需要隐藏状态(无需绘图)
    // 2. 动画等控件, 在不使能状态时, 依旧可能需要触发事件
    // 所以 onticker 需要与控件状态 无关, 由控件自行决定 适当时机触发
    CallCommand(KLBUI_onticker, NULL, NULL, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
// 继承重写 函数

// 删除
void CTicker::OnDelete()
{
    CTickerCss* p_css = ToTickerCss();
    p_css->OnDelete();

    delete this;
}

// 绘制
int CTicker::OnPaint()
{
    //CTickerCss* p_css = ToTickerCss();
    //klb_rect_t* p_rect = GetCanvasRect();

    // 无绘图

    return 0;
}

// 本控件 事件响应
int CTicker::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLBUI_onload:
        OnLoad();
        break;

    case KLBUI_onticker:
        OnTicker();
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// CSS 函数

void CTicker::OnCssTicker(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTicker* p_this = dynamic_cast<CTicker*>(p_cwnd);
    CTickerCss* p_css = p_this->ToTickerCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTicker();
    }
    else
    {
        int start = 1;

        bool enable = false;
        if (p_this->CheckCssBool(p_in, start, &enable))
        {
            p_this->SetTicker(enable);

            //p_this->Update();
        }
    }
}

void CTicker::OnCssTickerDefault(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTicker* p_this = dynamic_cast<CTicker*>(p_cwnd);
    CTickerCss* p_css = p_this->ToTickerCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTickerDefault();
    }
    else
    {
        int start = 1;

        bool enable = false;
        if (p_this->CheckCssBool(p_in, start, &enable))
        {
            p_this->SetTickerDefault(enable);

            //p_this->Update();
        }
    }
}

void CTicker::OnCssTimeInterval(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTicker* p_this = dynamic_cast<CTicker*>(p_cwnd);
    CTickerCss* p_css = p_this->ToTickerCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTimeInterval();
    }
    else
    {
        int start = 1;

        int64_t interval = 0;
        if (p_this->CheckCssInteger(p_in, start, &interval))
        {
            p_this->SetTimeInterval(interval);

            //p_this->Update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// 初始化 CSS 函数表

bool CTicker::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first; // 不是第一次, 直接退出
    }

    // 绑定响应函数
    // 

    // 私有函数
    BindCssFunction("ticker", CTicker::OnCssTicker);                // 是否启用 定时器
    BindCssFunction("ticker_default", CTicker::OnCssTickerDefault); // 初始默认值

    BindCssFunction("time_interval", CTicker::OnCssTimeInterval);   // 设置定时器事件间隔

    return is_first;
}

//////////////////////////////////////////////////////////////////////////
// 创建/注册

static klb_wnd_t* klbuicpp_create_cticker(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);

    // step2 : new
    CTicker* p_ticker = new CTicker(p_cppgui, x, y, w, h);

    // step3 : 初始化CSS 支持的方法
    p_ticker->InitCssFunctionMap(KLBUI_cticker);

    return p_ticker->GetWnd();
}

// 注册类型: "cticker" (class ticker, CTicker)
int klbui_register_cticker(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_cticker, klbuicpp_create_cticker);
}

} // namespace klbui
//end
