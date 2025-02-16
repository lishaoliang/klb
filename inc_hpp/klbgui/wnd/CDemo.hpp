///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CDemo.hpp
/// @brief   CDemo控件, demo 示例(勿在正式代码中使用)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CDEMO_H__
#define __KLBUI_CDEMO_H__

#include "klb_type.h"
#include "klbgui/CGui.hpp"

namespace klbui {

KLB_EXTERN class KLB_API_CPP CDemoCss : public CCss
{
public:
    CDemoCss(CGui* p_gui);
    virtual ~CDemoCss();

public:
    ////////////////////////////////////////
    // 继承重写 函数
    virtual void OnDelete();
};


KLB_EXTERN class KLB_API_CPP CDemo : public CWnd
{
public:
    CDemo(CGui* p_gui, int x, int y, int w, int h);
    virtual ~CDemo();

private:
    ////////////////////////////////////////
    // 禁止拷贝
    CDemo& operator=(const CDemo& ths);

public:
    ////////////////////////////////////////
    // 对外提供的接口

    // 获取CSS
    CDemoCss* ToDemoCss();

    // 标题
    klb::CString& GetTitle();

    void SetTitle(const klb::CString& title);
    void SetTitle(const std::string& title);

private:
    ////////////////////////////////////////
    // 内部事件响应等函数

    void OnPaintNormal(CDemoCss* p_css, klb_rect_t* p_rect); // 常规状态绘图
    void OnPaintFocus(CDemoCss* p_css, klb_rect_t* p_rect); // 聚焦状态绘图
    void OnPaintDisable(CDemoCss* p_css, klb_rect_t* p_rect); // 不使能状态绘图


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
    static void OnCssTitle(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out);

    // 初始化 CSS 函数表
    virtual bool InitCssFunctionMap(const std::string& type);


private:
    ////////////////////////////////////////
    // 内部变量
    klb::CString        m_title;    // 标题

}; // CDemo

} // namespace klbui

#endif // __KLBUI_CDEMO_H__
//end
