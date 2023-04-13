// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


/// @struct klbui_check_t
/// @brief  选择框
///   参考: https://www.w3school.com.cn/jsref/dom_obj_checkbox.asp
typedef struct klbui_check_t_
{
    bool                    check;          ///< 是否选中

    // normal
    klbuicss_margin_t       margin;         ///< 外边框
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    klb_map_t*              p_func_map;     ///< 属性函数表
}klbui_check_t;

//////////////////////////////////////////////////////////////////////////

static void klbui_check_quit_attribute(klbui_check_t* p_check);
typedef void(*klbui_check_cb)(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_check_destroy(klb_wnd_t* p_wnd)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    // 清理属性
    klbui_check_quit_attribute(p_check);
    p_check->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_check_on_paint_status(klb_wnd_t* p_wnd, klbui_check_t* p_check, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (0 < sdslen(p_attr->background.image))
    {
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

        klb_rect_t paint_rect = *p_rect;

        // border
        klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_attr->border.width.top };
        klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_attr->border.color.top);

        klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_attr->border.width.right, paint_rect.y, p_attr->border.width.right, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_attr->border.color.right);

        klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_attr->border.width.bottom, paint_rect.w, p_attr->border.width.bottom };
        klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_attr->border.color.bottom);

        klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_attr->border.width.left, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_attr->border.color.left);

        if (p_check->check)
        {
            klb_rect_t check_rect = { paint_rect.x + paint_rect.w / 4, 
                                    paint_rect.y + paint_rect.h / 4,
                                    paint_rect.w / 2, 
                                    paint_rect.h / 2};

            klb_wnd_draw_fill_rect2(p_wnd, &check_rect, p_attr->text.color /*KLB_ARGB8888(255, 220, 20, 20)*/);
        }
    }
}

static int klbui_check_on_paint(klb_wnd_t* p_wnd)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_check->margin.left;
    paint_rect.y += p_check->margin.top;
    paint_rect.w -= (p_check->margin.left + p_check->margin.right);
    paint_rect.h -= (p_check->margin.top + p_check->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_check_on_paint_status(p_wnd, p_check, &p_check->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_check_on_paint_status(p_wnd, p_check, &p_check->focus, &paint_rect);
    }
    else
    {
        klbui_check_on_paint_status(p_wnd, p_check, &p_check->normal, &paint_rect);
    }

    return 0;
}

