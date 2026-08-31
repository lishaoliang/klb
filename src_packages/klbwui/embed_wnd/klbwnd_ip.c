// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_ip.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu_ip.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_ip_deinit_attribute(klbwnd_ip_t* p_ip);
static void klbwnd_ip_limit_octet(int* p_v);
static void klbwnd_ip_sync_value(klbwnd_ip_t* p_ip);
static void klbwnd_ip_parse_value(klbwnd_ip_t* p_ip, const char* p_value);
static void klbwnd_ip_apply_menu_value(klbwnd_ip_t* p_ip, int value);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_ip_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_ip_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_ip_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_ip_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static void klbwnd_ip_set_region_rect(klb_wnd_t* p_wnd, klbwnd_ip_t* p_ip, klbwnd_ip_attributes_t* p_attr, klb_rect_t* p_rect)
{
    int dot_w = 0;
    klb_wnd_text_size2(p_wnd, ".", 1, p_attr->font.size, &dot_w, NULL);

    int sx = p_rect->x;

    for (int i = 0; i < KLBWND_IP_octet_max; ++i)
    {
        char buf[8] = { 0 };
        snprintf(buf, sizeof(buf) - 1, "%d", p_ip->octet[i]);

        int ip_w = 0;
        klb_wnd_text_size2(p_wnd, buf, (int)strlen(buf), p_attr->font.size, &ip_w, NULL);

        klb_rect_init(&p_ip->octet_rect[i], sx, p_rect->y, ip_w, p_rect->h);
        sx += ip_w;

        if (i < KLBWND_IP_octet_max - 1)
        {
            klb_rect_init(&p_ip->dot_rect[i], sx, p_rect->y, dot_w, p_rect->h);
            sx += dot_w;
        }
    }
}

static void klbwnd_ip_on_paint_status(klb_wnd_t* p_wnd, klbwnd_ip_t* p_ip, klbwnd_ip_css_t* p_css, klbwnd_ip_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwnd_ip_draw_background_and_border(p_wnd, p_attr, p_rect);

    klb_rect_t region_rect = *p_rect;
    region_rect.x += p_attr->border.width.left;
    region_rect.y += p_attr->border.width.top;
    region_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
    region_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

    region_rect.x += p_css->padding.left;
    region_rect.y += p_css->padding.top;
    region_rect.w -= (p_css->padding.left + p_css->padding.right);
    region_rect.h -= (p_css->padding.top + p_css->padding.bottom);

    klbwnd_ip_set_region_rect(p_wnd, p_ip, p_attr, &region_rect);

    for (int i = 0; i < KLBWND_IP_octet_max - 1; ++i)
    {
        klbuicssex_draw_text2(p_wnd, ".", &p_ip->dot_rect[i], NULL, NULL, &p_attr->text, &p_attr->font);
    }

    for (int i = 0; i < KLBWND_IP_octet_max; ++i)
    {
        char buf[8] = { 0 };
        snprintf(buf, sizeof(buf) - 1, "%d", p_ip->octet[i]);
        klbuicssex_draw_text2(p_wnd, buf, &p_ip->octet_rect[i], NULL, NULL, &p_attr->text, &p_attr->font);
    }
}

static int klbwnd_ip_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_ip_css_t* p_css = p_ip->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_ip_on_paint_status(p_wnd, p_ip, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_ip_on_paint_status(p_wnd, p_ip, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_ip_on_paint_status(p_wnd, p_ip, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static void klbwnd_ip_select_octet(klb_wnd_t* p_wnd, klbwnd_ip_t* p_ip, int idx)
{
    if (idx < 0)
    {
        idx = 0;
    }

    if (idx >= KLBWND_IP_octet_max)
    {
        idx = KLBWND_IP_octet_max - 1;
    }

    p_ip->sel_idx = idx;
    klbshw_decimal_menu_ip_set_value(p_ip->p_menu, p_ip->octet[idx]);
    klb_wnd_update(p_wnd);
}

static int on_menu_popup_end_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    if (ok)
    {
        klbwnd_ip_apply_menu_value(p_ip, value);
        p_ip->sel_idx = -1;
        klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
        klb_wnd_update(p_wnd);
    }

    (void)p_wnd_menu;
    return 0;
}

static int on_menu_click_point_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_apply_menu_value(p_ip, value);

    if (p_ip->sel_idx < KLBWND_IP_octet_max - 1)
    {
        klbwnd_ip_select_octet(p_wnd, p_ip, p_ip->sel_idx + 1);
    }

    (void)p_wnd_menu;
    (void)ok;
    return 0;
}

static int on_menu_click_number_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_apply_menu_value(p_ip, value);

    if (value >= 100 || value > 25)
    {
        if (p_ip->sel_idx < KLBWND_IP_octet_max - 1)
        {
            klbwnd_ip_select_octet(p_wnd, p_ip, p_ip->sel_idx + 1);
        }
    }

    klb_wnd_update(p_wnd);

    (void)p_wnd_menu;
    (void)ok;
    return 0;
}

