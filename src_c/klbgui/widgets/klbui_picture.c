// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


/// @struct klbui_picture_t
/// @brief  图片框
typedef struct klbui_picture_t_
{
    sds                     title;          ///< 标题

    // normal
    klbuicss_margin_t       margin;         ///< 外边框
    klbuicss_padding_t      padding;        ///< 内边框

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数
    klbuicssex_attributes_t focus;          ///< focus 聚焦状态参数
    klbuicssex_attributes_t disable;        ///< disable 不使能状态参数

    klb_map_t*              p_func_map;     ///< 属性函数表
}klbui_picture_t;


//////////////////////////////////////////////////////////////////////////

static void klbui_picture_quit_attribute(klbui_picture_t* p_pic);
typedef void(*klbui_picture_cb)(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_picture_destroy(klb_wnd_t* p_wnd)
{
    klbui_picture_t* p_pic = (klbui_picture_t*)p_wnd->ctrl;

    // 清理属性
    klbui_picture_quit_attribute(p_pic);
    p_pic->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_picture_on_paint_status(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    if (0 < sdslen(p_attr->background.image))
    {
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }

    {
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

    if (0 < sdslen(p_pic->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_pic->padding.left;
        text_rect.y += p_pic->padding.top;
        text_rect.w -= (p_pic->padding.left + p_pic->padding.right);
        text_rect.h -= (p_pic->padding.top + p_pic->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_pic->title, sdslen(p_pic->title), p_attr->text.color, p_attr->font.size);
    }
}

static int klbui_picture_on_paint(klb_wnd_t* p_wnd)
{
    klbui_picture_t* p_pic = (klbui_picture_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_pic->margin.left;
    paint_rect.y += p_pic->margin.top;
    paint_rect.w -= (p_pic->margin.left + p_pic->margin.right);
    paint_rect.h -= (p_pic->margin.top + p_pic->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_picture_on_paint_status(p_wnd, p_pic, &p_pic->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_picture_on_paint_status(p_wnd, p_pic, &p_pic->focus, &paint_rect);
    }
    else
    {
        klbui_picture_on_paint_status(p_wnd, p_pic, &p_pic->normal, &paint_rect);
    }

    return 0;
}

static int klbui_picture_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_picture_t* p_pic = (klbui_picture_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_PAINT:
        return klbui_picture_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_picture_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_picture_t* p_pic = (klbui_picture_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_picture_cb func = (klbui_picture_cb)klb_map_to_ptr(p_pic->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_pic, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_picture_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_picture_t* p_pic = (klbui_picture_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_picture_cb func = (klbui_picture_cb)klb_map_to_ptr(p_pic->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_pic, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值
static void klbui_picture_init_attribute(klb_wnd_t* p_wnd, klbui_picture_t* p_pic)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_pic->title = sdsempty();

    klbuicssex_attributes_init(&p_pic->normal, p_default);
    klbuicssex_attributes_init(&p_pic->focus, p_default);
    klbuicssex_attributes_init(&p_pic->disable, p_default);
}

static void klbui_picture_quit_attribute(klbui_picture_t* p_pic)
{
    KLB_FREE_BY(p_pic->title, sdsfree);

    klbuicssex_attributes_quit(&p_pic->normal);
    klbuicssex_attributes_quit(&p_pic->focus);
    klbuicssex_attributes_quit(&p_pic->disable);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_picture_visibility(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_margin(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_pic->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_margin_top(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_pic->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_margin_right(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_pic->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_margin_bottom(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_pic->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_margin_left(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_pic->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_padding(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_pic->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_padding_top(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_pic->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_padding_right(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_pic->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_padding_bottom(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_pic->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_padding_left(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_pic->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_color(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_pic->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_color_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_pic->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_color_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_pic->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_align(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_pic->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_align_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_pic->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_text_align_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_pic->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_style(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_pic->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_style_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_pic->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_style_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_pic->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_weight(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_pic->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_weight_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_pic->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_weight_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_pic->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_size(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_pic->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_size_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_pic->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_font_size_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_pic->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_color(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_pic->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_color_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_pic->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_color_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_pic->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_image(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_pic->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_image_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_pic->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_background_image_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_pic->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_style(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_pic->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_style_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_pic->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_style_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_pic->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_width(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_pic->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_width_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_pic->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_width_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_pic->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_color(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_pic->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_color_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_pic->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_color_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_pic->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_radius(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_pic->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_radius_focus(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_pic->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_border_radius_disable(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_pic->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_picture_title(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_pic->title), p_wnd, method, p_in, p_out);
}

static void on_klbui_picture_image(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klbuicssex_background_image(&(p_pic->normal.background), p_wnd, method, p_in, p_out);
    }
    else
    {
        klbuicssex_background_image(&(p_pic->normal.background), p_wnd, method, p_in, p_out);
        klbuicssex_background_image(&(p_pic->focus.background), p_wnd, method, p_in, p_out);
        klbuicssex_background_image(&(p_pic->disable.background), p_wnd, method, p_in, p_out);
    }
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_pic_bind(KEY_, FUNC_) { klb_map_set_ptr(p_pic->p_func_map, (KEY_), (FUNC_), p_pic); }

static void klbui_picture_init_func_map(klb_wnd_t* p_wnd, klbui_picture_t* p_pic, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kpicture);
    if (NULL != ptr)
    {
        p_pic->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kpicture);
    p_pic->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_pic_bind("visibility", on_klbui_picture_visibility); // 显示/隐藏

                                                              // 外边距 margin
    KLBUI_pic_bind("margin", on_klbui_picture_margin);
    KLBUI_pic_bind("margin-top", on_klbui_picture_margin_top);
    KLBUI_pic_bind("margin-right", on_klbui_picture_margin_right);
    KLBUI_pic_bind("margin-bottom", on_klbui_picture_margin_bottom);
    KLBUI_pic_bind("margin-left", on_klbui_picture_margin_left);

    // 内边距 padding
    KLBUI_pic_bind("padding", on_klbui_picture_padding);
    KLBUI_pic_bind("padding-top", on_klbui_picture_padding_top);
    KLBUI_pic_bind("padding-right", on_klbui_picture_padding_right);
    KLBUI_pic_bind("padding-bottom", on_klbui_picture_padding_bottom);
    KLBUI_pic_bind("padding-left", on_klbui_picture_padding_left);

    // 文本颜色 color
    KLBUI_pic_bind("color", on_klbui_picture_text_color);
    KLBUI_pic_bind("color:focus", on_klbui_picture_text_color_focus);
    KLBUI_pic_bind("color:disable", on_klbui_picture_text_color_disable);

    // 文本对齐 text-align
    KLBUI_pic_bind("text-align", on_klbui_picture_text_align);
    KLBUI_pic_bind("text-align:focus", on_klbui_picture_text_align_focus);
    KLBUI_pic_bind("text-align:disable", on_klbui_picture_text_align_disable);

    // 斜体 font-style
    KLBUI_pic_bind("font-style", on_klbui_picture_font_style);
    KLBUI_pic_bind("font-style:focus", on_klbui_picture_font_style_focus);
    KLBUI_pic_bind("font-style:disable", on_klbui_picture_font_style_disable);

    // 字体粗细 font-weight
    KLBUI_pic_bind("font-weight", on_klbui_picture_font_weight);
    KLBUI_pic_bind("font-weight:focus", on_klbui_picture_font_weight_focus);
    KLBUI_pic_bind("font-weight:disable", on_klbui_picture_font_weight_disable);

    // 字体大小 font-size
    KLBUI_pic_bind("font-size", on_klbui_picture_font_size);
    KLBUI_pic_bind("font-size:focus", on_klbui_picture_font_size_focus);
    KLBUI_pic_bind("font-size:disable", on_klbui_picture_font_size_disable);

    // 背景色 background-color
    KLBUI_pic_bind("background-color", on_klbui_picture_background_color);
    KLBUI_pic_bind("background-color:focus", on_klbui_picture_background_color_focus);
    KLBUI_pic_bind("background-color:disable", on_klbui_picture_background_color_disable);

    // 背景图片 background-image
    KLBUI_pic_bind("background-image", on_klbui_picture_background_image);
    KLBUI_pic_bind("background-image:focus", on_klbui_picture_background_image_focus);
    KLBUI_pic_bind("background-image:disable", on_klbui_picture_background_image_disable);

    // 边框类型 border-style
    KLBUI_pic_bind("border-style", on_klbui_picture_border_style);
    KLBUI_pic_bind("border-style:focus", on_klbui_picture_border_style_focus);
    KLBUI_pic_bind("border-style:disable", on_klbui_picture_border_style_disable);

    // 边框的宽度 border-width
    KLBUI_pic_bind("border-width", on_klbui_picture_border_width);
    KLBUI_pic_bind("border-width:focus", on_klbui_picture_border_width_focus);
    KLBUI_pic_bind("border-width:disable", on_klbui_picture_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_pic_bind("border-color", on_klbui_picture_border_color);
    KLBUI_pic_bind("border-color:focus", on_klbui_picture_border_color_focus);
    KLBUI_pic_bind("border-color:disable", on_klbui_picture_border_color_disable);

    // 圆角边框 border-radius
    KLBUI_pic_bind("border-radius", on_klbui_picture_border_radius);
    KLBUI_pic_bind("border-radius:focus", on_klbui_picture_border_radius_focus);
    KLBUI_pic_bind("border-radius:disable", on_klbui_picture_border_radius_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_pic_bind("title", on_klbui_picture_title);
    KLBUI_pic_bind("value", on_klbui_picture_title);
    KLBUI_pic_bind("image", on_klbui_picture_image);
}

klb_wnd_t* klbui_picture_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_picture_t));
    klbui_picture_t* p_btn = (klbui_picture_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_picture_destroy;
    p_wnd->vtable.on_control = klbui_picture_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_picture_on_set;
    p_wnd->vtable.on_get = klbui_picture_on_get;

    p_wnd->p_gui = p_gui;

    // 初始化默认值
    klbui_picture_init_attribute(p_wnd, p_btn);

    // 初始化 支持的方法
    klbui_picture_init_func_map(p_wnd, p_btn, p_gui);

    return p_wnd;
}
