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
    uint32_t                    background_color;   ///< 整个背景色
    uint32_t                    foreground_color;   ///< 滑块颜色

    klbuicss_text_t             text;               ///< 左侧值占据的颜色

    uint32_t                    pos_color;          ///< 中间指示颜色(非图片时)
    sds                         pos_image;          ///< 中间指示位置图片
}klbui_slider_attributes_t;


/// @struct klbui_slider_t
/// @brief  滑动条
typedef struct klbui_slider_t_
{
    int                 min;            ///< 最小值
    int                 max;            ///< 最大值
    int                 value;          ///< 当前值

    int                 foreground_h;   ///< 滑块高度
    int                 pos_w;          ///< 中间指示宽度
    int                 pos_h;          ///< 中间指示高度

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
    // 全部背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background_color);

    // 滑动条 横杆
    int foreground_h = p_slider->foreground_h;
    klb_rect_t r_mid = {p_rect->x, p_rect->y + (p_rect->h - foreground_h) / 2, p_rect->w, foreground_h };
    klb_wnd_draw_fill_rect2(p_wnd, &r_mid, p_attr->foreground_color);

    // 左侧值占距
    int left_w = 0;
    if (p_slider->min == p_slider->value)
    {
        left_w = 0;
    }
    else if(p_slider->max == p_slider->value)
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
    int pos_w = p_slider->pos_w;
    int pos_h = p_slider->pos_h;
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

            int value = p_slider->min + (p_slider->max - p_slider->min + 1) * (p_pt1->x - paint_rect.x + 1) / (paint_rect.w);
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

static void klbui_slider_init_attribute(klb_wnd_t* p_wnd, klbui_slider_t* p_slider)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    klbui_slider_attributes_init(&p_slider->normal, &p_default->normal);
    klbui_slider_attributes_init(&p_slider->focus, &p_default->focus);
    klbui_slider_attributes_init(&p_slider->disable, &p_default->disable);

    p_slider->min = 0;
    p_slider->max = 100;
    p_slider->value = 50;

    p_slider->foreground_h = 6;
    p_slider->pos_w = 6;
    p_slider->pos_h = 16;
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

static void on_klbui_slider_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->normal.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->focus.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->disable.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->normal.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->focus.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->disable.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->foreground_h), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->normal.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->focus.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->disable.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->normal.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->focus.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->disable.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_width(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->pos_w), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->pos_h), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_slider_value(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->value), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_min(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->min), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_max(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->max), p_wnd, method, p_in, p_out);
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

    // 滑块区域左侧区域颜色 color
    KLBUI_slider_bind("color", on_klbui_slider_color);
    KLBUI_slider_bind("color:focus", on_klbui_slider_color_focus);
    KLBUI_slider_bind("color:disable", on_klbui_slider_color_disable);

    // 背景色: background-color
    KLBUI_slider_bind("background-color", on_klbui_slider_background_color);
    KLBUI_slider_bind("background-color:focus", on_klbui_slider_background_color_focus);
    KLBUI_slider_bind("background-color:disable", on_klbui_slider_background_color_disable);

    // 滑块区域前景色: foreground-color
    KLBUI_slider_bind("foreground-color", on_klbui_slider_foreground_color);
    KLBUI_slider_bind("foreground-color:focus", on_klbui_slider_foreground_color_focus);
    KLBUI_slider_bind("foreground-color:disable", on_klbui_slider_foreground_color_disable);

    // 滑块区域前景高度: foreground-height
    KLBUI_slider_bind("foreground-height", on_klbui_slider_foreground_height);

    // 子区域: 中间指示 - color
    KLBUI_slider_bind("pos.color", on_klbui_slider_pos_color);
    KLBUI_slider_bind("pos.color:focus", on_klbui_slider_pos_color_focus);
    KLBUI_slider_bind("pos.color:disable", on_klbui_slider_pos_color_disable);

    // 子区域: 中间指示 - 图片
    KLBUI_slider_bind("pos.image", on_klbui_slider_pos_image);
    KLBUI_slider_bind("pos.image:focus", on_klbui_slider_pos_image_focus);
    KLBUI_slider_bind("pos.image:disable", on_klbui_slider_pos_image_disable);

    // 子区域: 中间指示 - 宽度
    KLBUI_slider_bind("pos.width", on_klbui_slider_pos_width);

    // 子区域: 中间指示 - 高度
    KLBUI_slider_bind("pos.height", on_klbui_slider_pos_height);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_slider_bind("value", on_klbui_slider_value);
    KLBUI_slider_bind("min", on_klbui_slider_min);
    KLBUI_slider_bind("max", on_klbui_slider_max);
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
