// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_wnd_in.h"
#include "klbgui/klb_gui_in.h"
#include <string.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////


int klb_wnd_push_child(klb_wnd_t* p_parent, klb_wnd_t* p_wnd)
{
    assert(NULL != p_parent);

    // 父窗口
    p_wnd->p_parent = p_parent;

    // 使用父窗口环境
    p_wnd->env = p_parent->env;

    if (NULL == p_parent->p_child)
    {
        p_parent->p_child = p_wnd;
    }
    else
    {
        klb_wnd_t* p_next = p_parent->p_child;
        while (NULL != p_next->p_next)
        {
            p_next = p_next->p_next;
        }

        assert(NULL != p_next);

        p_next->p_next = p_wnd;
        p_wnd->p_prev = p_next;
    }

    // todo.
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);
    klb_wnd_set_calculate(p_top, true);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
klb_gui_t* klb_wnd_get_gui(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    return p_top->env.p_gui;
}

klb_canvas_t* klb_wnd_get_canvas(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    return p_top->env.p_gui->p_canvas;
}

uint32_t klb_wnd_get_style(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    return p_wnd->state.style;
}

void klb_wnd_set_style(klb_wnd_t* p_wnd, uint32_t style)
{
    assert(NULL != p_wnd);
    p_wnd->state.style = style;
}

void klb_wnd_show(klb_wnd_t* p_wnd, bool show)
{
    assert(NULL != p_wnd);
    if (show)
    {
        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_HIDE);
    }
    else
    {
        p_wnd->state.status |= KLB_WND_STATUS_HIDE;
    }
}

void klb_wnd_update(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);

    klb_gui_update_rect(p_wnd->env.p_gui, &p_wnd->pos.rect_in_canvas);
}

klb_wnd_t* klb_wnd_get_top(klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_top = p_wnd;

    while (p_top && p_top->p_parent)
    {
        p_top = p_top->p_parent;
    }

    return p_top;
}

//////////////////////////////////////////////////////////////////////////


int klb_wnd_set_top(klb_wnd_t* p_wnd, klb_gui_t* p_gui)
{
    assert(NULL == p_wnd->p_parent);

    p_wnd->state.style |= KLB_WND_STYLE_TOP;
    p_wnd->env.p_gui = p_gui;

    return 0;
}

bool klb_wnd_is_top(klb_wnd_t* p_wnd)
{
    return (p_wnd->state.style & KLB_WND_STYLE_TOP) ? true : false;
}

//void klb_wnd_set_hide(klb_wnd_t* p_wnd, bool hide)
//{
//    if (hide)
//    {
//        p_wnd->state.status |= KLB_WND_STATUS_HIDE;
//    }
//    else
//    {
//        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_HIDE);
//    }
//}

void klb_wnd_set_focus(klb_wnd_t* p_wnd, bool focus)
{
    if (focus)
    {
        p_wnd->state.status |= KLB_WND_STATUS_FOCUS;
    }
    else
    {
        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_FOCUS);
    }
}

void klb_wnd_set_calculate(klb_wnd_t* p_wnd, bool calculate)
{
    if (calculate)
    {
        p_wnd->state.status |= KLB_WND_STATUS_RE_CALCULATE;
    }
    else
    {
        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_RE_CALCULATE);
    }
}

//////////////////////////////////////////////////////////////////////////

int klb_wnd_draw(klb_wnd_t* p_wnd)
{
    // todo.
    klb_wnd_try_calculate_rect(p_wnd);

    klb_wnd_on_draw(p_wnd);

    return 0;
}

int klb_wnd_on_draw(klb_wnd_t* p_wnd)
{
    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klb_wnd_on_paint(p_wnd);

    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_on_draw(p_next);
        p_next = p_next->p_next;
    }

    return 0;
}

int klb_wnd_on_paint(klb_wnd_t* p_wnd)
{
    //KLB_GUI_CTRL_PAINT

    if (p_wnd->vtable.on_control)
    {
        return p_wnd->vtable.on_control(p_wnd, KLB_GUI_CTRL_PAINT, NULL, NULL, 0, 0);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klb_wnd_try_calculate_rect(klb_wnd_t* p_wnd)
{
    if (KLB_WND_STYLE_TOP & p_wnd->state.style)
    {
        if (KLB_WND_STATUS_RE_CALCULATE & p_wnd->state.status)
        {
            klb_wnd_calculate_rect_in_canvas(p_wnd);
            klb_wnd_set_calculate(p_wnd, false);
            return 0;
        }
    }
    
    return 1;
}

static void klb_wnd_calculate_rect(klb_wnd_t* p_wnd, int offset_x, int offset_y)
{
    p_wnd->pos.rect_in_canvas.x = p_wnd->pos.rect_in_parent.x + offset_x;
    p_wnd->pos.rect_in_canvas.y = p_wnd->pos.rect_in_parent.y + offset_y;
    p_wnd->pos.rect_in_canvas.w = p_wnd->pos.rect_in_parent.w;
    p_wnd->pos.rect_in_canvas.h = p_wnd->pos.rect_in_parent.h;

    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_calculate_rect(p_next, p_wnd->pos.rect_in_canvas.x, p_wnd->pos.rect_in_canvas.y);
        p_next = p_next->p_next;
    }
}

void klb_wnd_calculate_rect_in_canvas(klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    klb_wnd_calculate_rect(p_top, 0, 0);
}

klb_wnd_t* klb_wnd_pt_in(klb_wnd_t* p_wnd, int x, int y)
{
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_t* p_in = klb_wnd_pt_in(p_next, x, y);
        if (NULL != p_in)
        {
            return p_in;
        }

        p_next = p_next->p_next;
    }

    if (klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, x, y))
    {
        return p_wnd;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// command

int klb_wnd_bind_command(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* p_obj)
{
    p_wnd->p_udata = p_obj;
    p_wnd->vtable.on_command = on_command;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// get / set

int klb_wnd_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    if (p_wnd && p_wnd->vtable.on_set)
    {
        return p_wnd->vtable.on_set(p_wnd, p_map);
    }

    return 1;
}

klb_map_t* klb_wnd_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    if (p_wnd && p_wnd->vtable.on_get)
    {
        return p_wnd->vtable.on_get(p_wnd, p_map);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// 绘图

int klb_wnd_draw_clear(klb_wnd_t* p_wnd, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_clear(p_canvas);

    return ret;
}

int klb_wnd_draw_point(klb_wnd_t* p_wnd, int x, int y, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_point(p_canvas, x, y);

    return ret;
}

int klb_wnd_draw_points(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_points(p_canvas, p_points, count);

    return ret;
}

int klb_wnd_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_line(p_canvas, x1, y1, x2, y2);

    return ret;
}

int klb_wnd_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_lines(p_canvas, p_points, count);

    return ret;
}

int klb_wnd_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_rect(p_canvas, p_rect);

    return ret;
}

int klb_wnd_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_rects(p_canvas, p_rects, count);

    return ret;
}

int klb_wnd_draw_fill_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_fill_rect(p_canvas, p_rect);

    return ret;
}

int klb_wnd_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_draw_fill_rects(p_canvas, p_rects, count, color);

    return ret;
}

int klb_wnd_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);
    ret |= klb_canvas_set_font_height(p_canvas, font_h);
    ret |= klb_canvas_draw_text(p_canvas, p_rect, p_utf8, utf8_len);

    return ret;
}

int klb_wnd_draw_image(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_draw_image(p_canvas, p_dst_rect, p_path);

    return ret;
}
