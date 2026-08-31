// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_progress.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_progress_deinit_attribute(klbwnd_progress_t* p_prog);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_progress_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_progress_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_progress_on_paint_status(klb_wnd_t* p_wnd, klbwnd_progress_t* p_prog, klbwnd_progress_css_t* p_css, klbwnd_progress_attributes_t* p_attr, klb_rect_t* p_rect)
{
    (void)p_css;

    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    // 有背景图时图内自带边框; 纯色才画 border
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    if (0 < p_prog->value)
    {
        if (100 <= p_prog->value)
        {
            klb_rect_t rect_prog = { p_rect->x + 2, p_rect->y + 2, p_rect->w - 4, p_rect->h - 4 };
            klb_wnd_draw_fill_rect2(p_wnd, &rect_prog, p_attr->text.color);
        }
        else
        {
            int w_prog = (int)((int64_t)(p_rect->w - 4) * p_prog->value / 100);
            klb_rect_t rect_prog = { p_rect->x + 2, p_rect->y + 2, w_prog, p_rect->h - 4 };

            klb_wnd_draw_fill_rect2(p_wnd, &rect_prog, p_attr->text.color);
        }
    }
}

static int klbwnd_progress_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_progress_css_t* p_css = p_prog->p_css;

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
        klbwnd_progress_on_paint_status(p_wnd, p_prog, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_progress_on_paint_status(p_wnd, p_prog, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_progress_on_paint_status(p_wnd, p_prog, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int klbwnd_progress_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_progress_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_progress_set_css(klb_wnd_t* p_wnd, klbwnd_progress_css_t* p_css)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    p_prog->p_css = p_css;
}

void klbwnd_progress_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    p_prog->title = klb_sds_assign(p_prog->title, p_title);
}

const sds klbwnd_progress_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    return p_prog->title;
}

void klbwnd_progress_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    if (value < 0)
    {
        value = 0;
    }
    else if (100 < value)
    {
        value = 100;
    }

    p_prog->value = value;
}

int klbwnd_progress_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    return p_prog->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_progress_init_attribute(klbwnd_progress_t* p_prog)
{
    p_prog->title = NULL;
    p_prog->value = 0;
}

static void klbwnd_progress_deinit_attribute(klbwnd_progress_t* p_prog)
{
    KLB_FREE_BY(p_prog->title, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_progress_attributes_init(klbwnd_progress_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_progress_attributes_deinit(klbwnd_progress_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_progress_attributes_copy(klbwnd_progress_attributes_t* p_dst, const klbwnd_progress_attributes_t* p_src)
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

void klbwnd_progress_css_init(klbwnd_progress_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_progress_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_progress_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_progress_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_progress_css_deinit(klbwnd_progress_css_t* p_css)
{
    klbwnd_progress_attributes_deinit(&p_css->normal);
    klbwnd_progress_attributes_deinit(&p_css->focus);
    klbwnd_progress_attributes_deinit(&p_css->disable);
}

void klbwnd_progress_css_copy(klbwnd_progress_css_t* p_dst, klbwnd_progress_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_progress_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_progress_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_progress_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_progress_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_progress_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    klbwnd_progress_init_attribute(p_prog);
}

void klbwnd_progress_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_progress_t* p_prog = (klbwnd_progress_t*)p_wnd->ctrl;

    klbwnd_progress_deinit_attribute(p_prog);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_progress_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_progress_t));

    klbwnd_progress_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_progress_destroy;

    return p_wnd;
}

// end
