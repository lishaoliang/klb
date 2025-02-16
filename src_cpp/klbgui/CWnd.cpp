// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CWnd.hpp"
#include "klbgui/CGui.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>


using namespace klb;


namespace klbui {

//////////////////////////////////////////////////////////////////////////
// 内部结构体


/// @struct klb_cwnd_t
/// @brief  CWnd依赖的内部结构体 : [klb_wnd_t] + [klb_cwnd_t]
typedef struct klb_cwnd_t_
{
    CWnd*       p_cwnd;
}klb_cwnd_t;


///////////////////////////////////////////
static void klb_cwnd_quit(klb_wnd_t* p_wnd);


///////////////////////////////////////////
// 继承重写方法

static void klb_cwnd_destroy(klb_wnd_t* p_wnd)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    klb_cwnd_quit(p_wnd);

    KLB_FREE(p_wnd);
}


static int klb_cwnd_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    return p_cwnd->p_cwnd->OnControl(msg, p_pt1, p_pt2, lparam, wparam);
}

static int klb_cwnd_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    return p_cwnd->p_cwnd->OnCommand(msg, p_pt1, p_pt2, lparam, wparam);
}

static int klb_cwnd_on_paint(klb_wnd_t* p_wnd)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    return p_cwnd->p_cwnd->OnPaint();
}

static int klb_cwnd_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    return p_cwnd->p_cwnd->OnSet(p_map);
}

static klb_map_t* klb_cwnd_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    return p_cwnd->p_cwnd->OnGet(p_map);
}

///////////////////////////////////////////
// init / quit

static void klb_cwnd_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h, CWnd* p_cwnd)
{
    klb_cwnd_t* ptr = (klb_cwnd_t*)p_wnd->ctrl;
    ptr->p_cwnd = p_cwnd;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klb_cwnd_destroy;       // 由继承者重写
    p_wnd->vtable.on_control = klb_cwnd_on_control; // 这里指定
    p_wnd->vtable.on_command = klb_cwnd_on_command; // 这里指定
    p_wnd->vtable.on_paint = klb_cwnd_on_paint;     // 指定 on paint
    p_wnd->vtable.on_set = klb_cwnd_on_set;         // 这里指定
    p_wnd->vtable.on_get = klb_cwnd_on_get;         // 这里指定

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;
}

static void klb_cwnd_quit(klb_wnd_t* p_wnd)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->ctrl;

    // 窗口的创建可以能处于其他 动态库,
    // 这里 不能直接 delete, 需要使用 对象 提供专门的释放内存接口
    p_cwnd->p_cwnd->OnDelete();
}

//////////////////////////////////////
// create

static klb_wnd_t* klb_cwnd_create(klb_gui_t* p_gui, int x, int y, int w, int h, CWnd* p_cwnd)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klb_cwnd_t));

    klb_cwnd_init(p_wnd, p_gui, x, y, w, h, p_cwnd);

    return p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// 构造/析构

CWnd::CWnd(CGui* p_gui, int x, int y, int w, int h)
{
    // 
    m_type = "";

    m_tid = 0;
    m_css = NULL;
    m_cb_on_command = NULL;

    m_gui = NULL;
    m_wnd = NULL;
    m_css_func_map = NULL;

    // 初始化
    m_gui = p_gui;

    m_wnd = klb_cwnd_create(p_gui->GetGui(), x, y, w, h, this);
}

CWnd::~CWnd()
{
    m_type = "";

    m_tid = 0;
    m_css = NULL;
    m_cb_on_command = NULL;

    m_gui = NULL;
    m_wnd = NULL;
    m_css_func_map = NULL;
}

//////////////////////////////////////////////////////////////////////////
// cpp 额外函数

klb_wnd_t* CWnd::GetWnd()
{
    return m_wnd;
}

void CWnd::SetType(const std::string& type)
{
    m_type = type;
}

const klb::CString& CWnd::GetType()
{
    return m_type;
}

void CWnd::SetTID(int tid)
{
    m_tid = tid;
}

int CWnd::GetTID()
{
    return m_tid;
}

void CWnd::SetCss(CCss* p_css)
{
    m_css = p_css;
}

CCss* CWnd::GetCss()
{
    return m_css;
}

//////////////////////////////////////////////////////////////////////////

CWnd* CWnd::ToCWnd(klb_wnd_t* p_wnd)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd;

    return p_cwnd->p_cwnd;
}

void* CWnd::GetUserData(klb_wnd_t* p_wnd)
{
    return p_wnd->p_udata;
}

