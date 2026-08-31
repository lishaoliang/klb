// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_calendar.h"
#include "klbwui/embed_shwnd/klbwnd_calendar_btn.h"
#include "klbwui/embed_shwnd/klbwnd_calendar_years.h"
#include "klbwui/embed_shwnd/klbwnd_calendar_months.h"
#include "klbwui/embed_shwnd/klbwnd_calendar_days.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_datetime.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include <stdio.h>
#include "klbgui/klbui_util.h"
#include "klbwui/core/klbwui_css.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_calendar_deinit_attribute(klbwnd_calendar_t* p_cal);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_calendar_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    klbwnd_calendar_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_calendar_on_paint_status(klb_wnd_t* p_wnd, klbwnd_calendar_t* p_cal, klbwnd_calendar_css_t* p_css, klbwnd_calendar_attributes_t* p_attr, klb_rect_t* p_rect)
{
    (void)p_cal;
    (void)p_css;

    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static int klbwnd_calendar_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_calendar_css_t* p_css = p_cal->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_calendar_on_paint_status(p_wnd, p_cal, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_calendar_on_paint_status(p_wnd, p_cal, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_calendar_on_paint_status(p_wnd, p_cal, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int klbwnd_calendar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpredraw:
        break;
    case KLBUI_onpaint:
        return klbwnd_calendar_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数 

static void klbwnd_calendar_update_views(klb_wnd_t* p_wnd_calendar, klbwnd_calendar_t* p_cal, bool refresh)
{
    klbwnd_calendar_years_set_date_page(p_cal->p_years, &p_cal->date_page);
    klbwnd_calendar_months_set_date_page(p_cal->p_months, &p_cal->date_page);
    klbwnd_calendar_days_set_date_page(p_cal->p_days, &p_cal->date_page);

    char str[128] = { 0 };
    if (KLBWND_CAL_PAGE_years == p_cal->cur_page)
    {
        int year_min = 0, year_max = 0;
        klbwnd_calendar_years_get_year_ranges(p_cal->p_years, &year_min, &year_max);

        snprintf(str, sizeof(str) - 1, "%d - %d", year_min, year_max);
    }
    else if (KLBWND_CAL_PAGE_months == p_cal->cur_page)
    {
        snprintf(str, sizeof(str) - 1, "%d", p_cal->date_page.year);
    }
    else
    {
        snprintf(str, sizeof(str) - 1, "%d/%d", p_cal->date_page.year, p_cal->date_page.month);
    }

    klbwnd_calendar_btn_set_title(p_cal->p_btn_ymd, str);

    if (refresh)
    {
        klb_wnd_update(p_wnd_calendar);
    }
}


static void klbwnd_calendar_change_page(klb_wnd_t* p_wnd_calendar, klbwnd_calendar_t* p_cal, int page)
{
    if (page == p_cal->cur_page)
    {
        return;
    }

    if (KLBWND_CAL_PAGE_years == page)
    {
        p_cal->cur_page = KLBWND_CAL_PAGE_years;

        klb_wnd_show(p_cal->p_years, true);
        klb_wnd_show(p_cal->p_months, false);
        klb_wnd_show(p_cal->p_days, false);
    }
    else if (KLBWND_CAL_PAGE_months == page)
    {
        p_cal->cur_page = KLBWND_CAL_PAGE_months;

        klb_wnd_show(p_cal->p_years, false);
        klb_wnd_show(p_cal->p_months, true);
        klb_wnd_show(p_cal->p_days, false);
    }
    else
    {
        p_cal->cur_page = KLBWND_CAL_PAGE_days;

        klb_wnd_show(p_cal->p_years, false);
        klb_wnd_show(p_cal->p_months, false);
        klb_wnd_show(p_cal->p_days, true);
    }

    // 更新显示
    klbwnd_calendar_update_views(p_wnd_calendar, p_cal, true);
}

// 点击 年月日 切换按钮
static int on_command_btn_ymd_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            int page = p_cal->cur_page;
            if (KLBWND_CAL_PAGE_days == p_cal->cur_page)
            {
                page = KLBWND_CAL_PAGE_months;
            }
            else if (KLBWND_CAL_PAGE_months == p_cal->cur_page)
            {
                page = KLBWND_CAL_PAGE_years;
            }

            klbwnd_calendar_change_page(p_wnd_calendar, p_cal, page);
        }
        break;
    default:
        break;
    }

    return 0;
}

// 点击 向前 按钮
static int on_command_btn_prev_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            if (KLBWND_CAL_PAGE_years == p_cal->cur_page)
            {
                // 切换年范围
                klbwnd_calendar_set_date_page(p_wnd_calendar, p_cal->date_page.year - 16, p_cal->date_page.month, 1);
            }
            else if (KLBWND_CAL_PAGE_months == p_cal->cur_page)
            {
                // 切换年
                klbwnd_calendar_set_date_page(p_wnd_calendar, p_cal->date_page.year - 1, p_cal->date_page.month, 1);
            }
            else
            {
                // 切换月
                int prev_year = 0, prev_month = 0;
                klbui_prev_month(p_cal->date_page.year, p_cal->date_page.month, &prev_year, &prev_month);

                klbwnd_calendar_set_date_page(p_wnd_calendar, prev_year, prev_month, 1);
            }

            // 更新显示
            klbwnd_calendar_update_views(p_wnd_calendar, p_cal, true);
        }
        break;
    default:
        break;
    }

    return 0;
}

