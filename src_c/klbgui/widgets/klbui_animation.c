// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"


// 动态图片
typedef struct klbui_animation_t_
{
    int a;
}klbui_animation_t;


static void klbui_animation_destroy(klb_wnd_t* p_wnd)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    KLB_FREE(p_wnd);
}

static int klbui_animation_on_paint(klb_wnd_t* p_wnd)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 30, 30, 30));
    klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 120, 120, 120));


    return 0;
}

static int klbui_animation_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_animation_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_animation_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_animation_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    const char* p_value = klb_map_idx_to_string(p_map, 1);

    return 0;
}

static klb_map_t* klbui_animation_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);

    klb_map_t* p_out = klb_map_create();

    if (0 == strcmp(p_key, "title"))
    {

    }

    return p_out;
}

klb_wnd_t* klbui_animation_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_animation_t));
    klbui_animation_t* p_animation = (klbui_animation_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_animation_destroy;
    p_wnd->vtable.on_control = klbui_animation_on_control;
    p_wnd->vtable.on_command = klbui_animation_on_command;
    p_wnd->vtable.on_set = klbui_animation_on_set;
    p_wnd->vtable.on_get = klbui_animation_on_get;

    return p_wnd;
}
