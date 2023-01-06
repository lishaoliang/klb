#include "klbgui/klbui_css_ex.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// 默认初始化
void klbuicssex_attributes_init(klbuicssex_attributes_t* p_attr, const klbui_default_t* p_default)
{
    p_attr->text.color = p_default->text_color;
    p_attr->text.align = KLBUICSS_text1_center;

    p_attr->font.style = KLBUICSS_font1_normal;
    p_attr->font.weight = KLBUICSS_font2_normal;
    p_attr->font.size = p_default->font_size;

    p_attr->background.color = p_default->background_color;
    p_attr->background.image = sdsempty();

    p_attr->border.width.top = p_default->border_width;
    p_attr->border.width.right = p_default->border_width;
    p_attr->border.width.bottom = p_default->border_width;
    p_attr->border.width.left = p_default->border_width;
    p_attr->border.color.top = p_default->border_color;
    p_attr->border.color.right = p_default->border_color;
    p_attr->border.color.bottom = p_default->border_color;
    p_attr->border.color.left = p_default->border_color;
    p_attr->border.radius = p_default->border_radius;
}

void klbuicssex_attributes_quit(klbuicssex_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// 公共 单属性操作

void klbuicssex_attribute_sds(sds* p_sds, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_sds);

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, *p_sds);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        if (KLB_ADT_string == klb_map_array_type(p_in, start))
        {
            const char* p_value = klb_map_idx_to_string(p_in, start);
            *p_sds = sdscpy(*p_sds, p_value);

            klb_wnd_update(p_wnd);
        }
    }
}

void klbuicssex_attribute_int(int* p_int, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_int);

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, *p_int);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                *p_int = (int)klb_map_idx_to_int64(p_in, start);

                klb_wnd_update(p_wnd);
            }
            break;
        case KLB_ADT_uint64:
            {
                *p_int = (int)klb_map_idx_to_uint64(p_in, start);

                klb_wnd_update(p_wnd);
            }
            break;
        default:
            break;
        }
    }
}

void klbuicssex_attribute_color(uint32_t* p_color, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_color);

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, *p_color);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            *p_color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

void klbuicssex_attribute_image(sds* p_image, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    assert(NULL != p_image);

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, *p_image);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        if (KLB_ADT_string == klb_map_array_type(p_in, start))
        {
            const char* p_value = klb_map_idx_to_string(p_in, start);
            *p_image = sdscpy(*p_image, p_value);

            klb_wnd_update(p_wnd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// CSS基础

// 显隐 visibility
void klbuicssex_visibility(klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        if (p_wnd->state.status & KLB_WND_STATUS_HIDE)
        {
            klb_map_set_idx_string(p_out, 0, "hidden");
        }
        else
        {
            klb_map_set_idx_string(p_out, 0, "visible");
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);

        switch (type)
        {
        case KLB_ADT_bool:
            {
                klb_wnd_show(p_wnd, klb_map_idx_to_bool(p_in, start));
            }
            break;
        case KLB_ADT_string:
            {
                const char* p_value = (const char*)klb_map_idx_to_string(p_in, start);
                if (0 == strcmp(p_value, "visible"))
                {
                    klb_wnd_show(p_wnd, true);
                }
                else if (0 == strcmp(p_value, "hidden"))
                {
                    klb_wnd_show(p_wnd, false);
                }
            }
            break;
        case KLB_ADT_int64:
            {
                klb_wnd_show(p_wnd, (0 == klb_map_idx_to_int64(p_in, start)) ? false : true);
            }
            break;
        case KLB_ADT_uint64:
            {
                klb_wnd_show(p_wnd, (0 == klb_map_idx_to_uint64(p_in, start)) ? false : true);
            }
            break;
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// 文本

// 文本颜色 color
void klbuicssex_text_color(klbuicss_text_t* p_text, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_text->color), p_wnd, method, p_in, p_out);
}

// 文本对齐 text-align
void klbuicssex_text_align(klbuicss_text_t* p_text, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        switch (p_text->align)
        {
        case KLBUICSS_text1_left:
            {
                klb_map_set_idx_string(p_out, 0, "left");
            }
            break;
        case KLBUICSS_text1_right:
            {
                klb_map_set_idx_string(p_out, 0, "right");
            }
            break;
        case KLBUICSS_text1_center:
        default:
            {
                klb_map_set_idx_string(p_out, 0, "center");
            }
            break;
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "center"))
        {
            p_text->align = KLBUICSS_text1_center;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "left"))
        {
            p_text->align = KLBUICSS_text1_left;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "right"))
        {
            p_text->align = KLBUICSS_text1_right;
            klb_wnd_update(p_wnd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// 字体

// 斜体 font-style
void klbuicssex_font_style(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        switch (p_font->style)
        {
        case KLBUICSS_font1_normal:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        case KLBUICSS_font1_italic:
            {
                klb_map_set_idx_string(p_out, 0, "italic");
            }
            break;
        default:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "normal"))
        {
            p_font->style = KLBUICSS_font1_normal;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "italic"))
        {
            p_font->style = KLBUICSS_font1_italic;
            klb_wnd_update(p_wnd);
        }
    }
}