static int on_menu_click_moveleft_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_apply_menu_value(p_ip, value);

    if (p_ip->sel_idx > 0)
    {
        klbwnd_ip_select_octet(p_wnd, p_ip, p_ip->sel_idx - 1);
    }

    (void)p_wnd_menu;
    (void)ok;
    return 0;
}

static int on_menu_click_moveright_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_apply_menu_value(p_ip, value);

    if (p_ip->sel_idx < KLBWND_IP_octet_max - 1)
    {
        klbwnd_ip_select_octet(p_wnd, p_ip, p_ip->sel_idx + 1);
    }

    (void)p_wnd_menu;
    (void)ok;
    return 0;
}

static int on_menu_click_backspace_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    if (0 == value && p_ip->sel_idx > 0)
    {
        klbwnd_ip_apply_menu_value(p_ip, 0);
        klbwnd_ip_select_octet(p_wnd, p_ip, p_ip->sel_idx - 1);
    }
    else
    {
        klbwnd_ip_apply_menu_value(p_ip, value / 10);
        klbshw_decimal_menu_ip_set_value(p_ip->p_menu, p_ip->octet[p_ip->sel_idx]);
        klb_wnd_update(p_wnd);
    }

    (void)p_wnd_menu;
    (void)ok;
    return 0;
}

static int on_menu_click_clear_klbwnd_ip(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, int value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_apply_menu_value(p_ip, 0);
    klbshw_decimal_menu_ip_set_value(p_ip->p_menu, 0);
    klb_wnd_update(p_wnd);

    (void)p_wnd_menu;
    (void)ok;
    (void)value;
    return 0;
}

