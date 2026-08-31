// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_messagebox.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_messagebox_deinit_attribute(klbwnd_messagebox_t* p_msgbox);
static void klbwnd_messagebox_relayout(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_messagebox_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_messagebox_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_messagebox_css_t* p_css = p_msgbox->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwuicss_draw_background(p_wnd, &paint_rect, &p_css->normal.background);

    if (NULL == p_css->normal.background.image || 0 == sdslen(p_css->normal.background.image))
    {
        klbuicssex_draw_border(p_wnd, &paint_rect, &p_css->normal.border);
    }

    if (0 < p_msgbox->title_h && 0 < paint_rect.w)
    {
        klb_rect_t bar = paint_rect;

        bar.h = p_msgbox->title_h;
        if (bar.h > paint_rect.h)
        {
            bar.h = paint_rect.h;
        }

        klbwuicss_draw_background(p_wnd, &bar, &p_css->normal.titlebar);

        if (NULL != p_msgbox->title && 0 < sdslen(p_msgbox->title))
        {
            klbuicssex_draw_text(p_wnd, p_msgbox->title, &bar, &p_css->normal.border,
                &p_css->padding, &p_css->normal.text, &p_css->normal.font);
        }
    }

    return 0;
}

static int klbwnd_messagebox_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    if (KLBUI_onpaint == msg)
    {
        return klbwnd_messagebox_on_paint(p_wnd);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 子窗口响应

static int on_btn_ok_klbwnd_messagebox(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_msgbox = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd_msgbox->ctrl;

    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        p_msgbox->value = KLBWND_MESSAGEBOX_ok;
        klb_gui_messagebox_end(p_wnd->p_gui);
        klb_wnd_call_command(p_wnd_msgbox, KLBUI_onchange, NULL, NULL, 0, 0);
    }

    return 0;
}

static int on_btn_cancel_klbwnd_messagebox(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_msgbox = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd_msgbox->ctrl;

    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        p_msgbox->value = KLBWND_MESSAGEBOX_cancel;
        klb_gui_messagebox_end(p_wnd->p_gui);
        klb_wnd_call_command(p_wnd_msgbox, KLBUI_onchange, NULL, NULL, 0, 0);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_messagebox_set_css(klb_wnd_t* p_wnd, klbwnd_messagebox_css_t* p_css)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_msgbox->p_css = p_css;

    if (NULL != p_css)
    {
        klbwnd_static_set_css(p_msgbox->p_sta_txt, &p_css->css_sta);
        klbwnd_button_set_css(p_msgbox->p_btn_ok, &p_css->css_btn);
        klbwnd_button_set_css(p_msgbox->p_btn_cancel, &p_css->css_btn);
    }
}

void klbwnd_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_msgbox->title = klb_sds_assign(p_msgbox->title, p_title);
}

const sds klbwnd_messagebox_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    return p_msgbox->title;
}

void klbwnd_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klbwnd_static_set_title(p_msgbox->p_sta_txt, p_text);
}

int klbwnd_messagebox_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    return p_msgbox->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_messagebox_init_attribute(klbwnd_messagebox_t* p_msgbox)
{
    p_msgbox->title = NULL;
    p_msgbox->value = KLBWND_MESSAGEBOX_close;
    p_msgbox->title_h = KLBWND_MESSAGEBOX_title_h;
}