CWnd* CWnd::GetUserDataCWnd(klb_wnd_t* p_wnd)
{
    klb_cwnd_t* p_cwnd = (klb_cwnd_t*)p_wnd->p_udata;
    return p_cwnd->p_cwnd;
}

//////////////////////////////////////////////////////////////////////////
// 定义于 "klbgui/klb_wnd.h" 的导出函数


////////////////////////////////////
// 通用

klb_gui_t* CWnd::GetGui()
{
    return m_gui->GetGui();
}

void CWnd::GetGui(CGui** p_gui)
{
    *p_gui = m_gui;
}

CGui* CWnd::GetCGui()
{
    return m_gui;
}

klb_canvas_t* CWnd::GetCanvas()
{
    return klb_wnd_get_canvas(m_wnd);
}

////////////////////////////////////
// 样式

uint32_t CWnd::GetStyle()
{
    return klb_wnd_get_style(m_wnd);
}

void CWnd::SetStyle(uint32_t style)
{
    return klb_wnd_set_style(m_wnd, style);
}

////////////////////////////////////
// 状态

uint32_t CWnd::GetStatus()
{
    return m_wnd->state.status;
}

bool CWnd::IsFocus()
{
    return (KLB_WND_STATUS_FOCUS & m_wnd->state.status) ? false : true;
}

void CWnd::Show(bool show)
{
    klb_wnd_show(m_wnd, show);
}

bool CWnd::IsShow()
{
    return klb_wnd_is_show(m_wnd);
}

void CWnd::Hide(bool hide)
{
    klb_wnd_hide(m_wnd, hide);
}

bool CWnd::IsHide()
{
    return klb_wnd_is_hide(m_wnd);
}

void CWnd::Input(bool input)
{
    klb_wnd_input(m_wnd, input);
}

bool CWnd::IsInput()
{
    return klb_wnd_is_input(m_wnd);
}

void CWnd::Check(bool check)
{
    klb_wnd_check(m_wnd, check);
}


bool CWnd::IsCheck()
{
    return klb_wnd_is_check(m_wnd);
}

void CWnd::Disable(bool disable)
{
    klb_wnd_disable(m_wnd, disable);
}

bool CWnd::IsDisable()
{
    return klb_wnd_is_disable(m_wnd);
}

void CWnd::Enable(bool enable)
{
    klb_wnd_enable(m_wnd, enable);
}

bool CWnd::IsEnable()
{
    return klb_wnd_is_enable(m_wnd);
}

bool CWnd::IsTopMost()
{
    return klb_wnd_is_topmost(m_wnd);
}

void CWnd::DynTip(bool update)
{
    klb_wnd_dyntip(m_wnd, update);
}

bool CWnd::IsDynTip()
{
    return klb_wnd_is_dyntip(m_wnd);
}

////////////////////////////////////
// tip

/// @brief 设置,获取 tip
void CWnd::SetTip(const char* p_tip)
{
    klb_wnd_set_tip(m_wnd, p_tip);
}

void CWnd::SetTip(const std::string& tip)
{
    klb_wnd_set_tip(m_wnd, tip.c_str());
}

void CWnd::SetTip(const klb::CString& tip)
{
    klb_wnd_set_tip(m_wnd, tip.c_str());
}

void CWnd::GetTip(std::string& tip)
{
    const sds str = klb_wnd_get_tip(m_wnd);

    tip = (NULL != str) ? str : "";
}

void CWnd::GetTip(klb::CString& tip)
{
    tip = klb_wnd_get_tip(m_wnd);
}

void CWnd::SetTipDynamic(const char* p_tip)
{
    klb_wnd_set_tip_dynamic(m_wnd, p_tip);
}

void CWnd::SetTipDynamic(const std::string& tip)
{
    klb_wnd_set_tip_dynamic(m_wnd, tip.c_str());
}

void CWnd::SetTipDynamic(const klb::CString& tip)
{
    klb_wnd_set_tip_dynamic(m_wnd, tip.c_str());
}

void CWnd::GetTipDynamic(std::string& tip)
{
    const sds str = klb_wnd_get_tip_dynamic(m_wnd);

    tip = (NULL != str) ? str : "";
}

void CWnd::GetTipDynamic(klb::CString& tip)
{
    tip = klb_wnd_get_tip_dynamic(m_wnd);
}

void CWnd::TipUpdate()
{
    klb_wnd_tip_update(m_wnd);
}


////////////////////////////////////
// 窗口位置/大小

void CWnd::Move(int x, int y)
{
    klb_wnd_move(m_wnd, x, y);
}

