#include "klbgui/klbui_css_ex.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// 默认初始化
void klbuicssex_attributes_init(klbuicssex_attributes_t* p_dst, const klbuicssex_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.image = sdsempty();
    p_dst->background.color = p_src->background.color;
    p_dst->background.image = sdscpy(p_dst->background.image, p_src->background.image);
    p_dst->background.repeat = p_src->background.repeat;
    p_dst->background.position = p_src->background.position;
    p_dst->background.attachment = p_src->background.attachment;

    p_dst->border = p_src->border;
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

                if (NULL != p_wnd)
                {
                    klb_wnd_update(p_wnd);
                }
            }
            break;
        case KLB_ADT_uint64:
            {
                *p_int = (int)klb_map_idx_to_uint64(p_in, start);

                if (NULL != p_wnd)
                {
                    klb_wnd_update(p_wnd);
                }
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
        if (klb_gui_check_color(NULL, p_in, start, &color))
        {
            *p_color = color;

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
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

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
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
// 外边距

// 外边距 margin
void klbuicssex_margin(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_margin->top);
        klb_map_set_idx_int64(p_out, 1, p_margin->right);
        klb_map_set_idx_int64(p_out, 2, p_margin->bottom);
        klb_map_set_idx_int64(p_out, 3, p_margin->left);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int t = klb_map_array_type(p_in, start);
        if (KLB_ADT_map == t)
        {
            klb_map_t* ptr = klb_map_idx_to_map(p_in, start);
            p_margin->top = (int)klb_map_idx_to_int64(ptr, 0);
            p_margin->right = (int)klb_map_idx_to_int64(ptr, 1);
            p_margin->bottom = (int)klb_map_idx_to_int64(ptr, 2);
            p_margin->left = (int)klb_map_idx_to_int64(ptr, 3);

            if(NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
        else if (KLB_ADT_int64 == t)
        {
            int w = (int)klb_map_idx_to_int64(p_in, start);

            p_margin->top = w;
            p_margin->right = w;
            p_margin->bottom = w;
            p_margin->left = w;

            if (NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
        else if (KLB_ADT_uint64 == t)
        {
            int w = (int)klb_map_idx_to_uint64(p_in, start);

            p_margin->top = w;
            p_margin->right = w;
            p_margin->bottom = w;
            p_margin->left = w;

            if (NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
    }
}

// 外边距-上 margin-top
void klbuicssex_margin_top(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_margin->top, p_wnd, method, p_in, p_out);
}

// 外边距-右 margin-right
void klbuicssex_margin_right(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_margin->right, p_wnd, method, p_in, p_out);
}

// 外边距-下 margin-bottom
void klbuicssex_margin_bottom(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_margin->bottom, p_wnd, method, p_in, p_out);
}

// 外边距-上 margin-left
void klbuicssex_margin_left(klbuicss_margin_t* p_margin, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_margin->left, p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 内边距

// 内边距 padding
void klbuicssex_padding(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_padding->top);
        klb_map_set_idx_int64(p_out, 1, p_padding->right);
        klb_map_set_idx_int64(p_out, 2, p_padding->bottom);
        klb_map_set_idx_int64(p_out, 3, p_padding->left);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int t = klb_map_array_type(p_in, start);
        if (KLB_ADT_map == t)
        {
            klb_map_t* ptr = klb_map_idx_to_map(p_in, start);
            p_padding->top = (int)klb_map_idx_to_int64(ptr, 0);
            p_padding->right = (int)klb_map_idx_to_int64(ptr, 1);
            p_padding->bottom = (int)klb_map_idx_to_int64(ptr, 2);
            p_padding->left = (int)klb_map_idx_to_int64(ptr, 3);

            if (NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
        else if (KLB_ADT_int64 == t)
        {
            int w = (int)klb_map_idx_to_int64(p_in, start);

            p_padding->top = w;
            p_padding->right = w;
            p_padding->bottom = w;
            p_padding->left = w;

            if (NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
        else if (KLB_ADT_uint64 == t)
        {
            int w = (int)klb_map_idx_to_uint64(p_in, start);

            p_padding->top = w;
            p_padding->right = w;
            p_padding->bottom = w;
            p_padding->left = w;

            if (NULL != p_wnd)   klb_wnd_update(p_wnd);
        }
    }
}

// 内边距-上 padding-top
void klbuicssex_padding_top(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_padding->top, p_wnd, method, p_in, p_out);
}

// 内边距-右 padding-right
void klbuicssex_padding_right(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_padding->right, p_wnd, method, p_in, p_out);
}

// 内边距-下 padding-bottom
void klbuicssex_padding_bottom(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_padding->bottom, p_wnd, method, p_in, p_out);
}

// 内边距-上 padding-left
void klbuicssex_padding_left(klbuicss_padding_t* p_padding, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&p_padding->left, p_wnd, method, p_in, p_out);
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
        bool update = false;

        if (0 == strcmp(p_value, "center"))
        {
            p_text->align = KLBUICSS_text1_center;
            update = true;
        }
        else if (0 == strcmp(p_value, "left"))
        {
            p_text->align = KLBUICSS_text1_left;
            update = true;
        }
        else if (0 == strcmp(p_value, "right"))
        {
            p_text->align = KLBUICSS_text1_right;
            update = true;
        }

        if (NULL != p_wnd && update)
        {
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
        klb_map_set_idx_int64(p_out, 0, p_border->width.top);
        klb_map_set_idx_int64(p_out, 1, p_border->width.right);
        klb_map_set_idx_int64(p_out, 2, p_border->width.bottom);
        klb_map_set_idx_int64(p_out, 3, p_border->width.left);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int t = klb_map_array_type(p_in, start);
        if (KLB_ADT_map == t)
        {
            klb_map_t* ptr = klb_map_idx_to_map(p_in, start);
            p_border->width.top = (int)klb_map_idx_to_int64(ptr, 0);
            p_border->width.right = (int)klb_map_idx_to_int64(ptr, 1);
            p_border->width.bottom = (int)klb_map_idx_to_int64(ptr, 2);
            p_border->width.left = (int)klb_map_idx_to_int64(ptr, 3);

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
        }
        else if(KLB_ADT_int64 == t)
        {
            int w = (int)klb_map_idx_to_int64(p_in, start);

            p_border->width.top = w;
            p_border->width.right = w;
            p_border->width.bottom = w;
            p_border->width.left = w;

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
        }
        else if(KLB_ADT_uint64 == t)
        {
            int w = (int)klb_map_idx_to_uint64(p_in, start);

            p_border->width.top = w;
            p_border->width.right = w;
            p_border->width.bottom = w;
            p_border->width.left = w;

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
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
        if (klb_gui_check_color(NULL, p_in, start, &color))
        {
            p_border->color.top = color;
            p_border->color.right = color;
            p_border->color.bottom = color;
            p_border->color.left = color;

            if (NULL != p_wnd)
            {
                klb_wnd_update(p_wnd);
            }
        }
    }
}

// 圆角边框 border-radius
void klbuicssex_border_radius(klbuicss_border_t* p_border, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_border->radius), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 参考绘制


// 绘制边框
void klbuicssex_draw_border(klb_wnd_t* p_wnd, klb_rect_t* p_rect, klbuicss_border_t* p_border)
{
    assert(NULL != p_wnd);
    assert(NULL != p_rect);

    // border
    if (NULL == p_border)
    {
        return;
    }

    // top
    if (0 < p_border->width.top)
    {
        klb_rect_t border_top = { p_rect->x, p_rect->y, p_rect->w, p_border->width.top };
        klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_border->color.top);
    }

    // right
    if (0 < p_border->width.right)
    {
        klb_rect_t border_right = { p_rect->x + p_rect->w - p_border->width.right, p_rect->y, p_border->width.right, p_rect->h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_border->color.right);
    }
    
    // bottom
    if (0 < p_border->width.bottom)
    {
        klb_rect_t border_bottom = { p_rect->x, p_rect->y + p_rect->h - p_border->width.bottom, p_rect->w, p_border->width.bottom };
        klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_border->color.bottom);
    }

    // left
    if (0 < p_border->width.left)
    {
        klb_rect_t border_left = { p_rect->x, p_rect->y, p_border->width.left, p_rect->h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_border->color.left);
    }
}

// 绘制文本
void klbuicssex_draw_text(klb_wnd_t* p_wnd, sds txt, klb_rect_t* p_rect, klbuicss_border_t* p_border, klbuicss_padding_t* p_padding, klbuicss_text_t* p_css_text, klbuicss_font_t* p_css_font)
{
    assert(NULL != p_wnd);

    if (NULL == txt || sdslen(txt) <= 0)
    {
        return;
    }

    klb_rect_t text_rect = *p_rect;

    // 移除边框
    if (NULL != p_border)
    {
        text_rect.x += p_border->width.left;
        text_rect.y += p_border->width.top;
        text_rect.w -= (p_border->width.left + p_border->width.right);
        text_rect.h -= (p_border->width.top + p_border->width.bottom);
    }

    // 移除内边距
    if (NULL != p_padding)
    {
        text_rect.x += p_padding->left;
        text_rect.y += p_padding->top;
        text_rect.w -= (p_padding->left + p_padding->right);
        text_rect.h -= (p_padding->top + p_padding->bottom);
    }

    if (KLBUICSS_text1_center == p_css_text->align)
    {
        // 居中
        int need_w = 0;
        klb_wnd_text_size2(p_wnd, txt, sdslen(txt), p_css_font->size, &need_w, NULL);

        if (need_w < text_rect.w)
        {
            text_rect.x += (text_rect.w - need_w) / 2;
        }
    }
    else if(KLBUICSS_text1_right == p_css_text->align)
    {
        // 右对其
        int need_w = 0;
        klb_wnd_text_size2(p_wnd, txt, sdslen(txt), p_css_font->size, &need_w, NULL);

        if (need_w < text_rect.w)
        {
            text_rect.x += (text_rect.w - need_w);
        }
    }
    else
    {
        // 左对其, 无需处理
    }

    klb_wnd_draw_text2(p_wnd, &text_rect, txt, sdslen(txt), p_css_text->color, p_css_font->size);
}
