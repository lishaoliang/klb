// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_time.h"
#include "klbwui/embed_shwnd/klbshw_decimal_menu.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_datetime.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_time_deinit_attribute(klbwnd_time_t* p_time);
static void klbwnd_time_relayout(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klb_rect_t* p_rect, int font_h);
static void klbwnd_time_limit(int* p_value, int min, int max);
static void sync_title_klbwnd_time(klb_wnd_t* p_wnd, klbwnd_time_t* p_time);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_time_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_time_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_time_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_time_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static void klbwnd_time_on_paint_status(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klbwnd_time_css_t* p_css, klbwnd_time_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klb_rect_t rect = *p_rect;
    rect.x += p_css->padding.left;
    rect.y += p_css->padding.top;
    rect.w -= (p_css->padding.left + p_css->padding.right);
    rect.h -= (p_css->padding.top + p_css->padding.bottom);

    rect.x += p_attr->border.width.left;
    rect.y += p_attr->border.width.top;
    rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
    rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

    klbwnd_time_relayout(p_wnd, p_time, &rect, p_attr->font.size);

    klbwnd_time_draw_background_and_border(p_wnd, p_attr, p_rect);

    for (int i = 0; i < 2; ++i)
    {
        klbwnd_time_region_t* p_region = &p_time->colon[i];
        if (NULL != p_region->title && 0 < sdslen(p_region->title))
        {
            klbuicssex_draw_text(p_wnd, p_region->title, &p_region->rect, NULL, NULL, &p_attr->text, &p_attr->font);
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        klbwnd_time_region_t* p_region = &p_time->hms[i];
        if (NULL != p_region->title && 0 < sdslen(p_region->title))
        {
            klbuicssex_draw_text(p_wnd, p_region->title, &p_region->rect, NULL, NULL, &p_attr->text, &p_attr->font);
        }
    }

    if (NULL != p_time->apm.title && 0 < sdslen(p_time->apm.title) && 0 < p_time->apm.rect.w)
    {
        klbuicssex_draw_text(p_wnd, p_time->apm.title, &p_time->apm.rect, NULL, NULL, &p_attr->text, &p_attr->font);
    }
}

static int klbwnd_time_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_time_css_t* p_css = p_time->p_css;

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
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_popup_end_klbwnd_time(void* ptr, klb_wnd_t* p_wnd_dec, bool ok, int value)
{
    klb_wnd_t* p_wnd_time = (klb_wnd_t*)ptr;
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd_time->ctrl;

    if (ok)
    {
        if (KLBWND_TIME_idx_hour == p_time->sel_idx)
        {
            klbwnd_time_limit(&value, 0, 23);
            p_time->hour = value;
        }
        else if (KLBWND_TIME_idx_minute == p_time->sel_idx)
        {
            klbwnd_time_limit(&value, 0, 59);
            p_time->minute = value;
        }
        else
        {
            klbwnd_time_limit(&value, 0, 59);
            p_time->second = value;
        }

        sync_title_klbwnd_time(p_wnd_time, p_time);
        klb_wnd_call_command(p_wnd_time, KLBUI_onchange, NULL, NULL, 0, 0);
        klb_wnd_update(p_wnd_time);
    }

    (void)p_wnd_dec;
    return 0;
}

static int klbwnd_time_on_click(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, const klb_point_t* p_pt1)
{
    if (klb_wnd_is_disable(p_wnd) || NULL == p_time->p_decimal || NULL == p_pt1)
    {
        return 0;
    }

    klb_rect_t rect_pt = { 0 };
    int sel = -1;

    for (int i = 0; i < 3; ++i)
    {
        if (klb_pt_in_rect(&p_time->hms[i].rect, p_pt1->x, p_pt1->y))
        {
            sel = i;
            rect_pt = p_time->hms[i].rect;
            break;
        }
    }

    if (sel < 0)
    {
        return 0;
    }

    p_time->sel_idx = sel;

    int value = p_time->second;
    if (KLBWND_TIME_idx_hour == sel)
    {
        value = p_time->hour;
    }
    else if (KLBWND_TIME_idx_minute == sel)
    {
        value = p_time->minute;
    }

    klbshw_decimal_menu_bind(p_time->p_decimal, on_popup_end_klbwnd_time, p_wnd);
    klbshw_decimal_menu_set_value(p_time->p_decimal, value);
    klbshw_decimal_menu_set_ranges(p_time->p_decimal, 2);
    klbshw_decimal_menu_layout(p_time->p_decimal);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_decimal_menu_wh(p_time->p_decimal, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect_pt.x + menu_w <= screen_w) ? rect_pt.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_time->p_decimal, x, y);
    klb_wnd_resize(p_time->p_decimal, menu_w, menu_h);

    klb_gui_popup_wnd(p_wnd->p_gui, p_time->p_decimal);

    return 0;
}

static int klbwnd_time_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_time_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_time_on_click(p_wnd, p_time, p_pt1);

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

static int region_text_w(klb_wnd_t* p_wnd, sds title, int font_h)
{
    int w = 0;

    if (NULL != title && 0 < sdslen(title))
    {
        klb_wnd_text_size2(p_wnd, title, (int)sdslen(title), font_h, &w, NULL);
    }

    return w;
}