static int klbui_check_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbui_check_on_paint(p_wnd);
        break;
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            p_check->check = !p_check->check;
            klb_wnd_update(p_wnd);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_check_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_check_cb func = (klbui_check_cb)klb_map_to_ptr(p_check->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_check, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_check_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_check_cb func = (klbui_check_cb)klb_map_to_ptr(p_check->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_check, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值
static void klbui_check_init_attribute(klb_wnd_t* p_wnd, klbui_check_t* p_check)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_check->check = false;

    klbuicssex_attributes_init(&p_check->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_check->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_check->disable, &p_default->disable);
}

static void klbui_check_quit_attribute(klbui_check_t* p_check)
{
    klbuicssex_attributes_quit(&p_check->normal);
    klbuicssex_attributes_quit(&p_check->focus);
    klbuicssex_attributes_quit(&p_check->disable);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_check_visibility(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_check->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_top(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_check->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_right(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_check->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_bottom(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_check->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_left(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_check->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_check->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_top(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_check->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_right(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_check->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_bottom(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_check->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_left(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_check->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_align(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_check->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_align_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_check->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_align_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_check->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_style(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_check->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_style_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_check->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_style_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_check->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_weight(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_check->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_weight_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_check->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_weight_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_check->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_size(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_check->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_size_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_check->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_font_size_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_check->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_style(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_check->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_style_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_check->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_style_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_check->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_radius(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_check->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_radius_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_check->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_radius_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_check->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_check_value(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, p_check->check);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_bool:
            {
                p_check->check = klb_map_idx_to_bool(p_in, start);

                klb_wnd_update(p_wnd);
            }
            break;
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_check_bind(KEY_, FUNC_) { klb_map_set_ptr(p_check->p_func_map, (KEY_), (void*)(FUNC_), p_check); }

static void klbui_check_init_func_map(klb_wnd_t* p_wnd, klbui_check_t* p_check, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kcheck);
    if (NULL != ptr)
    {
        p_check->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kcheck);
    p_check->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_check_bind("visibility", on_klbui_check_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_check_bind("margin", on_klbui_check_margin);
    KLBUI_check_bind("margin-top", on_klbui_check_margin_top);
    KLBUI_check_bind("margin-right", on_klbui_check_margin_right);
    KLBUI_check_bind("margin-bottom", on_klbui_check_margin_bottom);
    KLBUI_check_bind("margin-left", on_klbui_check_margin_left);

    // 内边距 padding
    KLBUI_check_bind("padding", on_klbui_check_padding);
    KLBUI_check_bind("padding-top", on_klbui_check_padding_top);
    KLBUI_check_bind("padding-right", on_klbui_check_padding_right);
    KLBUI_check_bind("padding-bottom", on_klbui_check_padding_bottom);
    KLBUI_check_bind("padding-left", on_klbui_check_padding_left);

    // 文本颜色 color
    KLBUI_check_bind("color", on_klbui_check_text_color);
    KLBUI_check_bind("color:focus", on_klbui_check_text_color_focus);
    KLBUI_check_bind("color:disable", on_klbui_check_text_color_disable);

    // 文本对齐 text-align
    KLBUI_check_bind("text-align", on_klbui_check_text_align);
    KLBUI_check_bind("text-align:focus", on_klbui_check_text_align_focus);
    KLBUI_check_bind("text-align:disable", on_klbui_check_text_align_disable);

    // 斜体 font-style
    KLBUI_check_bind("font-style", on_klbui_check_font_style);
    KLBUI_check_bind("font-style:focus", on_klbui_check_font_style_focus);
    KLBUI_check_bind("font-style:disable", on_klbui_check_font_style_disable);

    // 字体粗细 font-weight
    KLBUI_check_bind("font-weight", on_klbui_check_font_weight);
    KLBUI_check_bind("font-weight:focus", on_klbui_check_font_weight_focus);
    KLBUI_check_bind("font-weight:disable", on_klbui_check_font_weight_disable);

    // 字体大小 font-size
    KLBUI_check_bind("font-size", on_klbui_check_font_size);
    KLBUI_check_bind("font-size:focus", on_klbui_check_font_size_focus);
    KLBUI_check_bind("font-size:disable", on_klbui_check_font_size_disable);

    // 背景色 background-color
    KLBUI_check_bind("background-color", on_klbui_check_background_color);
    KLBUI_check_bind("background-color:focus", on_klbui_check_background_color_focus);
    KLBUI_check_bind("background-color:disable", on_klbui_check_background_color_disable);

    // 背景图片 background-image
    KLBUI_check_bind("background-image", on_klbui_check_background_image);
    KLBUI_check_bind("background-image:focus", on_klbui_check_background_image_focus);
    KLBUI_check_bind("background-image:disable", on_klbui_check_background_image_disable);

    // 边框类型 border-style
    KLBUI_check_bind("border-style", on_klbui_check_border_style);
    KLBUI_check_bind("border-style:focus", on_klbui_check_border_style_focus);
    KLBUI_check_bind("border-style:disable", on_klbui_check_border_style_disable);

    // 边框的宽度 border-width
    KLBUI_check_bind("border-width", on_klbui_check_border_width);
    KLBUI_check_bind("border-width:focus", on_klbui_check_border_width_focus);
    KLBUI_check_bind("border-width:disable", on_klbui_check_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_check_bind("border-color", on_klbui_check_border_color);
    KLBUI_check_bind("border-color:focus", on_klbui_check_border_color_focus);
    KLBUI_check_bind("border-color:disable", on_klbui_check_border_color_disable);

    // 圆角边框 border-radius
    KLBUI_check_bind("border-radius", on_klbui_check_border_radius);
    KLBUI_check_bind("border-radius:focus", on_klbui_check_border_radius_focus);
    KLBUI_check_bind("border-radius:disable", on_klbui_check_border_radius_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_check_bind("value", on_klbui_check_value);
}

klb_wnd_t* klbui_check_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_check_t));
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_check_destroy;
    p_wnd->vtable.on_control = klbui_check_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_check_on_set;
    p_wnd->vtable.on_get = klbui_check_on_get;

    p_wnd->p_gui = p_gui;

    // 初始化默认值
    klbui_check_init_attribute(p_wnd, p_check);

    // 初始化 支持的方法
    klbui_check_init_func_map(p_wnd, p_check, p_gui);

    return p_wnd;
}
