// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_radio.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_radio_quit_attribute(klbwnd_radio_t* p_radio);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_radio_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    klbwnd_radio_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_radio_on_paint_status(klb_wnd_t* p_wnd, klbwnd_radio_t* p_radio, klbwnd_radio_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect, bool is_check)
{
    if (0 < sdslen(p_attr->background.image))
    {
        // 图片背景
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }
    else
    {
        // 纯色背景
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

        // 边框
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

        if (is_check)
        {
            klb_rect_t paint_rect = *p_rect;
            klb_rect_t check_rect = { paint_rect.x + paint_rect.w / 4,
                                        paint_rect.y + paint_rect.h / 4,
                                        paint_rect.w / 2,
                                        paint_rect.h / 2 };

            klb_wnd_draw_fill_rect2(p_wnd, &check_rect, p_attr->text.color /*KLB_ARGB8888(255, 220, 20, 20)*/);
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

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (klb_wnd_is_check(p_wnd))
    {
        // 选中

        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->on_disable, &paint_rect, true);
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->on_focus, &paint_rect, true);
        }
        else
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->on_normal, &paint_rect, true);
        }
    }
    else
    {
        // 未选中

        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->off_disable, &paint_rect, false);
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->off_focus, &paint_rect, false);
        }
        else
        {
            klbwnd_radio_on_paint_status(p_wnd, p_radio, p_css, &p_css->off_normal, &paint_rect, false);
        }
    }

    return 0;
}


static int klbwnd_radio_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_radio_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        {
            klb_wnd_check(p_wnd, !klb_wnd_is_check(p_wnd)); 
            klb_wnd_on_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
            klb_wnd_update(p_wnd);
        }
        break;

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

    p_radio->title = sdscpy(p_radio->title, p_title);
}

const sds klbwnd_radio_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    return p_radio->title;
}

void klbwnd_radio_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    p_radio->value = sdscpy(p_radio->value, p_value);
}

const sds klbwnd_radio_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    return p_radio->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_radio_init_attribute(klbwnd_radio_t* p_radio)
{
    p_radio->index = 0;
    p_radio->title = sdsempty();
    p_radio->value = sdsempty();
}

static void klbwnd_radio_quit_attribute(klbwnd_radio_t* p_radio)
{
    KLB_FREE_BY(p_radio->title, sdsfree);
    KLB_FREE_BY(p_radio->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_radio_css_init(klbwnd_radio_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->on_normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->on_focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->on_disable, &p_default->disable);

    klbuicssex_attributes_init(&p_css->off_normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->off_focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->off_disable, &p_default->disable);
}

void klbwnd_radio_css_quit(klbwnd_radio_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->on_normal);
    klbuicssex_attributes_quit(&p_css->on_focus);
    klbuicssex_attributes_quit(&p_css->on_disable);

    klbuicssex_attributes_quit(&p_css->off_normal);
    klbuicssex_attributes_quit(&p_css->off_focus);
    klbuicssex_attributes_quit(&p_css->off_disable);
}

void klbwnd_radio_css_copy(klbwnd_radio_css_t* p_dst, klbwnd_radio_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->on_normal, &p_src->on_normal);
    klbuicssex_attributes_copy(&p_dst->on_focus, &p_src->on_focus);
    klbuicssex_attributes_copy(&p_dst->on_disable, &p_src->on_disable);

    klbuicssex_attributes_copy(&p_dst->off_normal, &p_src->off_normal);
    klbuicssex_attributes_copy(&p_dst->off_focus, &p_src->off_focus);
    klbuicssex_attributes_copy(&p_dst->off_disable, &p_src->off_disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

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

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_radio_init_attribute(p_radio);
}

void klbwnd_radio_quit(klb_wnd_t* p_wnd)
{
    klbwnd_radio_t* p_radio = (klbwnd_radio_t*)p_wnd->ctrl;

    klbwnd_radio_quit_attribute(p_radio);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_radio_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_radio_t));

    klbwnd_radio_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_radio_destroy;

    return p_wnd;
}

