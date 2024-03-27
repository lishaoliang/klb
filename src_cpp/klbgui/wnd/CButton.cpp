// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/CButton.hpp"
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
    CButtonCss* p_css = ToButtonCss();

    if (NULL == p_css)
    {
        return 0;
    }

    DrawLine2(0, 0, 100, 100, p_css->GetColor());


    return 0;
}

int CButton::OnControl(int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    return 0;
}

} // klbui
