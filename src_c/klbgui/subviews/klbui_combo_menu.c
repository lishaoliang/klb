#include "klbgui/subviews/klbui_combo_menu.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"
#include "klbgui/klb_wnd_in.h"
#include "klbutil/klb_rect.h"


//////////////////////////////////////////////////////////////////////////
// klbui_combo_menu_item_t


typedef struct klbui_combo_menu_item_t_
{
    sds                     title;          ///< 标题
    sds                     value;          ///< 值

    klbui_combo_menu_t*     p_combo_menu;
}klbui_combo_menu_item_t;


//////////////////////////////////////////////////////////////////////////
static void klbui_combo_menu_click_ok(klbui_combo_menu_t* p_combo_menu, klbui_combo_menu_item_t* p_item);


///////////////////////////////////


static void klbui_combo_menu_item_set_combo_menu(klbui_combo_menu_item_t* p_item, klbui_combo_menu_t* p_menu)
{
    p_item->p_combo_menu = p_menu;
}

static void klbui_combo_menu_item_set_title(klbui_combo_menu_item_t* p_item, const char* p_title)
{
    p_item->title = sdscpy(p_item->title, p_title);
}

static void klbui_combo_menu_item_set_value(klbui_combo_menu_item_t* p_item, const char* p_value)
{
    p_item->value = sdscpy(p_item->value, p_value);
}

////////////////////////////////////////


static void klbui_combo_menu_item_destroy(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_item_t* p_item = (klbui_combo_menu_item_t*)p_wnd->ctrl;

    // 清理属性
    KLB_FREE_BY(p_item->title, sdsfree);
    KLB_FREE_BY(p_item->value, sdsfree);

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
        if (0 < p_attr->border.width.top)
        {
            klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_attr->border.width.top };
            klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_attr->border.color.top);
        }

        if (0 < p_attr->border.width.right)
        {
            klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_attr->border.width.right, paint_rect.y, p_attr->border.width.right, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_attr->border.color.right);
        }

        if (0 < p_attr->border.width.bottom)
        {
            klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_attr->border.width.bottom, paint_rect.w, p_attr->border.width.bottom };
            klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_attr->border.color.bottom);
        }

        if (0 < p_attr->border.width.left)
        {
            klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_attr->border.width.left, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_attr->border.color.left);
        }
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
        //text_rect.x += p_attr->padding.left;
        //text_rect.y += p_attr->padding.top;
        //text_rect.w -= (p_attr->padding.left + p_attr->padding.right);
        //text_rect.h -= (p_attr->padding.top + p_attr->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_btn->title, sdslen(p_btn->title), p_attr->text.color, p_attr->font.size);
    }
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

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_combo_menu_item_on_paint_status(p_wnd, p_item, &p_menu->p_ref_css->item_focus, &paint_rect);
    }
    else
    {
        klbui_combo_menu_item_on_paint_status(p_wnd, p_item, &p_menu->p_ref_css->item_normal, &paint_rect);
    }

    return 0;
}

static int klbui_combo_menu_item_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_combo_menu_item_t* p_item = KLB_WIDGETS_PTR(p_wnd, klbui_combo_menu_item_t); // (klbui_combo_menu_item_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbui_combo_menu_item_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
        klbui_combo_menu_click_ok(p_item->p_combo_menu, p_item);
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

    p_item->title = sdsempty();
    p_item->value = sdsempty();

    return p_item;
}


//////////////////////////////////////////////////////////////////////////
// klbui_combo_menu_t

static void klbui_combo_menu_click_ok(klbui_combo_menu_t* p_combo_menu, klbui_combo_menu_item_t* p_item)
{
    if (NULL != p_combo_menu->cb_combo)
    {
        p_combo_menu->cb_combo(p_combo_menu->p_combo, p_item->value, p_item->title);
    }
}

int klbui_combo_menu_bind(klb_wnd_t* p_wnd, klbui_combo_menu_css_t* p_css, klb_map_t* p_data_array, klbui_combo_menu_cb cb, klb_wnd_t* p_combo, int* p_out_w, int* p_out_h)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    p_menu->p_ref_css = p_css;
    p_menu->p_ref_array = p_data_array;
    p_menu->cb_combo = cb;
    p_menu->p_combo = p_combo;

    int array_size = (NULL != p_data_array) ? klb_map_array_size(p_data_array) : 0;
    int count = MIN(array_size, KLBUI_COMBO_MENU_item_max);

    // w, h
    int item_h = p_css->item_normal.font.size + 4;
    int w = p_wnd->pos.rect_in_parent.w;
    int h = item_h * count + p_css->padding.top + p_css->padding.bottom;
    int item_w = w - p_css->padding.left - p_css->padding.right;

    klb_wnd_resize(p_wnd, w, h);

    for (int i = 0; i < count; i++)
    {
        klb_map_t* p_item = klb_map_idx_to_map(p_data_array, i);
        klb_map_iter_t* p_item_iter = (NULL != p_item) ? klb_map_begin(p_item) : NULL;
        klb_adt_t* p_item_adt = (NULL != p_item_iter) ? klb_map_data(p_item_iter) : NULL;
        const char* p_title = (NULL != p_item_adt) ? klb_adt_to_string(p_item_adt) : "";
        const char* p_key = (NULL != p_item_iter) ? klb_map_key(p_item_iter) : "";

        klbui_combo_menu_item_set_title(p_menu->p_item[i], p_title);
        klbui_combo_menu_item_set_value(p_menu->p_item[i], p_key);

        klb_wnd_resize(KLB_WND_PTR(p_menu->p_item[i]), item_w, item_h);
        klb_wnd_move(KLB_WND_PTR(p_menu->p_item[i]), p_css->padding.left, p_css->padding.top + item_h * i);

        klb_wnd_show(KLB_WND_PTR(p_menu->p_item[i]), true);
    }

    for (int i = count; i < KLBUI_COMBO_MENU_item_max; i++)
    {
        klb_wnd_show(KLB_WND_PTR(p_menu->p_item[i]), false);
    }

    if (NULL != p_out_w)
    {
        *p_out_w = w;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = h;
    }

    return 0;
}

