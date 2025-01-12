// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CButton.hpp"
#include "klbgui/CUiWidgets.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////

CButtonCss::CButtonCss()
{

}

CButtonCss::~CButtonCss()
{

}

//////////////////////////////////////////////////////////////////////////

CButton::CButton()
{

}

CButton::~CButton()
{

}

//////////////////////////////////////////////////////////////////////////

CButtonCss* CButton::ToButtonCss()
{
    return dynamic_cast<CButtonCss*>(GetCss());
}

//////////////////////////////////////////////////////////////////////////
// 继承函数


void CButton::OnDelete()
{

}

int CButton::OnPaint()
{
    //CButtonCss* p_css = ToButtonCss();

    //if (NULL == p_css)
    //{
    //    return 0;
    //}

    //DrawLine2(0, 0, 100, 100, p_css->GetColor());

    klb_rect_t* p_rect = GetCanvasRect();

    DrawFillRect2(p_rect, KLB_ARGB8888(255, 180, 180, 180));
    DrawRect2(p_rect, KLB_ARGB8888(255, 80, 80, 80));


    return 0;
}

int CButton::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////

void CButton::OnColor(CWnd* p_cwnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    CButton* p_this = dynamic_cast<CButton*>(p_cwnd);
    CButtonCss* p_css = p_this->ToButtonCss();

    if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(NULL, p_in, start, &color))
        {
            p_css->SetColor(color);

            p_this->Update();
        }
    }
}

bool CButton::InitCssFunctionMap(const std::string& type)
{
    bool is_first = CWnd::InitCssFunctionMap(type);

    if (!is_first)
    {
        return is_first;
    }

    // 绑定响应函数

    return is_first;
}

//////////////////////////////////////////////////////////////////////////

klb_wnd_t* CButton::CreateCButton(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : cpp gui
    CGui* p_cppgui = KLB_CGUI_PTR(p_gui);

    // step2 : new
    CButton* p_button = new CButton();

    // step3 : Init
    p_button->Init(p_cppgui, x, y, w, h);

    // step4 : set type
    p_button->SetType(KLBUI_cbutton);

    // step5 : 初始化CSS 支持的方法
    p_button->InitCssFunctionMap(KLBUI_cbutton);

    // step6. 默认使用公用全局CSS属性
    //CButtonCss* p_globalcss = (CButtonCss*)p_cppgui->GlobalCssMap(KLBUI_cbutton);
    //p_button->SetCss(p_globalcss);

    return p_button->GetWnd();
}

int klbui_register_cbutton(klb_gui_t* p_gui)
{
    CGui* p_cppgui = KLB_CGUI_PTR(p_gui);
    return p_cppgui->Register(KLBUI_cbutton, CButton::CreateCButton);
}

} // klbui