/// @brief 重新设置控件大小
void CWnd::Resize(int w, int h)
{
    klb_wnd_resize(m_wnd, w, h);
}

////////////////////////////////////
// 刷新


void CWnd::Update()
{
    klb_wnd_update(m_wnd);
}

void CWnd::UpdateCanvasRect()
{
    klb_wnd_update_canvas_rect(m_wnd);
}


////////////////////////////////////
// 绑定外部回调函数


int CWnd::BindCommand(klb_wnd_on_command_cb on_command, void* p_obj)
{
    return klb_wnd_bind_command(m_wnd, on_command, p_obj);
}

int CWnd::BindCommand(klb_cwnd_on_command_cb on_command, void* p_obj)
{
    m_cb_on_command = on_command;

    return klb_wnd_bind_command(m_wnd, CWnd::on_command_klb_cwnd, p_obj);
}

////////////////////////////////////
// 调用函数


int CWnd::CallControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_wnd_call_control(m_wnd, msg, p_pt1, p_pt2, lparam, wparam);
}


int CWnd::CallCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_wnd_call_command(m_wnd, msg, p_pt1, p_pt2, lparam, wparam);
}


int CWnd::CallControlAndCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return klb_wnd_call_control_and_command(m_wnd, msg, p_pt1, p_pt2, lparam, wparam);
}

////////////////////////////////////
// 控件建议宽/高

int CWnd::SuggestW()
{
    return klb_wnd_suggestw(m_wnd);
}

int CWnd::SuggestH()
{
    return klb_wnd_suggesth(m_wnd);
}


////////////////////////////////////
// 窗口关系 

int CWnd::PushChild(CWnd* p_child)
{
    assert(NULL != m_gui);

    return klb_wnd_push_child(m_wnd, p_child->GetWnd());
}


////////////////////////////////////
// 设置 / 获取


int CWnd::Set(const klb_map_t* p_map)
{
    return klb_wnd_set(m_wnd, p_map);
}

klb_map_t* CWnd::Get(const klb_map_t* p_map)
{
    return klb_wnd_get(m_wnd, p_map);
}

////////////////////////////////////
// 时间

/// @brief 获取当前UI滴答数
int64_t CWnd::GetTickCount()
{
    return klb_wnd_get_tick_count(m_wnd);
}

////////////////////////////////////
// 绘图

// 获取画布区域
klb_rect_t* CWnd::GetCanvasRect()
{
    return &m_wnd->pos.rect_in_canvas;
}

// 获取父窗口区域
klb_rect_t* CWnd::GetParentRect()
{
    return &m_wnd->pos.rect_in_parent;
}

/// @brief 基础绘图接口
int CWnd::SetDrawColor(uint32_t color)
{
    return klb_wnd_set_draw_color(m_wnd, color);
}

uint32_t CWnd::GetDrawColor()
{
    return klb_wnd_get_draw_color(m_wnd);
}

int CWnd::SetFontHeight(int h)
{
    return klb_wnd_set_font_height(m_wnd, h);
}

int CWnd::GetFontHeight()
{
    return klb_wnd_get_font_height(m_wnd);
}

int CWnd::DrawClear(uint32_t* p_color)
{
    return klb_wnd_draw_clear(m_wnd, p_color);
}

int CWnd::DrawPoint(int x, int y, uint32_t* p_color)
{
    return klb_wnd_draw_point(m_wnd, x, y, p_color);
}

int CWnd::DrawPoints(const klb_point_t* p_points, int count, uint32_t* p_color)
{
    return klb_wnd_draw_points(m_wnd, p_points, count, p_color);
}

int CWnd::DrawLine(int x1, int y1, int x2, int y2, uint32_t* p_color)
{
    return klb_wnd_draw_line(m_wnd, x1, y2, x2, y2, p_color);
}

int CWnd::DrawLines(const klb_point_t* p_points, int count, uint32_t* p_color)
{
    return klb_wnd_draw_lines(m_wnd, p_points, count, p_color);
}

int CWnd::DrawRect(const klb_rect_t* p_rect, uint32_t* p_color)
{
    return klb_wnd_draw_rect(m_wnd, p_rect, p_color);
}

int CWnd::DrawRects(const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    return klb_wnd_draw_rects(m_wnd, p_rects, count, p_color);
}

int CWnd::DrawFillRect(const klb_rect_t* p_rect, uint32_t* p_color)
{
    return klb_wnd_draw_fill_rect(m_wnd, p_rect, p_color);
}

int CWnd::DrawFillRects(const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    return klb_wnd_draw_fill_rects(m_wnd, p_rects, count, p_color);
}