static void klbwnd_time_relayout(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klb_rect_t* p_rect, int font_h)
{
    int w_hour = region_text_w(p_wnd, p_time->hms[0].title, font_h);
    int w_minute = region_text_w(p_wnd, p_time->hms[1].title, font_h);
    int w_second = region_text_w(p_wnd, p_time->hms[2].title, font_h);
    int w_colon0 = region_text_w(p_wnd, p_time->colon[0].title, font_h);
    int w_colon1 = region_text_w(p_wnd, p_time->colon[1].title, font_h);
    int w_apm = region_text_w(p_wnd, p_time->apm.title, font_h);

    int w_fix = 2;
    int offx = 1;
    int sx = p_rect->x;
    int sy = p_rect->y;
    int h = p_rect->h;

    klb_rect_init(&p_time->hms[0].rect, sx, sy, w_hour + w_fix, h);
    sx += w_hour + w_fix + offx;

    klb_rect_init(&p_time->colon[0].rect, sx, sy, w_colon0 + w_fix, h);
    sx += w_colon0 + w_fix + offx;

    klb_rect_init(&p_time->hms[1].rect, sx, sy, w_minute + w_fix, h);
    sx += w_minute + w_fix + offx;

    klb_rect_init(&p_time->colon[1].rect, sx, sy, w_colon1 + w_fix, h);
    sx += w_colon1 + w_fix + offx;

    klb_rect_init(&p_time->hms[2].rect, sx, sy, w_second + w_fix, h);
    sx += w_second + w_fix + offx;

    if (0 < w_apm)
    {
        klb_rect_init(&p_time->apm.rect, sx, sy, w_apm + w_fix, h);
    }
    else
    {
        klb_rect_init(&p_time->apm.rect, sx, sy, 0, h);
    }
}

static void klbwnd_time_limit(int* p_value, int min, int max)
{
    if (*p_value < min)
    {
        *p_value = min;
    }

    if (max < *p_value)
    {
        *p_value = max;
    }
}

static sds klbwnd_time_assign_int2(sds s, int v)
{
    char str[16] = { 0 };
    snprintf(str, sizeof(str) - 1, "%02d", v);

    return klb_sds_assign(s, str);
}

static void sync_title_klbwnd_time(klb_wnd_t* p_wnd, klbwnd_time_t* p_time)
{
    int hour_show = p_time->hour;
    const char* p_apm = "";

    if (KLBUI_TIME_12H == klb_gui_get_timefmt(p_wnd->p_gui))
    {
        if (p_time->hour < 12)
        {
            p_apm = "AM";
            hour_show = (0 == p_time->hour) ? 12 : p_time->hour;
        }
        else
        {
            p_apm = "PM";
            hour_show = (12 == p_time->hour) ? 12 : (p_time->hour - 12);
        }
    }

    p_time->hms[0].title = klbwnd_time_assign_int2(p_time->hms[0].title, hour_show);
    p_time->hms[1].title = klbwnd_time_assign_int2(p_time->hms[1].title, p_time->minute);
    p_time->hms[2].title = klbwnd_time_assign_int2(p_time->hms[2].title, p_time->second);

    p_time->colon[0].title = klb_sds_assign(p_time->colon[0].title, ":");
    p_time->colon[1].title = klb_sds_assign(p_time->colon[1].title, ":");
    p_time->apm.title = klb_sds_assign(p_time->apm.title, p_apm);
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_time_set_css(klb_wnd_t* p_wnd, klbwnd_time_css_t* p_css)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    p_time->p_css = p_css;
}

void klbwnd_time_set_value(klb_wnd_t* p_wnd, int hour, int minute, int second)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    klbwnd_time_limit(&hour, 0, 23);
    klbwnd_time_limit(&minute, 0, 59);
    klbwnd_time_limit(&second, 0, 59);

    p_time->hour = hour;
    p_time->minute = minute;
    p_time->second = second;

    sync_title_klbwnd_time(p_wnd, p_time);
}

void klbwnd_time_get_value(klb_wnd_t* p_wnd, int* p_hour, int* p_minute, int* p_second)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    if (NULL != p_hour)
    {
        *p_hour = p_time->hour;
    }

    if (NULL != p_minute)
    {
        *p_minute = p_time->minute;
    }

    if (NULL != p_second)
    {
        *p_second = p_time->second;
    }
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_time_init_attribute(klb_wnd_t* p_wnd, klbwnd_time_t* p_time)
{
    p_time->sel_idx = -1;
    p_time->hour = 0;
    p_time->minute = 0;
    p_time->second = 0;

    sync_title_klbwnd_time(p_wnd, p_time);
}

static void klbwnd_time_deinit_attribute(klbwnd_time_t* p_time)
{
    for (int i = 0; i < 3; ++i)
    {
        KLB_FREE_BY(p_time->hms[i].title, sdsfree);
    }

    for (int i = 0; i < 2; ++i)
    {
        KLB_FREE_BY(p_time->colon[i].title, sdsfree);
    }

    KLB_FREE_BY(p_time->apm.title, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_time_attributes_init(klbwnd_time_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_time_attributes_deinit(klbwnd_time_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_time_attributes_copy(klbwnd_time_attributes_t* p_dst, const klbwnd_time_attributes_t* p_src)
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

void klbwnd_time_css_init(klbwnd_time_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_time_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_time_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_time_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_time_css_deinit(klbwnd_time_css_t* p_css)
{
    klbwnd_time_attributes_deinit(&p_css->normal);
    klbwnd_time_attributes_deinit(&p_css->focus);
    klbwnd_time_attributes_deinit(&p_css->disable);
}

void klbwnd_time_css_copy(klbwnd_time_css_t* p_dst, klbwnd_time_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_time_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_time_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_time_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_time_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_time_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    klbwnd_time_init_attribute(p_wnd, p_time);

    p_time->p_decimal = klbui_shwnd_get_decimal_menu(p_gui);
}

void klbwnd_time_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    klbwnd_time_deinit_attribute(p_time);
    p_time->p_decimal = NULL;
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_time_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_time_t));

    klbwnd_time_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_time_destroy;

    return p_wnd;
}

// end
