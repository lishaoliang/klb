// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_radio.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_radio_deinit_attribute(klbwnd_radio_t* p_radio);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_radio_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_radio_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_radio_on_paint_status(klb_wnd_t* p_wnd, klbwnd_radio_t* p_radio, klbwnd_radio_css_t* p_css, klbwnd_radio_attributes_t* p_attr, klb_rect_t* p_rect, bool is_check)
{
    (void)p_radio;
    (void)p_css;

    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    // 有背景图时图内自带边框; 纯色才画 border + 选中指示
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

        if (is_check)
        {
            klb_rect_t check_rect = { p_rect->x + p_rect->w / 4,
                                      p_rect->y + p_rect->h / 4,
                                      p_rect->w / 2,
                                      p_rect->h / 2 };

            klb_wnd_draw_fill_rect2(p_wnd, &check_rect, p_attr->text.color);
        }
    }
}

static int klbwnd_radio_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_radio_css_t* p_css = p_radio->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (klb_wnd_is_check(p_wnd))
    {
        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->check_disable, &paint_rect, true);
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->check_focus, &paint_rect, true);
        }
        else
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->check_normal, &paint_rect, true);
        }
    }
    else
    {
        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->disable, &paint_rect, false);
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->focus, &paint_rect, false);
        }
        else
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->normal, &paint_rect, false);
        }
    }

    return 0;
}

static int klbwnd_radio_on_click(klb_wnd_t* p_wnd)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    klb_wnd_check(p_wnd, !klb_wnd_is_check(p_wnd));
    klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
    klb_wnd_update(p_wnd);

    return 0;
}

static int klbwnd_radio_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_radio_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_radio_on_click(p_wnd);

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_radio_set_css(klb_wnd_t* p_wnd, klbwnd_radio_css_t* p_css)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_radio->p_css = p_css;
}

void klbwnd_radio_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_radio->index = index;
}

int klbwnd_radio_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    return p_radio->index;
}

void klbwnd_radio_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_radio->title = klb_sds_assign(p_radio->title, p_title);
}

const sds klbwnd_radio_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    return p_radio->title;
}

void klbwnd_radio_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_radio->value = klb_sds_assign(p_radio->value, p_value);
}

const sds klbwnd_radio_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    return p_radio->value;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_radio_init_attribute(klbwnd_radio_t* p_radio)
{
    p_radio->index = 0;
    p_radio->title = NULL;
    p_radio->value = NULL;
}

static void klbwnd_radio_deinit_attribute(klbwnd_radio_t* p_radio)
{
    KLB_FREE_BY(p_radio->title, sdsfree);
    KLB_FREE_BY(p_radio->value, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_radio_attributes_init(klbwnd_radio_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
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

static void klbwnd_radio_attributes_deinit(klbwnd_radio_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_radio_attributes_copy(klbwnd_radio_attributes_t* p_dst, const klbwnd_radio_attributes_t* p_src)
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

void klbwnd_radio_css_init(klbwnd_radio_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_radio_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_radio_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_radio_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_radio_attributes_init(&p_css->check_normal, &p_default->normal);
    klbwnd_radio_attributes_init(&p_css->check_focus, &p_default->focus);
    klbwnd_radio_attributes_init(&p_css->check_disable, &p_default->disable);
}

void klbwnd_radio_css_deinit(klbwnd_radio_css_t* p_css)
{
    klbwnd_radio_attributes_deinit(&p_css->normal);
    klbwnd_radio_attributes_deinit(&p_css->focus);
    klbwnd_radio_attributes_deinit(&p_css->disable);

    klbwnd_radio_attributes_deinit(&p_css->check_normal);
    klbwnd_radio_attributes_deinit(&p_css->check_focus);
    klbwnd_radio_attributes_deinit(&p_css->check_disable);
}

void klbwnd_radio_css_copy(klbwnd_radio_css_t* p_dst, klbwnd_radio_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_radio_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_radio_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_radio_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_radio_attributes_copy(&p_dst->check_normal, &p_src->check_normal);
    klbwnd_radio_attributes_copy(&p_dst->check_focus, &p_src->check_focus);
    klbwnd_radio_attributes_copy(&p_dst->check_disable, &p_src->check_disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_radio_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_radio_on_control;     // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = 0x0;

    klbwnd_radio_init_attribute(p_radio);
}

void klbwnd_radio_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    klbwnd_radio_deinit_attribute(p_radio);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_radio_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_radio_t));

    klbwnd_radio_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_radio_destroy;

    return p_wnd;
}

// end
