// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/CWnd.hpp"
#include "klbgui/CGui.hpp"
#include "klbmem/klb_mem.h"
#include <assert.h>


using namespace klb;


namespace klbui {


//////////////////////////////////////////////////////////////////////////
// 辅助函数

bool CWnd::CheckCssBool(klb::CMap* p_map, int start, bool* p_out_bool)
{
    if (KLB_ADT_bool == p_map->Type(start))
    {
        bool b = false;
        p_map->Get(start, b);

        if (NULL != p_out_bool) { *p_out_bool = b; }

        return true;
    }

    return false;
}

bool CWnd::CheckCssInteger(klb::CMap* p_map, int start, int* p_out_int)
{
    klb_adt_type_e type = p_map->Type(start);
    switch (type)
    {
    case KLB_ADT_uint64:
        {
            uint64_t v = 0;
            p_map->Get(start, v);

            if (NULL != p_out_int) { *p_out_int = (int)v; }

            return true;
        }
        break;

    case KLB_ADT_int64:
        {
            int64_t v = 0;
            p_map->Get(start, v);

            if (NULL != p_out_int) { *p_out_int = (int)v; }

            return true;
        }
        break;

    default:
        break;
    }

    return false;
}

bool CWnd::CheckCssInteger(klb::CMap* p_map, int start, int64_t* p_out_int)
{
    klb_adt_type_e type = p_map->Type(start);
    switch (type)
    {
    case KLB_ADT_uint64:
        {
            uint64_t v = 0;
            p_map->Get(start, v);

            if (NULL != p_out_int) { *p_out_int = (int64_t)v; }

            return true;
        }
        break;

    case KLB_ADT_int64:
        {
            int64_t v = 0;
            p_map->Get(start, v);

            if (NULL != p_out_int) { *p_out_int = (int64_t)v; }

            return true;
        }
        break;

    default:
        break;
    }

    return false;
}

bool CWnd::CheckCssString(klb::CMap* p_map, int start, klb::CString& out_str)
{
    if (KLB_ADT_string == p_map->Type(start))
    {
        p_map->Get(start, out_str);

        return true;
    }

    return false;
}

bool CWnd::CheckCssColor(klb::CMap* p_map, int start, uint32_t* p_out_color)
{
    return m_gui->CheckCssColor(p_map, start, p_out_color);
}

//////////////////////////////////////////////////////////////////////////
// CSS 方法

/////////////////////////////////////
// CSS - 外边距 margin

void CWnd::OnCssMargin(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetMargin(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[1] = (int64_t)right;
        (*p_out)[2] = (int64_t)bottom;
        (*p_out)[3] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        klb_adt_type_e type = p_in->Type(start);
        switch (type)
        {
        case KLB_ADT_uint64:
            {
                uint64_t v = 0;
                p_in->Get(start, v);

                p_css->SetMargin((int)v);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_int64:
            {
                int64_t v = 0;
                p_in->Get(start, v);

                p_css->SetMargin((int)v);

                p_cwnd->Update();
            }
            break;

        default:
            break;
        }
    }
}

void CWnd::OnCssMarginTop(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0;
        p_css->GetMargin(&top, NULL, NULL, NULL);

        (*p_out)[0] = (int64_t)top;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int top = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &top))
        {
            p_css->SetMarginTop(top);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssMarginRight(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int right = 0;
        p_css->GetMargin(NULL, &right, NULL, NULL);

        (*p_out)[0] = (int64_t)right;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int right = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &right))
        {
            p_css->SetMarginRight(right);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssMarginBottom(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int bottom = 0;
        p_css->GetMargin(NULL, NULL, &bottom, NULL);

        (*p_out)[0] = (int64_t)bottom;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int bottom = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &bottom))
        {
            p_css->SetMarginBottom(bottom);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssMarginLeft(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int left = 0;
        p_css->GetMargin(NULL, NULL, NULL, &left);

        (*p_out)[0] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int left = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &left))
        {
            p_css->SetMarginLeft(left);

            p_cwnd->Update();
        }
    }
}

