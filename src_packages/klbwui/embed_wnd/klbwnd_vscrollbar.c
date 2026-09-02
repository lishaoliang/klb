// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_vscrollbar.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbwui/core/klbwui_css.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_vscrollbar_relayout(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc);
static void klbwnd_vscrollbar_update_value(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int value);
static int on_mousewheel_klbwnd_vscrollbar(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int lparam);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_vscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_vscrollbar_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_vscrollbar_css_t* p_css = p_vsc->p_css;
    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_vscrollbar_attributes_t* p_attr = &p_css->normal;
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

static int klbwnd_vscrollbar_on_click(klb_wnd_t* p_wnd, klbwnd_vscrollbar_t* p_vsc, const klb_point_t* p_pt1)
{
    if (NULL == p_pt1)
    {
        return 0;
    }

    int y1 = p_vsc->p_up->pos.rect_in_parent.h;
    int y2 = p_vsc->p_down->pos.rect_in_parent.h;
    int h = p_wnd->pos.rect_in_parent.h;
    int offy = p_pt1->y - p_wnd->pos.rect_in_canvas.y;
    int span = h - y1 - y2;

    if (y1 < offy && offy < h - y2 && 0 < span)
    {
        int v = p_vsc->min + (offy - y1) * (p_vsc->max - p_vsc->min + 1) / span;
        klbwnd_vscrollbar_update_value(p_wnd, p_vsc, v);
    }

    return 0;
}

static int klbwnd_vscrollbar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt2;
    (void)wparam;

    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_vscrollbar_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_vscrollbar_on_click(p_wnd, p_vsc, p_pt1);

    case KLBUI_mousewheel:
        return on_mousewheel_klbwnd_vscrollbar(p_wnd, p_vsc, lparam);

    case KLBUI_onparsewindow:
        klbwnd_vscrollbar_relayout(p_wnd, p_vsc);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 值 / 布局

static void klbwnd_vscrollbar_update_value(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int value)
{
    if (value < p_vsc->min)
    {
        value = p_vsc->min;
    }

    if (p_vsc->max < value)
    {
        value = p_vsc->max;
    }

    if (value != p_vsc->value)
    {
        p_vsc->value = value;

        if (NULL != p_wnd_vsc->vtable.on_command)
        {
            klb_point_t pt = { 0, 0 };
            p_wnd_vsc->vtable.on_command(p_wnd_vsc, KLBUI_onchange, &pt, &pt, 0, 0);
        }

        klbwnd_vscrollbar_relayout(p_wnd_vsc, p_vsc);
        klb_wnd_update(p_wnd_vsc);
    }
}

static void klbwnd_vscrollbar_relayout(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc)
{
    int w = p_wnd_vsc->pos.rect_in_parent.w;
    int h = p_wnd_vsc->pos.rect_in_parent.h;
    int w_btn = w - 2;
    int h_btn = w - 2;
    int track_y = 0;
    int track_h = 0;
    int h_middle = p_vsc->h_middle;
    int sy = 0;
    int range = 0;

    if (w_btn < 1)
    {
        w_btn = 1;
    }

    if (h_btn < 1)
    {
        h_btn = 1;
    }

    klb_wnd_move(p_vsc->p_up, 1, 1);
    klb_wnd_resize(p_vsc->p_up, w_btn, h_btn);

    klb_wnd_move(p_vsc->p_down, 1, h - h_btn - 1);
    klb_wnd_resize(p_vsc->p_down, w_btn, h_btn);

    track_y = h_btn + 2;
    track_h = h - (h_btn + 2) * 2;
    if (track_h < 1)
    {
        track_h = 1;
    }

    if (h_middle > track_h)
    {
        h_middle = track_h;
    }

    if (h_middle < 8)
    {
        h_middle = 8;
        if (h_middle > track_h)
        {
            h_middle = track_h;
        }
    }

    range = p_vsc->max - p_vsc->min;
    if (p_vsc->value <= p_vsc->min)
    {
        sy = track_y;
    }
    else if (p_vsc->max <= p_vsc->value)
    {
        sy = track_y + track_h - h_middle;
    }
    else if (0 < range)
    {
        sy = track_y + (p_vsc->value - p_vsc->min) * (track_h - h_middle) / range;
    }
    else
    {
        sy = track_y;
    }

    klb_wnd_move(p_vsc->p_middle, 1, sy);
    klb_wnd_resize(p_vsc->p_middle, w_btn, h_middle);

    klb_wnd_update_canvas_rect(p_wnd_vsc);
}


