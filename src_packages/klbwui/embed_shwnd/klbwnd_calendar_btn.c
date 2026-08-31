// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_calendar_btn.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include <stdio.h>
#include "klbgui/klbui_util.h"
#include "klbwui/core/klbwui_css.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_calendar_btn_deinit_attribute(klbwnd_calendar_btn_t* p_btn);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_calendar_btn_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    klbwnd_calendar_btn_deinit(p_wnd);

    KLB_FREE(p_wnd);
}


// 普通按钮: KLBWND_CAL_BTN_normal
static void klbwnd_calendar_btn_on_paint_status_normal(klb_wnd_t* p_wnd, klbwnd_calendar_btn_t* p_btn, klbwnd_calendar_btn_css_t* p_css, klbwnd_calendar_btn_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    // 标题文本
    klbuicssex_draw_text(p_wnd, p_btn->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

// 向前(时间减小) : KLBWND_CAL_BTN_prev
static void klbwnd_calendar_btn_on_paint_status_prev(klb_wnd_t* p_wnd, klbwnd_calendar_btn_t* p_btn, klbwnd_calendar_btn_css_t* p_css, klbwnd_calendar_btn_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

        // 向左 三角
        klb_rect_t src = *p_rect;
        src.w = src.w * 2 / 3;
        src.h = src.h * 2 / 3;
        src.x = src.x + (p_rect->w - src.w) / 2;
        src.y = src.y + (p_rect->h - src.h) / 2;

        klbuiutil_draw_triangle_left(p_wnd, &src, p_attr->text.color);
    }
}

// 向后(时间增大) : KLBWND_CAL_BTN_next
static void klbwnd_calendar_btn_on_paint_status_next(klb_wnd_t* p_wnd, klbwnd_calendar_btn_t* p_btn, klbwnd_calendar_btn_css_t* p_css, klbwnd_calendar_btn_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

        // 向右 三角
        klb_rect_t src = *p_rect;
        src.w = src.w * 2 / 3;
        src.h = src.h * 2 / 3;
        src.x = src.x + (p_rect->w - src.w) / 2;
        src.y = src.y + (p_rect->h - src.h) / 2;

        klbuiutil_draw_triangle_right(p_wnd, &src, p_attr->text.color);
    }
}

static void klbwnd_calendar_btn_on_paint_status(klb_wnd_t* p_wnd, klbwnd_calendar_btn_t* p_btn, klbwnd_calendar_btn_css_t* p_css, klbwnd_calendar_btn_attributes_t* p_attr, klb_rect_t* p_rect)
{
    switch (p_btn->type)
    {
    case KLBWND_CAL_BTN_prev:
        {
            klbwnd_calendar_btn_on_paint_status_prev(p_wnd, p_btn, p_css, p_attr, p_rect);
        }
        break;
    case KLBWND_CAL_BTN_next:
        {
            klbwnd_calendar_btn_on_paint_status_next(p_wnd, p_btn, p_css, p_attr, p_rect);
        }
        break;
    default:
        {
            klbwnd_calendar_btn_on_paint_status_normal(p_wnd, p_btn, p_css, p_attr, p_rect);
        }
        break;
    }
}

static int klbwnd_calendar_btn_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_calendar_btn_css_t* p_css = p_btn->p_css;

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
        klbwnd_calendar_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_calendar_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_calendar_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_calendar_btn_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_calendar_btn_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_calendar_btn_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_btn_css_t* p_css)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    p_btn->p_css = p_css;
}

void klbwnd_calendar_btn_set_type(klb_wnd_t* p_wnd, int type)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    switch (type)
    {
    case KLBWND_CAL_BTN_prev:
        {
            p_btn->type = KLBWND_CAL_BTN_prev;
        }
        break;
    case KLBWND_CAL_BTN_next:
        {
            p_btn->type = KLBWND_CAL_BTN_next;
        }
        break;
    default:
        {
            p_btn->type = KLBWND_CAL_BTN_normal;
        }
        break;
    }
}

void klbwnd_calendar_btn_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    p_btn->title = klb_sds_assign(p_btn->title, p_title);
}

const sds klbwnd_calendar_btn_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    return p_btn->title;
}

void klbwnd_calendar_btn_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    p_btn->value = klb_sds_assign(p_btn->value, p_value);
}

const sds klbwnd_calendar_btn_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    return p_btn->value;
}

void klbwnd_calendar_btn_set_ymd(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;
    
    p_btn->year = year;
    p_btn->month = month;
    p_btn->day = day;
}

void klbwnd_calendar_btn_get_ymd(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    if (p_year) { *p_year = p_btn->year; };
    if (p_month) { *p_month = p_btn->month; };
    if (p_day) { *p_day = p_btn->day; };
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_calendar_btn_init_attribute(klbwnd_calendar_btn_t* p_btn)
{
    p_btn->type = KLBWND_CAL_BTN_normal;

    p_btn->title = NULL;
    p_btn->value = NULL;

    p_btn->year = KLBWND_CAL_YEAR_min;
    p_btn->month = 1;
    p_btn->day = 1;
}

static void klbwnd_calendar_btn_deinit_attribute(klbwnd_calendar_btn_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE_BY(p_btn->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_calendar_btn_attributes_init(klbwnd_calendar_btn_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_calendar_btn_attributes_deinit(klbwnd_calendar_btn_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_calendar_btn_attributes_copy(klbwnd_calendar_btn_attributes_t* p_dst, const klbwnd_calendar_btn_attributes_t* p_src)
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

void klbwnd_calendar_btn_css_init(klbwnd_calendar_btn_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_calendar_btn_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_calendar_btn_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_calendar_btn_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_center;
    p_css->focus.text.align = KLBUICSS_text1_center;
    p_css->disable.text.align = KLBUICSS_text1_center;
}

void klbwnd_calendar_btn_css_deinit(klbwnd_calendar_btn_css_t* p_css)
{
    klbwnd_calendar_btn_attributes_deinit(&p_css->normal);
    klbwnd_calendar_btn_attributes_deinit(&p_css->focus);
    klbwnd_calendar_btn_attributes_deinit(&p_css->disable);
}

void klbwnd_calendar_btn_css_copy(klbwnd_calendar_btn_css_t* p_dst, klbwnd_calendar_btn_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_calendar_btn_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_calendar_btn_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_calendar_btn_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_calendar_btn_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                               // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_calendar_btn_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                            // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                                // 由继承者重写
    p_wnd->vtable.on_get = NULL;                                // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化属性
    klbwnd_calendar_btn_init_attribute(p_btn);
}

void klbwnd_calendar_btn_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_btn_t* p_btn = (klbwnd_calendar_btn_t*)p_wnd->ctrl;

    klbwnd_calendar_btn_deinit_attribute(p_btn);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_calendar_btn_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_calendar_btn_t));

    klbwnd_calendar_btn_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_calendar_btn_destroy;

    return p_wnd;
}

// end
