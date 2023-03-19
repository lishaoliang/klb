// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"
#include "klbgui/subviews/klbui_combo_menu.h"


/// @struct klbui_combo_botton_t
/// @brief  下拉组合控件的按钮
typedef struct klbui_combo_botton_t_
{
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数
}klbui_combo_botton_t;


/// @struct klbui_button_t
/// @brief  下拉组合控件
typedef struct klbui_combo_t_
{
    sds                     title;          ///< 标题

    // normal
    klbuicss_margin_t       margin;         ///< 外边框
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    //klbui_combo_botton_t    btn_left;       ///< 左侧按钮
    klbui_combo_botton_t    btn_right;      ///< 右侧按钮

    klb_wnd_t*              p_menu;

    klb_map_t*              p_func_map;     ///< 属性函数表
}klbui_combo_t;


//////////////////////////////////////////////////////////////////////////

static void klbui_combo_quit_attribute(klbui_combo_t* p_combo);
typedef void(*klbui_combo_cb)(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_combo_destroy(klb_wnd_t* p_wnd)
{
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;

    // 清理属性
    klbui_combo_quit_attribute(p_combo);
    p_combo->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_combo_on_paint_button_status(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klb_rect_t r = *p_rect;

    r.x += p_combo->btn_right.padding.left;
    r.y += p_combo->btn_right.padding.top;
    r.w -= (p_combo->btn_right.padding.left + p_combo->btn_right.padding.right);
    r.h -= (p_combo->btn_right.padding.top + p_combo->btn_right.padding.bottom);

    // 倒三角
    int h = r.w / 2;
    int x = r.x + (r.w - h) /2 - 1;
    int y = r.y + (r.h - h) / 2;
    for (int i = h; i >= 0; i--)
    {
        klb_wnd_draw_line2(p_wnd, x - i, y, x + i, y, p_attr->text.color);
        y += 1;
    }
}

static void klbui_combo_on_paint_status(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
    }

    if (0 < sdslen(p_combo->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_combo->padding.left;
        text_rect.y += p_combo->padding.top;
        text_rect.w -= (p_combo->padding.left + p_combo->padding.right);
        text_rect.h -= (p_combo->padding.top + p_combo->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_combo->title, sdslen(p_combo->title), p_attr->text.color, p_attr->font.size);
    }
}

static int klbui_combo_on_paint(klb_wnd_t* p_wnd)
{
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_combo->margin.left;
    paint_rect.y += p_combo->margin.top;
    paint_rect.w -= (p_combo->margin.left + p_combo->margin.right);
    paint_rect.h -= (p_combo->margin.top + p_combo->margin.bottom);

    int btn_right_w = 12;
    klb_rect_t btn_right_rect = { paint_rect.x + paint_rect.w - btn_right_w, paint_rect.y + 1,
        btn_right_w, paint_rect.h - 2};

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_combo_on_paint_status(p_wnd, p_combo, &p_combo->disable, &paint_rect);
        klbui_combo_on_paint_button_status(p_wnd, p_combo, &(p_combo->btn_right.disable), &btn_right_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_combo_on_paint_status(p_wnd, p_combo, &p_combo->focus, &paint_rect);
        klbui_combo_on_paint_button_status(p_wnd, p_combo, &(p_combo->btn_right.focus), &btn_right_rect);
    }
    else
    {
        klbui_combo_on_paint_status(p_wnd, p_combo, &p_combo->normal, &paint_rect);
        klbui_combo_on_paint_button_status(p_wnd, p_combo, &(p_combo->btn_right.normal), &btn_right_rect);
    }

    return 0;
}

static int klbui_combo_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_combo_on_paint(p_wnd);
        break;
    case KLB_WM_LBUTTONDOWN:
    case KLB_WM_LBUTTONDBLCLK:
        {
            klb_rect_t rect = p_wnd->pos.rect_in_canvas;
            int x = rect.x;
            int y = rect.y + rect.h;

            klb_wnd_move(p_combo->p_menu, x, y);

            klb_gui_popup_wnd(p_wnd->p_gui, p_combo->p_menu);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_combo_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_combo_cb func = (klbui_combo_cb)klb_map_to_ptr(p_combo->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_combo, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_combo_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_combo_cb func = (klbui_combo_cb)klb_map_to_ptr(p_combo->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_combo, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值
static void klbui_combo_init_attribute(klb_wnd_t* p_wnd, klbui_combo_t* p_combo)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_combo->title = sdsempty();

    klbuicssex_attributes_init(&p_combo->normal, p_default);
    klbuicssex_attributes_init(&p_combo->focus, p_default);
    klbuicssex_attributes_init(&p_combo->disable, p_default);

    klbuicssex_attributes_init(&(p_combo->btn_right.normal), p_default);
    klbuicssex_attributes_init(&(p_combo->btn_right.focus), p_default);
    klbuicssex_attributes_init(&(p_combo->btn_right.disable), p_default);
}

static void klbui_combo_quit_attribute(klbui_combo_t* p_combo)
{
    KLB_FREE_BY(p_combo->title, sdsfree);

    klbuicssex_attributes_quit(&p_combo->normal);
    klbuicssex_attributes_quit(&p_combo->focus);
    klbuicssex_attributes_quit(&p_combo->disable);

    klbuicssex_attributes_quit(&(p_combo->btn_right.normal));
    klbuicssex_attributes_quit(&(p_combo->btn_right.focus));
    klbuicssex_attributes_quit(&(p_combo->btn_right.disable));
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_combo_visibility(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_margin(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_combo->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_margin_top(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_combo->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_margin_right(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_combo->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_margin_bottom(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_combo->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_margin_left(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_combo->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_padding(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_combo->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_padding_top(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_combo->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_padding_right(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_combo->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_padding_bottom(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_combo->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_padding_left(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_combo->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_color(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_color_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_color_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_align(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_combo->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_align_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_combo->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_text_align_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_combo->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_style(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_combo->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_style_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_combo->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_style_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_combo->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_weight(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_combo->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_weight_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_combo->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_weight_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_combo->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_size(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_combo->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_size_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_combo->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_font_size_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_combo->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_color(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_combo->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_color_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_combo->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_color_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_combo->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_image(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_combo->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_image_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_combo->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_background_image_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_combo->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_style(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_combo->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_style_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_combo->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_style_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_combo->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_width(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_combo->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_width_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_combo->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_width_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_combo->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_color(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_combo->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_color_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_combo->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_color_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_combo->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_radius(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_combo->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_radius_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_combo->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_border_radius_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_combo->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_botton_right_color(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->btn_right.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_botton_right_color_focus(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->btn_right.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_combo_botton_right_color_disable(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_combo->btn_right.disable.text), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_combo_title(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_combo->title), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_combo_bind(KEY_, FUNC_) { klb_map_set_ptr(p_combo->p_func_map, (KEY_), (FUNC_), p_combo); }

static void klbui_combo_init_func_map(klb_wnd_t* p_wnd, klbui_combo_t* p_combo, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kcombo);
    if (NULL != ptr)
    {
        p_combo->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kcombo);
    p_combo->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_combo_bind("visibility", on_klbui_combo_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_combo_bind("margin", on_klbui_combo_margin);
    KLBUI_combo_bind("margin-top", on_klbui_combo_margin_top);
    KLBUI_combo_bind("margin-right", on_klbui_combo_margin_right);
    KLBUI_combo_bind("margin-bottom", on_klbui_combo_margin_bottom);
    KLBUI_combo_bind("margin-left", on_klbui_combo_margin_left);

    // 内边距 padding
    KLBUI_combo_bind("padding", on_klbui_combo_padding);
    KLBUI_combo_bind("padding-top", on_klbui_combo_padding_top);
    KLBUI_combo_bind("padding-right", on_klbui_combo_padding_right);
    KLBUI_combo_bind("padding-bottom", on_klbui_combo_padding_bottom);
    KLBUI_combo_bind("padding-left", on_klbui_combo_padding_left);

    // 文本颜色 color
    KLBUI_combo_bind("color", on_klbui_combo_text_color);
    KLBUI_combo_bind("color:focus", on_klbui_combo_text_color_focus);
    KLBUI_combo_bind("color:disable", on_klbui_combo_text_color_disable);

    // 文本对齐 text-align
    KLBUI_combo_bind("text-align", on_klbui_combo_text_align);
    KLBUI_combo_bind("text-align:focus", on_klbui_combo_text_align_focus);
    KLBUI_combo_bind("text-align:disable", on_klbui_combo_text_align_disable);

    // 斜体 font-style
    KLBUI_combo_bind("font-style", on_klbui_combo_font_style);
    KLBUI_combo_bind("font-style:focus", on_klbui_combo_font_style_focus);
    KLBUI_combo_bind("font-style:disable", on_klbui_combo_font_style_disable);

    // 字体粗细 font-weight
    KLBUI_combo_bind("font-weight", on_klbui_combo_font_weight);
    KLBUI_combo_bind("font-weight:focus", on_klbui_combo_font_weight_focus);
    KLBUI_combo_bind("font-weight:disable", on_klbui_combo_font_weight_disable);

    // 字体大小 font-size
    KLBUI_combo_bind("font-size", on_klbui_combo_font_size);
    KLBUI_combo_bind("font-size:focus", on_klbui_combo_font_size_focus);
    KLBUI_combo_bind("font-size:disable", on_klbui_combo_font_size_disable);

    // 背景色 background-color
    KLBUI_combo_bind("background-color", on_klbui_combo_background_color);
    KLBUI_combo_bind("background-color:focus", on_klbui_combo_background_color_focus);
    KLBUI_combo_bind("background-color:disable", on_klbui_combo_background_color_disable);

    // 背景图片 background-image
    KLBUI_combo_bind("background-image", on_klbui_combo_background_image);
    KLBUI_combo_bind("background-image:focus", on_klbui_combo_background_image_focus);
    KLBUI_combo_bind("background-image:disable", on_klbui_combo_background_image_disable);

    // 边框类型 border-style
    KLBUI_combo_bind("border-style", on_klbui_combo_border_style);
    KLBUI_combo_bind("border-style:focus", on_klbui_combo_border_style_focus);
    KLBUI_combo_bind("border-style:disable", on_klbui_combo_border_style_disable);

    // 边框的宽度 border-width
    KLBUI_combo_bind("border-width", on_klbui_combo_border_width);
    KLBUI_combo_bind("border-width:focus", on_klbui_combo_border_width_focus);
    KLBUI_combo_bind("border-width:disable", on_klbui_combo_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_combo_bind("border-color", on_klbui_combo_border_color);
    KLBUI_combo_bind("border-color:focus", on_klbui_combo_border_color_focus);
    KLBUI_combo_bind("border-color:disable", on_klbui_combo_border_color_disable);

    // 圆角边框 border-radius
    KLBUI_combo_bind("border-radius", on_klbui_combo_border_radius);
    KLBUI_combo_bind("border-radius:focus", on_klbui_combo_border_radius_focus);
    KLBUI_combo_bind("border-radius:disable", on_klbui_combo_border_radius_disable);

    //////////////////////////////////////////////
    // 右侧倒三角
    KLBUI_combo_bind("botton-right.color", on_klbui_combo_botton_right_color);
    KLBUI_combo_bind("botton-right.color:focus", on_klbui_combo_botton_right_color_focus);
    KLBUI_combo_bind("botton-right.color:disable", on_klbui_combo_botton_right_color_disable);


    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_combo_bind("title", on_klbui_combo_title);
    KLBUI_combo_bind("value", on_klbui_combo_title);
}

klb_wnd_t* klbui_combo_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_combo_t));
    klbui_combo_t* p_combo = (klbui_combo_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_combo_destroy;
    p_wnd->vtable.on_control = klbui_combo_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_combo_on_set;
    p_wnd->vtable.on_get = klbui_combo_on_get;

    p_wnd->p_gui = p_gui;

    // 初始化弹出菜单
    p_combo->p_menu = klbui_combo_menu_create(p_gui, 0, 0, 128, 28 * 9);

    // 初始化默认值
    klbui_combo_init_attribute(p_wnd, p_combo);

    // 初始化 支持的方法
    klbui_combo_init_func_map(p_wnd, p_combo, p_gui);

    return p_wnd;
}
