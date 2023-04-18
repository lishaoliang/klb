// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


/// @struct klbui_slider_attributes_t
/// @brief  滑动条CSS属性
typedef struct klbui_slider_attributes_t_
{
    klbuicss_text_t             text;               ///< 文本/前景属性
    klbuicss_background_t       background;         ///< 背景属性

    uint32_t                    pos_color;          ///< 中间指示颜色(非图片时)
    sds                         pos_image;          ///< 中间指示位置图片
    int                         pos_w;
    int                         pos_h;
}klbui_slider_attributes_t;


/// @struct klbui_slider_t
/// @brief  滑动条
typedef struct klbui_slider_t_
{
    int                 min;            ///< 最小值
    int                 max;            ///< 最大值
    int                 value;          ///< 当前值

    // normal
    klbuicss_margin_t   margin;         ///< 外边距
    klbuicss_padding_t  padding;        ///< 内边距

    klbui_slider_attributes_t normal;         ///< normal 常规状态参数
    klbui_slider_attributes_t focus;          ///< focus 聚焦状态参数
    klbui_slider_attributes_t disable;        ///< disable 不使能状态参数

    klb_map_t*                p_func_map;     ///< 属性函数表
}klbui_slider_t;

//////////////////////////////////////////////////////////////////////////

static void klbui_slider_quit_attribute(klbui_slider_t* p_slider);
typedef void(*klbui_slider_cb)(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out);

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

