// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"


// 常规按钮
typedef struct klbui_button_t_
{
    sds         title;      // 标题

    sds         normal;     // 普通状态图
    sds         focus;      // 聚焦状态图
    sds         disable;    // 不使能状态图
}klbui_button_t;


static void klbui_button_destroy(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    KLB_FREE_BY(p_btn->title, sdsfree);

    KLB_FREE_BY(p_btn->normal, sdsfree);
    KLB_FREE_BY(p_btn->focus, sdsfree);
    KLB_FREE_BY(p_btn->disable, sdsfree);

    KLB_FREE(p_wnd);
}

static int klbui_button_on_paint(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        if (0 < sdslen(p_btn->normal))
        {
            klb_wnd_draw_image(p_wnd, p_rect, p_btn->focus, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect(p_wnd, p_rect, KLB_ARGB8888(255, 61, 71, 51));
            klb_wnd_draw_rect(p_wnd, p_rect, KLB_ARGB8888(255, 180, 180, 180));
        }
    }
    else
    {
        if (0 < sdslen(p_btn->normal))
        {
            klb_wnd_draw_image(p_wnd, p_rect, p_btn->normal, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect(p_wnd, p_rect, KLB_ARGB8888(255, 30, 30, 30));
            klb_wnd_draw_rect(p_wnd, p_rect, KLB_ARGB8888(255, 120, 120, 120));
        }
    }

    if (0 < sdslen(p_btn->title))
    {
        int x = p_rect->x + 2, y = p_rect->y + 2, w = p_rect->w - 4, h = p_rect->h - 4 - 2;

        klb_rect_t r = { x, y, w, h };
        klb_wnd_draw_text(p_wnd, &r, p_btn->title, sdslen(p_btn->title), KLB_ARGB8888(255, 255, 128, 0), h - 2);
    }

    return 0;
}

static int klbui_button_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_GUI_PAINT:
        return klbui_button_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_button_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

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

static int klbui_button_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    const char* p_value = klb_map_idx_to_string(p_map, 1);

    if (0 == strcmp(p_key, "title"))
    {
        p_btn->title = sdscpy(p_btn->title, p_value);

        klb_wnd_update(p_wnd);
    }
    else if(0 == strcmp(p_key, "normal"))
    {
        p_btn->normal = sdscpy(p_btn->normal, p_value);
    }
    else if (0 == strcmp(p_key, "focus"))
    {
        p_btn->focus = sdscpy(p_btn->focus, p_value);
    }
    else if (0 == strcmp(p_key, "disable"))
    {
        p_btn->disable = sdscpy(p_btn->disable, p_value);
    }

    return 0;
}

static klb_map_t* klbui_button_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);

    klb_map_t* p_out = klb_map_create();

    if (0 == strcmp(p_key, "title"))
    {
        klb_map_set_idx_string(p_out, 0, p_btn->title);
    }

    return p_out;
}

klb_wnd_t* klbui_button_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_button_t));

    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_button_destroy;
    p_wnd->vtable.on_control = klbui_button_on_control;
    p_wnd->vtable.on_command = klbui_button_on_command;
    p_wnd->vtable.on_set = klbui_button_on_set;
    p_wnd->vtable.on_get = klbui_button_on_get;

    // init
    p_btn->title = sdsempty();

    p_btn->normal = sdsempty();
    p_btn->focus = sdsempty();
    p_btn->disable = sdsempty();

    return p_wnd;
}
