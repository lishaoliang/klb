// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_tip.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_tip_quit_attribute(klbwnd_tip_t* p_tip);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_tip_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    klbwnd_tip_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_tip_on_paint_status(klb_wnd_t* p_wnd, klbwnd_tip_t* p_tip, klbwnd_tip_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (NULL != p_attr->background.image && 0 < sdslen(p_attr->background.image))
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
    klbuicssex_draw_text(p_wnd, p_tip->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_tip_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_tip_css_t* p_css = p_tip->p_css;

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
        klbwnd_tip_on_paint_status(p_wnd, p_tip, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_tip_on_paint_status(p_wnd, p_tip, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_tip_on_paint_status(p_wnd, p_tip, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_tip_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_tip_on_paint(p_wnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_tip_set_css(klb_wnd_t* p_wnd, klbwnd_tip_css_t* p_css)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    p_tip->p_css = p_css;
}

void klbwnd_tip_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    if (NULL != p_title)
    {
        p_tip->title = sdscpy(p_tip->title, p_title);
    }
    else
    {
        sdsclear(p_tip->title);
    }
}

const sds klbwnd_tip_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    return p_tip->title;
}

void klbwnd_tip_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    p_tip->value = sdscpy(p_tip->value, p_value);
}

const sds klbwnd_tip_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    return p_tip->value;
}

void klbwnd_tip_layout(klb_wnd_t* p_wnd, int max_w, int max_h, int* p_out_w, int* p_out_h)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    int font_h = (NULL != p_tip->p_css) ? p_tip->p_css->normal.font.size : 24;
    int txt_w = 0, txt_h = 0;
    
    klb_wnd_text_size2(p_wnd, p_tip->title, sdslen(p_tip->title), font_h, &txt_w, &txt_h);

    int w = txt_w + 12;
    int h = txt_h + 8;

    w = MIN(w, max_w);
    h = MIN(h, max_h);

    klb_wnd_resize(p_wnd, w, h);

    if (NULL != p_out_w)
    {
        *p_out_w = w;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = h;
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_tip_init_attribute(klb_wnd_t* p_wnd, klbwnd_tip_t* p_tip)
{
    p_tip->title = sdsempty();
    p_tip->value = sdsempty();
}

static void klbwnd_tip_quit_attribute(klbwnd_tip_t* p_tip)
{
    KLB_FREE_BY(p_tip->title, sdsfree);
    KLB_FREE_BY(p_tip->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_tip_css_init(klbwnd_tip_css_t* p_css, klb_gui_t* p_gui)
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
}

void klbwnd_tip_css_quit(klbwnd_tip_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_tip_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_tip_on_control;       // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW | KLB_WND_STYLE_LAYER_TIP;

    // 初始化内部变量
    klbwnd_tip_init_attribute(p_wnd, p_tip);
}

void klbwnd_tip_quit(klb_wnd_t* p_wnd)
{
    klbwnd_tip_t* p_tip = (klbwnd_tip_t*)p_wnd->ctrl;

    klbwnd_tip_quit_attribute(p_tip);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_tip_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_tip_t));

    klbwnd_tip_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_tip_destroy;

    return p_wnd;
}
