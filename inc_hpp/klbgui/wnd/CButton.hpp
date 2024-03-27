///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2024, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CButton.hpp
/// @brief   CButton控件
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_CBUTTON_HPP__
#define __KLB_CBUTTON_HPP__

#include "klb_type.h"
#include "klbgui/CGui.hpp"
#include "klbgui/CWnd.hpp"
#include "klbgui/CCss.hpp"
#include <string>

namespace klbui {

class CButtonCss : public CCss
{
public:
    CButtonCss();
    virtual ~CButtonCss();
};

KLB_EXTERN class KLB_API_CPP CButton : public CWnd
{
public:
    CButton();
    virtual ~CButton();

public:


public:
    CButtonCss* ToButtonCss();

public:
    //////////////////////////////////////////////////////////////////////////
    // 继承函数

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


}; // CButton

} // klbui

#endif // __KLB_CBUTTON_HPP__
//end