/////////////////////////////////////
// CSS - 内边距 padding

void CWnd::OnCssPadding(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetPadding(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[1] = (int64_t)right;
        (*p_out)[2] = (int64_t)bottom;
        (*p_out)[3] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        klb_adt_type_e type = p_in->Type(start);
        switch (type)
        {
        case KLB_ADT_uint64:
            {
                uint64_t v = 0;
                p_in->Get(start, v);

                p_css->SetPadding((int)v);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_int64:
            {
                int64_t v = 0;
                p_in->Get(start, v);

                p_css->SetPadding((int)v);

                p_cwnd->Update();
            }
        break;

        default:
            break;
        }
    }
}

void CWnd::OnCssPaddingTop(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0;
        p_css->GetPadding(&top, NULL, NULL, NULL);

        (*p_out)[0] = (int64_t)top;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int top = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &top))
        {
            p_css->SetPaddingTop(top);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssPaddingRight(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int right = 0;
        p_css->GetPadding(NULL, &right, NULL, NULL);

        (*p_out)[0] = (int64_t)right;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int right = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &right))
        {
            p_css->SetPaddingRight(right);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssPaddingBottom(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int bottom = 0;
        p_css->GetPadding(NULL, NULL, &bottom, NULL);

        (*p_out)[0] = (int64_t)bottom;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int bottom = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &bottom))
        {
            p_css->SetPaddingBottom(bottom);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssPaddingLeft(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int left = 0;
        p_css->GetPadding(NULL, NULL, NULL, &left);

        (*p_out)[0] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        int left = 0;
        if (p_cwnd->CheckCssInteger(p_in, start, &left))
        {
            p_css->SetPaddingLeft(left);

            p_cwnd->Update();
        }
    }
}

/////////////////////////////////////
// CSS - 文本颜色 color

void CWnd::OnCssColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetColor();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetColor(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetColorFocus();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetColorFocus(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetColorDisable();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetColorDisable(color);

            p_cwnd->Update();
        }
    }
}


/////////////////////////////////////
// CSS - 文本对齐 text-align

void CWnd::OnCssTextAlign(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int align = p_css->GetTextAlign();

        if (KLBUICSS_text1_left == align)
        {
            (*p_out)[0] = "left";
        }
        else if(KLBUICSS_text1_center == align)
        {
            (*p_out)[0] = "center";
        }
        else if (KLBUICSS_text1_right == align)
        {
            (*p_out)[0] = "right";
        }
        else
        {
            (*p_out)[0] = "center";
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString align;

        if (p_cwnd->CheckCssString(p_in, start, align))
        {
            if (align == "left")
            {
                p_css->SetTextAlign(KLBUICSS_text1_left);
                p_cwnd->Update();
            }
            else if (align == "center")
            {
                p_css->SetTextAlign(KLBUICSS_text1_center);
                p_cwnd->Update();
            }
            else if(align == "right")
            {
                p_css->SetTextAlign(KLBUICSS_text1_right);
                p_cwnd->Update();
            }
        }
    }
}

void CWnd::OnCssTextAlignFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int align = p_css->GetTextAlignFocus();

        if (KLBUICSS_text1_left == align)
        {
            (*p_out)[0] = "left";
        }
        else if (KLBUICSS_text1_center == align)
        {
            (*p_out)[0] = "center";
        }
        else if (KLBUICSS_text1_right == align)
        {
            (*p_out)[0] = "right";
        }
        else
        {
            (*p_out)[0] = "center";
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString align;

        if (p_cwnd->CheckCssString(p_in, start, align))
        {
            if (align == "left")
            {
                p_css->SetTextAlignFocus(KLBUICSS_text1_left);
                p_cwnd->Update();
            }
            else if (align == "center")
            {
                p_css->SetTextAlignFocus(KLBUICSS_text1_center);
                p_cwnd->Update();
            }
            else if (align == "right")
            {
                p_css->SetTextAlignFocus(KLBUICSS_text1_right);
                p_cwnd->Update();
            }
        }
    }
}

