// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"


// 水平滚动条
typedef struct klbui_hscrollbar_t_
{
    int a;
}klbui_hscrollbar_t;


static void klbui_hscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    KLB_FREE(p_wnd);
}

static int klbui_hscrollbar_on_paint(klb_wnd_t* p_wnd)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 30, 30, 30));
    klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 120, 120, 120));

    return 0;
}

static int klbui_hscrollbar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_GUI_PAINT:
        return klbui_hscrollbar_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_hscrollbar_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_hscrollbar_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    const char* p_value = klb_map_idx_to_string(p_map, 1);

    if (0 == strcmp(p_key, "title"))
    {

    }

    return 0;
}

static klb_map_t* klbui_hscrollbar_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);

    klb_map_t* p_out = klb_map_create();
    if (0 == strcmp(p_key, "title"))
    {

    }

    return p_out;
}

klb_wnd_t* klbui_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_hscrollbar_t));
    klbui_hscrollbar_t* p_hscrollbar = (klbui_hscrollbar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_hscrollbar_destroy;
    p_wnd->vtable.on_control = klbui_hscrollbar_on_control;
    p_wnd->vtable.on_command = klbui_hscrollbar_on_command;
    p_wnd->vtable.on_set = klbui_hscrollbar_on_set;
    p_wnd->vtable.on_get = klbui_hscrollbar_on_get;

    return p_wnd;
}
