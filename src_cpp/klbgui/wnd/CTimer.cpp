// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CTimer.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CTimerCss

CTimerCss::CTimerCss(CGui* p_gui) : CCss(p_gui)
{

}

CTimerCss::~CTimerCss()
{

}

/////////////////////////////////////
// 继承重写 函数

void CTimerCss::OnDelete()
{
    delete this;
}

//////////////////////////////////////////////////////////////////////////
// 构造/析构 CTimer

CTimer::CTimer(CGui* p_gui, int x, int y, int w, int h) : CWnd(p_gui, x, y, w, h)
{
    // step1 : set type / tid
    SetType(KLBUI_ctimer);
    SetTID(KLBUI_ctimer_tid);

    // step2. CSS属性
    CTimerCss* p_css = new CTimerCss(p_gui);
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

CTimer::~CTimer()
{

}

////////////////////////////////////////
// 禁止拷贝

CTimer& CTimer::operator=(const CTimer& ths)
{
    return *this; 
};

////////////////////////////////////////
// 对外提供的接口

CTimerCss* CTimer::ToTimerCss()
{
    return dynamic_cast<CTimerCss*>(GetCss());
}

bool CTimer::GetTimer()
{
    return m_enable;
}

void CTimer::SetTimer(bool enable)
{
    // 是否启用 定时器
    m_enable = enable;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

bool CTimer::GetTimerDefault()
{
    return m_enable_default;
}

void CTimer::SetTimerDefault(bool enable)
{
    m_enable_default = enable;
}

int64_t CTimer::GetTimeInterval()
{
    return m_tc_interval;
}

void CTimer::SetTimeInterval(int64_t interval)
{
    // 更新
    m_tc_interval = interval;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

//////////////////////////////////////////////////////////////////////////
// 内部事件响应等函数

void CTimer::OnLoad()
{
    // 页面加载之后, 使用默认值更新
    m_enable = m_enable_default;

    // 更新计时
    m_tick_count = GetCGui()->GetTickCount();
}

void CTimer::OnTicker()
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

    // 调用之后, 关闭
    m_enable = false;
}

//////////////////////////////////////////////////////////////////////////
// 继承重写 函数

// 删除
void CTimer::OnDelete()
{
    CTimerCss* p_css = ToTimerCss();
    p_css->OnDelete();

    delete this;
}

// 绘制
int CTimer::OnPaint()
{
    //CTimerCss* p_css = ToTimerCss();
    //klb_rect_t* p_rect = GetCanvasRect();

    // 无绘图

    return 0;
}

// 本控件 事件响应
int CTimer::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
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

void CTimer::OnCssTimer(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTimer* p_this = dynamic_cast<CTimer*>(p_cwnd);
    CTimerCss* p_css = p_this->ToTimerCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTimer();
    }
    else
    {
        int start = 1;

        bool enable = false;
        if (p_this->CheckCssBool(p_in, start, &enable))
        {
            p_this->SetTimer(enable);

            //p_this->Update();
        }
    }
}

void CTimer::OnCssTimerDefault(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTimer* p_this = dynamic_cast<CTimer*>(p_cwnd);
    CTimerCss* p_css = p_this->ToTimerCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_this->GetTimerDefault();
    }
    else
    {
        int start = 1;

        bool enable = false;
        if (p_this->CheckCssBool(p_in, start, &enable))
        {
            p_this->SetTimerDefault(enable);

            //p_this->Update();
        }
    }
}

void CTimer::OnCssTimeInterval(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CTimer* p_this = dynamic_cast<CTimer*>(p_cwnd);
    CTimerCss* p_css = p_this->ToTimerCss();

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

bool CTimer::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first; // 不是第一次, 直接退出
    }

    // 绑定响应函数
    // 

    // 私有函数
    BindCssFunction("timer", CTimer::OnCssTimer);                   // 是否启用 定时器
    BindCssFunction("timer_default", CTimer::OnCssTimerDefault);    // 初始默认值

    BindCssFunction("time_interval", CTimer::OnCssTimeInterval);    // 设置定时器事件间隔

    return is_first;
}

//////////////////////////////////////////////////////////////////////////
// 创建/注册

static klb_wnd_t* klbuicpp_create_ctimer(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);

    // step2 : new
    CTimer* p_ticker = new CTimer(p_cppgui, x, y, w, h);

    // step3 : 初始化CSS 支持的方法
    p_ticker->InitCssFunctionMap(KLBUI_ctimer);

    return p_ticker->GetWnd();
}

// 注册类型: "ctimer" (class timer, CTimer)
int klbui_register_ctimer(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CPPGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_ctimer, klbuicpp_create_ctimer);
}

} // namespace klbui
//end