void CWnd::OnCssTextAlignDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        int align = p_css->GetTextAlignDisable();

        if (KLBUICSS_text1_left == align)
        {
            (*p_out)[0] = "left";
        }
        else if (KLBUICSS_text1_center == align)
        {
            (*p_out)[0] = "center";
        }
        else if (KLBUICSS_text1_right == align)
        {
            (*p_out)[0] = "right";
        }
        else
        {
            (*p_out)[0] = "center";
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString align;

        if (p_cwnd->CheckCssString(p_in, start, align))
        {
            if (align == "left")
            {
                p_css->SetTextAlignDisable(KLBUICSS_text1_left);
                p_cwnd->Update();
            }
            else if (align == "center")
            {
                p_css->SetTextAlignDisable(KLBUICSS_text1_center);
                p_cwnd->Update();
            }
            else if (align == "right")
            {
                p_css->SetTextAlignDisable(KLBUICSS_text1_right);
                p_cwnd->Update();
            }
        }
    }
}

/////////////////////////////////////
// CSS - 字体大小 font-size

void CWnd::OnCssFontSize(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetFontSize();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int font = 20;

        if (p_cwnd->CheckCssInteger(p_in, start, &font))
        {
            p_css->SetFontSize(font);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssFontSizeFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetFontSizeFocus();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int font = 20;

        if (p_cwnd->CheckCssInteger(p_in, start, &font))
        {
            p_css->SetFontSizeFocus(font);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssFontSizeDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetFontSizeDisable();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int font = 20;

        if (p_cwnd->CheckCssInteger(p_in, start, &font))
        {
            p_css->SetFontSizeDisable(font);

            p_cwnd->Update();
        }
    }
}

/////////////////////////////////////
// CSS - 背景色 background-color

void CWnd::OnCssBackgroundColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetBackgroundColor();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBackgroundColor(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBackgroundColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetBackgroundColor();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBackgroundColorFocus(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBackgroundColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = (int64_t)p_css->GetBackgroundColor();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBackgroundColorDisable(color);

            p_cwnd->Update();
        }
    }
}

/////////////////////////////////////
// CSS - 背景图片 background-image

