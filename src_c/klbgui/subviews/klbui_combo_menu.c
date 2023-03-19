#include "klbgui/subviews/klbui_combo_menu.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"
#include "klbgui/klb_wnd_in.h"


//////////////////////////////////////////////////////////////////////////
// klbui_combo_menu_item_t


typedef struct klbui_combo_menu_item_t_
{
    klbui_combo_menu_t*     p_combo_menu;
}klbui_combo_menu_item_t;


static void klbui_combo_menu_item_set_combo_menu(klbui_combo_menu_item_t* p_item, klbui_combo_menu_t* p_menu)
{
    p_item->p_combo_menu = p_menu;
}

////////////////////////////////////////


static void klbui_combo_menu_item_destroy(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_item_t* p_item = (klbui_combo_menu_item_t*)p_wnd->ctrl;

    // 清理属性

    KLB_FREE(p_wnd);
}



static void klbui_combo_menu_item_on_paint_status(klb_wnd_t* p_wnd, klbui_combo_menu_item_t* p_btn, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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

    //if (0 < sdslen(p_btn->title))
    //{
    //    klb_rect_t text_rect = *p_rect;

    //    // 移除边框
    //    text_rect.x += p_attr->border.width.left;
    //    text_rect.y += p_attr->border.width.top;
    //    text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
    //    text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

    //    // 移除内边距
    //    text_rect.x += p_btn->padding.left;
    //    text_rect.y += p_btn->padding.top;
    //    text_rect.w -= (p_btn->padding.left + p_btn->padding.right);
    //    text_rect.h -= (p_btn->padding.top + p_btn->padding.bottom);

    //    klb_wnd_draw_text2(p_wnd, &text_rect, p_btn->title, sdslen(p_btn->title), p_attr->text.color, p_attr->font.size);
    //}
}

static int klbui_combo_menu_item_on_paint(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_item_t* p_item = (klbui_combo_menu_item_t*)p_wnd->ctrl;
    klbui_combo_menu_t* p_menu = p_item->p_combo_menu;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    //paint_rect.x += p_btn->margin.left;
    //paint_rect.y += p_btn->margin.top;
    //paint_rect.w -= (p_btn->margin.left + p_btn->margin.right);
    //paint_rect.h -= (p_btn->margin.top + p_btn->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_combo_menu_item_on_paint_status(p_wnd, p_item, &p_menu->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_combo_menu_item_on_paint_status(p_wnd, p_item, &p_menu->focus, &paint_rect);
    }
    else
    {
        klbui_combo_menu_item_on_paint_status(p_wnd, p_item, &p_menu->normal, &paint_rect);
    }

    return 0;
}

static int klbui_combo_menu_item_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_combo_menu_item_t* p_item = KLB_WIDGETS_PTR(p_wnd, klbui_combo_menu_item_t); // (klbui_combo_menu_item_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_combo_menu_item_on_paint(p_wnd);
    case KLB_WM_LBUTTONDOWN:
    case KLB_WM_LBUTTONDBLCLK:
        klb_gui_popup_end(p_wnd->p_gui, true);
        break;
    default:
        break;
    }

    return 0;
}

static klbui_combo_menu_item_t* klbui_combo_menu_item_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_combo_menu_item_t));
    klbui_combo_menu_item_t* p_item = (klbui_combo_menu_item_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_combo_menu_item_destroy;
    p_wnd->vtable.on_control = klbui_combo_menu_item_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    return p_item;
}


//////////////////////////////////////////////////////////////////////////
// klbui_combo_menu_t


static void klbui_combo_menu_destroy(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    // 清理属性

    KLB_FREE(p_wnd);
}

static int klbui_combo_menu_on_paint(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;


    klb_wnd_draw_fill_rect2(p_wnd, &paint_rect, KLB_ARGB8888(255, 31, 41, 21));

    return 0;
}

static int klbui_combo_menu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_combo_menu_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////


klb_wnd_t* klbui_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_combo_menu_t));
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_combo_menu_destroy;
    p_wnd->vtable.on_control = klbui_combo_menu_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    //
    p_wnd->state.style = KLB_WND_STYLE_TOP;

    //
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    klbuicssex_attributes_init(&p_menu->normal, p_default);
    klbuicssex_attributes_init(&p_menu->focus, p_default);
    klbuicssex_attributes_init(&p_menu->disable, p_default);

    p_menu->focus.border.color.top = KLB_ARGB8888(255, 220, 20, 20);
    p_menu->focus.border.color.right = KLB_ARGB8888(255, 220, 20, 20);
    p_menu->focus.border.color.bottom = KLB_ARGB8888(255, 220, 20, 20);
    p_menu->focus.border.color.left = KLB_ARGB8888(255, 220, 20, 20);


    // item
    int item_w = w - 2;
    int item_h = 28;
    for (int i = 0; i < KLBUI_COMBO_MENU_item_max; i++)
    {
        p_menu->p_item[i] = klbui_combo_menu_item_create(p_gui, 1, 1 + i * item_h, item_w, item_h);
        klbui_combo_menu_item_set_combo_menu(p_menu->p_item[i], p_menu);

        klb_wnd_push_child(p_wnd, KLB_WND_PTR(p_menu->p_item[i]));
    }

    return p_wnd;
}