static void klbui_slider_destroy(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    // 清理属性
    klbui_slider_quit_attribute(p_slider);
    p_slider->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_slider_on_paint_status(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, klbui_slider_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 滑动条 横杆
    int mid_h = 6;
    klb_rect_t r_mid = {p_rect->x, p_rect->y + (p_rect->h - mid_h) / 2, p_rect->w, mid_h };
    klb_wnd_draw_fill_rect2(p_wnd, &r_mid, KLB_ARGB8888(255, 120, 120, 120));

    // 左侧值占距
    int left_w = p_rect->w * (p_slider->value - p_slider->min) / (p_slider->max - p_slider->min);
    left_w = klbui_slider_fix(left_w, 0, p_rect->w);

    if (0 < left_w)
    {
        klb_rect_t r_left = { p_rect->x, p_rect->y + (p_rect->h - mid_h) / 2, left_w, mid_h };
        klb_wnd_draw_fill_rect2(p_wnd, &r_left, KLB_ARGB8888(255, 120, 120, 220));
    }

    // 中间
    int pos_w = 6;
    int pos_h = 12;
    klb_rect_t r_pos = { p_rect->x + left_w - pos_w / 2, p_rect->y + (p_rect->h - pos_h) / 2, pos_w, pos_h };

    klb_wnd_draw_fill_rect2(p_wnd, &r_pos, KLB_ARGB8888(255, 220, 120, 220));

}

static int klbui_slider_on_paint(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_slider->margin.left;
    paint_rect.y += p_slider->margin.top;
    paint_rect.w -= (p_slider->margin.left + p_slider->margin.right);
    paint_rect.h -= (p_slider->margin.top + p_slider->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_slider_on_paint_status(p_wnd, p_slider, &p_slider->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_slider_on_paint_status(p_wnd, p_slider, &p_slider->focus, &paint_rect);
    }
    else
    {
        klbui_slider_on_paint_status(p_wnd, p_slider, &p_slider->normal, &paint_rect);
    }

    return 0;
}

static int klbui_slider_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbui_slider_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

            // 绘图区域
            klb_rect_t paint_rect = *p_rect;

            // 移除外边距
            paint_rect.x += p_slider->margin.left;
            paint_rect.y += p_slider->margin.top;
            paint_rect.w -= (p_slider->margin.left + p_slider->margin.right);
            paint_rect.h -= (p_slider->margin.top + p_slider->margin.bottom);

            int value = (p_slider->max - p_slider->min) * (p_pt1->x - paint_rect.x) / (paint_rect.w);
            value = klbui_slider_fix(value, p_slider->min, p_slider->max);

            bool change = (value == p_slider->value) ? false : true;
            p_slider->value = value;

            if (change)
            {
                // 内容变更事件 KLBUI_onchange
                if (NULL != p_wnd->vtable.on_command)
                {
                    p_wnd->vtable.on_command(p_wnd, KLBUI_onchange, p_pt1, p_pt2, lparam, wparam);
                }
            }

            klb_wnd_update(p_wnd);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_slider_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_slider_cb func = (klbui_slider_cb)klb_map_to_ptr(p_slider->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_slider, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_slider_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_slider_cb func = (klbui_slider_cb)klb_map_to_ptr(p_slider->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_slider, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值

static void klbui_slider_attributes_init(klbui_slider_attributes_t* p_dst, const klbuicssex_attributes_t* p_src)
{
    p_dst->background.image = sdsempty();
    p_dst->pos_image = sdsempty();

    p_dst->text = p_src->text;
    p_dst->background.color = p_src->background.color;
    p_dst->background.image = sdscpy(p_dst->background.image, p_src->background.image);
    p_dst->background.repeat = p_src->background.repeat;
    p_dst->background.position = p_src->background.position;
    p_dst->background.attachment = p_src->background.attachment;
}

static void klbui_slider_attributes_quit(klbui_slider_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
    KLB_FREE_BY(p_attr->pos_image, sdsfree);
}

static void klbui_slider_init_attribute(klb_wnd_t* p_wnd, klbui_slider_t* p_slider)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    klbui_slider_attributes_init(&p_slider->normal, &p_default->normal);
    klbui_slider_attributes_init(&p_slider->focus, &p_default->focus);
    klbui_slider_attributes_init(&p_slider->disable, &p_default->disable);

    p_slider->min = 0;
    p_slider->max = 100;
    p_slider->value = 50;
}

static void klbui_slider_quit_attribute(klbui_slider_t* p_slider)
{
    klbui_slider_attributes_quit(&p_slider->normal);
    klbui_slider_attributes_quit(&p_slider->focus);
    klbui_slider_attributes_quit(&p_slider->disable);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_slider_visibility(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_slider->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_slider->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_slider->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_slider->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_slider->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_slider->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_slider->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_slider->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_slider->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_slider->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_slider->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_slider->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_slider->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_image(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_slider->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_image_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_slider->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_image_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_slider->disable.background), p_wnd, method, p_in, p_out);
}



//////////////////////////////////////
// 自定义属性

static void on_klbui_slider_value(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->value), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_slider_bind(KEY_, FUNC_) { klb_map_set_ptr(p_slider->p_func_map, (KEY_), (void*)(FUNC_), p_slider); }

static void klbui_slider_init_func_map(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kslider);
    if (NULL != ptr)
    {
        p_slider->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kslider);
    p_slider->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_slider_bind("visibility", on_klbui_slider_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_slider_bind("margin", on_klbui_slider_margin);
    KLBUI_slider_bind("margin-top", on_klbui_slider_margin_top);
    KLBUI_slider_bind("margin-right", on_klbui_slider_margin_right);
    KLBUI_slider_bind("margin-bottom", on_klbui_slider_margin_bottom);
    KLBUI_slider_bind("margin-left", on_klbui_slider_margin_left);

    // 内边距 padding
    KLBUI_slider_bind("padding", on_klbui_slider_padding);
    KLBUI_slider_bind("padding-top", on_klbui_slider_padding_top);
    KLBUI_slider_bind("padding-right", on_klbui_slider_padding_right);
    KLBUI_slider_bind("padding-bottom", on_klbui_slider_padding_bottom);
    KLBUI_slider_bind("padding-left", on_klbui_slider_padding_left);

    // 文本颜色 color
    KLBUI_slider_bind("color", on_klbui_slider_text_color);
    KLBUI_slider_bind("color:focus", on_klbui_slider_text_color_focus);
    KLBUI_slider_bind("color:disable", on_klbui_slider_text_color_disable);

    // 背景色 background-color
    KLBUI_slider_bind("background-color", on_klbui_slider_background_color);
    KLBUI_slider_bind("background-color:focus", on_klbui_slider_background_color_focus);
    KLBUI_slider_bind("background-color:disable", on_klbui_slider_background_color_disable);

    // 背景图片 background-image
    KLBUI_slider_bind("background-image", on_klbui_slider_background_image);
    KLBUI_slider_bind("background-image:focus", on_klbui_slider_background_image_focus);
    KLBUI_slider_bind("background-image:disable", on_klbui_slider_background_image_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_slider_bind("value", on_klbui_slider_value);
}

klb_wnd_t* klbui_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_slider_t));
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_slider_destroy;
    p_wnd->vtable.on_control = klbui_slider_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_slider_on_set;
    p_wnd->vtable.on_get = klbui_slider_on_get;

    p_wnd->p_gui = p_gui;

    // 样式
    p_wnd->state.style = 0x0;

    // 初始化默认值
    klbui_slider_init_attribute(p_wnd, p_slider);

    // 初始化 支持的方法
    klbui_slider_init_func_map(p_wnd, p_slider, p_gui);

    return p_wnd;
}
