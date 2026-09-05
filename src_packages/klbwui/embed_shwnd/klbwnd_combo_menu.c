// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_combo_menu.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbwui/core/klbwui_css.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_adt.h"
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_combo_menu_relayout(klb_wnd_t* p_wnd);
static void update_items_klbwnd_combo_menu(klbwnd_combo_menu_t* p_menu, int pos);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_combo_menu_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_combo_menu_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_combo_menu_css_t* p_css = &p_menu->css;
    klbuicssex_attributes_t* p_attr = &p_css->normal;
    klb_rect_t paint_rect = *p_rect;
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwuicss_draw_background(p_wnd, &paint_rect, &p_attr->background);
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, &paint_rect, &p_attr->border);
    }

    return 0;
}

static int klbwnd_combo_menu_on_outwindow(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu)
{
    if (NULL != p_menu->cb)
    {
        p_menu->cb(p_menu->ptr, false, NULL, NULL);
    }

    klb_gui_popup_end(p_wnd->p_gui, false);
    return 0;
}

static int scroll_wheel_klbwnd_combo_menu(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu, int lparam)
{
    if (!klb_wnd_is_show(p_menu->p_vscrollbar))
    {
        return 0;
    }

    int v = 1;

    if (KLBUI_MOUSEWHEEL_is_up(lparam))
    {
        if (0 == klbwnd_vscrollbar_up(p_menu->p_vscrollbar, v))
        {
            update_items_klbwnd_combo_menu(p_menu, klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar));
            klb_wnd_update(p_wnd);
        }
    }
    else if (KLBUI_MOUSEWHEEL_is_down(lparam))
    {
        if (0 == klbwnd_vscrollbar_down(p_menu->p_vscrollbar, v))
        {
            update_items_klbwnd_combo_menu(p_menu, klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar));
            klb_wnd_update(p_wnd);
        }
    }

    return -1;
}

static int klbwnd_combo_menu_on_mousewheel(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu, const klb_point_t* p_pt1, int lparam)
{
    if (NULL != p_pt1 && !klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, p_pt1->x, p_pt1->y))
    {
        return 0;
    }

    return scroll_wheel_klbwnd_combo_menu(p_wnd, p_menu, lparam);
}

static int klbwnd_combo_menu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_combo_menu_on_paint(p_wnd);

    case KLBUI_onresize:
        return klbwnd_combo_menu_layout(p_wnd);

    case KLBUI_outwindow:
        return klbwnd_combo_menu_on_outwindow(p_wnd, p_menu);

    case KLBUI_mousewheel:
        return klbwnd_combo_menu_on_mousewheel(p_wnd, p_menu, p_pt1, lparam);

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 子窗口响应

static int on_command_item_klbwnd_combo_menu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd_menu->ctrl;

    if (KLBUI_mousewheel == msg)
    {
        return scroll_wheel_klbwnd_combo_menu(p_wnd_menu, p_menu, lparam);
    }

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        if (NULL != p_menu->cb)
        {
            p_menu->cb(p_menu->ptr, true, klbwnd_button_get_value(p_wnd), klbwnd_button_get_title(p_wnd));
        }

        klb_gui_popup_end(p_wnd->p_gui, false);
    }

    return 0;
}

static int on_command_vscrollbar_klbwnd_combo_menu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd_menu->ctrl;

    if (KLBUI_onchange == msg)
    {
        update_items_klbwnd_combo_menu(p_menu, klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar));
        klb_wnd_update(p_wnd_menu);
    }

    return 0;
}

static void fill_item_klbwnd_combo_menu(klbwnd_combo_menu_t* p_menu, klb_wnd_t* p_item, int data_idx)
{
    if (data_idx < p_menu->array_size)
    {
        klb_map_t* p_data = klb_map_idx_to_map(p_menu->p_ref_data_array, data_idx);
        klb_map_iter_t* p_iter = (NULL != p_data) ? klb_map_begin(p_data) : NULL;

        if (NULL != p_iter)
        {
            klbwnd_button_set_value(p_item, klb_map_key(p_iter));
            klbwnd_button_set_title(p_item, klb_adt_to_string(klb_map_data(p_iter)));
            klb_wnd_hide(p_item, false);
        }
        else
        {
            klb_wnd_hide(p_item, true);
        }
    }
    else
    {
        klb_wnd_hide(p_item, true);
    }
}

