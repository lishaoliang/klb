// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CDrawDC.hpp"
#include "klbgui/CGui.hpp"
#include <assert.h>


using namespace klb;


namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 构造/析构

CDrawDC::CDrawDC(CWnd* p_wnd)
{
    assert(NULL != p_wnd);

    m_wnd = p_wnd;
}

CDrawDC::~CDrawDC()
{
    m_wnd = NULL;
}

//////////////////////////////////////

CWnd* CDrawDC::GetCWnd()
{
    return m_wnd;
}

//////////////////////////////////////
// 基础绘图函数

int CDrawDC::SetDrawColor(uint32_t color)
{
    return m_wnd->SetDrawColor(color);
}

uint32_t CDrawDC::GetDrawColor()
{
    return m_wnd->GetDrawColor();
}

int CDrawDC::SetFontHeight(int h)
{
    return m_wnd->SetFontHeight(h);
}

int CDrawDC::GetFontHeight()
{
    return m_wnd->GetFontHeight();
}

int CDrawDC::DrawClear(uint32_t* p_color)
{
    return m_wnd->DrawClear(p_color);
}

int CDrawDC::DrawPoint(int x, int y, uint32_t* p_color)
{
    return m_wnd->DrawPoint(x, y, p_color);
}

int CDrawDC::DrawPoints(const klb_point_t* p_points, int count, uint32_t* p_color)
{
    return m_wnd->DrawPoints(p_points, count, p_color);
}

int CDrawDC::DrawLine(int x1, int y1, int x2, int y2, uint32_t* p_color)
{
    return m_wnd->DrawLine(x1, y1, x2, y2, p_color);
}

int CDrawDC::DrawLines(const klb_point_t* p_points, int count, uint32_t* p_color)
{
    return m_wnd->DrawLines(p_points, count, p_color);
}

int CDrawDC::DrawRect(const klb_rect_t* p_rect, uint32_t* p_color)
{
    return m_wnd->DrawRect(p_rect, p_color);
}

int CDrawDC::DrawRects(const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    return m_wnd->DrawRects(p_rects, count, p_color);
}

int CDrawDC::DrawFillRect(const klb_rect_t* p_rect, uint32_t* p_color)
{
    return m_wnd->DrawFillRect(p_rect, p_color);
}

int CDrawDC::DrawFillRects(const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    return m_wnd->DrawFillRects(p_rects, count, p_color);
}

int CDrawDC::DrawText(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h)
{
    return m_wnd->DrawText(p_rect, p_utf8, utf8_len, p_color, p_font_h);
}

int CDrawDC::DrawText(const klb_rect_t* p_rect, const std::string& utf8, uint32_t* p_color, int* p_font_h)
{
    return m_wnd->DrawText(p_rect, utf8, p_color, p_font_h);
}

int CDrawDC::DrawText(const klb_rect_t* p_rect, const klb::CString& str, uint32_t* p_color, int* p_font_h)
{
    return m_wnd->DrawText(p_rect, str, p_color, p_font_h);
}

int CDrawDC::TextSize(const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize(p_utf8, utf8_len, p_font_h, p_out_w, p_out_h);
}

int CDrawDC::TextSize(const std::string& utf8, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize(utf8, p_font_h, p_out_w, p_out_h);
}

int CDrawDC::TextSize(const klb::CString& str, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize(str, p_font_h, p_out_w, p_out_h);
}

int CDrawDC::DrawImage(const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    return m_wnd->DrawImage(p_dst_rect, p_path, p_src_rect);
}

int CDrawDC::DrawImage(const klb_rect_t* p_dst_rect, const std::string& path, const klb_rect_t* p_src_rect)
{
    return m_wnd->DrawImage(p_dst_rect, path, p_src_rect);
}

int CDrawDC::DrawImage(const klb_rect_t* p_dst_rect, const klb::CString& path, const klb_rect_t* p_src_rect)
{
    return m_wnd->DrawImage(p_dst_rect, path, p_src_rect);
}

int CDrawDC::ImageSize(const char* p_path, int* p_out_w, int* p_out_h)
{
    return m_wnd->ImageSize(p_path, p_out_w, p_out_h);
}

int CDrawDC::ImageSize(const std::string& path, int* p_out_w, int* p_out_h)
{
    return m_wnd->ImageSize(path, p_out_w, p_out_h);
}

int CDrawDC::ImageSize(const klb::CString& path, int* p_out_w, int* p_out_h)
{
    return m_wnd->ImageSize(path, p_out_w, p_out_h);
}

int CDrawDC::DrawClear2(uint32_t color)
{
    return m_wnd->DrawClear2(color);
}

int CDrawDC::DrawPoint2(int x, int y, uint32_t color)
{
    return m_wnd->DrawPoint2(x, y, color);
}

int CDrawDC::DrawPoints2(const klb_point_t* p_points, int count, uint32_t color)
{
    return m_wnd->DrawPoints2(p_points, count, color);
}

int CDrawDC::DrawLine2(int x1, int y1, int x2, int y2, uint32_t color)
{
    return m_wnd->DrawLine2(x1, y1, x2, y2, color);
}

int CDrawDC::DrawLines2(const klb_point_t* p_points, int count, uint32_t color)
{
    return m_wnd->DrawLines2(p_points, count, color);
}

int CDrawDC::DrawRect2(const klb_rect_t* p_rect, uint32_t color)
{
    return m_wnd->DrawRect2(p_rect, color);
}

int CDrawDC::DrawRects2(const klb_rect_t* p_rects, int count, uint32_t color)
{
    return m_wnd->DrawRects2(p_rects, count, color);
}

int CDrawDC::DrawFillRect2(const klb_rect_t* p_rect, uint32_t color)
{
    return m_wnd->DrawFillRect2(p_rect, color);
}

int CDrawDC::DrawFillRects2(const klb_rect_t* p_rects, int count, uint32_t color)
{
    return m_wnd->DrawFillRects2(p_rects, count, color);
}

int CDrawDC::TextSize2(const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize2(p_utf8, utf8_len, font_h, p_out_w, p_out_h);
}

int CDrawDC::TextSize2(const std::string& utf8, int font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize2(utf8, font_h, p_out_w, p_out_h);
}

int CDrawDC::TextSize2(const klb::CString& str, int font_h, int* p_out_w, int* p_out_h)
{
    return m_wnd->TextSize2(str, font_h, p_out_w, p_out_h);
}

int CDrawDC::DrawText2(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
{
    return m_wnd->DrawText2(p_rect, p_utf8, utf8_len, color, font_h);
}

int CDrawDC::DrawText2(const klb_rect_t* p_rect, const std::string& utf8, uint32_t color, int font_h)
{
    return m_wnd->DrawText2(p_rect, utf8, color, font_h);
}

int CDrawDC::DrawText2(const klb_rect_t* p_rect, const klb::CString& str, uint32_t color, int font_h)
{
    return m_wnd->DrawText2(p_rect, str, color, font_h);
}

//////////////////////////////////////////////////////////////////////////
} // namespace klbui
//end