static void klbui_combo_menu_destroy(klb_wnd_t* p_wnd)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    // 清理属性

    KLB_FREE(p_wnd);
}


static void klbui_combo_menu_on_paint_status(klb_wnd_t* p_wnd, klbui_combo_menu_t* p_menu, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
        if (0 < p_attr->border.width.top)
        {
            klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_attr->border.width.top };
            klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_attr->border.color.top);
        }

        if (0 < p_attr->border.width.right)
        {
            klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_attr->border.width.right, paint_rect.y, p_attr->border.width.right, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_attr->border.color.right);
        }

        if (0 < p_attr->border.width.bottom)
        {
            klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_attr->border.width.bottom, paint_rect.w, p_attr->border.width.bottom };
            klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_attr->border.color.bottom);
        }

        if (0 < p_attr->border.width.left)
        {
            klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_attr->border.width.left, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_attr->border.color.left);
        }
    }
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

    klbui_combo_menu_on_paint_status(p_wnd, p_menu, &p_menu->p_ref_css->normal, &paint_rect);

    return 0;
}

static int klbui_combo_menu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_combo_menu_t* p_menu = (klbui_combo_menu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbui_combo_menu_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
    case KLBUI_mousedown:
        {
            klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;
            if (!klb_pt_in_rect(p_rect, p_pt1->x, p_pt1->y))
            {
                klb_gui_popup_end(p_wnd->p_gui, true);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

void klbui_combo_menu_css_init(const klbui_default_t* p_default, klbui_combo_menu_css_t* p_menu_css)
{
    klbuicssex_attributes_init(&p_menu_css->normal, p_default);
    klbuicssex_attributes_init(&p_menu_css->item_normal, p_default);
    klbuicssex_attributes_init(&p_menu_css->item_focus, p_default);

    p_menu_css->padding.top = 1;
    p_menu_css->padding.right = 1;
    p_menu_css->padding.bottom = 1;
    p_menu_css->padding.left = 1;

    p_menu_css->item_focus.border.color.top = KLB_ARGB8888(255, 220, 20, 20);
    p_menu_css->item_focus.border.color.right = KLB_ARGB8888(255, 220, 20, 20);
    p_menu_css->item_focus.border.color.bottom = KLB_ARGB8888(255, 220, 20, 20);
    p_menu_css->item_focus.border.color.left = KLB_ARGB8888(255, 220, 20, 20);

    p_menu_css->item_normal.border.width.top = 0;
    p_menu_css->item_normal.border.width.right = 0;
    p_menu_css->item_normal.border.width.bottom = 0;
    p_menu_css->item_normal.border.width.left = 0;
}

void klbui_combo_menu_css_quit(klbui_combo_menu_css_t* p_menu_css)
{
    klbuicssex_attributes_quit(&p_menu_css->normal);
    klbuicssex_attributes_quit(&p_menu_css->item_normal);
    klbuicssex_attributes_quit(&p_menu_css->item_focus);
}

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

    // 初始化CSS等属性参数
    //klbui_combo_menu_init_css(klb_gui_get_std_default(p_gui), &p_menu->css);

    // item
    int item_w = w - 2;
    int item_h = 28;
    for (int i = 0; i < KLBUI_COMBO_MENU_item_max; i++)
    {
        p_menu->p_item[i] = klbui_combo_menu_item_create(p_gui, 1, 1 + i * item_h, item_w, item_h);
        klbui_combo_menu_item_set_combo_menu(p_menu->p_item[i], p_menu);
        klbui_combo_menu_item_set_title(p_menu->p_item[i], "");

        klb_wnd_push_child(p_wnd, KLB_WND_PTR(p_menu->p_item[i]));
    }

    p_wnd->pos.rect_in_parent.h = item_h * KLBUI_COMBO_MENU_item_max + 2;

    return p_wnd;
}
