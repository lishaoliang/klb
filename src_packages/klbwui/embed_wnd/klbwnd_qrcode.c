// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_qrcode.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"
#include <string.h>

#ifndef __KLB_NO_QRENCODE__
#include "qrencode.h"
#endif


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_qrcode_deinit_attribute(klbwnd_qrcode_t* p_qr);
static void deinit_qrcode_klbwnd_qrcode(klbwnd_qrcode_t* p_qr);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_qrcode_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

#ifndef __KLB_NO_QRENCODE__
static void klbwnd_qrcode_draw_modules(klb_wnd_t* p_wnd, QRcode* p_code, klb_rect_t* p_rect)
{
    uint32_t color_bg = KLB_ARGB8888(255, 255, 255, 255);
    uint32_t color_fg = KLB_ARGB8888(255, 0, 0, 0);
    int width = p_code->width;
    int size = 4;
    int ox = 0;
    int oy = 0;
    int i = 0;
    int j = 0;
    unsigned char* p = p_code->data;

    if (0 < width)
    {
        int side = MIN(p_rect->w, p_rect->h);

        size = side / width;
        if (size < 1)
        {
            size = 1;
        }

        ox = (p_rect->w - size * width) / 2;
        oy = (p_rect->h - size * width) / 2;
    }

    for (j = 0; j < width; j++)
    {
        for (i = 0; i < width; i++)
        {
            klb_rect_t cell;

            cell.x = p_rect->x + ox + i * size;
            cell.y = p_rect->y + oy + j * size;
            cell.w = size;
            cell.h = size;

            if (0 != (*p & 1))
            {
                klb_wnd_draw_fill_rect2(p_wnd, &cell, color_fg);
            }
            else
            {
                klb_wnd_draw_fill_rect2(p_wnd, &cell, color_bg);
            }

            p++;
        }
    }
}
#endif

static int klbwnd_qrcode_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_qrcode_css_t* p_css = p_qr->p_css;

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

#ifndef __KLB_NO_QRENCODE__
    if (NULL != p_qr->p_qrcode)
    {
        klb_rect_t qr_rect = paint_rect;

        qr_rect.x += p_css->padding.left + p_css->normal.border.width.left;
        qr_rect.y += p_css->padding.top + p_css->normal.border.width.top;
        qr_rect.w -= (p_css->padding.left + p_css->padding.right
            + p_css->normal.border.width.left + p_css->normal.border.width.right);
        qr_rect.h -= (p_css->padding.top + p_css->padding.bottom
            + p_css->normal.border.width.top + p_css->normal.border.width.bottom);

        klbwnd_qrcode_draw_modules(p_wnd, (QRcode*)p_qr->p_qrcode, &qr_rect);
    }
#else
    (void)p_qr;
#endif

    return 0;
}

static int klbwnd_qrcode_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    if (KLBUI_onpaint == msg)
    {
        return klbwnd_qrcode_on_paint(p_wnd);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// qrcode encode

static void deinit_qrcode_klbwnd_qrcode(klbwnd_qrcode_t* p_qr)
{
#ifndef __KLB_NO_QRENCODE__
    if (NULL != p_qr->p_qrcode)
    {
        QRcode_free((QRcode*)p_qr->p_qrcode);
        p_qr->p_qrcode = NULL;
    }
#else
    p_qr->p_qrcode = NULL;
#endif
}

static void encode_qrcode_klbwnd_qrcode(klbwnd_qrcode_t* p_qr)
{
    deinit_qrcode_klbwnd_qrcode(p_qr);

    if (NULL == p_qr->value || 0 == sdslen(p_qr->value))
    {
        return;
    }

#ifndef __KLB_NO_QRENCODE__
    p_qr->p_qrcode = QRcode_encodeString8bit(p_qr->value, 0, QR_ECLEVEL_M);
#else
    (void)p_qr;
#endif
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_qrcode_set_css(klb_wnd_t* p_wnd, klbwnd_qrcode_css_t* p_css)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_qr->p_css = p_css;
}

void klbwnd_qrcode_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_qr->title = klb_sds_assign(p_qr->title, p_title);
}

const sds klbwnd_qrcode_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    return p_qr->title;
}

void klbwnd_qrcode_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;
    bool same = false;

    if (NULL != p_qr->value && NULL != p_value && 0 == strcmp(p_qr->value, p_value))
    {
        same = true;
    }

    if (same)
    {
        return;
    }

    p_qr->value = klb_sds_assign(p_qr->value, p_value);
    encode_qrcode_klbwnd_qrcode(p_qr);
}

const sds klbwnd_qrcode_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    return p_qr->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_qrcode_init_attribute(klbwnd_qrcode_t* p_qr)
{
    p_qr->title = NULL;
    p_qr->value = NULL;
    p_qr->p_qrcode = NULL;
}

static void klbwnd_qrcode_deinit_attribute(klbwnd_qrcode_t* p_qr)
{
    KLB_FREE_BY(p_qr->title, sdsfree);
    KLB_FREE_BY(p_qr->value, sdsfree);
    deinit_qrcode_klbwnd_qrcode(p_qr);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_qrcode_attributes_init(klbwnd_qrcode_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_qrcode_attributes_deinit(klbwnd_qrcode_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_qrcode_attributes_copy(klbwnd_qrcode_attributes_t* p_dst, const klbwnd_qrcode_attributes_t* p_src)
{
    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_qrcode_css_init(klbwnd_qrcode_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_qrcode_attributes_init(&p_css->normal, &p_default->normal);

    p_css->normal.background.color = KLB_ARGB8888(255, 255, 255, 255);
}

void klbwnd_qrcode_css_deinit(klbwnd_qrcode_css_t* p_css)
{
    klbwnd_qrcode_attributes_deinit(&p_css->normal);
}

void klbwnd_qrcode_css_copy(klbwnd_qrcode_css_t* p_dst, klbwnd_qrcode_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_qrcode_attributes_copy(&p_dst->normal, &p_src->normal);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_qrcode_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_qrcode_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    klbwnd_qrcode_init_attribute(p_qr);
}

void klbwnd_qrcode_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qr = (klbwnd_qrcode_t*)p_wnd->ctrl;

    klbwnd_qrcode_deinit_attribute(p_qr);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_qrcode_t));

    klbwnd_qrcode_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_qrcode_destroy;

    return p_wnd;
}

// end