static void update_items_klbwnd_combo_menu(klbwnd_combo_menu_t* p_menu, int pos)
{
    int col = p_menu->col_count;
    int start = 0;

    if (col < 1)
    {
        col = 1;
    }

    start = pos * col;

    for (int i = 0; i < p_menu->row_count; ++i)
    {
        for (int s = 0; s < 4; ++s)
        {
            klb_wnd_t* p_item = p_menu->p_items[i + KLBWND_COMBO_MENU_row_max * s];

            if (s < col)
            {
                int visual = col - 1 - s;
                fill_item_klbwnd_combo_menu(p_menu, p_item, start + visual);
            }
            else
            {
                klb_wnd_hide(p_item, true);
            }
        }

        start += col;
    }
}


//////////////////////////////////////////////////////////////////////////
// 估算宽度

static int auto_width_klbwnd_combo_menu(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu, klb_map_t* p_data_array)
{
    klbwnd_combo_menu_css_t* p_css = &p_menu->css;
    int w = 64;
    int font_size = 0;

    if (NULL == p_data_array)
    {
        return w;
    }

    font_size = p_css->css_item.normal.font.size;

    if (font_size < p_css->css_item.focus.font.size)
    {
        font_size = p_css->css_item.focus.font.size;
    }

    if (font_size < p_css->css_item.disable.font.size)
    {
        font_size = p_css->css_item.disable.font.size;
    }

    int array_size = klb_map_array_size(p_data_array);
    for (int i = 0; i < array_size; ++i)
    {
        klb_map_t* p_item = klb_map_idx_to_map(p_data_array, i);
        klb_map_iter_t* p_iter = (NULL != p_item) ? klb_map_begin(p_item) : NULL;

        if (NULL != p_iter)
        {
            const char* p_title = klb_adt_to_string(klb_map_data(p_iter));
            int txt_w = 0;

            if (NULL != p_title)
            {
                klb_wnd_text_size2(p_wnd, p_title, (int)strlen(p_title), font_size, &txt_w, NULL);
                if (w < txt_w)
                {
                    w = txt_w;
                }
            }
        }
    }

    return w;
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_combo_menu_set_css(klb_wnd_t* p_wnd, klbwnd_combo_menu_t* p_menu)
{
    klbwnd_combo_menu_t* p_self = (klbwnd_combo_menu_t*)p_wnd->ctrl;

    if (NULL != p_menu)
    {
        for (int i = 0; i < KLBWND_COMBO_MENU_item_max; ++i)
        {
            klbwnd_button_set_css(p_menu->p_items[i], &p_self->css.css_item);
        }

        klbwnd_vscrollbar_set_css(p_self->p_vscrollbar, &p_self->css.css_vscrollbar);
    }
}

int klbwnd_combo_menu_bind(klb_wnd_t* p_wnd, klbshw_combo_menu_cb cb, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h, bool is_more_col, int menu_min_w)
{
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;
    klbwnd_combo_menu_css_t* p_css = &p_menu->css;
    int item_h = p_css->item_h;
    int count_max = 0;
    int count = 0;
    int need_h = 0;
    int need_w = 0;
    int offset_w = 36;
    int screen_w = 1920;
    int screen_h = 1080;

    p_menu->cb = cb;
    p_menu->ptr = ptr;
    p_menu->p_ref_data_array = p_data_array;
    p_menu->array_size = (NULL != p_data_array) ? klb_map_array_size(p_data_array) : 0;

    if (is_more_col)
    {
        if (64 <= p_menu->array_size)
        {
            p_menu->col_count = KLBWND_COMBO_MENU_col_4;
        }
        else if (32 <= p_menu->array_size)
        {
            p_menu->col_count = KLBWND_COMBO_MENU_col_2;
        }
        else
        {
            p_menu->col_count = KLBWND_COMBO_MENU_col_1;
        }
    }
    else
    {
        p_menu->col_count = KLBWND_COMBO_MENU_col_1;
    }

    if (item_h < 1)
    {
        item_h = 28;
    }

    count_max = (p_css->height_max - 2) / item_h;
    count = MIN(p_menu->array_size, KLBWND_COMBO_MENU_row_max);
    count = MIN(count, count_max);
    if (count < 1)
    {
        count = 1;
    }

    need_h = count * item_h + 2;
    if (p_css->height_max < need_h)
    {
        need_h = p_css->height_max;
    }

    p_menu->need_h = need_h;

    need_w = auto_width_klbwnd_combo_menu(p_wnd, p_menu, p_data_array);
    if (KLBWND_COMBO_MENU_col_4 == p_menu->col_count)
    {
        need_w *= 4;
    }
    else if (KLBWND_COMBO_MENU_col_2 == p_menu->col_count)
    {
        need_w *= 2;
    }

    need_w += offset_w;
    p_menu->need_w = need_w;

    if (menu_min_w <= 0)
    {
        if (p_menu->need_w < 390)
        {
            p_menu->need_w = 390;
        }
    }
    else if (p_menu->need_w < menu_min_w)
    {
        p_menu->need_w = menu_min_w;
    }

    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);
    if (screen_w < p_menu->need_w)
    {
        p_menu->need_w = screen_w;
    }

    if (screen_h < p_menu->need_h)
    {
        p_menu->need_h = screen_h;
    }

    if (NULL != p_out_w)
    {
        *p_out_w = p_menu->need_w;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = p_menu->need_h;
    }

    return 0;
}

