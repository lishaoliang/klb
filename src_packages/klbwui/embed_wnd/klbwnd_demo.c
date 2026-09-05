// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_demo.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


#define KLBWND_DEMO_TEXT        "kdemo"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_demo_deinit_attribute(klbwnd_demo_t* p_demo);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_demo_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_demo_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_demo_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_demo_t* p_demo = (klbwnd_demo_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    if (NULL == p_demo->p_css)
    {
        return 0;
    }

    klbwnd_demo_css_t* p_css = p_demo->p_css;
    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, &paint_rect, &p_css->normal.background);

    // 有背景图时图内自带边框; 纯色才画 border
    if (NULL == p_css->normal.background.image || 0 == sdslen(p_css->normal.background.image))
    {
        klbuicssex_draw_border(p_wnd, &paint_rect, &p_css->normal.border);
    }

    klbuicssex_draw_text(p_wnd, KLBWND_DEMO_TEXT, &paint_rect, &p_css->normal.border, &p_css->padding, &p_css->normal.text, &p_css->normal.font);

    return 0;
}

static int klbwnd_demo_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (KLBUI_onpaint == msg)
    {
        return klbwnd_demo_on_paint(p_wnd);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_demo_set_css(klb_wnd_t* p_wnd, klbwnd_demo_css_t* p_css)
{
    klbwnd_demo_t* p_demo = (klbwnd_demo_t*)p_wnd->ctrl;

    p_demo->p_css = p_css;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_demo_init_attribute(klbwnd_demo_t* p_demo)
{
}

static void klbwnd_demo_deinit_attribute(klbwnd_demo_t* p_demo)
{
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_demo_css_init(klbwnd_demo_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;
    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
}

void klbwnd_demo_css_deinit(klbwnd_demo_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
}

void klbwnd_demo_css_copy(klbwnd_demo_css_t* p_dst, klbwnd_demo_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;
    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_demo_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_demo_t* p_demo = (klbwnd_demo_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_demo_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    klbwnd_demo_init_attribute(p_demo);
}

void klbwnd_demo_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_demo_t* p_demo = (klbwnd_demo_t*)p_wnd->ctrl;

    klbwnd_demo_deinit_attribute(p_demo);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_demo_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_demo_t));

    klbwnd_demo_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_demo_destroy;

    return p_wnd;
}

// end