static void klbwnd_messagebox_deinit_attribute(klbwnd_messagebox_t* p_msgbox)
{
    KLB_FREE_BY(p_msgbox->title, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_messagebox_attributes_init(klbwnd_messagebox_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;

    p_attr->titlebar.image = NULL;
    p_attr->titlebar.color = KLB_ARGB8888(255, 45, 45, 48);
    p_attr->titlebar.image_mode = 0;
    p_attr->titlebar.image_flags = 0;
}

static void klbwnd_messagebox_attributes_deinit(klbwnd_messagebox_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
    KLB_FREE_BY(p_attr->titlebar.image, sdsfree);
}

static void klbwnd_messagebox_attributes_copy(klbwnd_messagebox_attributes_t* p_dst, const klbwnd_messagebox_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;

    p_dst->titlebar.color = p_src->titlebar.color;
    p_dst->titlebar.image = klb_sds_assign(p_dst->titlebar.image, p_src->titlebar.image);
    p_dst->titlebar.image_mode = p_src->titlebar.image_mode;
    p_dst->titlebar.image_flags = p_src->titlebar.image_flags;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_messagebox_css_init(klbwnd_messagebox_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_messagebox_attributes_init(&p_css->normal, &p_default->normal);
    p_css->normal.text.align = KLBUICSS_text1_left;

    klbwnd_static_css_init(&p_css->css_sta, p_gui);
    klbwnd_button_css_init(&p_css->css_btn, p_gui);
}

void klbwnd_messagebox_css_deinit(klbwnd_messagebox_css_t* p_css)
{
    klbwnd_messagebox_attributes_deinit(&p_css->normal);
    klbwnd_static_css_deinit(&p_css->css_sta);
    klbwnd_button_css_deinit(&p_css->css_btn);
}

void klbwnd_messagebox_css_copy(klbwnd_messagebox_css_t* p_dst, klbwnd_messagebox_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_messagebox_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_static_css_copy(&p_dst->css_sta, &p_src->css_sta);
    klbwnd_button_css_copy(&p_dst->css_btn, &p_src->css_btn);
}


//////////////////////////////////////////////////////////////////////////
// 子窗口

static void klbwnd_messagebox_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;
    int btn_w = 120;
    int btn_h = 32;

    p_msgbox->p_sta_txt = klbwnd_static_create(p_gui, 0, 0, btn_w, btn_h);
    klb_wnd_push_child(p_wnd, p_msgbox->p_sta_txt);

    p_msgbox->p_btn_ok = klbwnd_button_create(p_gui, 0, 0, btn_w, btn_h);
    klbwnd_button_set_title(p_msgbox->p_btn_ok, "Ok");
    klb_wnd_push_child(p_wnd, p_msgbox->p_btn_ok);
    klb_wnd_bind_command(p_msgbox->p_btn_ok, on_btn_ok_klbwnd_messagebox, p_wnd);

    p_msgbox->p_btn_cancel = klbwnd_button_create(p_gui, 0, 0, btn_w, btn_h);
    klbwnd_button_set_title(p_msgbox->p_btn_cancel, "Cancel");
    klb_wnd_push_child(p_wnd, p_msgbox->p_btn_cancel);
    klb_wnd_bind_command(p_msgbox->p_btn_cancel, on_btn_cancel_klbwnd_messagebox, p_wnd);
}

static void klbwnd_messagebox_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
    klb_rect_t rect = p_wnd->pos.rect_in_parent;
    int w = rect.w;
    int h = rect.h;
    int title_h = p_msgbox->title_h;
    int btn_w = 120;
    int btn_h = 32;
    int sta_w = w - 20;
    int sta_h = 32;
    int sta_x = 10;
    int sta_y = title_h + (h - title_h - btn_h - 20 - sta_h) / 2;
    int x = w - btn_w - 10;
    int y = h - btn_h - 10;

    if (sta_y < title_h + 8)
    {
        sta_y = title_h + 8;
    }

    klb_wnd_resize(p_msgbox->p_sta_txt, sta_w, sta_h);
    klb_wnd_move(p_msgbox->p_sta_txt, sta_x, sta_y);

    klb_wnd_move(p_msgbox->p_btn_cancel, x, y);

    x -= (btn_w + 6);
    klb_wnd_move(p_msgbox->p_btn_ok, x, y);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_messagebox_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_messagebox_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_LAYER_MSGBOX | KLB_WND_STYLE_PEEK_EVENT;

    klbwnd_messagebox_init_attribute(p_msgbox);
    klbwnd_messagebox_init_subwnds(p_wnd);
    klbwnd_messagebox_relayout(p_wnd);
}

void klbwnd_messagebox_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_deinit_attribute(p_msgbox);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_messagebox_t));

    klbwnd_messagebox_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_messagebox_destroy;

    return p_wnd;
}

// end
