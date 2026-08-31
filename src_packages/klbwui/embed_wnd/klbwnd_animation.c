// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_animation.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_animation_deinit_attribute(klbwnd_animation_t* p_ani);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_animation_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_animation_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_animation_on_paint_status(klb_wnd_t* p_wnd, klbwnd_animation_t* p_ani,
    klbwnd_animation_css_t* p_css, klbwnd_animation_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    if (NULL != p_ani->title && 0 < sdslen(p_ani->title))
    {
        klbuicssex_draw_text(p_wnd, p_ani->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }
}

static int klbwnd_animation_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_animation_css_t* p_css = p_ani->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_animation_on_paint_status(p_wnd, p_ani, p_css, &p_css->normal, &paint_rect);

    return 0;
}

static int klbwnd_animation_on_ticker(klb_wnd_t* p_wnd, klbwnd_animation_t* p_ani)
{
    int64_t tc = klb_wnd_get_tick_count(p_wnd);

    if (!p_ani->enable)
    {
        p_ani->tick_count = tc;
        return 0;
    }

    if (ABS_SUB(tc, p_ani->tick_count) < p_ani->tc_interval)
    {
        return 0;
    }

    p_ani->tick_count = tc;
    p_ani->index += 1;

    klb_wnd_update_canvas_rect(p_wnd);
    klb_wnd_call_command(p_wnd, KLBUI_onticker, NULL, NULL, 0, 0);

    return 0;
}

static int klbwnd_animation_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    if (KLBUI_onpaint == msg)
    {
        return klbwnd_animation_on_paint(p_wnd);
    }
    else if (KLBUI_onticker == msg)
    {
        return klbwnd_animation_on_ticker(p_wnd, p_ani);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_animation_set_css(klb_wnd_t* p_wnd, klbwnd_animation_css_t* p_css)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->p_css = p_css;
}

void klbwnd_animation_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->title = klb_sds_assign(p_ani->title, p_title);
}

const sds klbwnd_animation_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    return p_ani->title;
}

void klbwnd_animation_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->value = klb_sds_assign(p_ani->value, p_value);
}

const sds klbwnd_animation_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    return p_ani->value;
}

void klbwnd_animation_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->index = index;
}

int klbwnd_animation_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    return p_ani->index;
}

void klbwnd_animation_set_enable(klb_wnd_t* p_wnd, bool is_enable)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->enable = is_enable;
}

bool klbwnd_animation_get_enable(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    return p_ani->enable;
}

void klbwnd_animation_set_interval(klb_wnd_t* p_wnd, int64_t interval)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_ani->tc_interval = interval;
}

int64_t klbwnd_animation_get_interval(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    return p_ani->tc_interval;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_animation_init_attribute(klb_wnd_t* p_wnd, klbwnd_animation_t* p_ani)
{
    p_ani->title = NULL;
    p_ani->value = NULL;
    p_ani->index = 0;
    p_ani->enable = true;
    p_ani->tick_count = klb_wnd_get_tick_count(p_wnd);
    p_ani->tc_interval = 100;
}

static void klbwnd_animation_deinit_attribute(klbwnd_animation_t* p_ani)
{
    KLB_FREE_BY(p_ani->title, sdsfree);
    KLB_FREE_BY(p_ani->value, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_animation_attributes_init(klbwnd_animation_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_animation_attributes_deinit(klbwnd_animation_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_animation_attributes_copy(klbwnd_animation_attributes_t* p_dst, const klbwnd_animation_attributes_t* p_src)
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

void klbwnd_animation_css_init(klbwnd_animation_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_animation_attributes_init(&p_css->normal, &p_default->normal);
}

void klbwnd_animation_css_deinit(klbwnd_animation_css_t* p_css)
{
    klbwnd_animation_attributes_deinit(&p_css->normal);
}

void klbwnd_animation_css_copy(klbwnd_animation_css_t* p_dst, klbwnd_animation_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_animation_attributes_copy(&p_dst->normal, &p_src->normal);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_animation_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_animation_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_TICKER;

    klbwnd_animation_init_attribute(p_wnd, p_ani);
}

void klbwnd_animation_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_animation_t* p_ani = (klbwnd_animation_t*)p_wnd->ctrl;

    klbwnd_animation_deinit_attribute(p_ani);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_animation_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_animation_t));

    klbwnd_animation_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_animation_destroy;

    return p_wnd;
}

// end