void CWnd::OnCssBackgroundImage(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_css->GetBackgroundImage();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString image;

        if (p_cwnd->CheckCssString(p_in, start, image))
        {
            p_css->SetBackgroundImage(image);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBackgroundImageFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_css->GetBackgroundImageFocus();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString image;

        if (p_cwnd->CheckCssString(p_in, start, image))
        {
            p_css->SetBackgroundImageFocus(image);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBackgroundImageDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        (*p_out)[0] = p_css->GetBackgroundImageDisable();
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb::CString image;

        if (p_cwnd->CheckCssString(p_in, start, image))
        {
            p_css->SetBackgroundImageDisable(image);

            p_cwnd->Update();
        }
    }
}

/////////////////////////////////////
// CSS - 边框的宽度 border-width

void CWnd::OnCssBorderWidth(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();
    klb_map_t* p_map_in = (klb_map_t*)p_in->GetMap();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderWidth(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[1] = (int64_t)right;
        (*p_out)[2] = (int64_t)bottom;
        (*p_out)[3] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1; 
        
        klb_adt_type_e type = p_in->Type(start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                int64_t w = (*p_in)[start].ToInt64();
                p_css->SetBorderWidth((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_uint64:
            {
                uint64_t w = (*p_in)[start].ToUint64();
                p_css->SetBorderWidth((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_map:
            {
                klb_map_t* ptr = klb_map_idx_to_map(p_map_in, start);
                int top = (int)klb_map_idx_to_int64(ptr, 0);
                int right = (int)klb_map_idx_to_int64(ptr, 1);
                int bottom = (int)klb_map_idx_to_int64(ptr, 2);
                int left = (int)klb_map_idx_to_int64(ptr, 3);
                
                p_css->SetBorderWidth(top, right, bottom, left);

                p_cwnd->Update();
            }
            break;

        default:
            break;
        }
    }
}

void CWnd::OnCssBorderWidthFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();
    klb_map_t* p_map_in = (klb_map_t*)p_in->GetMap();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderWidthFocus(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[1] = (int64_t)right;
        (*p_out)[2] = (int64_t)bottom;
        (*p_out)[3] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        klb_adt_type_e type = p_in->Type(start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                int64_t w = (*p_in)[start].ToInt64();
                p_css->SetBorderWidthFocus((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_uint64:
            {
                uint64_t w = (*p_in)[start].ToUint64();
                p_css->SetBorderWidthFocus((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_map:
            {
                klb_map_t* ptr = klb_map_idx_to_map(p_map_in, start);
                int top = (int)klb_map_idx_to_int64(ptr, 0);
                int right = (int)klb_map_idx_to_int64(ptr, 1);
                int bottom = (int)klb_map_idx_to_int64(ptr, 2);
                int left = (int)klb_map_idx_to_int64(ptr, 3);

                p_css->SetBorderWidthFocus(top, right, bottom, left);

                p_cwnd->Update();
            }
            break;

        default:
            break;
        }
    }
}

void CWnd::OnCssBorderWidthDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();
    klb_map_t* p_map_in = (klb_map_t*)p_in->GetMap();

    if (KLBUI_CSSEX_get == method)
    {
        int top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderWidthDisable(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[1] = (int64_t)right;
        (*p_out)[2] = (int64_t)bottom;
        (*p_out)[3] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        klb_adt_type_e type = p_in->Type(start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                int64_t w = (*p_in)[start].ToInt64();
                p_css->SetBorderWidthDisable((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_uint64:
            {
                uint64_t w = (*p_in)[start].ToUint64();
                p_css->SetBorderWidthDisable((int)w);

                p_cwnd->Update();
            }
            break;

        case KLB_ADT_map:
            {
                klb_map_t* ptr = klb_map_idx_to_map(p_map_in, start);
                int top = (int)klb_map_idx_to_int64(ptr, 0);
                int right = (int)klb_map_idx_to_int64(ptr, 1);
                int bottom = (int)klb_map_idx_to_int64(ptr, 2);
                int left = (int)klb_map_idx_to_int64(ptr, 3);

                p_css->SetBorderWidthDisable(top, right, bottom, left);

                p_cwnd->Update();
            }
            break;

        default:
            break;
        }
    }
}

/////////////////////////////////////
// CSS - 边框的颜色 border-color

void CWnd::OnCssBorderColor(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        uint32_t top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderColor(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[0] = (int64_t)right;
        (*p_out)[0] = (int64_t)bottom;
        (*p_out)[0] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBorderColor(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBorderColorFocus(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        uint32_t top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderColorFocus(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[0] = (int64_t)right;
        (*p_out)[0] = (int64_t)bottom;
        (*p_out)[0] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBorderColorFocus(color);

            p_cwnd->Update();
        }
    }
}

void CWnd::OnCssBorderColorDisable(CWnd* p_cwnd, int method, klb::CMap* p_in, klb::CMap* p_out)
{
    CCss* p_css = p_cwnd->GetCss();

    if (KLBUI_CSSEX_get == method)
    {
        uint32_t top = 0, right = 0, bottom = 0, left = 0;
        p_css->GetBorderColorDisable(&top, &right, &bottom, &left);

        (*p_out)[0] = (int64_t)top;
        (*p_out)[0] = (int64_t)right;
        (*p_out)[0] = (int64_t)bottom;
        (*p_out)[0] = (int64_t)left;
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;

        if (p_cwnd->CheckCssColor(p_in, start, &color))
        {
            p_css->SetBorderColorDisable(color);

            p_cwnd->Update();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CSS 辅助函数

klb_map_t* CWnd::GetCssFunctionMap()
{
    return m_css_func_map;
}

void CWnd::BindCssFunction(const std::string& str, klb_cwnd_css_cb cb_func)
{
    klb_map_t* p_css_func_map = GetCssFunctionMap();
    if (p_css_func_map)
    {
        // Bug. C 与 C++ 回调函数 的定义 在参数有差异
        // 约定: C 指针绑定在 第一个位置, CPP指针绑定在第二个指针
        klb_map_set_ptr(p_css_func_map, str.c_str(), NULL, (void*)(cb_func));
    }
}

bool CWnd::InitCssFunctionMap(const std::string& type)
{
    klb_map_t* ptr = m_gui->CssMap(type);
    if (NULL != ptr)
    {
        m_css_func_map = ptr;
        return false; // 有解析 map, 则直接使用; 不是第一次
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = m_gui->NewCssMap(type);
    m_css_func_map = ptr;


    // 标准 CSS 方法

    // 外边距 margin
    BindCssFunction("margin", CWnd::OnCssMargin);
    BindCssFunction("margin-top", CWnd::OnCssMarginTop);
    BindCssFunction("margin-right", CWnd::OnCssMarginRight);
    BindCssFunction("margin-bottom", CWnd::OnCssMarginBottom);
    BindCssFunction("margin-left", CWnd::OnCssMarginLeft);

    // 内边距 padding
    BindCssFunction("padding", CWnd::OnCssPadding);
    BindCssFunction("padding-top", CWnd::OnCssPaddingTop);
    BindCssFunction("padding-right", CWnd::OnCssPaddingRight);
    BindCssFunction("padding-bottom", CWnd::OnCssPaddingBottom);
    BindCssFunction("padding-left", CWnd::OnCssPaddingLeft);

    // 文本颜色 color
    BindCssFunction("color", CWnd::OnCssColor);
    BindCssFunction("color:focus", CWnd::OnCssColorFocus);
    BindCssFunction("color:disable", CWnd::OnCssColorDisable);

    // 文本对齐 text-align
    BindCssFunction("text-align", CWnd::OnCssTextAlign);
    BindCssFunction("text-align:focus", CWnd::OnCssTextAlignFocus);
    BindCssFunction("text-align:disable", CWnd::OnCssTextAlignDisable);

    // 字体大小 font-size
    BindCssFunction("font-size", CWnd::OnCssFontSize);
    BindCssFunction("font-size:focus", CWnd::OnCssFontSizeFocus);
    BindCssFunction("font-size:disable", CWnd::OnCssFontSizeDisable);

    // 背景色 background-color
    BindCssFunction("background-color", CWnd::OnCssBackgroundColor);
    BindCssFunction("background-color:focus", CWnd::OnCssBackgroundColorFocus);
    BindCssFunction("background-color:disable", CWnd::OnCssBackgroundColorDisable);

    // 背景图片 background-image
    BindCssFunction("background-image", CWnd::OnCssBackgroundImage);
    BindCssFunction("background-image:focus", CWnd::OnCssBackgroundImageFocus);
    BindCssFunction("background-image:disable", CWnd::OnCssBackgroundImageDisable);

    // 边框的宽度 border-width
    BindCssFunction("border-width", CWnd::OnCssBorderWidth);
    BindCssFunction("border-width:focus", CWnd::OnCssBorderWidthFocus);
    BindCssFunction("border-width:disable", CWnd::OnCssBorderWidthDisable);

    // 边框的颜色 border-color
    BindCssFunction("border-color", CWnd::OnCssBorderColor);
    BindCssFunction("border-color:focus", CWnd::OnCssBorderColorFocus);
    BindCssFunction("border-color:disable", CWnd::OnCssBorderColorDisable);


    return true; // 是第一次
}


//////////////////////////////////////////////////////////////////////////
} // namespace klbui
//end
