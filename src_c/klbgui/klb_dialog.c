#include "klbgui/klb_widget.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"

typedef struct klb_dialog_t_
{
    int a;
}klb_dialog_t;


static void klb_dialog_destroy(klb_wnd_t* p_wnd)
{
    KLB_FREE(p_wnd);
}


static int klb_dialog_on_paint(klb_wnd_t* p_wnd)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);
    KLG_GUI_CHECK_CANVAS_RETRUN(p_canvas);

    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    klb_canvas_draw_fill(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 31, 41, 21));
    klb_canvas_draw_rect(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 81, 81, 81), 1);

    return 0;
}

static int klb_dialog_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLB_GUI_CTRL_PAINT:
        return klb_dialog_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

klb_wnd_t* klb_dialog_create(int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOC(klb_wnd_t, 1, sizeof(klb_dialog_t));
    KLB_MEMSET(p_wnd, 0, sizeof(klb_wnd_t) + sizeof(klb_dialog_t));
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klb_dialog_destroy;
    p_wnd->vtable.on_control = klb_dialog_on_control;

    return p_wnd;
}
