// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_hscrollbar.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbwui/core/klbwui_css.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_hscrollbar_relayout(klb_wnd_t* p_wnd_hsc, klbwnd_hscrollbar_t* p_hsc);
static void klbwnd_hscrollbar_update_value(klb_wnd_t* p_wnd_hsc, klbwnd_hscrollbar_t* p_hsc, int value);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_hscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_hscrollbar_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_hscrollbar_css_t* p_css = p_hsc->p_css;
    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_hscrollbar_attributes_t* p_attr = &p_css->normal;
    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        p_attr = &p_css->disable;
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        p_attr = &p_css->focus;
    }

    klbwuicss_draw_background(p_wnd, &paint_rect, &p_attr->background);
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, &paint_rect, &p_attr->border);
    }

    return 0;
}

static int klbwnd_hscrollbar_on_click(klb_wnd_t* p_wnd, klbwnd_hscrollbar_t* p_hsc, const klb_point_t* p_pt1)
{
    if (NULL == p_pt1)
    {
        return 0;
    }

    int x1 = p_hsc->p_left->pos.rect_in_parent.w;
    int x2 = p_hsc->p_right->pos.rect_in_parent.w;
    int w = p_wnd->pos.rect_in_parent.w;
    int offx = p_pt1->x - p_wnd->pos.rect_in_canvas.x;
    int span = w - x1 - x2;

    if (x1 < offx && offx < w - x2 && 0 < span)
    {
        int v = p_hsc->min + (offx - x1) * (p_hsc->max - p_hsc->min + 1) / span;
        klbwnd_hscrollbar_update_value(p_wnd, p_hsc, v);
    }

    return 0;
}

static int klbwnd_hscrollbar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_hscrollbar_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_hscrollbar_on_click(p_wnd, p_hsc, p_pt1);

    case KLBUI_onparsewindow:
        klbwnd_hscrollbar_relayout(p_wnd, p_hsc);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 值 / 布局

static void klbwnd_hscrollbar_update_value(klb_wnd_t* p_wnd_hsc, klbwnd_hscrollbar_t* p_hsc, int value)
{
    if (value < p_hsc->min)
    {
        value = p_hsc->min;
    }

    if (p_hsc->max < value)
    {
        value = p_hsc->max;
    }

    if (value != p_hsc->value)
    {
        p_hsc->value = value;

        if (NULL != p_wnd_hsc->vtable.on_command)
        {
            klb_point_t pt = { 0, 0 };
            p_wnd_hsc->vtable.on_command(p_wnd_hsc, KLBUI_onchange, &pt, &pt, 0, 0);
        }

        klbwnd_hscrollbar_relayout(p_wnd_hsc, p_hsc);
        klb_wnd_update(p_wnd_hsc);
    }
}

static void klbwnd_hscrollbar_relayout(klb_wnd_t* p_wnd_hsc, klbwnd_hscrollbar_t* p_hsc)
{
    int w = p_wnd_hsc->pos.rect_in_parent.w;
    int h = p_wnd_hsc->pos.rect_in_parent.h;
    int w_btn = h - 2;
    int h_btn = h - 2;
    int track_x = 0;
    int track_w = 0;
    int w_middle = p_hsc->w_middle;
    int sx = 0;
    int range = 0;

    if (w_btn < 1)
    {
        w_btn = 1;
    }

    if (h_btn < 1)
    {
        h_btn = 1;
    }

    klb_wnd_move(p_hsc->p_left, 1, 1);
    klb_wnd_resize(p_hsc->p_left, w_btn, h_btn);

    klb_wnd_move(p_hsc->p_right, w - w_btn - 1, 1);
    klb_wnd_resize(p_hsc->p_right, w_btn, h_btn);

    track_x = w_btn + 2;
    track_w = w - (w_btn + 2) * 2;
    if (track_w < 1)
    {
        track_w = 1;
    }

    if (w_middle > track_w)
    {
        w_middle = track_w;
    }

    if (w_middle < 8)
    {
        w_middle = 8;
        if (w_middle > track_w)
        {
            w_middle = track_w;
        }
    }

    range = p_hsc->max - p_hsc->min;
    if (p_hsc->value <= p_hsc->min)
    {
        sx = track_x;
    }
    else if (p_hsc->max <= p_hsc->value)
    {
        sx = track_x + track_w - w_middle;
    }
    else if (0 < range)
    {
        sx = track_x + (p_hsc->value - p_hsc->min) * (track_w - w_middle) / range;
    }
    else
    {
        sx = track_x;
    }

    klb_wnd_move(p_hsc->p_middle, sx, 1);
    klb_wnd_resize(p_hsc->p_middle, w_middle, h_btn);

    klb_wnd_update_canvas_rect(p_wnd_hsc);
}


