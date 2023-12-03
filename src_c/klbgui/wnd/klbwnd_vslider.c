// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_vslider.h"
#include "klbutil/klb_sds.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_vslider_quit_attribute(klbwnd_vslider_t* p_vslider);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_vslider_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    klbwnd_vslider_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_vslider_on_paint_status(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider, klbwnd_vslider_css_t* p_css, klbui_vslider_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // step1. 整个区域的背景色, 需要和大环境融合
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background_color);

    // step2. 滑动区域颜色
    int foreground_w = p_css->foreground_width;
    klb_rect_t r_foreground = { p_rect->x + (p_rect->w - foreground_w) / 2, p_rect->y, foreground_w, p_rect->h };
    klb_wnd_draw_fill_rect2(p_wnd, &r_foreground, p_attr->foreground_color);

    // step3. 值按比例 占用的的区域颜色
    klb_rect_t r_value = r_foreground;

    // 计算 
    {
        if (p_vslider->value >= p_vslider->max)
        {
            // 全部, 和滑动区域大小一致
        }
        else if(p_vslider->value <= p_vslider->min)
        {
            // 没有
            r_value.h = 0;
        }
        else
        {
            int value_h = r_value.h * (p_vslider->value - p_vslider->min + 1) / (p_vslider->max - p_vslider->min + 1);

            if (value_h <= 0) { value_h = 0; };
            if (value_h >= r_value.h) { value_h = r_value.h; };

            r_value.y += (r_value.h - value_h);
            r_value.h = value_h;
        }

        if (0 < r_value.w && 0 < r_value.h)
        {
            klb_wnd_draw_fill_rect2(p_wnd, &r_value, p_attr->color);
        }
    }

    // step4. 滑动块(矩形)
    //klb_rect_t r_thumb = { r_value.x - 3 , r_value.y, p_css->thumb_width, p_css->thumb_height};
    //klb_wnd_draw_fill_rect2(p_wnd, &r_thumb, p_attr->thumb_color);
}

// 绘图事件
static int klbwnd_vslider_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_vslider_css_t* p_css = p_vslider->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除内边距
    paint_rect.x += p_css->padding.left;
    paint_rect.y += p_css->padding.top;
    paint_rect.w -= (p_css->padding.left + p_css->padding.right);
    paint_rect.h -= (p_css->padding.top + p_css->padding.bottom);

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_vslider_on_paint_status(p_wnd, p_vslider, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_vslider_on_paint_status(p_wnd, p_vslider, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_vslider_on_paint_status(p_wnd, p_vslider, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

//////////////////////////////////////

static void update_tip_klbwnd_vslider(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider)
{
    char str[64] = { 0 };
    snprintf(str, sizeof(str) - 1, "%d", p_vslider->value);

    klb_wnd_set_tip(p_wnd, str);
    klb_wnd_tip_update(p_wnd);
}

// 聚焦事件
static void klbwnd_vslider_on_focus(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider)
{
    update_tip_klbwnd_vslider(p_wnd, p_vslider);
}

// 鼠标点击事件
static void klbwnd_vslider_on_click(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider, const klb_point_t* p_pt1)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return;
    }

    // note. 鼠标点击的坐标值, 存在 "人为" 精度 问题
    // 一般情况下, 较难以命中两端位置(最小/最大值)
    // 这里采用 padding.top / padding.bottom 来消除这种操作精度
    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    // 移除内边距
    klbwnd_vslider_css_t* p_css = p_vslider->p_css;
    if (NULL != p_css)
    {
        rect.x += p_css->padding.left;
        rect.y += p_css->padding.top;
        rect.w -= (p_css->padding.left + p_css->padding.right);
        rect.h -= (p_css->padding.top + p_css->padding.bottom);
    }

    // 等比换算
    // (rect.h - (y - rect.x)) / rect.h   =   value / (max - min + 1)
    // value = (max - min + 1) * (rect.h - (y - rect.y)) / rect.h
    int y = p_pt1->y;

    int value = (p_vslider->max - p_vslider->min + 1) * (rect.h - (y - rect.y)) / rect.h;

    if (value <= p_vslider->min) { value = p_vslider->min; };
    if (value >= p_vslider->max) { value = p_vslider->max; };

    if (value != p_vslider->value)
    {
        p_vslider->value = value;

        update_tip_klbwnd_vslider(p_wnd, p_vslider);
        klb_wnd_update(p_wnd);
    }
}

// 鼠标拖拽事件
static void klbwnd_vslider_on_mousedrag(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return;
    }
}

