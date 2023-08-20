// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_slider.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_slider_quit_attribute(klbwnd_slider_t* p_slider);


static int klbui_slider_fix(int v, int min, int max)
{
    if (v <= min)
    {
        return min;
    }

    if (max <= v)
    {
        return max;
    }

    return v;
}

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_slider_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    klbwnd_slider_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_slider_on_paint_status(klb_wnd_t* p_wnd, klbwnd_slider_t* p_slider, klbwnd_slider_css_t* p_css, klbui_slider_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 全部背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background_color);

    // 滑动条 横杆
    int foreground_h = p_css->foreground_h;
    klb_rect_t r_mid = { p_rect->x, p_rect->y + (p_rect->h - foreground_h) / 2, p_rect->w, foreground_h };
    klb_wnd_draw_fill_rect2(p_wnd, &r_mid, p_attr->foreground_color);

    // 左侧值占距
    int left_w = 0;
    if (p_slider->min == p_slider->value)
    {
        left_w = 0;
    }
    else if (p_slider->max == p_slider->value)
    {
        left_w = p_rect->w;
    }
    else
    {
        left_w = p_rect->w * (p_slider->value - p_slider->min + 1) / (p_slider->max - p_slider->min + 1);
        left_w = klbui_slider_fix(left_w, 0, p_rect->w);
    }

    if (0 < left_w)
    {
        klb_rect_t r_left = { p_rect->x, p_rect->y + (p_rect->h - foreground_h) / 2, left_w, foreground_h };
        klb_wnd_draw_fill_rect2(p_wnd, &r_left, p_attr->text.color);
    }

    // 指示区域
    int pos_w = p_css->pos_w;
    int pos_h = p_css->pos_h;
    int pos_off_x = left_w - pos_w / 2;
    pos_off_x = klbui_slider_fix(pos_off_x, 0, p_rect->w - pos_w);

    klb_rect_t r_pos = { p_rect->x + pos_off_x, p_rect->y + (p_rect->h - pos_h) / 2, pos_w, pos_h };

    if (0 < sdslen(p_attr->pos_image))
    {
        klb_wnd_draw_image(p_wnd, &r_pos, p_attr->pos_image, NULL);
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, &r_pos, p_attr->pos_color);
    }
}

static int klbwnd_slider_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_slider_css_t* p_css = p_slider->p_css;

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
        klbwnd_slider_on_paint_status(p_wnd, p_slider, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_slider_on_paint_status(p_wnd, p_slider, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_slider_on_paint_status(p_wnd, p_slider, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_slider_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;
    klbwnd_slider_css_t* p_css = p_slider->p_css;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_slider_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        {
            if (NULL != p_css)
            {
                klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

                // 绘图区域
                klb_rect_t paint_rect = *p_rect;

                // 移除外边距
                paint_rect.x += p_css->margin.left;
                paint_rect.y += p_css->margin.top;
                paint_rect.w -= (p_css->margin.left + p_css->margin.right);
                paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

                int value = p_slider->min + (p_slider->max - p_slider->min + 1) * (p_pt1->x - paint_rect.x + 1) / (paint_rect.w);
                value = klbui_slider_fix(value, p_slider->min, p_slider->max);

                bool change = (value == p_slider->value) ? false : true;
                p_slider->value = value;

                if (change)
                {
                    // 内容变更事件 KLBUI_onchange
                    klb_wnd_on_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
                }

                klb_wnd_update(p_wnd);
            }
        }
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_slider_set_css(klb_wnd_t* p_wnd, klbwnd_slider_css_t* p_css)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    p_slider->p_css = p_css;
}

void klbwnd_slider_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    p_slider->title = sdscpy(p_slider->title, p_title);
}

const sds klbwnd_slider_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    return p_slider->title;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_slider_init_attribute(klbwnd_slider_t* p_slider)
{
    p_slider->title = sdsempty();

    p_slider->min = 0;
    p_slider->max = 100;
    p_slider->value = 50;
}

static void klbwnd_slider_quit_attribute(klbwnd_slider_t* p_slider)
{
    KLB_FREE_BY(p_slider->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

static void klbui_slider_attributes_init(klbui_slider_attributes_t* p_dst, const klbuicssex_attributes_t* p_src)
{
    p_dst->text = p_src->text;

    p_dst->background_color = p_src->background.color;
    p_dst->foreground_color = KLB_ARGB8888(255, 61, 61, 61);

    p_dst->pos_image = sdsempty();
    p_dst->pos_color = KLB_ARGB8888(255, 128, 128, 220);
}

static void klbui_slider_attributes_quit(klbui_slider_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->pos_image, sdsfree);
}

void klbwnd_slider_css_init(klbwnd_slider_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbui_slider_attributes_init(&p_css->normal, &p_default->normal);
    klbui_slider_attributes_init(&p_css->focus, &p_default->focus);
    klbui_slider_attributes_init(&p_css->disable, &p_default->disable);

    p_css->foreground_h = 6;
    p_css->pos_w = 6;
    p_css->pos_h = 16;
}

void klbwnd_slider_css_quit(klbwnd_slider_css_t* p_css)
{
    klbui_slider_attributes_quit(&p_css->normal);
    klbui_slider_attributes_quit(&p_css->focus);
    klbui_slider_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_slider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_slider_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_slider_init_attribute(p_slider);
}

void klbwnd_slider_quit(klb_wnd_t* p_wnd)
{
    klbwnd_slider_t* p_slider = (klbwnd_slider_t*)p_wnd->ctrl;

    klbwnd_slider_quit_attribute(p_slider);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_slider_t));

    klbwnd_slider_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_slider_destroy;

    return p_wnd;
}
