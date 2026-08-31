// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_date.h"
#include "klbwui/embed_shwnd/klbshw_calendar_menu.h"
#include "klbwui/embed_shwnd/klbwnd_calendar.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_datetime.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_date_deinit_attribute(klbwnd_date_t* p_date);
static void sync_title_klbwnd_date(klb_wnd_t* p_wnd, klbwnd_date_t* p_date);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_date_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_date_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_date_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_date_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static bool klbwnd_date_has_title(const klbwnd_date_t* p_date)
{
    return (NULL != p_date->title && 0 < sdslen(p_date->title));
}

static int klbwnd_date_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_date_css_t* p_css = p_date->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_date_attributes_t* p_attr = NULL;

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

    klbwnd_date_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    if (klbwnd_date_has_title(p_date))
    {
        klbuicssex_draw_text(p_wnd, p_date->title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }

    return 0;
}

static int on_popup_end_klbwnd_date(void* ptr, klb_wnd_t* p_wnd_cal, bool ok, int year, int month, int day)
{
    klb_wnd_t* p_wnd_date = (klb_wnd_t*)ptr;
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd_date->ctrl;

    if (ok)
    {
        p_date->year = year;
        p_date->month = month;
        p_date->day = day;

        sync_title_klbwnd_date(p_wnd_date, p_date);
        klb_wnd_call_command(p_wnd_date, KLBUI_onchange, NULL, NULL, 0, 0);
        klb_wnd_update(p_wnd_date);
    }

    (void)p_wnd_cal;
    return 0;
}

static int klbwnd_date_on_click(klb_wnd_t* p_wnd, klbwnd_date_t* p_date)
{
    if (klb_wnd_is_disable(p_wnd) || NULL == p_date->p_calendar)
    {
        return 0;
    }

    klbshw_calendar_menu_set_date(p_date->p_calendar, p_date->year, p_date->month, p_date->day);
    klbshw_calendar_menu_bind(p_date->p_calendar, on_popup_end_klbwnd_date, p_wnd);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_calendar_menu_wh(p_date->p_calendar, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_date->p_calendar, x, y);
    klb_wnd_resize(p_date->p_calendar, menu_w, menu_h);

    klb_gui_popup_wnd(p_wnd->p_gui, p_date->p_calendar);

    return 0;
}

static int klbwnd_date_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_date_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_date_on_click(p_wnd, p_date);

    default:
        break;
    }

    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 内部私有函数

static void sync_title_klbwnd_date(klb_wnd_t* p_wnd, klbwnd_date_t* p_date)
{
    char buf[32] = { 0 };
    int fmt = klb_gui_get_datefmt(p_wnd->p_gui);

    if (KLBUI_DATE_MDY1 == fmt)
    {
        snprintf(buf, sizeof(buf) - 1, "%02d/%02d/%04d", p_date->month, p_date->day, p_date->year);
    }
    else if (KLBUI_DATE_DMY1 == fmt)
    {
        snprintf(buf, sizeof(buf) - 1, "%02d/%02d/%04d", p_date->day, p_date->month, p_date->year);
    }
    else if (KLBUI_DATE_YMD2 == fmt)
    {
        snprintf(buf, sizeof(buf) - 1, "%04d-%02d-%02d", p_date->year, p_date->month, p_date->day);
    }
    else if (KLBUI_DATE_MDY2 == fmt)
    {
        snprintf(buf, sizeof(buf) - 1, "%02d-%02d-%04d", p_date->month, p_date->day, p_date->year);
    }
    else if (KLBUI_DATE_DMY2 == fmt)
    {
        snprintf(buf, sizeof(buf) - 1, "%02d-%02d-%04d", p_date->day, p_date->month, p_date->year);
    }
    else
    {
        snprintf(buf, sizeof(buf) - 1, "%04d/%02d/%02d", p_date->year, p_date->month, p_date->day);
    }

    p_date->title = klb_sds_assign(p_date->title, buf);
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_date_set_css(klb_wnd_t* p_wnd, klbwnd_date_css_t* p_css)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    p_date->p_css = p_css;
}

void klbwnd_date_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    bool ok = false;
    int year = 0;
    int month = 0;
    int day = 0;

    if (NULL != p_title)
    {
        if (3 == sscanf(p_title, "%d/%d/%d", &year, &month, &day))
        {
            ok = true;
        }
        else if (3 == sscanf(p_title, "%d-%d-%d", &year, &month, &day))
        {
            ok = true;
        }
    }

    if (ok)
    {
        klbwnd_calendar_limit(&year, &month, &day);

        p_date->year = year;
        p_date->month = month;
        p_date->day = day;

        sync_title_klbwnd_date(p_wnd, p_date);
    }
}

const sds klbwnd_date_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    return p_date->title;
}

void klbwnd_date_set_value(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    klbwnd_calendar_limit(&year, &month, &day);

    p_date->year = year;
    p_date->month = month;
    p_date->day = day;

    sync_title_klbwnd_date(p_wnd, p_date);
}

void klbwnd_date_get_value(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    if (NULL != p_year)
    {
        *p_year = p_date->year;
    }

    if (NULL != p_month)
    {
        *p_month = p_date->month;
    }

    if (NULL != p_day)
    {
        *p_day = p_date->day;
    }
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_date_init_attribute(klb_wnd_t* p_wnd, klbwnd_date_t* p_date)
{
    p_date->title = NULL;
    p_date->year = KLBWND_CAL_YEAR_min;
    p_date->month = 1;
    p_date->day = 1;

    sync_title_klbwnd_date(p_wnd, p_date);
}

static void klbwnd_date_deinit_attribute(klbwnd_date_t* p_date)
{
    KLB_FREE_BY(p_date->title, sdsfree);
    p_date->p_calendar = NULL;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_date_attributes_init(klbwnd_date_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_date_attributes_deinit(klbwnd_date_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_date_attributes_copy(klbwnd_date_attributes_t* p_dst, const klbwnd_date_attributes_t* p_src)
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

void klbwnd_date_css_init(klbwnd_date_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_date_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_date_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_date_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_date_css_deinit(klbwnd_date_css_t* p_css)
{
    klbwnd_date_attributes_deinit(&p_css->normal);
    klbwnd_date_attributes_deinit(&p_css->focus);
    klbwnd_date_attributes_deinit(&p_css->disable);
}

void klbwnd_date_css_copy(klbwnd_date_css_t* p_dst, klbwnd_date_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_date_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_date_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_date_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_date_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_date_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    klbwnd_date_init_attribute(p_wnd, p_date);

    p_date->p_calendar = klbui_shwnd_get_calendar_menu(p_gui);
}

void klbwnd_date_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    klbwnd_date_deinit_attribute(p_date);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_date_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_date_t));

    klbwnd_date_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_date_destroy;

    return p_wnd;
}

// end
