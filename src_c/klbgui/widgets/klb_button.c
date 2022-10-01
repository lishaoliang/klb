// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"

typedef struct klb_button_t_
{
    sds         title;
}klb_button_t;

static void klb_button_destroy(klb_wnd_t* p_wnd)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE(p_wnd);
}

static int klb_button_on_paint(klb_wnd_t* p_wnd)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);
    KLG_GUI_CHECK_CANVAS_RETRUN(p_canvas);
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klb_canvas_draw_fill(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 61, 71, 51));
        klb_canvas_draw_rect(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 180, 180, 180), 1);
    }
    else
    {
        klb_canvas_draw_fill(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 61, 71, 51));
        klb_canvas_draw_rect(p_canvas, p_rect->x, p_rect->y, p_rect->w, p_rect->h, KLB_ARGB8888(255, 120, 120, 120), 1);
    }

    return 0;
}

static int klb_button_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_GUI_CTRL_PAINT:
        return klb_button_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klb_button_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        //KLB_LOG("klb_button_on_command\n");
        break;
    default:
        break;
    }

    return 0;
}

static int klb_button_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    const char* p_value = klb_map_idx_to_string(p_map, 1);

    if (0 == strcmp(p_key, "title"))
    {
        p_btn->title = sdscpy(p_btn->title, p_value);
    }

    return 0;
}

static klb_map_t* klb_button_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);

    klb_map_t* p_out = klb_map_create();

    if (0 == strcmp(p_key, "title"))
    {
        klb_map_set_idx_string(p_out, 0, p_btn->title);
    }

    return p_out;
}

klb_wnd_t* klb_button_create(int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klb_button_t));

    klb_button_t* p_btn = (klb_button_t*)p_wnd->ctrl;
    p_btn->title = sdsempty();

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klb_button_destroy;
    p_wnd->vtable.on_control = klb_button_on_control;
    p_wnd->vtable.on_command = klb_button_on_command;
    p_wnd->vtable.on_set = klb_button_on_set;
    p_wnd->vtable.on_get = klb_button_on_get;

    return p_wnd;
}
