// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_hscrollbar.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_hscrollbar_quit_attribute(klbwnd_hscrollbar_t* p_hsc);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_hscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    klbwnd_hscrollbar_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_hscrollbar_on_paint_status(klb_wnd_t* p_wnd, klbwnd_hscrollbar_t* p_hsc, klbwnd_hscrollbar_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
    klbuicssex_draw_text(p_wnd, p_hsc->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_hscrollbar_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_hscrollbar_css_t* p_css = p_hsc->p_css;

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
        klbwnd_hscrollbar_on_paint_status(p_wnd, p_hsc, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_hscrollbar_on_paint_status(p_wnd, p_hsc, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_hscrollbar_on_paint_status(p_wnd, p_hsc, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_hscrollbar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_hscrollbar_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

// 向左按钮响应
static int on_command_bnt_left_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_click == e || KLBUI_dblclick == e)
    {

    }

    return 0;
}

// 向右按钮响应
static int on_command_bnt_right_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_click == e || KLBUI_dblclick == e)
    {

    }

    return 0;
}

// 中间滑块响应
static int on_command_bnt_middle_klbwnd_hscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd_vsc->ctrl;

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_hscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_hscrollbar_css_t* p_css)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_hsc->p_css = p_css;

    klbwnd_btnex_set_css(p_hsc->p_left, &p_css->btnex);
    klbwnd_btnex_set_css(p_hsc->p_right, &p_css->btnex);
    klbwnd_btnex_set_css(p_hsc->p_middle, &p_css->btnex);
}

void klbwnd_hscrollbar_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_hsc->title = sdscpy(p_hsc->title, p_title);
}

const sds klbwnd_hscrollbar_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    return p_hsc->title;
}

void klbwnd_hscrollbar_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_hsc->value = sdscpy(p_hsc->value, p_value);
}

const sds klbwnd_hscrollbar_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    return p_hsc->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_hscrollbar_init_attribute(klb_wnd_t* p_wnd, klbwnd_hscrollbar_t* p_hsc)
{
    p_hsc->title = sdsempty();
    p_hsc->value = sdsempty();
}

static void klbwnd_hscrollbar_quit_attribute(klbwnd_hscrollbar_t* p_hsc)
{
    KLB_FREE_BY(p_hsc->title, sdsfree);
    KLB_FREE_BY(p_hsc->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_hscrollbar_css_init(klbwnd_hscrollbar_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_btnex_css_init(&p_css->btnex, p_gui);
}

void klbwnd_hscrollbar_css_quit(klbwnd_hscrollbar_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);

    klbwnd_btnex_css_quit(&p_css->btnex);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_hscrollbar_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int w_left = h;
    int w_right = h;

    // 向左
    {
        p_hsc->p_left = klbwnd_btnex_create(p_gui, 1, 1, w_left, h - 2);
        klb_wnd_push_child(p_wnd, p_hsc->p_left);

        klbwnd_btnex_set_type(p_hsc->p_left, KLBWND_BTNEX_triangle_left);
        klb_wnd_bind_command(p_hsc->p_left, on_command_bnt_left_klbwnd_hscrollbar, p_wnd);
    }

    // 向右
    {
        p_hsc->p_right = klbwnd_btnex_create(p_gui, w - w_right - 1, 1, w_right, h - 2);
        klb_wnd_push_child(p_wnd, p_hsc->p_right);

        klbwnd_btnex_set_type(p_hsc->p_right, KLBWND_BTNEX_triangle_right);
        klb_wnd_bind_command(p_hsc->p_right, on_command_bnt_right_klbwnd_hscrollbar, p_wnd);
    }

    // 中间
    {
        p_hsc->p_middle = klbwnd_btnex_create(p_gui, w / 2, 1, 80, h - 2);
        klb_wnd_push_child(p_wnd, p_hsc->p_middle);

        klbwnd_btnex_set_type(p_hsc->p_middle, KLBWND_BTNEX_rectangle);
        klb_wnd_bind_command(p_hsc->p_middle, on_command_bnt_middle_klbwnd_hscrollbar, p_wnd);
    }
}

void klbwnd_hscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                               // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_hscrollbar_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                            // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                                // 由继承者重写
    p_wnd->vtable.on_get = NULL;                                // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化内置属性
    klbwnd_hscrollbar_init_attribute(p_wnd, p_hsc);

    // 初始化子窗口
    klbwnd_hscrollbar_init_subwnds(p_wnd);
}

void klbwnd_hscrollbar_quit(klb_wnd_t* p_wnd)
{
    klbwnd_hscrollbar_t* p_hsc = (klbwnd_hscrollbar_t*)p_wnd->ctrl;

    klbwnd_hscrollbar_quit_attribute(p_hsc);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_hscrollbar_t));

    klbwnd_hscrollbar_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_hscrollbar_destroy;

    return p_wnd;
}