// 鼠标滚轮事件
static void klbwnd_vslider_on_mousewheel(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider, const klb_point_t* p_pt1, int lparam)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return;
    }

    if (KLBUI_MOUSEWHEEL_is_up(lparam))
    {
        int value = p_vslider->value + p_vslider->step;
        if (value >= p_vslider->max) { value = p_vslider->max; };

        if (value != p_vslider->value)
        {
            p_vslider->value = value;

            update_tip_klbwnd_vslider(p_wnd, p_vslider);
            klb_wnd_update(p_wnd);
        }
    }
    else if(KLBUI_MOUSEWHEEL_is_down(lparam))
    {
        int value = p_vslider->value - p_vslider->step;
        if (value <= p_vslider->min) { value = p_vslider->min; };

        if (value != p_vslider->value)
        {
            p_vslider->value = value;

            update_tip_klbwnd_vslider(p_wnd, p_vslider);
            klb_wnd_update(p_wnd);
        }
    }
}

// 滑动条控件的所有事件响应
static int klbwnd_vslider_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        klbwnd_vslider_on_paint(p_wnd);
        break;

    case KLBUI_focus:
        klbwnd_vslider_on_focus(p_wnd, p_vslider);
        break;

    case KLBUI_click:
    case KLBUI_dblclick:
        klbwnd_vslider_on_click(p_wnd, p_vslider, p_pt1);
        break;

    case KLBUI_mousedrag:
        klbwnd_vslider_on_mousedrag(p_wnd, p_vslider, p_pt1, p_pt2, lparam, wparam);
        break;

    case KLBUI_mousewheel:
        klbwnd_vslider_on_mousewheel(p_wnd, p_vslider, p_pt1, lparam);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_vslider_set_css(klb_wnd_t* p_wnd, klbwnd_vslider_css_t* p_css)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->p_css = p_css;
}

void klbwnd_vslider_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->value = value;
}

int klbwnd_vslider_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    return p_vslider->value;
}

void klbwnd_vslider_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->min = min;
    p_vslider->max = max;
    p_vslider->step = step;
}

void klbwnd_vslider_set_min(klb_wnd_t* p_wnd, int min)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->min = min;
}

void klbwnd_vslider_set_max(klb_wnd_t* p_wnd, int max)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->max = max;
}

void klbwnd_vslider_set_step(klb_wnd_t* p_wnd, int step)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_vslider->step = step;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_vslider_init_attribute(klb_wnd_t* p_wnd, klbwnd_vslider_t* p_vslider)
{
    p_vslider->min = 0;
    p_vslider->max = 100;
    p_vslider->step = 1;
    p_vslider->value = 0;
}

static void klbwnd_vslider_quit_attribute(klbwnd_vslider_t* p_vslider)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit

static void klbui_vslider_attributes_init(klbui_vslider_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->background_color = p_src->background.color;
    p_attr->foreground_color = KLB_ARGB8888(255, 61, 61, 61);

    p_attr->color = p_src->text.color;

    p_attr->thumb_color = KLB_ARGB8888(255, 128, 128, 220);
    p_attr->thumb_image = sdsempty();
}

static void klbui_vslider_attributes_quit(klbui_vslider_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->thumb_image, sdsfree);
}

static void klbui_vslider_attributes_copy(klbui_vslider_attributes_t* p_dst, klbui_vslider_attributes_t* p_src)
{
    p_dst->background_color = p_src->background_color;
    p_dst->foreground_color = p_src->foreground_color;

    p_dst->color = p_src->color;

    p_dst->thumb_color = p_src->thumb_color;
    p_dst->thumb_image = klb_sdscpy(p_dst->thumb_image, p_src->thumb_image);
}

void klbwnd_vslider_css_init(klbwnd_vslider_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbui_vslider_attributes_init(&p_css->normal, &p_default->normal);
    klbui_vslider_attributes_init(&p_css->focus, &p_default->focus);
    klbui_vslider_attributes_init(&p_css->disable, &p_default->disable);

    p_css->foreground_width = 6;    // 滑块区宽度
    p_css->thumb_width = 12;        // 滑动块宽
    p_css->thumb_height = 6;         // 滑动块高

    //
    p_css->padding.top = 2;
    p_css->padding.bottom = 2;
}

void klbwnd_vslider_css_quit(klbwnd_vslider_css_t* p_css)
{
    klbui_vslider_attributes_quit(&p_css->normal);
    klbui_vslider_attributes_quit(&p_css->focus);
    klbui_vslider_attributes_quit(&p_css->disable);
}

void klbwnd_vslider_css_copy(klbwnd_vslider_css_t* p_dst, klbwnd_vslider_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbui_vslider_attributes_copy(&p_dst->normal, &p_src->normal);
    klbui_vslider_attributes_copy(&p_dst->focus, &p_src->focus);
    klbui_vslider_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_vslider_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_vslider_on_control;   // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化内部变量
    klbwnd_vslider_init_attribute(p_wnd, p_vslider);
}

void klbwnd_vslider_quit(klb_wnd_t* p_wnd)
{
    klbwnd_vslider_t* p_vslider = (klbwnd_vslider_t*)p_wnd->ctrl;

    klbwnd_vslider_quit_attribute(p_vslider);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_vslider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_vslider_t));

    klbwnd_vslider_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_vslider_destroy;

    return p_wnd;
}