//////////////////////////////////////////////////////////////////////////
// 子按钮 command

static int on_command_btn_left_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    klb_wnd_t* p_wnd_hsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_hsc->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klbwnd_hscrollbar_update_value(p_wnd_hsc, p_hsc, p_hsc->value - p_hsc->step);
    }

    return 0;
}

static int on_command_btn_right_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    klb_wnd_t* p_wnd_hsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_hsc->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klbwnd_hscrollbar_update_value(p_wnd_hsc, p_hsc, p_hsc->value + p_hsc->step);
    }

    return 0;
}

static int on_command_btn_middle_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt2;

    klb_wnd_t* p_wnd_hsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_hsc->ctrl;

    if (KLBUI_mousedrag == msg)
    {
        if (KLBUI_MOUSE_left != wparam)
        {
            return 0;
        }

        if (KLBUI_MOUSEDRAG_start == lparam)
        {
            return 0;
        }

        if (NULL == p_pt1)
        {
            return 0;
        }

        int span = p_wnd_hsc->pos.rect_in_parent.w - p_hsc->p_left->pos.rect_in_parent.w - p_hsc->p_right->pos.rect_in_parent.w;
        int dx = p_pt1->x - p_wnd_hsc->pos.rect_in_canvas.x - p_hsc->p_left->pos.rect_in_parent.w;
        int v = 0;

        if (dx <= 0)
        {
            v = p_hsc->min;
        }
        else if (span <= dx)
        {
            v = p_hsc->max;
        }
        else if (0 < span)
        {
            v = p_hsc->min + (p_hsc->max - p_hsc->min + 1) * dx / span;
        }
        else
        {
            v = p_hsc->min;
        }

        klbwnd_hscrollbar_update_value(p_wnd_hsc, p_hsc, v);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_hscrollbar_attributes_init(klbwnd_hscrollbar_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_hscrollbar_attributes_deinit(klbwnd_hscrollbar_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_hscrollbar_attributes_copy(klbwnd_hscrollbar_attributes_t* p_dst, const klbwnd_hscrollbar_attributes_t* p_src)
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
// css

void klbwnd_hscrollbar_css_init(klbwnd_hscrollbar_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_hscrollbar_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_hscrollbar_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_hscrollbar_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_button_css_init(&p_css->css_btn, p_gui);

    p_css->css_btn.normal.border.width.top = 0;
    p_css->css_btn.normal.border.width.right = 0;
    p_css->css_btn.normal.border.width.bottom = 0;
    p_css->css_btn.normal.border.width.left = 0;

    p_css->css_btn.focus.border.width.top = 0;
    p_css->css_btn.focus.border.width.right = 0;
    p_css->css_btn.focus.border.width.bottom = 0;
    p_css->css_btn.focus.border.width.left = 0;

    p_css->css_btn.disable.border.width.top = 0;
    p_css->css_btn.disable.border.width.right = 0;
    p_css->css_btn.disable.border.width.bottom = 0;
    p_css->css_btn.disable.border.width.left = 0;
}

void klbwnd_hscrollbar_css_deinit(klbwnd_hscrollbar_css_t* p_css)
{
    klbwnd_hscrollbar_attributes_deinit(&p_css->normal);
    klbwnd_hscrollbar_attributes_deinit(&p_css->focus);
    klbwnd_hscrollbar_attributes_deinit(&p_css->disable);

    klbwnd_button_css_deinit(&p_css->css_btn);
}

void klbwnd_hscrollbar_css_copy(klbwnd_hscrollbar_css_t* p_dst, klbwnd_hscrollbar_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_hscrollbar_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_hscrollbar_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_hscrollbar_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_button_css_copy(&p_dst->css_btn, &p_src->css_btn);
}

void klbwnd_hscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_hscrollbar_css_t* p_css)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_hsc->p_css = p_css;

    if (NULL != p_css)
    {
        klbwnd_button_set_css(p_hsc->p_left, &p_css->css_btn);
        klbwnd_button_set_css(p_hsc->p_right, &p_css->css_btn);
        klbwnd_button_set_css(p_hsc->p_middle, &p_css->css_btn);
    }
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_hscrollbar_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    if (value < p_hsc->min)
    {
        value = p_hsc->min;
    }

    if (p_hsc->max < value)
    {
        value = p_hsc->max;
    }

    p_hsc->value = value;
    klbwnd_hscrollbar_relayout(p_wnd, p_hsc);
}

int klbwnd_hscrollbar_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    return p_hsc->value;
}

