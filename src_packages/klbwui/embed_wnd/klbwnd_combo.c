// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_combo.h"
#include "klbwui/embed_shwnd/klbshw_combo_menu.h"
#include "klbwui/core/klbwui_css.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbgui/klbui_util.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_sds.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_combo_deinit_attribute(klbwnd_combo_t* p_combo);


static bool klbwnd_combo_sds_eq(sds a, sds b)
{
    if (a == b)
    {
        return true;
    }

    if (NULL == a || NULL == b)
    {
        return false;
    }

    return (0 == sdscmp(a, b));
}


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_combo_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_combo_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_combo_on_paint_status(klb_wnd_t* p_wnd, klbwnd_combo_t* p_combo, klbwnd_combo_css_t* p_css, klbwnd_combo_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    if (NULL != p_combo->title && 0 < sdslen(p_combo->title))
    {
        klbuicssex_draw_text(p_wnd, p_combo->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }

    klb_rect_t rect_triangle = { 0 };
    rect_triangle.h = p_rect->h - 2;
    rect_triangle.w = rect_triangle.h / 2;
    rect_triangle.x = p_rect->x + p_rect->w - rect_triangle.w - 2;
    rect_triangle.y = p_rect->y + 1;

    klbuiutil_draw_triangle_down(p_wnd, &rect_triangle, p_attr->text.color);
}

static int klbwnd_combo_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_combo_css_t* p_css = p_combo->p_css;
    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_result_combo_menu_klbwnd_combo(void* ptr, bool ok, const sds value, const sds title)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;
    bool change = false;

    if (!ok)
    {
        return 0;
    }

    change = !klbwnd_combo_sds_eq(p_combo->value, value);

    p_combo->value = klb_sds_assign(p_combo->value, value);
    p_combo->title = klb_sds_assign(p_combo->title, title);

    if (change)
    {
        klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
    }

    klb_wnd_update(p_wnd);
    return 0;
}

static int klbwnd_combo_on_click(klb_wnd_t* p_wnd, klbwnd_combo_t* p_combo)
{
    int menu_w = 0;
    int menu_h = 0;
    int screen_w = 0;
    int screen_h = 0;
    klb_rect_t rect = { 0 };
    int x = 0;
    int y = 0;

    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    if (NULL == p_combo->p_menu)
    {
        return 0;
    }

    klbshw_combo_menu_bind(p_combo->p_menu, on_result_combo_menu_klbwnd_combo, p_wnd, &p_combo->data, &menu_w, &menu_h, false, 0);

    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);
    rect = p_wnd->pos.rect_in_canvas;

    x = (rect.x + menu_w <= screen_w) ? rect.x : (screen_w - menu_w);
    y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : (rect.y - menu_h);

    klb_wnd_move(p_combo->p_menu, x, y);
    klb_wnd_resize(p_combo->p_menu, menu_w, menu_h);
    klb_gui_popup_wnd(p_wnd->p_gui, p_combo->p_menu);

    return 0;
}

static int klbwnd_combo_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_combo_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_combo_on_click(p_wnd, p_combo);

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_combo_set_css(klb_wnd_t* p_wnd, klbwnd_combo_css_t* p_css)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->p_css = p_css;
}

void klbwnd_combo_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->index = index;
}

int klbwnd_combo_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->index;
}

void klbwnd_combo_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->title = klb_sds_assign(p_combo->title, p_title);
}

const sds klbwnd_combo_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->title;
}

void klbwnd_combo_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->value = klb_sds_assign(p_combo->value, p_value);
}

const sds klbwnd_combo_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->value;
}

void klbwnd_combo_append(klb_wnd_t* p_wnd, klb_map_t* p_array)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;
    int count = 0;
    int i = 0;

    if (NULL == p_array)
    {
        return;
    }

    count = klb_map_array_size(p_array);
    for (i = 0; i < count; i++)
    {
        klb_map_append_adt_clone(&p_combo->data, klb_map_idx_to_adt(p_array, i));
    }
}

void klbwnd_combo_clear(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    klb_map_clear(&p_combo->data);

    KLB_FREE_BY(p_combo->title, sdsfree);
    KLB_FREE_BY(p_combo->value, sdsfree);
    p_combo->title = NULL;
    p_combo->value = NULL;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_combo_attributes_init(klbwnd_combo_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_combo_attributes_deinit(klbwnd_combo_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_combo_attributes_copy(klbwnd_combo_attributes_t* p_dst, const klbwnd_combo_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css

void klbwnd_combo_css_init(klbwnd_combo_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_combo_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_combo_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_combo_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_combo_css_deinit(klbwnd_combo_css_t* p_css)
{
    klbwnd_combo_attributes_deinit(&p_css->normal);
    klbwnd_combo_attributes_deinit(&p_css->focus);
    klbwnd_combo_attributes_deinit(&p_css->disable);
}

void klbwnd_combo_css_copy(klbwnd_combo_css_t* p_dst, klbwnd_combo_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_combo_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_combo_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_combo_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

static void klbwnd_combo_init_attribute(klbwnd_combo_t* p_combo)
{
    p_combo->index = 0;
    p_combo->title = NULL;
    p_combo->value = NULL;

    klb_map_init(&p_combo->data);
}

static void klbwnd_combo_deinit_attribute(klbwnd_combo_t* p_combo)
{
    klb_map_quit(&p_combo->data);

    KLB_FREE_BY(p_combo->title, sdsfree);
    KLB_FREE_BY(p_combo->value, sdsfree);
    p_combo->title = NULL;
    p_combo->value = NULL;
}

void klbwnd_combo_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_combo_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = 0x0;

    klbwnd_combo_init_attribute(p_combo);
    p_combo->p_menu = klbui_shwnd_get_combo_menu(p_gui);
}

void klbwnd_combo_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    klbwnd_combo_deinit_attribute(p_combo);
}

klb_wnd_t* klbwnd_combo_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_combo_t));

    klbwnd_combo_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_combo_destroy;

    return p_wnd;
}

// end
