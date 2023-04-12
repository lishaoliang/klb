// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klb_wnd_in.h"


typedef struct klb_dialog_t_
{
    sds         title;

    klb_wnd_t*  p_btn_close;
}klb_dialog_t;


static void klb_dialog_destroy(klb_wnd_t* p_wnd)
{
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;

    KLB_FREE_BY(p_dlg->title, sdsfree);
    KLB_FREE(p_wnd);
}

static int klb_dialog_on_paint(klb_wnd_t* p_wnd)
{
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STYLE_BORDERLESS & p_wnd->state.style)
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 31, 41, 21));
        klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 81, 81, 81));

        //
        klb_wnd_show(p_dlg->p_btn_close, false);
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 31, 41, 21));

        int title_h = 42;
        klb_rect_t title_rect = { p_rect->x, p_rect->y, p_rect->w, title_h };
        klb_wnd_draw_fill_rect2(p_wnd, &title_rect, KLB_ARGB8888(255, 45, 45, 48));

        klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 81, 81, 81));

        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y, p_rect->x + p_rect->w - 2, p_rect->y, KLB_ARGB8888(255, 180, 180, 180));
        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y + title_h, p_rect->x + p_rect->w - 2, p_rect->y + title_h, KLB_ARGB8888(255, 120, 120, 120));

        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y, p_rect->x, p_rect->y + p_rect->h - 2, KLB_ARGB8888(255, 180, 180, 180));
    
        if (0 < sdslen(p_dlg->title))
        {
            int x = p_rect->x + 8, y = p_rect->y + 4, w = p_rect->w - 16, h = title_h - 8;

            klb_rect_t r = { x, y, w, h };
            klb_wnd_draw_text2(p_wnd, &r, p_dlg->title, sdslen(p_dlg->title), KLB_ARGB8888(255, 255, 128, 0), h - 2);
        }
    }

    return 0;
}

static int klb_dialog_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLBUI_onpaint:
        return klb_dialog_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klb_dialog_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    const char* p_value = klb_map_idx_to_string(p_map, 1);

    if (0 == strcmp(p_key, "title"))
    {
        p_dlg->title = sdscpy(p_dlg->title, p_value);

        klb_wnd_update(p_wnd);
    }

    return 0;
}

static klb_map_t* klb_dialog_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);

    klb_map_t* p_out = klb_map_create();

    if (0 == strcmp(p_key, "title"))
    {
        klb_map_set_idx_string(p_out, 0, p_dlg->title);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////

static int on_cmd_btn_close_klb_dialog(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    return 0;
}

//////////////////////////////////////////////////////////////////////////

klb_wnd_t* klb_dialog_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klb_dialog_t));
    klb_dialog_t* p_dlg = (klb_dialog_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klb_dialog_destroy;
    p_wnd->vtable.on_control = klb_dialog_on_control;
    p_wnd->vtable.on_set = klb_dialog_on_set;
    p_wnd->vtable.on_get = klb_dialog_on_get;

    p_dlg->title = sdsempty();

    //
    p_dlg->p_btn_close = klbui_button_create(p_gui, w - 42 - 4, (42 - 30) / 2, 42, 30);

    //klb_wnd_set_style(p_dlg->p_btn_close, style | klb_wnd_get_style(p_wnd));
    klb_wnd_push_child(p_wnd, p_dlg->p_btn_close);

    klb_map_t r = { 0 };
    klb_map_init(&r);

    klb_map_clear(&r);
    klb_map_set_idx_string(&r, 0, "normal");
    klb_map_set_idx_string(&r, 1, "dialog_close_normal.bmp");
    klb_wnd_set(p_dlg->p_btn_close, &r);

    klb_map_clear(&r);
    klb_map_set_idx_string(&r, 0, "focus");
    klb_map_set_idx_string(&r, 1, "dialog_close_focus.bmp");
    klb_wnd_set(p_dlg->p_btn_close, &r);

    klb_map_quit(&r);

    klb_wnd_bind_command(p_dlg->p_btn_close, on_cmd_btn_close_klb_dialog, p_dlg);

    return p_wnd;
}