void klbwnd_hscrollbar_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_hsc->min = min;
    p_hsc->max = max;
    p_hsc->step = step;
}

int klbwnd_hscrollbar_enable_mousewheel(klb_wnd_t* p_wnd, bool b)
{
    (void)p_wnd;
    (void)b;

    return 0;
}

int klbwnd_hscrollbar_left(klb_wnd_t* p_wnd, int v)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    int value = 0;

    if (v <= 0)
    {
        return 1;
    }

    value = p_hsc->value - v;
    if (value < p_hsc->min)
    {
        value = p_hsc->min;
    }

    if (value == p_hsc->value)
    {
        return 1;
    }

    klbwnd_hscrollbar_set_value(p_wnd, value);
    return 0;
}

int klbwnd_hscrollbar_right(klb_wnd_t* p_wnd, int v)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    int value = 0;

    if (v <= 0)
    {
        return 1;
    }

    value = p_hsc->value + v;
    if (p_hsc->max < value)
    {
        value = p_hsc->max;
    }

    if (value == p_hsc->value)
    {
        return 1;
    }

    klbwnd_hscrollbar_set_value(p_wnd, value);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

static void klbwnd_hscrollbar_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;
    int w_btn = h;

    p_hsc->p_left = klbwnd_button_create(p_gui, 1, 1, w_btn, h - 2);
    klbwnd_button_set_title(p_hsc->p_left, "<");
    klb_wnd_bind_command(p_hsc->p_left, on_command_btn_left_klbwnd_hscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_hsc->p_left);

    p_hsc->p_right = klbwnd_button_create(p_gui, w - w_btn - 1, 1, w_btn, h - 2);
    klbwnd_button_set_title(p_hsc->p_right, ">");
    klb_wnd_bind_command(p_hsc->p_right, on_command_btn_right_klbwnd_hscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_hsc->p_right);

    p_hsc->p_middle = klbwnd_button_create(p_gui, w / 2, 1, 42, h - 2);
    klb_wnd_bind_command(p_hsc->p_middle, on_command_btn_middle_klbwnd_hscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_hsc->p_middle);
}

void klbwnd_hscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_hscrollbar_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    p_hsc->min = 0;
    p_hsc->max = 10;
    p_hsc->step = 1;
    p_hsc->value = 0;
    p_hsc->w_middle = 42;

    klbwnd_hscrollbar_init_subwnds(p_wnd);
}

void klbwnd_hscrollbar_deinit(klb_wnd_t* p_wnd)
{
    (void)p_wnd;
}

klb_wnd_t* klbwnd_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_hscrollbar_t));

    klbwnd_hscrollbar_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_hscrollbar_destroy;

    return p_wnd;
}

// end
