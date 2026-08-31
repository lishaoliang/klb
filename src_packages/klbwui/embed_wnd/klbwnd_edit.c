// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_edit.h"
#include "klbwui/embed_shwnd/klbshw_edit_menu.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_util.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_edit_deinit_attribute(klbwnd_edit_t* p_edit);
static void klbwnd_edit_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_edit_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_edit_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_edit_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_edit_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static bool klbwnd_edit_has_title(const klbwnd_edit_t* p_edit)
{
    return (NULL != p_edit->title && 0 < sdslen(p_edit->title));
}

static int klbwnd_edit_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_edit_css_t* p_css = p_edit->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_edit_attributes_t* p_attr = NULL;

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        p_attr = &p_css->disable;
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        p_attr = &p_css->focus;
    }
    else
    {
        p_attr = &p_css->normal;
    }

    klbwnd_edit_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    if (klbwnd_edit_has_title(p_edit))
    {
        klbuicssex_draw_text(p_wnd, p_edit->title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }

    return 0;
}

static int on_menu_popup_end_klbwnd_edit(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, const sds value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;

    if (ok)
    {
        klbwnd_edit_set_title(p_wnd, value);
        klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
        klb_wnd_update(p_wnd);
    }

    (void)p_wnd_menu;
    return 0;
}

static int klbwnd_edit_on_click(klb_wnd_t* p_wnd, klbwnd_edit_t* p_edit)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    if (NULL == p_edit->p_menu)
    {
        return 0;
    }

    klbshw_edit_menu_bind(p_edit->p_menu, on_menu_popup_end_klbwnd_edit, p_wnd);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_edit_menu_wh(p_edit->p_menu, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_edit->p_menu, x, y);
    klb_wnd_resize(p_edit->p_menu, menu_w, menu_h);

    klbshw_edit_menu_set_page_letter(p_edit->p_menu, KLBSHW_EDIT_MENU_lowercase);
    klbshw_edit_menu_set_hide_letter(p_edit->p_menu, false);
    klbshw_edit_menu_layout(p_edit->p_menu);

    const char* p_value = "";
    if (NULL != p_edit->title)
    {
        p_value = p_edit->title;
    }
    klbshw_edit_menu_set_value(p_edit->p_menu, p_value);

    klb_gui_popup_wnd(p_wnd->p_gui, p_edit->p_menu);

    return 0;
}

static int klbwnd_edit_on_focusdelay(klb_wnd_t* p_wnd, klbwnd_edit_t* p_edit)
{
    klbwnd_edit_css_t* p_css = p_edit->p_css;

    if (NULL == p_css || !klb_wnd_is_dyntip(p_wnd))
    {
        return 0;
    }

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;
    klbuiutil_remove_margin(&rect, &p_css->margin);

    int font_size = p_css->normal.font.size;
    if (font_size < p_css->focus.font.size)
    {
        font_size = p_css->focus.font.size;
    }
    if (font_size < p_css->disable.font.size)
    {
        font_size = p_css->disable.font.size;
    }

    if (klbwnd_edit_has_title(p_edit))
    {
        klbwnd_edit_update_tip_dynamic(p_wnd, &rect, p_edit->title, font_size);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }

    return 0;
}

static int klbwnd_edit_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_edit_on_paint(p_wnd);
        break;

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_edit_on_click(p_wnd, p_edit);
        break;

    case KLBUI_focusdelay:
        klbwnd_edit_on_focusdelay(p_wnd, p_edit);
        break;

    default:
        break;
    }

    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;
    return 0;
}

static void klbwnd_edit_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size)
{
    if (NULL == p_wnd || NULL == p_rect || NULL == title)
    {
        return;
    }

    int txt_w = 0;
    klb_wnd_text_size2(p_wnd, title, (int)sdslen(title), font_size, &txt_w, NULL);

    if (p_rect->w < txt_w)
    {
        klb_wnd_set_tip_dynamic(p_wnd, title);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_edit_set_css(klb_wnd_t* p_wnd, klbwnd_edit_css_t* p_css)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    p_edit->p_css = p_css;
}

void klbwnd_edit_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    p_edit->title = klb_sds_assign(p_edit->title, p_title);
}

const sds klbwnd_edit_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    return p_edit->title;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_edit_init_attribute(klb_wnd_t* p_wnd, klbwnd_edit_t* p_edit)
{
    klb_wnd_dyntip(p_wnd, true);

    p_edit->title = NULL;
    p_edit->p_menu = NULL;
}

static void klbwnd_edit_deinit_attribute(klbwnd_edit_t* p_edit)
{
    KLB_FREE_BY(p_edit->title, sdsfree);
    p_edit->p_menu = NULL;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_edit_attributes_init(klbwnd_edit_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_edit_attributes_deinit(klbwnd_edit_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_edit_attributes_copy(klbwnd_edit_attributes_t* p_dst, const klbwnd_edit_attributes_t* p_src)
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
// css init / deinit

void klbwnd_edit_css_init(klbwnd_edit_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_edit_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_edit_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_edit_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_left;
    p_css->focus.text.align = KLBUICSS_text1_left;
    p_css->disable.text.align = KLBUICSS_text1_left;
}

void klbwnd_edit_css_deinit(klbwnd_edit_css_t* p_css)
{
    klbwnd_edit_attributes_deinit(&p_css->normal);
    klbwnd_edit_attributes_deinit(&p_css->focus);
    klbwnd_edit_attributes_deinit(&p_css->disable);
}

void klbwnd_edit_css_copy(klbwnd_edit_css_t* p_dst, klbwnd_edit_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_edit_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_edit_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_edit_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_edit_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_edit_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = KLB_WND_STYLE_FOCUS_DELAY;

    klbwnd_edit_init_attribute(p_wnd, p_edit);

    p_edit->p_menu = klbui_shwnd_get_edit_menu(p_gui);
}

void klbwnd_edit_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_edit_t* p_edit = (klbwnd_edit_t*)p_wnd->ctrl;

    klbwnd_edit_deinit_attribute(p_edit);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_edit_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_edit_t));

    klbwnd_edit_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_edit_destroy;

    return p_wnd;
}

// end