//////////////////////////////////////////////////////////////////////////
// 子按钮 command

static int on_mousewheel_klbwnd_vscrollbar(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int lparam)
{
    if (!p_vsc->enable_mousewheel)
    {
        return 0;
    }

    int delta = p_vsc->step;

    if (delta < 1)
    {
        delta = 1;
    }

    if (KLBUI_MOUSEWHEEL_is_up(lparam))
    {
        klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value - delta);
        return -1;
    }
    else if (KLBUI_MOUSEWHEEL_is_down(lparam))
    {
        klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value + delta);
        return -1;
    }

    return 0;
}

static int on_command_btn_up_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)wparam;

    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_mousewheel == msg)
    {
        return on_mousewheel_klbwnd_vscrollbar(p_wnd_vsc, p_vsc, lparam);
    }

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value - p_vsc->step);
    }

    return 0;
}

static int on_command_btn_down_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)wparam;

    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_mousewheel == msg)
    {
        return on_mousewheel_klbwnd_vscrollbar(p_wnd_vsc, p_vsc, lparam);
    }

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value + p_vsc->step);
    }

    return 0;
}

static int on_command_btn_middle_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt2;

    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_mousewheel == msg)
    {
        return on_mousewheel_klbwnd_vscrollbar(p_wnd_vsc, p_vsc, lparam);
    }

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

        int h = p_wnd_vsc->pos.rect_in_parent.h - p_vsc->p_up->pos.rect_in_parent.h - p_vsc->p_down->pos.rect_in_parent.h;
        int dy = p_pt1->y - p_wnd_vsc->pos.rect_in_canvas.y - p_vsc->p_up->pos.rect_in_parent.h;
        int v = 0;

        if (dy <= 0)
        {
            v = p_vsc->min;
        }
        else if (h <= dy)
        {
            v = p_vsc->max;
        }
        else if (0 < h)
        {
            v = p_vsc->min + (p_vsc->max - p_vsc->min + 1) * dy / h;
        }
        else
        {
            v = p_vsc->min;
        }

        klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, v);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_vscrollbar_attributes_init(klbwnd_vscrollbar_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_vscrollbar_attributes_deinit(klbwnd_vscrollbar_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_vscrollbar_attributes_copy(klbwnd_vscrollbar_attributes_t* p_dst, const klbwnd_vscrollbar_attributes_t* p_src)
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

void klbwnd_vscrollbar_css_init(klbwnd_vscrollbar_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_vscrollbar_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_vscrollbar_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_vscrollbar_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_btnex_css_init(&p_css->css_btn, p_gui);

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

void klbwnd_vscrollbar_css_deinit(klbwnd_vscrollbar_css_t* p_css)
{
    klbwnd_vscrollbar_attributes_deinit(&p_css->normal);
    klbwnd_vscrollbar_attributes_deinit(&p_css->focus);
    klbwnd_vscrollbar_attributes_deinit(&p_css->disable);

    klbwnd_btnex_css_deinit(&p_css->css_btn);
}

void klbwnd_vscrollbar_css_copy(klbwnd_vscrollbar_css_t* p_dst, klbwnd_vscrollbar_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_vscrollbar_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_vscrollbar_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_vscrollbar_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_btnex_css_copy(&p_dst->css_btn, &p_src->css_btn);
}

void klbwnd_vscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_vscrollbar_css_t* p_css)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_vsc->p_css = p_css;

    if (NULL != p_css)
    {
        klbwnd_btnex_set_css(p_vsc->p_up, &p_css->css_btn);
        klbwnd_btnex_set_css(p_vsc->p_down, &p_css->css_btn);
        klbwnd_btnex_set_css(p_vsc->p_middle, &p_css->css_btn);
    }
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_vscrollbar_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    if (value < p_vsc->min)
    {
        value = p_vsc->min;
    }

    if (p_vsc->max < value)
    {
        value = p_vsc->max;
    }

    p_vsc->value = value;
    klbwnd_vscrollbar_relayout(p_wnd, p_vsc);
}

int klbwnd_vscrollbar_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    return p_vsc->value;
}