// 点击 向后 按钮
static int on_command_btn_next_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            if (KLBWND_CAL_PAGE_years == p_cal->cur_page)
            {
                // 切换年范围
                klbwnd_calendar_set_date_page(p_wnd_calendar, p_cal->date_page.year + 16, p_cal->date_page.month, 1);
            }
            else if (KLBWND_CAL_PAGE_months == p_cal->cur_page)
            {
                // 切换年
                klbwnd_calendar_set_date_page(p_wnd_calendar, p_cal->date_page.year + 1, p_cal->date_page.month, 1);
            }
            else
            {
                // 切换月
                int next_year = 0, next_month = 0;
                klbui_next_month(p_cal->date_page.year, p_cal->date_page.month, &next_year, &next_month);

                klbwnd_calendar_set_date_page(p_wnd_calendar, next_year, next_month, 1);
            }

            // 更新显示
            klbwnd_calendar_update_views(p_wnd_calendar, p_cal, true);
        }
        break;
    default:
        break;
    }

    return 0;
}

// 点击 年页面的 按钮
static int on_command_btn_years_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            // 获取年份
            int year = 0;
            klbwnd_calendar_btn_get_ymd(p_wnd, &year, NULL, NULL);
            klbwnd_calendar_set_date_page(p_wnd_calendar, year, p_cal->date_page.month, p_cal->date_page.day);

            // 显示月份
            klbwnd_calendar_change_page(p_wnd_calendar, p_cal, KLBWND_CAL_PAGE_months);
        }
        break;
    default:
        break;
    }

    return 0;
}

// 点击 月页面的 按钮
static int on_command_btn_months_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            // 获取年月, 可能夸年了
            int year = 0, month = 0;
            klbwnd_calendar_btn_get_ymd(p_wnd, &year, &month, NULL);
            klbwnd_calendar_set_date_page(p_wnd_calendar, year, month, p_cal->date_page.day);

            klbwnd_calendar_change_page(p_wnd_calendar, p_cal, KLBWND_CAL_PAGE_days);
        }
        break;
    default:
        break;
    }

    return 0;
}

