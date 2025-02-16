///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    CDrawDC.hpp
/// @brief   CWnd的 绘图函数封装
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_CDRAWDC_HPP__
#define __KLBUI_CDRAWDC_HPP__

#include "klb_type.h"
#include "klbutil/CString.hpp"
#include "klbgui/CGui.hpp"
#include <string>

namespace klbui {


/// @class CDrawDC
/// @brief draw dc 绘图函数封装 基类
///   继承本类, 并扩展 CWnd::DrawOpt* 系列 扩展绘图函数
///   使用扩展类 来做绘图 操作, 完成控件绘制
KLB_EXTERN class KLB_API_CPP CDrawDC
{
public:
    CDrawDC(CWnd* p_wnd);
    virtual ~CDrawDC();

public:
    ////////////////////////////////////////
    // 窗口指针
    CWnd* GetCWnd();

public:
    ////////////////////////////////////////
    // 基础绘图函数
    // 因无法预知, 具体硬件平台支持绘图的能力, 基础绘图函数仅提供弱鸡功能

    int SetDrawColor(uint32_t color);
    uint32_t GetDrawColor();
    int SetFontHeight(int h);
    int GetFontHeight();

    int DrawClear(uint32_t* p_color);
    int DrawPoint(int x, int y, uint32_t* p_color);
    int DrawPoints(const klb_point_t* p_points, int count, uint32_t* p_color);
    int DrawLine(int x1, int y1, int x2, int y2, uint32_t* p_color);
    int DrawLines(const klb_point_t* p_points, int count, uint32_t* p_color);
    int DrawRect(const klb_rect_t* p_rect, uint32_t* p_color);
    int DrawRects(const klb_rect_t* p_rects, int count, uint32_t* p_color);
    int DrawFillRect(const klb_rect_t* p_rect, uint32_t* p_color);
    int DrawFillRects(const klb_rect_t* p_rects, int count, uint32_t* p_color);
    int DrawText(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h);
    int DrawText(const klb_rect_t* p_rect, const std::string& utf8, uint32_t* p_color, int* p_font_h);
    int DrawText(const klb_rect_t* p_rect, const klb::CString& str, uint32_t* p_color, int* p_font_h);
    int TextSize(const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h);
    int TextSize(const std::string& utf8, int* p_font_h, int* p_out_w, int* p_out_h);
    int TextSize(const klb::CString& str, int* p_font_h, int* p_out_w, int* p_out_h);
    int DrawImage(const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect);
    int DrawImage(const klb_rect_t* p_dst_rect, const std::string& path, const klb_rect_t* p_src_rect);
    int DrawImage(const klb_rect_t* p_dst_rect, const klb::CString& path, const klb_rect_t* p_src_rect);
    int ImageSize(const char* p_path, int* p_out_w, int* p_out_h);
    int ImageSize(const std::string& path, int* p_out_w, int* p_out_h);
    int ImageSize(const klb::CString& path, int* p_out_w, int* p_out_h);

    int DrawClear2(uint32_t color);
    int DrawPoint2(int x, int y, uint32_t color);
    int DrawPoints2(const klb_point_t* p_points, int count, uint32_t color);
    int DrawLine2(int x1, int y1, int x2, int y2, uint32_t color);
    int DrawLines2(const klb_point_t* p_points, int count, uint32_t color);
    int DrawRect2(const klb_rect_t* p_rect, uint32_t color);
    int DrawRects2(const klb_rect_t* p_rects, int count, uint32_t color);
    int DrawFillRect2(const klb_rect_t* p_rect, uint32_t color);
    int DrawFillRects2(const klb_rect_t* p_rects, int count, uint32_t color);
    int TextSize2(const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h);
    int TextSize2(const std::string& utf8, int font_h, int* p_out_w, int* p_out_h);
    int TextSize2(const klb::CString& str, int font_h, int* p_out_w, int* p_out_h);
    int DrawText2(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h);
    int DrawText2(const klb_rect_t* p_rect, const std::string& utf8, uint32_t color, int font_h);
    int DrawText2(const klb_rect_t* p_rect, const klb::CString& str, uint32_t color, int font_h);


public:
    ////////////////////////////////////////
    // 扩展绘图函数; 基础绘图函数对于实际项目而言, 基本上不够用
    // 由 子类 完成对 CWnd::DrawOpt* 系列函数的封装

protected:
    CWnd*       m_wnd;          ///< 窗口指针

}; // class CDrawDC

} // namespace klbui

#endif // __KLBUI_CDRAWDC_HPP__
//end