int klbwnd_combo_menu_layout(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_relayout(p_wnd);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 子窗口 / 布局

static void klbwnd_combo_menu_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    for (int i = 0; i < KLBWND_COMBO_MENU_item_max; ++i)
    {
        klb_wnd_t* p_item = klbwnd_button_create(p_gui, 0, 0, 80, 32);

        klb_wnd_push_child(p_wnd, p_item);
        klb_wnd_hide(p_item, true);
        klb_wnd_bind_command(p_item, on_command_item_klbwnd_combo_menu, p_wnd);

        p_menu->p_items[i] = p_item;
    }

    p_menu->p_vscrollbar = klbwnd_vscrollbar_create(p_gui, 0, 0, 20, 80);
    klb_wnd_push_child(p_wnd, p_menu->p_vscrollbar);
    klb_wnd_hide(p_menu->p_vscrollbar, true);
    klb_wnd_bind_command(p_menu->p_vscrollbar, on_command_vscrollbar_klbwnd_combo_menu, p_wnd);
    klbwnd_vscrollbar_enable_mousewheel(p_menu->p_vscrollbar, true);

    p_menu->row_count = 0;
}

static void klbwnd_combo_menu_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;
    klbwnd_combo_menu_css_t* p_css = &p_menu->css;
    klb_rect_t rect = p_wnd->pos.rect_in_parent;
    int total_h = rect.h;
    int w = rect.w - 2;
    int h = p_css->item_h;
    int x = 1;
    int y = 1;
    int vscrollbar_w = p_css->vscrollbar_w;
    int row_count = 0;
    int data_count = 0;
    bool is_show_vscrollbar = false;
    int col_w = 0;
    int col = p_menu->col_count;

    if (h < 1)
    {
        h = 28;
    }

    if (vscrollbar_w < 1)
    {
        vscrollbar_w = 20;
    }

    if (col < 1)
    {
        col = 1;
    }

    row_count = total_h / h;
    row_count = MIN(row_count, KLBWND_COMBO_MENU_row_max);
    p_menu->row_count = row_count;

    data_count = (p_menu->array_size + col - 1) / col;
    is_show_vscrollbar = (data_count <= row_count) ? false : true;

    if (is_show_vscrollbar)
    {
        w -= (vscrollbar_w + x);
    }

    if (w < 1)
    {
        w = 1;
    }

    col_w = w / col;
    if (col_w < 1)
    {
        col_w = 1;
    }

    for (int i = 0; i < KLBWND_COMBO_MENU_row_max; ++i)
    {
        klb_wnd_t* p_item0 = p_menu->p_items[i];
        klb_wnd_t* p_item1 = p_menu->p_items[i + KLBWND_COMBO_MENU_row_max];
        klb_wnd_t* p_item2 = p_menu->p_items[i + KLBWND_COMBO_MENU_row_max * 2];
        klb_wnd_t* p_item3 = p_menu->p_items[i + KLBWND_COMBO_MENU_row_max * 3];

        if (y + h <= total_h)
        {
            klb_wnd_resize(p_item0, col_w, h);
            klb_wnd_resize(p_item1, col_w, h);
            klb_wnd_resize(p_item2, col_w, h);
            klb_wnd_resize(p_item3, col_w, h);

            if (KLBWND_COMBO_MENU_col_1 == col)
            {
                klb_wnd_move(p_item0, x, y);
                klb_wnd_hide(p_item0, false);
                klb_wnd_hide(p_item1, true);
                klb_wnd_hide(p_item2, true);
                klb_wnd_hide(p_item3, true);
            }
            else if (KLBWND_COMBO_MENU_col_2 == col)
            {
                klb_wnd_move(p_item0, x + col_w, y);
                klb_wnd_hide(p_item0, false);
                klb_wnd_move(p_item1, x, y);
                klb_wnd_hide(p_item1, false);
                klb_wnd_hide(p_item2, true);
                klb_wnd_hide(p_item3, true);
            }
            else if (KLBWND_COMBO_MENU_col_3 == col)
            {
                klb_wnd_move(p_item0, x + col_w * 2, y);
                klb_wnd_hide(p_item0, false);
                klb_wnd_move(p_item1, x + col_w, y);
                klb_wnd_hide(p_item1, false);
                klb_wnd_move(p_item2, x, y);
                klb_wnd_hide(p_item2, false);
                klb_wnd_hide(p_item3, true);
            }
            else
            {
                klb_wnd_move(p_item0, x + col_w * 3, y);
                klb_wnd_hide(p_item0, false);
                klb_wnd_move(p_item1, x + col_w * 2, y);
                klb_wnd_hide(p_item1, false);
                klb_wnd_move(p_item2, x + col_w, y);
                klb_wnd_hide(p_item2, false);
                klb_wnd_move(p_item3, x, y);
                klb_wnd_hide(p_item3, false);
            }

            y += h;
        }
        else
        {
            klb_wnd_hide(p_item0, true);
            klb_wnd_hide(p_item1, true);
            klb_wnd_hide(p_item2, true);
            klb_wnd_hide(p_item3, true);
        }
    }

    if (is_show_vscrollbar)
    {
        int max = data_count - row_count;
        if (max < 0)
        {
            max = 0;
        }

        klb_wnd_move(p_menu->p_vscrollbar, x + w, 1);
        klb_wnd_resize(p_menu->p_vscrollbar, vscrollbar_w, total_h - 2);
        klbwnd_vscrollbar_set_ranges(p_menu->p_vscrollbar, 0, max, 1);
        klbwnd_vscrollbar_set_value(p_menu->p_vscrollbar, 0);
        klb_wnd_hide(p_menu->p_vscrollbar, false);
    }
    else
    {
        klb_wnd_hide(p_menu->p_vscrollbar, true);
    }

    update_items_klbwnd_combo_menu(p_menu, 0);
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_combo_menu_css_init(klbwnd_combo_menu_t* p_menu, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);
    klbwnd_combo_menu_css_t* p_css = &p_menu->css;

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);

    p_css->normal.border.color.top = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.right = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.bottom = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.left = KLB_ARGB8888(255, 160, 160, 160);

    klbwnd_button_css_init(&p_css->css_item, p_gui);
    p_css->css_item.normal.border.width.top = 0;
    p_css->css_item.normal.border.width.right = 0;
    p_css->css_item.normal.border.width.bottom = 0;
    p_css->css_item.normal.border.width.left = 0;

    klbwnd_vscrollbar_css_init(&p_css->css_vscrollbar, p_gui);
    p_css->css_vscrollbar.normal.border.width.top = 0;
    p_css->css_vscrollbar.normal.border.width.right = 0;
    p_css->css_vscrollbar.normal.border.width.bottom = 0;
    p_css->css_vscrollbar.focus.border.width.top = 0;
    p_css->css_vscrollbar.focus.border.width.right = 0;
    p_css->css_vscrollbar.focus.border.width.bottom = 0;
    p_css->css_vscrollbar.disable.border.width.top = 0;
    p_css->css_vscrollbar.disable.border.width.right = 0;
    p_css->css_vscrollbar.disable.border.width.bottom = 0;

    p_css->item_h = 28;
    p_css->vscrollbar_w = 20;
    p_css->height_max = 362;
}

void klbwnd_combo_menu_css_deinit(klbwnd_combo_menu_t* p_menu)
{
    klbuicssex_attributes_quit(&p_menu->css.normal);
    klbwnd_button_css_deinit(&p_menu->css.css_item);
    klbwnd_vscrollbar_css_deinit(&p_menu->css.css_vscrollbar);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

void klbwnd_combo_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_combo_menu_t* p_menu = (klbwnd_combo_menu_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_combo_menu_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_PEEK_EVENT | KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    p_menu->col_count = KLBWND_COMBO_MENU_col_1;
    klbwnd_combo_menu_init_subwnds(p_wnd);
}

void klbwnd_combo_menu_deinit(klb_wnd_t* p_wnd)
{
}

klb_wnd_t* klbwnd_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_combo_menu_t));

    klbwnd_combo_menu_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_combo_menu_destroy;

    return p_wnd;
}

// end
