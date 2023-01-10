#include "klbgui/subviews/klbui_tab_button.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


static void klbui_tab_botton_quit_attribute(klbui_tab_botton_t* p_btn);

static void klbui_tab_botton_destroy(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    // 清理属性
    klbui_tab_botton_quit_attribute(p_btn);

    KLB_FREE(p_wnd);
}

static void klbui_tab_botton_on_paint_status(klb_wnd_t* p_wnd, klbui_tab_botton_t* p_btn, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (0 < sdslen(p_attr->background.image))
    {
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

        klb_rect_t paint_rect = *p_rect;

        // border
        klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_attr->border.width.top };
        klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_attr->border.color.top);

        klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_attr->border.width.right, paint_rect.y, p_attr->border.width.right, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_attr->border.color.right);

        klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_attr->border.width.bottom, paint_rect.w, p_attr->border.width.bottom };
        klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_attr->border.color.bottom);

        klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_attr->border.width.left, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_attr->border.color.left);
    }

    if (0 < sdslen(p_btn->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_btn->padding.left;
        text_rect.y += p_btn->padding.top;
        text_rect.w -= (p_btn->padding.left + p_btn->padding.right);
        text_rect.h -= (p_btn->padding.top + p_btn->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_btn->title, sdslen(p_btn->title), p_attr->text.color, p_attr->font.size);
    }
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

    // 移除外边距
    paint_rect.x += p_btn->margin.left;
    paint_rect.y += p_btn->margin.top;
    paint_rect.w -= (p_btn->margin.left + p_btn->margin.right);
    paint_rect.h -= (p_btn->margin.top + p_btn->margin.bottom);

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_tab_botton_on_paint_status(p_wnd, p_btn, &p_btn->focus, &paint_rect);
    }
    else if(p_btn->is_check)
    {
        klbui_tab_botton_on_paint_status(p_wnd, p_btn, &p_btn->check, &paint_rect);
    }
    else
    {
        klbui_tab_botton_on_paint_status(p_wnd, p_btn, &p_btn->normal, &paint_rect);
    }

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

//////////////////////////////////////////////////////////////////////////

void klbui_tab_botton_set_index(klb_wnd_t* p_wnd, int idx)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    p_btn->index = idx;
}

int klbui_tab_botton_get_index(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    return p_btn->index;
}

void klbui_tab_botton_bind_wnd(klb_wnd_t* p_wnd, klb_wnd_t* p_dst)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    p_btn->p_bind_wnd = p_dst;
}

klb_wnd_t* klbui_tab_botton_get_bind_wnd(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    return p_btn->p_bind_wnd;
}

void klbui_tab_botton_check(klb_wnd_t* p_wnd, bool check)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    p_btn->is_check = check;
}

void klbui_tab_botton_update_title(klb_wnd_t* p_wnd)
{
    klbui_tab_botton_t* p_btn = (klbui_tab_botton_t*)p_wnd->ctrl;

    if (NULL == p_btn->p_bind_wnd)
    {
        return;
    }
    klb_map_t in = { 0 };
    klb_map_init(&in);
    klb_map_set_idx_string(&in, 0, "title");

    klb_map_t* p_out = klb_wnd_get(p_btn->p_bind_wnd, &in);
    if (NULL != p_out)
    {
        const char* p_tile = klb_map_idx_to_string(p_out, 0);
        if (NULL != p_tile)
        {
            p_btn->title = sdscpy(p_btn->title, p_tile);
        }

        klb_map_destroy(p_out);
    }

    klb_map_quit(&in);
}

static void klbui_tab_botton_init_attribute(klb_wnd_t* p_wnd, klbui_tab_botton_t* p_btn)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_btn->title = sdsempty();

    klbuicssex_attributes_init(&p_btn->normal, p_default);
    klbuicssex_attributes_init(&p_btn->focus, p_default);
    klbuicssex_attributes_init(&p_btn->check, p_default);

    p_btn->is_check = false;
}

static void klbui_tab_botton_quit_attribute(klbui_tab_botton_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);

    klbuicssex_attributes_quit(&p_btn->normal);
    klbuicssex_attributes_quit(&p_btn->focus);
    klbuicssex_attributes_quit(&p_btn->check);
}

//////////////////////////////////////////////////////////////////////////

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
    klbui_tab_botton_init_attribute(p_wnd, p_btn);

    return p_wnd;
}
