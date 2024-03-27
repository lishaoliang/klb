// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CUiWidgets.hpp"
#include "klbgui/wnd/CButton.hpp"
#include <assert.h>

using namespace klb;

namespace klbui {

//////////////////////////////////////////////////////////////////////////

class CUiButton : public CButton
{
public:
    CUiButton();
    virtual ~CUiButton();

    static void OnColor(CWnd* p_cwnd, int method, const klb_map_t* p_in, klb_map_t* p_out);
    static void OnTextAlign(CWnd* p_cwnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

public:
    void InitFuncMap();

private:
    CButtonCss*     m_globalcss;
    CButtonCss*     m_mycss;
};

//////////////////////////////////////////////////////////////////////////


CUiButton::CUiButton()
{

}

CUiButton::~CUiButton()
{

}

void CUiButton::OnColor(CWnd* p_cwnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attributes_t* p_attr = p_cwnd->GetCss()->Normal();
    klbuicssex_text_color(&p_attr->text, p_cwnd->GetWnd(), method, p_in, p_out);
}

void CUiButton::OnTextAlign(CWnd* p_cwnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attributes_t* p_attr = p_cwnd->GetCss()->Normal();
    klbuicssex_text_align(&p_attr->text, p_cwnd->GetWnd(), method, p_in, p_out);
}

void CUiButton::InitFuncMap()
{
    BindFunction("color", CUiButton::OnColor);
    BindFunction("text-align", CUiButton::OnTextAlign);
}

//////////////////////////////////////////////////////////////////////////

static klb_wnd_t* klbui_cbutton_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1 : get klbui::CGui*
    CGui* p_cppgui = KLB_CGUI_PTR(p_gui);

    // step2 : new
    CUiButton* p_button = new CUiButton();

    // step3 : Init
    p_button->Init(p_cppgui, x, y, w, h);

    // step4 : set type
    p_button->SetType(KLBUI_cbutton);

    // step5 : 初始化CSS 支持的方法
    p_button->InitFuncMap();

    // step6. 默认使用公用全局CSS属性
    CButtonCss* p_globalcss = (CButtonCss*)klb_gui_globalcss_get_ptr(p_gui, KLBUI_cbutton);
    p_button->SetCss(p_globalcss);

    return p_button->GetWnd();
}

int klbui_register_cbutton(klb_gui_t* p_gui)
{
    return klb_gui_register(p_gui, KLBUI_cbutton, klbui_cbutton_create);
}

} // klbui