int CWnd::DrawText(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h)
{
    return klb_wnd_draw_text(m_wnd, p_rect, p_utf8, utf8_len, p_color, p_font_h);
}

int CWnd::DrawText(const klb_rect_t* p_rect, const std::string& utf8, uint32_t* p_color, int* p_font_h)
{
    return klb_wnd_draw_text(m_wnd, p_rect, utf8.c_str(), utf8.length(), p_color, p_font_h);
}

int CWnd::DrawText(const klb_rect_t* p_rect, const klb::CString& str, uint32_t* p_color, int* p_font_h)
{
    return klb_wnd_draw_text(m_wnd, p_rect, str.c_str(), (int)(str.Length()), p_color, p_font_h);
}

int CWnd::TextSize(const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size(m_wnd, p_utf8, utf8_len, p_font_h, p_out_w, p_out_h);
}

int CWnd::TextSize(const std::string& utf8, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size(m_wnd, utf8.c_str(), utf8.length(), p_font_h, p_out_w, p_out_h);
}

int CWnd::TextSize(const klb::CString& str, int* p_font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size(m_wnd, str.c_str(), str.Length(), p_font_h, p_out_w, p_out_h);
}

int CWnd::DrawImage(const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    return klb_wnd_draw_image(m_wnd, p_dst_rect, p_path, p_src_rect);
}

int CWnd::DrawImage(const klb_rect_t* p_dst_rect, const std::string& path, const klb_rect_t* p_src_rect)
{
    return klb_wnd_draw_image(m_wnd, p_dst_rect, path.c_str(), p_src_rect);
}

int CWnd::DrawImage(const klb_rect_t* p_dst_rect, const klb::CString& path, const klb_rect_t* p_src_rect)
{
    return klb_wnd_draw_image(m_wnd, p_dst_rect, path.c_str(), p_src_rect);
}

int CWnd::ImageSize(const char* p_path, int* p_out_w, int* p_out_h)
{
    return klb_wnd_image_size(m_wnd, p_path, p_out_w, p_out_h);
}

int CWnd::ImageSize(const std::string& path, int* p_out_w, int* p_out_h)
{
    return klb_wnd_image_size(m_wnd, path.c_str(), p_out_w, p_out_h);
}

int CWnd::ImageSize(const klb::CString& path, int* p_out_w, int* p_out_h)
{
    return klb_wnd_image_size(m_wnd, path.c_str(), p_out_w, p_out_h);
}

int CWnd::DrawClear2(uint32_t color)
{
    return klb_wnd_draw_clear2(m_wnd, color);
}

int CWnd::DrawPoint2(int x, int y, uint32_t color)
{
    return klb_wnd_draw_point2(m_wnd, x, y, color);
}

int CWnd::DrawPoints2(const klb_point_t* p_points, int count, uint32_t color)
{
    return klb_wnd_draw_points2(m_wnd, p_points, count, color);

}
int CWnd::DrawLine2(int x1, int y1, int x2, int y2, uint32_t color)
{
    return klb_wnd_draw_line2(m_wnd, x1, y1, x2, y2, color);
}

int CWnd::DrawLines2(const klb_point_t* p_points, int count, uint32_t color)
{
    return klb_wnd_draw_lines2(m_wnd, p_points, count, color);
}

int CWnd::DrawRect2(const klb_rect_t* p_rect, uint32_t color)
{
    return klb_wnd_draw_rect2(m_wnd, p_rect, color);
}

int CWnd::DrawRects2(const klb_rect_t* p_rects, int count, uint32_t color)
{
    return klb_wnd_draw_rects2(m_wnd, p_rects, count, color);
}

int CWnd::DrawFillRect2(const klb_rect_t* p_rect, uint32_t color)
{
    return klb_wnd_draw_fill_rect2(m_wnd, p_rect, color);
}

int CWnd::DrawFillRects2(const klb_rect_t* p_rects, int count, uint32_t color)
{
    return klb_wnd_draw_fill_rects2(m_wnd, p_rects, count, color);
}

int CWnd::TextSize2(const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size2(m_wnd, p_utf8, utf8_len, font_h, p_out_w, p_out_h);
}

int CWnd::TextSize2(const std::string& utf8, int font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size2(m_wnd, utf8.c_str(), utf8.length(), font_h, p_out_w, p_out_h);
}

int CWnd::TextSize2(const klb::CString& str, int font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size2(m_wnd, str.c_str(), str.Length(), font_h, p_out_w, p_out_h);
}

