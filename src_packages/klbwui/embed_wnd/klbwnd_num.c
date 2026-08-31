// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_num.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void do_set_value_klbwnd_num(klbwnd_num_t* p_num, int value);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_num_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_num_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_num_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_num_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static int klbwnd_num_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_num_css_t* p_css = p_num->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_num_attributes_t* p_attr = NULL;

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        p_attr = &p_css->disable;
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        p_attr = &p_css->focus;
    }
    else
    {
        p_attr = &p_css->normal;
    }

    klbwnd_num_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    char title[32] = { 0 };
    snprintf(title, sizeof(title) - 1, "%d", p_num->value);
    klbuicssex_draw_text2(p_wnd, title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);

    return 0;
}

static int on_decimal_klbwnd_num(void* ptr, klb_wnd_t* p_wnd_dec, bool ok, int value)
{
    klb_wnd_t* p_wnd_num = (klb_wnd_t*)ptr;
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd_num->ctrl;

    if (ok)
    {
        do_set_value_klbwnd_num(p_num, value);
        klb_wnd_call_command(p_wnd_num, KLBUI_onchange, NULL, NULL, 0, 0);
        klb_wnd_update(p_wnd_num);
    }

    (void)p_wnd_dec;
    return 0;
}

static int klbwnd_num_on_click(klb_wnd_t* p_wnd, klbwnd_num_t* p_num)
{
    if (klb_wnd_is_disable(p_wnd) || klb_wnd_is_hide(p_wnd))
    {
        return 0;
    }

    if (NULL == p_num->p_decimal)
    {
        return 0;
    }

    char str[32] = { 0 };
    snprintf(str, sizeof(str) - 1, "%d", p_num->max);
    int len1 = (int)strlen(str);

    snprintf(str, sizeof(str) - 1, "%d", p_num->min);
    int len2 = (int)strlen(str);

    int max_len = MAX(len1, len2);
    if (max_len < 1)
    {
        max_len = 1;
    }

    klbshw_decimal_menu_bind(p_num->p_decimal, on_decimal_klbwnd_num, p_wnd);
    klbshw_decimal_menu_set_value(p_num->p_decimal, p_num->value);
    klbshw_decimal_menu_set_ranges(p_num->p_decimal, max_len);
    klbshw_decimal_menu_layout(p_num->p_decimal);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_decimal_menu_wh(p_num->p_decimal, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_num->p_decimal, x, y);
    klb_wnd_resize(p_num->p_decimal, menu_w, menu_h);

    klb_gui_popup_wnd(p_wnd->p_gui, p_num->p_decimal);

    return 0;
}

static int klbwnd_num_on_mousewheel(klb_wnd_t* p_wnd, klbwnd_num_t* p_num, int lparam)
{
    int delta = KLBUI_MOUSEWHEEL_value(lparam);

    if (delta <= 0 || klb_wnd_is_disable(p_wnd) || klb_wnd_is_hide(p_wnd))
    {
        return 0;
    }

    if (KLBUI_MOUSEWHEEL_is_up(lparam))
    {
        int v = p_num->value - 1;
        if (p_num->min <= v)
        {
            do_set_value_klbwnd_num(p_num, v);
            klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
            klb_wnd_update(p_wnd);
        }
    }
    else if (KLBUI_MOUSEWHEEL_is_down(lparam))
    {
        int v = p_num->value + 1;
        if (v <= p_num->max)
        {
            do_set_value_klbwnd_num(p_num, v);
            klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
            klb_wnd_update(p_wnd);
        }
    }

    return 0;
}

static int klbwnd_num_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_num_on_paint(p_wnd);

    case KLBUI_onparsewindow:
        do_set_value_klbwnd_num(p_num, p_num->value);
        break;

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_num_on_click(p_wnd, p_num);

    case KLBUI_mousewheel:
        return klbwnd_num_on_mousewheel(p_wnd, p_num, lparam);

    default:
        break;
    }

    (void)p_pt1;
    (void)p_pt2;
    (void)wparam;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 私有函数

static void do_set_value_klbwnd_num(klbwnd_num_t* p_num, int value)
{
    if (value < p_num->min)
    {
        value = p_num->min;
    }

    if (p_num->max < value)
    {
        value = p_num->max;
    }

    p_num->value = value;
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_num_set_css(klb_wnd_t* p_wnd, klbwnd_num_css_t* p_css)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->p_css = p_css;
}

void klbwnd_num_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    do_set_value_klbwnd_num(p_num, value);
}

int klbwnd_num_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    return p_num->value;
}

void klbwnd_num_set_ranges(klb_wnd_t* p_wnd, int min, int max)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->min = min;
    p_num->max = max;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_num_attributes_init(klbwnd_num_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_num_attributes_deinit(klbwnd_num_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_num_attributes_copy(klbwnd_num_attributes_t* p_dst, const klbwnd_num_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_num_css_init(klbwnd_num_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_num_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_num_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_num_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_num_css_deinit(klbwnd_num_css_t* p_css)
{
    klbwnd_num_attributes_deinit(&p_css->normal);
    klbwnd_num_attributes_deinit(&p_css->focus);
    klbwnd_num_attributes_deinit(&p_css->disable);
}

void klbwnd_num_css_copy(klbwnd_num_css_t* p_dst, klbwnd_num_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_num_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_num_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_num_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_num_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_num_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    p_num->value = 0;
    p_num->min = 0;
    p_num->max = 65536;
    p_num->p_decimal = klbui_shwnd_get_decimal_menu(p_gui);
}

void klbwnd_num_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->p_decimal = NULL;
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_num_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_num_t));

    klbwnd_num_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_num_destroy;

    return p_wnd;
}

// end
