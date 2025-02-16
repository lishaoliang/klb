///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CTimer.hpp
/// @brief   CTimer控件, 单次定时器; 启用之后只运行一次
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CTIMER_H__
#define __KLBUI_CTIMER_H__

#include "klb_type.h"
#include "klbgui/CGui.hpp"

namespace klbui {

KLB_EXTERN class KLB_API_CPP CTimerCss : public CCss
{
public:
    CTimerCss(CGui* p_gui);
    virtual ~CTimerCss();

public:
    ////////////////////////////////////////
    // 继承重写 函数
    virtual void OnDelete();
};


KLB_EXTERN class KLB_API_CPP CTimer : public CWnd
{
public:
    CTimer(CGui* p_gui, int x, int y, int w, int h);
    virtual ~CTimer();

private:
    ////////////////////////////////////////
    // 禁止拷贝
    CTimer& operator=(const CTimer& ths);

public:
    ////////////////////////////////////////
    // 对外提供的接口

    // 获取CSS
    CTimerCss* ToTimerCss();

    // 获取/设置 是否启用 timer
    bool GetTimer();
    void SetTimer(bool enable);

    // 获取/设置 默认值
    bool GetTimerDefault();
    void SetTimerDefault(bool enable);

    // 获取/设置 时间间隔
    int64_t GetTimeInterval();
    void SetTimeInterval(int64_t interval);

private:
    ////////////////////////////////////////
    // 内部事件响应等函数

    // KLBUI_onload 事件响应
    void OnLoad();

    // KLBUI_onticker 事件响应
    void OnTicker();

public:
    ////////////////////////////////////////
    // 继承重写 函数

    /// @brief 销毁内存
    virtual void OnDelete();

    /// @brief 窗口绘制函数
    /// @return int
    virtual int OnPaint();

    /// @brief 消息控制函数
    /// @param [in] msg         消息命令
    /// @param [in] *p_p1       点1
    /// @param [in] *p_p2       点2
    /// @return int 0
    virtual int OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);

public:
    ////////////////////////////////////////
    // CSS 函数
    static void OnCssTimer(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssTimerDefault(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);
    static void OnCssTimeInterval(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 初始化 CSS 函数表
    virtual bool InitCssFunctionMap(const std::string& type);


private:
    ////////////////////////////////////////
    // 内部变量
    bool        m_enable;           // 是否启用定时器
    bool        m_enable_default;   // 定时器的默认值
    int64_t     m_tick_count;       // 计时(单位毫秒ms)
    int64_t     m_tc_interval;      // 事件响应间隔(单位毫秒ms)

}; // CTimer

} // namespace klbui

#endif // __KLBUI_CTIMER_H__
//end