static int klbwnd_ip_on_click(klb_wnd_t* p_wnd, klbwnd_ip_t* p_ip, const klb_point_t* p_pt1)
{
    if (klb_wnd_is_disable(p_wnd) || NULL == p_ip->p_menu)
    {
        return 0;
    }

    int sel = 0;

    if (NULL != p_pt1)
    {
        for (int i = 0; i < KLBWND_IP_octet_max; ++i)
        {
            if (klb_pt_in_rect(&p_ip->octet_rect[i], p_pt1->x, p_pt1->y))
            {
                sel = i;
                break;
            }
        }
    }

    p_ip->sel_idx = sel;

    klbshw_decimal_menu_ip_set_ranges(p_ip->p_menu, 3);
    klbshw_decimal_menu_ip_set_value(p_ip->p_menu, p_ip->octet[sel]);

    klbshw_decimal_menu_ip_bind(p_ip->p_menu, on_menu_popup_end_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_point(p_ip->p_menu, on_menu_click_point_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_number(p_ip->p_menu, on_menu_click_number_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_moveleft(p_ip->p_menu, on_menu_click_moveleft_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_moveright(p_ip->p_menu, on_menu_click_moveright_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_backspace(p_ip->p_menu, on_menu_click_backspace_klbwnd_ip, p_wnd);
    klbshw_decimal_menu_ip_bind_click_clear(p_ip->p_menu, on_menu_click_clear_klbwnd_ip, p_wnd);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_decimal_menu_ip_wh(p_ip->p_menu, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_ip->p_menu, x, y);
    klb_wnd_resize(p_ip->p_menu, menu_w, menu_h);
    klbshw_decimal_menu_ip_layout(p_ip->p_menu);

    klb_gui_popup_wnd(p_wnd->p_gui, p_ip->p_menu);

    return 0;
}

static int klbwnd_ip_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_ip_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_ip_on_click(p_wnd, p_ip, p_pt1);

    default:
        break;
    }

    (void)p_pt2;
    (void)lparam;
    (void)wparam;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 私有函数

static void klbwnd_ip_limit_octet(int* p_v)
{
    if (*p_v < 0)
    {
        *p_v = 0;
    }

    if (255 < *p_v)
    {
        *p_v = 255;
    }
}

static void klbwnd_ip_sync_value(klbwnd_ip_t* p_ip)
{
    char buf[32] = { 0 };
    snprintf(buf, sizeof(buf) - 1, "%d.%d.%d.%d", p_ip->octet[0], p_ip->octet[1], p_ip->octet[2], p_ip->octet[3]);
    p_ip->value = klb_sds_assign(p_ip->value, buf);
}

static void klbwnd_ip_parse_value(klbwnd_ip_t* p_ip, const char* p_value)
{
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;

    if (NULL != p_value && 4 == sscanf(p_value, "%d.%d.%d.%d", &a, &b, &c, &d))
    {
        p_ip->octet[0] = a;
        p_ip->octet[1] = b;
        p_ip->octet[2] = c;
        p_ip->octet[3] = d;
    }

    for (int i = 0; i < KLBWND_IP_octet_max; ++i)
    {
        klbwnd_ip_limit_octet(&p_ip->octet[i]);
    }

    klbwnd_ip_sync_value(p_ip);
}

static void klbwnd_ip_apply_menu_value(klbwnd_ip_t* p_ip, int value)
{
    if (p_ip->sel_idx < 0 || p_ip->sel_idx >= KLBWND_IP_octet_max)
    {
        return;
    }

    klbwnd_ip_limit_octet(&value);
    p_ip->octet[p_ip->sel_idx] = value;
    klbwnd_ip_sync_value(p_ip);
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_ip_set_css(klb_wnd_t* p_wnd, klbwnd_ip_css_t* p_css)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    p_ip->p_css = p_css;
}

void klbwnd_ip_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_parse_value(p_ip, p_value);
}

const sds klbwnd_ip_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    return p_ip->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_ip_init_attribute(klbwnd_ip_t* p_ip)
{
    p_ip->value = NULL;
    p_ip->sel_idx = -1;
    p_ip->p_menu = NULL;

    for (int i = 0; i < KLBWND_IP_octet_max; ++i)
    {
        p_ip->octet[i] = 0;
    }

    klbwnd_ip_sync_value(p_ip);
}

static void klbwnd_ip_deinit_attribute(klbwnd_ip_t* p_ip)
{
    KLB_FREE_BY(p_ip->value, sdsfree);
    p_ip->p_menu = NULL;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_ip_attributes_init(klbwnd_ip_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_ip_attributes_deinit(klbwnd_ip_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_ip_attributes_copy(klbwnd_ip_attributes_t* p_dst, const klbwnd_ip_attributes_t* p_src)
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

void klbwnd_ip_css_init(klbwnd_ip_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_ip_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_ip_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_ip_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_ip_css_deinit(klbwnd_ip_css_t* p_css)
{
    klbwnd_ip_attributes_deinit(&p_css->normal);
    klbwnd_ip_attributes_deinit(&p_css->focus);
    klbwnd_ip_attributes_deinit(&p_css->disable);
}

void klbwnd_ip_css_copy(klbwnd_ip_css_t* p_dst, klbwnd_ip_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_ip_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_ip_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_ip_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_ip_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_ip_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    klbwnd_ip_init_attribute(p_ip);

    p_ip->p_menu = klbui_shwnd_get_decimal_menu_ip(p_gui);
}

void klbwnd_ip_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_ip_t* p_ip = (klbwnd_ip_t*)p_wnd->ctrl;

    klbwnd_ip_deinit_attribute(p_ip);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_ip_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_ip_t));

    klbwnd_ip_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_ip_destroy;

    return p_wnd;
}

// end