// 点击了 日页面的 按钮
static int on_command_btn_days_klbwnd_calendar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_calendar = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd_calendar->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            // 获取年月日, 可能夸年夸月了
            int year = 0, month = 0, day = 0;
            klbwnd_calendar_btn_get_ymd(p_wnd, &year, &month, &day);
            //klbwnd_calendar_set_date_page(p_wnd_calendar, year, month, day);

            bool change = false;
            if (year != p_cal->date.year || month != p_cal->date.month || day != p_cal->date.day)
            {
                change = true;
            }

            klbwnd_calendar_set_date(p_wnd_calendar, year, month, day);

            // 内容变更事件
            if (change)
            {
                if (p_wnd_calendar->vtable.on_command)
                {
                    klb_point_t pt = { 0 };
                    p_wnd_calendar->vtable.on_command(p_wnd_calendar, KLBUI_onchange, &pt, &pt, 0, 0);
                }
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_calendar_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    p_cal->p_css = p_css;

    if (NULL != p_css)
    {
        klbwnd_calendar_btn_set_css(p_cal->p_btn_ymd, &p_css->css_btn);
        klbwnd_calendar_btn_set_css(p_cal->p_btn_prev, &p_css->css_btn);
        klbwnd_calendar_btn_set_css(p_cal->p_btn_next, &p_css->css_btn);

        klbwnd_calendar_years_set_css(p_cal->p_years, p_css);
        klbwnd_calendar_months_set_css(p_cal->p_months, p_css);
        klbwnd_calendar_days_set_css(p_cal->p_days, p_css);
    }
    else
    {
        klbwnd_calendar_btn_set_css(p_cal->p_btn_ymd, NULL);
        klbwnd_calendar_btn_set_css(p_cal->p_btn_prev, NULL);
        klbwnd_calendar_btn_set_css(p_cal->p_btn_next, NULL);

        klbwnd_calendar_years_set_css(p_cal->p_years, NULL);
        klbwnd_calendar_months_set_css(p_cal->p_months, NULL);
        klbwnd_calendar_days_set_css(p_cal->p_days, NULL);
    }
}

void klbwnd_calendar_limit(int* p_year, int* p_month, int* p_day)
{
    if (*p_year < KLBWND_CAL_YEAR_min)
    {
        *p_year = KLBWND_CAL_YEAR_min;
        *p_month = 1;
        *p_day = 1;
    }
    else if (KLBWND_CAL_YEAR_max < *p_year)
    {
        *p_year = KLBWND_CAL_YEAR_max;
        *p_month = 1;
        *p_day = 1;
    }
    else
    {
        if (*p_month < 1) { *p_month = 1; };
        if (12 < *p_month) { *p_month = 12; };

        int days = klbui_month_days(*p_year, *p_month);

        if (*p_day < 1) { *p_day = 1; };
        if (days < *p_day) { *p_day = days; };
    }
}

void klbwnd_calendar_set_date(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    klbwnd_calendar_limit(&year, &month, &day);

    p_cal->date.year = year;
    p_cal->date.month = month;
    p_cal->date.day = day;
}

void klbwnd_calendar_get_date(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    if (p_year) { *p_year = p_cal->date.year; };
    if (p_month) { *p_month = p_cal->date.month; };
    if (p_day) { *p_day = p_cal->date.day; };
}

