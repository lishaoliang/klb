// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


/// @struct klbui_progress_t
/// @brief  进度条
typedef struct klbui_progress_t_
{
    sds                     title;          ///< 标题
    int                     progress;       ///< 进度

    // normal
    klbuicss_margin_t       margin;         ///< 外边框
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    klb_map_t*              p_func_map;     ///< 属性函数表
}klbui_progress_t;

//////////////////////////////////////////////////////////////////////////

static void klbui_progress_quit_attribute(klbui_progress_t* p_progress);
typedef void(*klbui_progress_cb)(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_progress_destroy(klb_wnd_t* p_wnd)
{
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;

    // 清理属性
    klbui_progress_quit_attribute(p_progress);
    p_progress->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_progress_on_paint_status(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    //if (0 < sdslen(p_attr->background.image))
    //{
    //    klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    //}
    //else
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

        int progress_w = (int64_t)(paint_rect.w - 4) * p_progress->progress / 100;

        klb_rect_t progress_rect = { paint_rect.x + 2, paint_rect.y + 2, progress_w, paint_rect.h - 4 };
        klb_wnd_draw_fill_rect2(p_wnd, &progress_rect, p_attr->text.color);
    }

    if (0 < sdslen(p_progress->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_progress->padding.left;
        text_rect.y += p_progress->padding.top;
        text_rect.w -= (p_progress->padding.left + p_progress->padding.right);
        text_rect.h -= (p_progress->padding.top + p_progress->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_progress->title, sdslen(p_progress->title), p_attr->text.color, p_attr->font.size);
    }
}

static int klbui_progress_on_paint(klb_wnd_t* p_wnd)
{
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_progress->margin.left;
    paint_rect.y += p_progress->margin.top;
    paint_rect.w -= (p_progress->margin.left + p_progress->margin.right);
    paint_rect.h -= (p_progress->margin.top + p_progress->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_progress_on_paint_status(p_wnd, p_progress, &p_progress->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_progress_on_paint_status(p_wnd, p_progress, &p_progress->focus, &paint_rect);
    }
    else
    {
        klbui_progress_on_paint_status(p_wnd, p_progress, &p_progress->normal, &paint_rect);
    }

    return 0;
}

static int klbui_progress_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_progress_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_progress_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_progress_cb func = (klbui_progress_cb)klb_map_to_ptr(p_progress->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_progress, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_progress_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_progress_cb func = (klbui_progress_cb)klb_map_to_ptr(p_progress->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_progress, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值
static void klbui_progress_init_attribute(klb_wnd_t* p_wnd, klbui_progress_t* p_progress)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_progress->title = sdsempty();

    klbuicssex_attributes_init(&p_progress->normal, p_default);
    klbuicssex_attributes_init(&p_progress->focus, p_default);
    klbuicssex_attributes_init(&p_progress->disable, p_default);
}

static void klbui_progress_quit_attribute(klbui_progress_t* p_progress)
{
    KLB_FREE_BY(p_progress->title, sdsfree);

    klbuicssex_attributes_quit(&p_progress->normal);
    klbuicssex_attributes_quit(&p_progress->focus);
    klbuicssex_attributes_quit(&p_progress->disable);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_progress_visibility(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_margin(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_progress->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_margin_top(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_progress->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_margin_right(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_progress->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_margin_bottom(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_progress->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_margin_left(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_progress->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_padding(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_progress->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_padding_top(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_progress->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_padding_right(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_progress->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_padding_bottom(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_progress->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_padding_left(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_progress->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_color(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_progress->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_color_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_progress->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_color_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_progress->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_align(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_progress->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_align_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_progress->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_text_align_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_progress->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_style(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_progress->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_style_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_progress->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_style_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_progress->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_weight(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_progress->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_weight_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_progress->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_weight_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_progress->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_size(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_progress->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_size_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_progress->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_font_size_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_progress->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_color(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_progress->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_color_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_progress->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_color_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_progress->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_image(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_progress->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_image_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_progress->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_background_image_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_progress->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_style(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_progress->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_style_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_progress->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_style_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_progress->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_width(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_progress->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_width_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_progress->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_width_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_progress->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_color(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_progress->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_color_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_progress->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_color_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_progress->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_radius(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_progress->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_radius_focus(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_progress->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_progress_border_radius_disable(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_progress->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_progress_title(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_progress->title), p_wnd, method, p_in, p_out);
}

static int klbui_progress_value_range(int value)
{
    if (value < 0)
    {
        return 0;
    }

    if (100 < value)
    {
        return 100;
    }

    return value;
}

static void on_klbui_progress_value(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_progress->progress);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
            case KLB_ADT_int64:
            {
                int value = (int)klb_map_idx_to_int64(p_in, start);
                p_progress->progress = klbui_progress_value_range(value);

                klb_wnd_update(p_wnd);
            }
            break;
        case KLB_ADT_uint64:
            {
                int value = (int)klb_map_idx_to_uint64(p_in, start);
                p_progress->progress = klbui_progress_value_range(value);

                klb_wnd_update(p_wnd);
            }
            break;
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_progress_bind(KEY_, FUNC_) { klb_map_set_ptr(p_progress->p_func_map, (KEY_), (FUNC_), p_progress); }

static void klbui_progress_init_func_map(klb_wnd_t* p_wnd, klbui_progress_t* p_progress, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kprogress);
    if (NULL != ptr)
    {
        p_progress->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kprogress);
    p_progress->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_progress_bind("visibility", on_klbui_progress_visibility); // 显示/隐藏

                                                              // 外边距 margin
    KLBUI_progress_bind("margin", on_klbui_progress_margin);
    KLBUI_progress_bind("margin-top", on_klbui_progress_margin_top);
    KLBUI_progress_bind("margin-right", on_klbui_progress_margin_right);
    KLBUI_progress_bind("margin-bottom", on_klbui_progress_margin_bottom);
    KLBUI_progress_bind("margin-left", on_klbui_progress_margin_left);

    // 内边距 padding
    KLBUI_progress_bind("padding", on_klbui_progress_padding);
    KLBUI_progress_bind("padding-top", on_klbui_progress_padding_top);
    KLBUI_progress_bind("padding-right", on_klbui_progress_padding_right);
    KLBUI_progress_bind("padding-bottom", on_klbui_progress_padding_bottom);
    KLBUI_progress_bind("padding-left", on_klbui_progress_padding_left);

    // 文本颜色 color
    KLBUI_progress_bind("color", on_klbui_progress_text_color);
    KLBUI_progress_bind("color:focus", on_klbui_progress_text_color_focus);
    KLBUI_progress_bind("color:disable", on_klbui_progress_text_color_disable);

    // 文本对齐 text-align
    KLBUI_progress_bind("text-align", on_klbui_progress_text_align);
    KLBUI_progress_bind("text-align:focus", on_klbui_progress_text_align_focus);
    KLBUI_progress_bind("text-align:disable", on_klbui_progress_text_align_disable);

    // 斜体 font-style
    KLBUI_progress_bind("font-style", on_klbui_progress_font_style);
    KLBUI_progress_bind("font-style:focus", on_klbui_progress_font_style_focus);
    KLBUI_progress_bind("font-style:disable", on_klbui_progress_font_style_disable);

    // 字体粗细 font-weight
    KLBUI_progress_bind("font-weight", on_klbui_progress_font_weight);
    KLBUI_progress_bind("font-weight:focus", on_klbui_progress_font_weight_focus);
    KLBUI_progress_bind("font-weight:disable", on_klbui_progress_font_weight_disable);

    // 字体大小 font-size
    KLBUI_progress_bind("font-size", on_klbui_progress_font_size);
    KLBUI_progress_bind("font-size:focus", on_klbui_progress_font_size_focus);
    KLBUI_progress_bind("font-size:disable", on_klbui_progress_font_size_disable);

    // 背景色 background-color
    KLBUI_progress_bind("background-color", on_klbui_progress_background_color);
    KLBUI_progress_bind("background-color:focus", on_klbui_progress_background_color_focus);
    KLBUI_progress_bind("background-color:disable", on_klbui_progress_background_color_disable);

    // 背景图片 background-image
    KLBUI_progress_bind("background-image", on_klbui_progress_background_image);
    KLBUI_progress_bind("background-image:focus", on_klbui_progress_background_image_focus);
    KLBUI_progress_bind("background-image:disable", on_klbui_progress_background_image_disable);

    // 边框类型 border-style
    KLBUI_progress_bind("border-style", on_klbui_progress_border_style);
    KLBUI_progress_bind("border-style:focus", on_klbui_progress_border_style_focus);
    KLBUI_progress_bind("border-style:disable", on_klbui_progress_border_style_disable);

    // 边框的宽度 border-width
    KLBUI_progress_bind("border-width", on_klbui_progress_border_width);
    KLBUI_progress_bind("border-width:focus", on_klbui_progress_border_width_focus);
    KLBUI_progress_bind("border-width:disable", on_klbui_progress_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_progress_bind("border-color", on_klbui_progress_border_color);
    KLBUI_progress_bind("border-color:focus", on_klbui_progress_border_color_focus);
    KLBUI_progress_bind("border-color:disable", on_klbui_progress_border_color_disable);

    // 圆角边框 border-radius
    KLBUI_progress_bind("border-radius", on_klbui_progress_border_radius);
    KLBUI_progress_bind("border-radius:focus", on_klbui_progress_border_radius_focus);
    KLBUI_progress_bind("border-radius:disable", on_klbui_progress_border_radius_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_progress_bind("title", on_klbui_progress_title);
    KLBUI_progress_bind("value", on_klbui_progress_value);
}

klb_wnd_t* klbui_progress_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_progress_t));
    klbui_progress_t* p_progress = (klbui_progress_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_progress_destroy;
    p_wnd->vtable.on_control = klbui_progress_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_progress_on_set;
    p_wnd->vtable.on_get = klbui_progress_on_get;

    p_wnd->p_gui = p_gui;

    // 初始化默认值
    klbui_progress_init_attribute(p_wnd, p_progress);

    // 初始化 支持的方法
    klbui_progress_init_func_map(p_wnd, p_progress, p_gui);

    return p_wnd;
}
