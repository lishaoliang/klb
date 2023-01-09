#include "klbgui/subviews/klbui_tab_button.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


static void klbui_tab_botton_destroy(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    // 清理属性

    KLB_FREE(p_wnd);
}

static int klbui_tab_botton_on_paint(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    klb_wnd_draw_fill_rect2(p_wnd, &paint_rect, KLB_ARGB8888(255, 250, 20, 20));

    return 0;
}

static int klbui_tab_botton_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_tab_botton_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

klb_wnd_t* klbui_tab_botton_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_tab_botton_t));
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_tab_botton_destroy;
    p_wnd->vtable.on_control = klbui_tab_botton_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    // 初始化默认值
    //klbui_button_init_attribute(p_wnd, p_btn);

    // 初始化 支持的方法
    //klbui_button_init_func_map(p_wnd, p_btn, p_gui);

    return p_wnd;
}