void klbwnd_calendar_set_date_page(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    klbwnd_calendar_limit(&year, &month, &day);

    p_cal->date_page.year = year;
    p_cal->date_page.month = month;
    p_cal->date_page.day = day;

    klbwnd_calendar_update_views(p_wnd, p_cal, false);
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_calendar_init_attribute(klbwnd_calendar_t* p_cal)
{
    p_cal->cur_page = KLBWND_CAL_PAGE_days;

    p_cal->date_page.year = KLBWND_CAL_YEAR_min;
    p_cal->date_page.month = 1;
    p_cal->date_page.day = 1;

    p_cal->date.year = KLBWND_CAL_YEAR_min;
    p_cal->date.month = 1;
    p_cal->date.day = 1;
}

static void klbwnd_calendar_deinit_attribute(klbwnd_calendar_t* p_cal)
{

}

//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_calendar_attributes_init(klbwnd_calendar_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_calendar_attributes_deinit(klbwnd_calendar_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_calendar_attributes_copy(klbwnd_calendar_attributes_t* p_dst, const klbwnd_calendar_attributes_t* p_src)
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
// css init / quit

void klbwnd_calendar_css_init(klbwnd_calendar_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_calendar_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_calendar_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_calendar_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_calendar_btn_css_init(&p_css->css_btn, p_gui);
    klbwnd_static_css_init(&p_css->css_sta, p_gui);
}

void klbwnd_calendar_css_deinit(klbwnd_calendar_css_t* p_css)
{
    klbwnd_calendar_attributes_deinit(&p_css->normal);
    klbwnd_calendar_attributes_deinit(&p_css->focus);
    klbwnd_calendar_attributes_deinit(&p_css->disable);

    klbwnd_calendar_btn_css_deinit(&p_css->css_btn);
    klbwnd_static_css_deinit(&p_css->css_sta);
}

void klbwnd_calendar_css_copy(klbwnd_calendar_css_t* p_dst, klbwnd_calendar_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_calendar_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_calendar_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_calendar_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_calendar_btn_css_copy(&p_dst->css_btn, &p_src->css_btn);
    klbwnd_static_css_copy(&p_dst->css_sta, &p_src->css_sta);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_calendar_relayout(klb_wnd_t* p_wnd_cal, klbwnd_calendar_t* p_cal)
{
    // 重新布局
    int w = p_wnd_cal->pos.rect_in_parent.w;
    int h = p_wnd_cal->pos.rect_in_parent.h;

    int top_y = 1;
    int left_x = 1;
    int sx = left_x;
    int sy = top_y;

    // 第一排, 按钮
    int h1 = 42;
    int w1 = w - left_x - 1;

    {
        int item_h = 32;
        int item_w = 160;

        // 切换年月日
        klb_wnd_move(p_cal->p_btn_ymd, sx, sy + (h1 - item_h) / 2);
        klb_wnd_resize(p_cal->p_btn_ymd, item_w, item_h);

        item_w = 42;
        int item_off_x = 4;

        // 向后
        klb_wnd_move(p_cal->p_btn_next, w1 - item_w, sy + (h1 - item_h) / 2);
        klb_wnd_resize(p_cal->p_btn_next, item_w, item_h);

        // 向前
        klb_wnd_move(p_cal->p_btn_prev, w1 - item_w * 2 - item_off_x, sy + (h1 - item_h) / 2);
        klb_wnd_resize(p_cal->p_btn_prev, item_w, item_h);
    }

    // 第二排
    sx = left_x;
    sy += h1;
    int w2 = w - left_x - 1;
    int h2 = h - sy - 1;

    {
        klb_wnd_move(p_cal->p_years, sx, sy);
        klb_wnd_resize(p_cal->p_years, w2, h2);
        klbwnd_calendar_years_relayout(p_cal->p_years);

        klb_wnd_move(p_cal->p_months, sx, sy);
        klb_wnd_resize(p_cal->p_months, w2, h2);
        klbwnd_calendar_months_relayout(p_cal->p_months);

        klb_wnd_move(p_cal->p_days, sx, sy);
        klb_wnd_resize(p_cal->p_days, w2, h2);
        klbwnd_calendar_days_relayout(p_cal->p_days);
    }

}

static int klbwnd_calendar_init_subviews(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    // 顶部
    {
        // 左上角 年/月/日 切换按钮
        p_cal->p_btn_ymd = klbwnd_calendar_btn_create(p_gui, 0, 0, 120, 32);
        klb_wnd_push_child(p_wnd, p_cal->p_btn_ymd);
        klb_wnd_bind_command(p_cal->p_btn_ymd, on_command_btn_ymd_klbwnd_calendar, p_wnd);

        // 右上角 向前
        p_cal->p_btn_prev = klbwnd_calendar_btn_create(p_gui, 0, 0, 32, 32);
        klbwnd_calendar_btn_set_type(p_cal->p_btn_prev, KLBWND_CAL_BTN_prev);
        klb_wnd_push_child(p_wnd, p_cal->p_btn_prev);
        klb_wnd_bind_command(p_cal->p_btn_prev, on_command_btn_prev_klbwnd_calendar, p_wnd);

        // 右上角 向后
        p_cal->p_btn_next = klbwnd_calendar_btn_create(p_gui, 0, 0, 32, 32);
        klbwnd_calendar_btn_set_type(p_cal->p_btn_next, KLBWND_CAL_BTN_next);
        klb_wnd_push_child(p_wnd, p_cal->p_btn_next);
        klb_wnd_bind_command(p_cal->p_btn_next, on_command_btn_next_klbwnd_calendar, p_wnd);
    }

    // 年
    {
        p_cal->p_years = klbwnd_calendar_years_create(p_gui, 0, 0, 200, 200);
        klb_wnd_push_child(p_wnd, p_cal->p_years);
        klb_wnd_show(p_cal->p_years, false);

        // 绑定响应
        for (int m = 0; m < KLBWND_CAL_YEARS_row; m++)
        {
            for (int n = 0; n < KLBWND_CAL_YEARS_column; n++)
            {
                klbwnd_calendar_years_t* p_years = KLB_WIDGETS_PTR(p_cal->p_years, klbwnd_calendar_years_t);

                klb_wnd_bind_command(p_years->p_btns[m][n], on_command_btn_years_klbwnd_calendar, p_wnd);
            }
        }
    }

    // 月
    {
        p_cal->p_months = klbwnd_calendar_months_create(p_gui, 0, 0, 200, 200);
        klb_wnd_push_child(p_wnd, p_cal->p_months);
        klb_wnd_show(p_cal->p_months, false);

        // 绑定响应
        for (int m = 0; m < KLBWND_CAL_MONTHS_row; m++)
        {
            for (int n = 0; n < KLBWND_CAL_MONTHS_column; n++)
            {
                klbwnd_calendar_months_t* p_months = KLB_WIDGETS_PTR(p_cal->p_months, klbwnd_calendar_months_t);

                klb_wnd_bind_command(p_months->p_btns[m][n], on_command_btn_months_klbwnd_calendar, p_wnd);
            }
        }
    }

    // 日
    {
        p_cal->p_days = klbwnd_calendar_days_create(p_gui, 0, 0, 200, 200);
        klb_wnd_push_child(p_wnd, p_cal->p_days);
        klb_wnd_show(p_cal->p_days, true);

        // 绑定响应
        for (int m = 0; m < KLBWND_CAL_DAYS_row - 1; m++)
        {
            for (int n = 0; n < KLBWND_CAL_DAYS_column; n++)
            {
                klbwnd_calendar_days_t* p_days = KLB_WIDGETS_PTR(p_cal->p_days, klbwnd_calendar_days_t);

                klb_wnd_bind_command(p_days->p_btns[m * KLBWND_CAL_DAYS_column + n], on_command_btn_days_klbwnd_calendar, p_wnd);
            }
        }
    }

    // 重新布局
    klbwnd_calendar_relayout(p_wnd, p_cal);

    // 初始化, 更新显示
    klbwnd_calendar_update_views(p_wnd, p_cal, false);

    return 0;
}

void klbwnd_calendar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_calendar_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 自身属性
    klbwnd_calendar_init_attribute(p_cal);

    // 子控件
    klbwnd_calendar_init_subviews(p_wnd);
}

void klbwnd_calendar_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_t* p_cal = (klbwnd_calendar_t*)p_wnd->ctrl;

    klbwnd_calendar_deinit_attribute(p_cal);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_calendar_t));

    klbwnd_calendar_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_calendar_destroy;

    return p_wnd;
}

// end