int CWnd::DrawText2(const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
{
    return klb_wnd_draw_text2(m_wnd, p_rect, p_utf8, utf8_len, color, font_h);
}

int CWnd::DrawText2(const klb_rect_t* p_rect, const std::string& utf8, uint32_t color, int font_h)
{
    return klb_wnd_draw_text2(m_wnd, p_rect, utf8.c_str(), utf8.length(), color, font_h);
}

int CWnd::DrawText2(const klb_rect_t* p_rect, const klb::CString& str, uint32_t color, int font_h)
{
    return klb_wnd_draw_text2(m_wnd, p_rect, str.c_str(), str.Length(), color, font_h);
}

/// @brief 可扩展绘图接口
int CWnd::DrawOpt1(int opt, const void* ptr1)
{
    return klb_wnd_draw_opt1(m_wnd, opt, ptr1);
}

int CWnd::DrawOpt2(int opt, const void* ptr1, const void* ptr2)
{
    return klb_wnd_draw_opt2(m_wnd, opt, ptr1, ptr2);
}

int CWnd::DrawOpt3(int opt, const void* ptr1, const void* ptr2, const void* ptr3)
{
    return klb_wnd_draw_opt3(m_wnd, opt, ptr1, ptr2, ptr3);
}

int CWnd::DrawOpt4(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4)
{
    return klb_wnd_draw_opt4(m_wnd, opt, ptr1, ptr2, ptr3, ptr4);
}

int CWnd::DrawOpt5(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5)
{
    return klb_wnd_draw_opt5(m_wnd, opt, ptr1, ptr2, ptr3, ptr4, ptr5);
}

int CWnd::DrawOpt6(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6)
{
    return klb_wnd_draw_opt6(m_wnd, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6);
}

int CWnd::DrawOpt7(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7)
{
    return klb_wnd_draw_opt7(m_wnd, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7);
}

int CWnd::DrawOpt8(int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6, const void* ptr7, const void* ptr8)
{
    return klb_wnd_draw_opt8(m_wnd, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8);
}


//////////////////////////////////////////////////////////////////////////
// 

void CWnd::OnDelete()
{

}

int CWnd::OnPaint()
{
    return 0;
}

int CWnd::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}

int CWnd::OnCommand(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}


/// @brief C标准 CSS处理函数
typedef void(*klb_cwnd_std_function_cb)(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out);


int CWnd::OnSet(const klb_map_t* p_map)
{   
    const char* p_key = klb_map_idx_to_string(p_map, 0);

    // 注意 C 指针绑定在 第一个位置, CPP指针绑定在第二个指针
    klb_cwnd_css_cb cb_cpp = NULL;
    klb_cwnd_std_function_cb cb_std = (NULL != m_css_func_map) ? (klb_cwnd_std_function_cb)klb_map_to_ptr(m_css_func_map, p_key, (const void**)&cb_cpp) : NULL;

    if (cb_cpp)
    {
        // 优先处理 CPP 绑定的CSS函数
        klb::CMap in((klb_map_t*)p_map, true);

        cb_cpp(this, KLBUI_CSSEX_set, &in, NULL);
    }
    else if(cb_std)
    {
        // C 标准 绑定的CSS函数
        cb_std(m_wnd, NULL, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

klb_map_t* CWnd::OnGet(const klb_map_t* p_map)
{
    klb_map_t* p_out = klb_map_create();
    const char* p_key = klb_map_idx_to_string(p_map, 0);

    // 注意 C 指针绑定在 第一个位置, CPP指针绑定在第二个指针
    klb_cwnd_css_cb cb_cpp = NULL;
    klb_cwnd_std_function_cb cb_std = (NULL != m_css_func_map) ? (klb_cwnd_std_function_cb)klb_map_to_ptr(m_css_func_map, p_key, (const void**)&cb_cpp) : NULL;

    if (cb_cpp)
    {
        // 优先处理 CPP 绑定的CSS函数
        klb::CMap in((klb_map_t*)p_map, true);
        klb::CMap out(p_out, true);

        cb_cpp(this, KLBUI_CSSEX_get, &in, &out);
    }
    else if (cb_std)
    {
        // C 标准 绑定的CSS函数
        cb_std(m_wnd, NULL, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

int CWnd::on_command_klb_cwnd(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    CWnd* p_this = CWnd::ToCWnd(p_wnd);

    klb_cwnd_on_command_cb cb = p_this->m_cb_on_command;

    if (NULL != cb)
    {
        return cb(p_this, msg, p_pt1, p_pt2, lparam, wparam);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
} // namespace klbui
//end
