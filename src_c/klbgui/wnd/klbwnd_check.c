// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_check.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_check_quit_attribute(klbwnd_check_t* p_check);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_check_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;

    klbwnd_check_quit(p_wnd);

    KLB_FREE(p_wnd);
}


static void klbwnd_check_on_paint_status(klb_wnd_t* p_wnd, klbwnd_check_t* p_check, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect, bool is_check)
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

static int klbwnd_check_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_check_css_t* p_css = p_check->p_css;

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
        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->on_disable, &paint_rect, true);   // 选中 - 不使能
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->on_focus, &paint_rect, true);   // 选中 - 聚焦
        }
        else
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->on, &paint_rect, true);         // 选中 - 常规
        }
    }
    else
    {
        if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->off_disable, &paint_rect, false);  // 未选中 - 不使能
        }
        else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->off_focus, &paint_rect, false);  // 未选中 - 常规
        }
        else
        {
            klbwnd_check_on_paint_status(p_wnd, p_check, &p_css->off, &paint_rect, false);        // 未选中 - 常规
        }
    }

    return 0;
}


static int klbwnd_check_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_check_on_paint(p_wnd);
        break;

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

void klbwnd_check_set_css(klb_wnd_t* p_wnd, klbwnd_check_css_t* p_css)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;

    p_check->p_css = p_css;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_check_init_attribute(klbwnd_check_t* p_check)
{

}

static void klbwnd_check_quit_attribute(klbwnd_check_t* p_check)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_check_css_init(klbwnd_check_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->on, &p_default->normal);
    klbuicssex_attributes_init(&p_css->on_focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->on_disable, &p_default->disable);

    klbuicssex_attributes_init(&p_css->off, &p_default->normal);
    klbuicssex_attributes_init(&p_css->off_focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->off_disable, &p_default->disable);
}

void klbwnd_check_css_quit(klbwnd_check_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->on);
    klbuicssex_attributes_quit(&p_css->on_focus);
    klbuicssex_attributes_quit(&p_css->on_disable);

    klbuicssex_attributes_quit(&p_css->off);
    klbuicssex_attributes_quit(&p_css->off_focus);
    klbuicssex_attributes_quit(&p_css->off_disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_check_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                       // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_check_on_control; // 这里指定
    p_wnd->vtable.on_command = NULL;                    // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                        // 由继承者重写
    p_wnd->vtable.on_get = NULL;                        // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_check_init_attribute(p_check);
}

void klbwnd_check_quit(klb_wnd_t* p_wnd)
{
    klbwnd_check_t* p_check = (klbwnd_check_t*)p_wnd->ctrl;

    klbwnd_check_quit_attribute(p_check);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_check_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_check_t));

    klbwnd_check_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_check_destroy;

    return p_wnd;
}
