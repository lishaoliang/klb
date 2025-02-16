///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CLabel.hpp
/// @brief   CLabel控件, 静态文本框
///   参考: https://learn.microsoft.com/zh-cn/dotnet/api/system.windows.controls.label?view=windowsdesktop-7.0
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CLABEL_H__
#define __KLBUI_CLABEL_H__

#include "klb_type.h"
#include "klbgui/CGui.hpp"

namespace klbui {

KLB_EXTERN class KLB_API_CPP CLabelCss : public CCss
{
public:
    CLabelCss(CGui* p_gui);
    virtual ~CLabelCss();

public:
    ////////////////////////////////////////
    // 继承重写 函数
    virtual void OnDelete();
};


KLB_EXTERN class KLB_API_CPP CLabel : public CWnd
{
public:
    CLabel(CGui* p_gui, int x, int y, int w, int h);
    virtual ~CLabel();

private:
    ////////////////////////////////////////
    // 禁止拷贝
    CLabel& operator=(const CLabel& ths);

public:
    ////////////////////////////////////////
    // 对外提供的接口

    // 获取CSS
    CLabelCss* ToLabelCss();

    // 标题
    klb::CString& GetTitle();

    void SetTitle(const klb::CString& title);
    void SetTitle(const std::string& title);

private:
    ////////////////////////////////////////
    // 内部事件响应等函数

    void OnPaintNormal(CLabelCss* p_css, klb_rect_t* p_rect); // 常规状态绘图

    void OnFocusDelay(CLabelCss* p_css, klb_rect_t* p_rect);

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

}; // CLabel

} // namespace klbui

#endif // __KLBUI_CLABEL_H__
//end