// 字体粗细 font-weight
void klbuicssex_font_weight(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        switch (p_font->weight)
        {
        case KLBUICSS_font2_normal:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        case KLBUICSS_font2_bold:
            {
                klb_map_set_idx_string(p_out, 0, "bold");
            }
            break;
        default:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        }
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "normal"))
        {
            p_font->weight = KLBUICSS_font2_normal;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "bold"))
        {
            p_font->weight = KLBUICSS_font2_bold;
            klb_wnd_update(p_wnd);
        }
    }
}

// 字体大小 font-size
void klbuicssex_font_size(klbuicss_font_t* p_font, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_font->size), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 背景

// 背景色 background-color
void klbuicssex_background_color(klbuicss_background_t* p_background, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_background->color), p_wnd, method, p_in, p_out);
}

// 背景图片 background-image
void klbuicssex_background_image(klbuicss_background_t* p_background, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_background->image), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 边框

// 边框类型 border-style
void klbuicssex_border_style(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, "normal");
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "none"))
        {
            p_border->style.top = KLBUICSS_border1_none;
            p_border->style.right = KLBUICSS_border1_none;
            p_border->style.bottom = KLBUICSS_border1_none;
            p_border->style.left = KLBUICSS_border1_none;

            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "dotted"))
        {
            p_border->style.top = KLBUICSS_border1_dotted;
            p_border->style.right = KLBUICSS_border1_dotted;
            p_border->style.bottom = KLBUICSS_border1_dotted;
            p_border->style.left = KLBUICSS_border1_dotted;

            klb_wnd_update(p_wnd);
        }
    }
}

// 边框宽度 border-width
void klbuicssex_border_width(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, 1);
        klb_map_set_idx_int64(p_out, 1, 1);
        klb_map_set_idx_int64(p_out, 2, 1);
        klb_map_set_idx_int64(p_out, 3, 1);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        if (KLB_ADT_map == klb_map_array_type(p_in, start))
        {
            klb_map_t* ptr = klb_map_idx_to_map(p_in, start);
            p_border->width.top = (int)klb_map_idx_to_int64(ptr, 0);
            p_border->width.right = (int)klb_map_idx_to_int64(ptr, 1);
            p_border->width.bottom = (int)klb_map_idx_to_int64(ptr, 2);
            p_border->width.left = (int)klb_map_idx_to_int64(ptr, 3);

            klb_wnd_update(p_wnd);
        }
    }
}

// 边框颜色 border-color
void klbuicssex_border_color(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_border->color.top);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_border->color.top = color;
            p_border->color.right = color;
            p_border->color.bottom = color;
            p_border->color.left = color;

            klb_wnd_update(p_wnd);
        }
    }
}

// 圆角边框 border-radius
void klbuicssex_border_radius(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_border->radius), p_wnd, method, p_in, p_out);
}
