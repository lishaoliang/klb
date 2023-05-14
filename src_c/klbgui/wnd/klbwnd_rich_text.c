// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_rich_text.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_rich_text_quit_attribute(klbwnd_rich_text_t* p_rich);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_rich_text_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    klbwnd_rich_text_quit_attribute(p_rich);

    KLB_FREE(p_wnd);
}

static void klbwnd_rich_text_on_paint_status(klb_wnd_t* p_wnd, klbwnd_rich_text_t* p_rich, klbwnd_rich_text_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
    }

    // 标题文本
    klbuicssex_draw_text(p_wnd, p_rich->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_rich_text_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_rich_text_css_t* p_css = p_rich->p_css;

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

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbwnd_rich_text_on_paint_status(p_wnd, p_rich, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_rich_text_on_paint_status(p_wnd, p_rich, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_rich_text_on_paint_status(p_wnd, p_rich, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_rich_text_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_rich_text_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_rich_text_set_css(klb_wnd_t* p_wnd, klbwnd_rich_text_css_t* p_css)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    p_rich->p_css = p_css;
}

void klbwnd_rich_text_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    p_rich->title = sdscpy(p_rich->title, p_title);
}

const sds klbwnd_rich_text_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    return p_rich->title;
}

void klbwnd_rich_text_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    p_rich->value = sdscpy(p_rich->value, p_value);
}

const sds klbwnd_rich_text_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    return p_rich->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_rich_text_init_attribute(klbwnd_rich_text_t* p_rich)
{
    p_rich->title = sdsempty();
    p_rich->value = sdsempty();
}

static void klbwnd_rich_text_quit_attribute(klbwnd_rich_text_t* p_rich)
{
    KLB_FREE_BY(p_rich->title, sdsfree);
    KLB_FREE_BY(p_rich->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_rich_text_css_init(klbwnd_rich_text_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_rich_text_css_quit(klbwnd_rich_text_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_rich_text_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_rich_text_on_control; // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_rich_text_init_attribute(p_rich);
}

void klbwnd_rich_text_quit(klb_wnd_t* p_wnd)
{
    klbwnd_rich_text_t* p_rich = (klbwnd_rich_text_t*)p_wnd->ctrl;

    klbwnd_rich_text_quit_attribute(p_rich);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_rich_text_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_rich_text_t));

    klbwnd_rich_text_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_rich_text_destroy;

    return p_wnd;
}
