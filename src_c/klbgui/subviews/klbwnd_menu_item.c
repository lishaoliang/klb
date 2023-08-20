// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_menu_item.h"
#include "klbgui/klbui_util.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_menu_item_quit_attribute(klbwnd_menu_item_t* p_item);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_menu_item_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    klbwnd_menu_item_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_menu_item_on_paint_status(klb_wnd_t* p_wnd, klbwnd_menu_item_t* p_item, klbwnd_menu_item_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    //klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    // 标题文本
    klb_rect_t rect_txt = *p_rect;
    rect_txt.w -= (rect_txt.h / 2 + 2);
    klbuicssex_draw_text(p_wnd, p_item->title, &rect_txt, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);

    // 右侧向右三角
    if (p_item->has_2rd)
    {
        klb_rect_t rect_2rd = { 0 };
        rect_2rd.h = p_rect->h / 2 - 4;
        rect_2rd.w = rect_2rd.h;
        rect_2rd.x = p_rect->x + p_rect->w - rect_2rd.w - 2;
        rect_2rd.y = p_rect->y + (p_rect->h - rect_2rd.h) / 2;

        klbuiutil_draw_triangle_right(p_wnd, &rect_2rd, p_attr->text.color);
    }
}

static int klbwnd_menu_item_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_menu_item_css_t* p_css = p_item->p_css;

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

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_menu_item_on_paint_status(p_wnd, p_item, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_menu_item_on_paint_status(p_wnd, p_item, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_menu_item_on_paint_status(p_wnd, p_item, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_menu_item_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_menu_item_on_paint(p_wnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_menu_item_set_css(klb_wnd_t* p_wnd, klbwnd_menu_item_css_t* p_css)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    p_item->p_css = p_css;
}

void klbwnd_menu_item_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    p_item->title = sdscpy(p_item->title, p_title);
}

const sds klbwnd_menu_item_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    return p_item->title;
}

void klbwnd_menu_item_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    p_item->value = sdscpy(p_item->value, p_value);
}

const sds klbwnd_menu_item_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    return p_item->value;
}

void klbwnd_menu_item_set_has_2rd(klb_wnd_t* p_wnd, bool has_2rd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    p_item->has_2rd = has_2rd;
}

bool klbwnd_menu_item_has_2rd(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;
    
    return p_item->has_2rd;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_menu_item_init_attribute(klb_wnd_t* p_wnd, klbwnd_menu_item_t* p_item)
{
    p_item->title = sdsempty();
    p_item->value = sdsempty();

    p_item->has_2rd = false;
}

static void klbwnd_menu_item_quit_attribute(klbwnd_menu_item_t* p_item)
{
    KLB_FREE_BY(p_item->title, sdsfree);
    KLB_FREE_BY(p_item->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_menu_item_css_init(klbwnd_menu_item_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_center;
    p_css->focus.text.align = KLBUICSS_text1_center;
    p_css->disable.text.align = KLBUICSS_text1_center;

    p_css->normal.background.color = KLB_ARGB8888(255, 32, 32, 32);
    p_css->focus.background.color = KLB_ARGB8888(255, 42, 42, 42);
}

void klbwnd_menu_item_css_quit(klbwnd_menu_item_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_menu_item_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_menu_item_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化内部变量
    klbwnd_menu_item_init_attribute(p_wnd, p_item);
}

void klbwnd_menu_item_quit(klb_wnd_t* p_wnd)
{
    klbwnd_menu_item_t* p_item = (klbwnd_menu_item_t*)p_wnd->ctrl;

    klbwnd_menu_item_quit_attribute(p_item);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_menu_item_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_menu_item_t));

    klbwnd_menu_item_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_menu_item_destroy;

    return p_wnd;
}
