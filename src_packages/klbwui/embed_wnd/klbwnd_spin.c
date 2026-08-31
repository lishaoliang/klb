// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_spin.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_spin_deinit_attribute(klbwnd_spin_t* p_spin);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_spin_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_spin_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_spin_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_spin_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static bool klbwnd_spin_has_title(const klbwnd_spin_t* p_spin)
{
    return (NULL != p_spin->title && 0 < sdslen(p_spin->title));
}

static int klbwnd_spin_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_spin_css_t* p_css = p_spin->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_spin_attributes_t* p_attr = NULL;

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

    klbwnd_spin_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    if (klbwnd_spin_has_title(p_spin))
    {
        klbuicssex_draw_text(p_wnd, p_spin->title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }

    return 0;
}

static int klbwnd_spin_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_spin_on_paint(p_wnd);

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
// export

void klbwnd_spin_set_css(klb_wnd_t* p_wnd, klbwnd_spin_css_t* p_css)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    p_spin->p_css = p_css;
}

void klbwnd_spin_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    p_spin->title = klb_sds_assign(p_spin->title, p_title);
}

const sds klbwnd_spin_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    return p_spin->title;
}

void klbwnd_spin_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    p_spin->value = klb_sds_assign(p_spin->value, p_value);
}

const sds klbwnd_spin_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    return p_spin->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_spin_init_attribute(klbwnd_spin_t* p_spin)
{
    p_spin->title = NULL;
    p_spin->value = NULL;
}

static void klbwnd_spin_deinit_attribute(klbwnd_spin_t* p_spin)
{
    KLB_FREE_BY(p_spin->title, sdsfree);
    KLB_FREE_BY(p_spin->value, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_spin_attributes_init(klbwnd_spin_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_spin_attributes_deinit(klbwnd_spin_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_spin_attributes_copy(klbwnd_spin_attributes_t* p_dst, const klbwnd_spin_attributes_t* p_src)
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

void klbwnd_spin_css_init(klbwnd_spin_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_spin_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_spin_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_spin_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_spin_css_deinit(klbwnd_spin_css_t* p_css)
{
    klbwnd_spin_attributes_deinit(&p_css->normal);
    klbwnd_spin_attributes_deinit(&p_css->focus);
    klbwnd_spin_attributes_deinit(&p_css->disable);
}

void klbwnd_spin_css_copy(klbwnd_spin_css_t* p_dst, klbwnd_spin_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_spin_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_spin_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_spin_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_spin_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_spin_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    klbwnd_spin_init_attribute(p_spin);
}

void klbwnd_spin_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_spin_t* p_spin = (klbwnd_spin_t*)p_wnd->ctrl;

    klbwnd_spin_deinit_attribute(p_spin);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_spin_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_spin_t));

    klbwnd_spin_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_spin_destroy;

    return p_wnd;
}

// end