void klbwnd_vscrollbar_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_vsc->min = min;
    p_vsc->max = max;
    p_vsc->step = step;
}

int klbwnd_vscrollbar_enable_mousewheel(klb_wnd_t* p_wnd, bool b)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_vsc->enable_mousewheel = b;
    return 0;
}

int klbwnd_vscrollbar_up(klb_wnd_t* p_wnd, int v)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    int value = 0;

    if (v <= 0)
    {
        return 1;
    }

    value = p_vsc->value - v;
    if (value < p_vsc->min)
    {
        value = p_vsc->min;
    }

    if (value == p_vsc->value)
    {
        return 1;
    }

    klbwnd_vscrollbar_set_value(p_wnd, value);
    return 0;
}

int klbwnd_vscrollbar_down(klb_wnd_t* p_wnd, int v)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    int value = 0;

    if (v <= 0)
    {
        return 1;
    }

    value = p_vsc->value + v;
    if (p_vsc->max < value)
    {
        value = p_vsc->max;
    }

    if (value == p_vsc->value)
    {
        return 1;
    }

    klbwnd_vscrollbar_set_value(p_wnd, value);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

static void klbwnd_vscrollbar_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;
    int h_btn = w;

    p_vsc->p_up = klbwnd_btnex_create(p_gui, 1, 1, w - 2, h_btn);
    klbwnd_btnex_set_type(p_vsc->p_up, KLBWND_BTNEX_triangle_up);
    klb_wnd_bind_command(p_vsc->p_up, on_command_btn_up_klbwnd_vscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_vsc->p_up);

    p_vsc->p_down = klbwnd_btnex_create(p_gui, 1, h - h_btn - 1, w - 2, h_btn);
    klbwnd_btnex_set_type(p_vsc->p_down, KLBWND_BTNEX_triangle_down);
    klb_wnd_bind_command(p_vsc->p_down, on_command_btn_down_klbwnd_vscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_vsc->p_down);

    p_vsc->p_middle = klbwnd_btnex_create(p_gui, 1, h / 2, w - 2, 42);
    klbwnd_btnex_set_type(p_vsc->p_middle, KLBWND_BTNEX_rectangle);
    klb_wnd_bind_command(p_vsc->p_middle, on_command_btn_middle_klbwnd_vscrollbar, p_wnd);
    klb_wnd_push_child(p_wnd, p_vsc->p_middle);
}

void klbwnd_vscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_vscrollbar_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    p_vsc->min = 0;
    p_vsc->max = 10;
    p_vsc->step = 1;
    p_vsc->value = 0;
    p_vsc->h_middle = 42;
    p_vsc->enable_mousewheel = false;

    klbwnd_vscrollbar_init_subwnds(p_wnd);
}

void klbwnd_vscrollbar_deinit(klb_wnd_t* p_wnd)
{
    (void)p_wnd;
}

klb_wnd_t* klbwnd_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_vscrollbar_t));

    klbwnd_vscrollbar_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_vscrollbar_destroy;

    return p_wnd;
}

// end
