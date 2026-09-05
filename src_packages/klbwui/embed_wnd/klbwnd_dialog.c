// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_dialog.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_dialog_deinit_attribute(klbwnd_dialog_t* p_dlg);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_dialog_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_dialog_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_dialog_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_dialog_css_t* p_css = p_dlg->p_css;

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

    if (0 < p_dlg->title_h && 0 < paint_rect.w)
    {
        klb_rect_t bar = paint_rect;

        bar.h = p_dlg->title_h;
        if (bar.h > paint_rect.h)
        {
            bar.h = paint_rect.h;
        }

        klbwuicss_draw_background(p_wnd, &bar, &p_css->normal.titlebar);

        if (NULL != p_dlg->title && 0 < sdslen(p_dlg->title))
        {
            klbuicssex_draw_text(p_wnd, p_dlg->title, &bar, &p_css->normal.border,
                &p_css->padding, &p_css->normal.text, &p_css->normal.font);
        }
    }

    return 0;
}

static int klbwnd_dialog_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{

    if (KLBUI_onpaint == msg)
    {
        return klbwnd_dialog_on_paint(p_wnd);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_dialog_set_css(klb_wnd_t* p_wnd, klbwnd_dialog_css_t* p_css)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    p_dlg->p_css = p_css;
}

void klbwnd_dialog_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    p_dlg->title = klb_sds_assign(p_dlg->title, p_title);
}

const sds klbwnd_dialog_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    return p_dlg->title;
}

void klbwnd_dialog_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    p_dlg->value = klb_sds_assign(p_dlg->value, p_value);
}

const sds klbwnd_dialog_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    return p_dlg->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_dialog_init_attribute(klbwnd_dialog_t* p_dlg)
{
    p_dlg->title = NULL;
    p_dlg->value = NULL;
    p_dlg->title_h = KLBWND_DIALOG_title_h;
}

static void klbwnd_dialog_deinit_attribute(klbwnd_dialog_t* p_dlg)
{
    KLB_FREE_BY(p_dlg->title, sdsfree);
    KLB_FREE_BY(p_dlg->value, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_dialog_attributes_init(klbwnd_dialog_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_dialog_attributes_deinit(klbwnd_dialog_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
    KLB_FREE_BY(p_attr->titlebar.image, sdsfree);
}

static void klbwnd_dialog_attributes_copy(klbwnd_dialog_attributes_t* p_dst, const klbwnd_dialog_attributes_t* p_src)
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

void klbwnd_dialog_css_init(klbwnd_dialog_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_dialog_attributes_init(&p_css->normal, &p_default->normal);
    p_css->normal.text.align = KLBUICSS_text1_left;
}

void klbwnd_dialog_css_deinit(klbwnd_dialog_css_t* p_css)
{
    klbwnd_dialog_attributes_deinit(&p_css->normal);
}

void klbwnd_dialog_css_copy(klbwnd_dialog_css_t* p_dst, klbwnd_dialog_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_dialog_attributes_copy(&p_dst->normal, &p_src->normal);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_dialog_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_dialog_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_PEEK_EVENT;

    klbwnd_dialog_init_attribute(p_dlg);
}

void klbwnd_dialog_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_dialog_t* p_dlg = (klbwnd_dialog_t*)p_wnd->ctrl;

    klbwnd_dialog_deinit_attribute(p_dlg);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_dialog_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_dialog_t));

    klbwnd_dialog_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_dialog_destroy;

    return p_wnd;
}

// end
