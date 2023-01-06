// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_map.h"
#include "klbgui/klb_gui.h"


// 静态文本框
typedef struct klbui_static_t_
{
    sds                         title;          ///< 标题

    // normal
    klbuicss_margin_t           margin;         ///< 外边框
    klbuicss_padding_t          padding;        ///< 内边框

    klbuicssex_attributes_t     normal;         ///< 集合属性

    klb_map_t*                  p_func_map;     ///< 属性函数表
}klbui_static_t;


//////////////////////////////////////////////////////////////////////////

static void klbui_static_quit_attribute(klbui_static_t* p_static);
typedef void(*klbui_static_cb)(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out);


static void klbui_static_destroy(klb_wnd_t* p_wnd)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    klbui_static_quit_attribute(p_static);
    p_static->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_button_on_paint_status(klb_wnd_t* p_wnd, klbui_static_t* p_static, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 普通状态
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

    if (0 < sdslen(p_static->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_static->padding.left;
        text_rect.y += p_static->padding.top;
        text_rect.w -= (p_static->padding.left + p_static->padding.right);
        text_rect.h -= (p_static->padding.top + p_static->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_static->title, sdslen(p_static->title), p_attr->text.color, p_attr->font.size);
    }
}

static int klbui_static_on_paint(klb_wnd_t* p_wnd)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // button的绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_static->margin.left;
    paint_rect.y += p_static->margin.top;
    paint_rect.w -= (p_static->margin.left + p_static->margin.right);
    paint_rect.h -= (p_static->margin.top + p_static->margin.bottom);

    klbui_button_on_paint_status(p_wnd, p_static, &(p_static->normal), &paint_rect);

    return 0;
}

static int klbui_static_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_GUI_PAINT:
        return klbui_static_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_static_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_static_cb func = (klbui_static_cb)klb_map_to_ptr(p_static->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_static, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_static_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_static_cb func = (klbui_static_cb)klb_map_to_ptr(p_static->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_static, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值

static void klbui_static_init_attribute(klb_wnd_t* p_wnd, klbui_static_t* p_static, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    // init
    p_static->title = sdsempty();

    // style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    klbuicssex_attributes_init(&p_static->normal, p_default);
}

static void klbui_static_quit_attribute(klbui_static_t* p_static)
{
    klbuicssex_attributes_quit(&p_static->normal);
}

//////////////////////////////////////////////////////////////////////////
// 属性处理方法


/////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_static_visibility(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_top(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->margin.top), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_right(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->margin.right), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_bottom(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->margin.bottom), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_left(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->margin.left), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_top(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->padding.top), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_right(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->padding.right), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_bottom(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->padding.bottom), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_left(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->padding.left), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_text_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_static->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_text_align(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_static->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_font_style(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_static->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_font_weight(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_static->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_font_size(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_static->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_static->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_image(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_static->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_style(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_static->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_width(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_static->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_static->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_radius(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_static->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_title(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_static->title), p_wnd, method, p_in, p_out);
}

/////////////////////////////////////
#define KLBUI_static_bind(KEY_, FUNC_) { klb_map_set_ptr(p_static->p_func_map, (KEY_), (FUNC_), p_static); }

static void klbui_static_init_func_map(klb_wnd_t* p_wnd, klbui_static_t* p_static, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, "kstatic");
    if (NULL != ptr)
    {
        p_static->p_func_map = ptr;
        return;
    }
    
    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, "kstatic");
    p_static->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_static_bind("visibility", on_klbui_static_visibility);

    // 外边距 margin
    KLBUI_static_bind("margin-top", on_klbui_static_margin_top);
    KLBUI_static_bind("margin-right", on_klbui_static_margin_right);
    KLBUI_static_bind("margin-bottom", on_klbui_static_margin_bottom);
    KLBUI_static_bind("margin-left", on_klbui_static_margin_left);

    // 内边距 padding
    KLBUI_static_bind("padding-top", on_klbui_static_padding_top);
    KLBUI_static_bind("padding-right", on_klbui_static_padding_right);
    KLBUI_static_bind("padding-bottom", on_klbui_static_padding_bottom);
    KLBUI_static_bind("padding-left", on_klbui_static_padding_left);

    // 文本颜色 color
    KLBUI_static_bind("color", on_klbui_static_text_color);

    // 文本对齐 text-align
    KLBUI_static_bind("text-align", on_klbui_static_text_align);

    // 斜体 font-style
    KLBUI_static_bind("font-style", on_klbui_static_font_style);

    // 字体粗细 font-weight
    KLBUI_static_bind("font-weight", on_klbui_static_font_weight);

    // 字体大小 font-size
    KLBUI_static_bind("font-size", on_klbui_static_font_size);

    // 背景色 background-color
    KLBUI_static_bind("background-color", on_klbui_static_background_color);

    // 背景图片 background-image
    KLBUI_static_bind("background-image", on_klbui_static_background_image);

    // 边框类型 border-style
    KLBUI_static_bind("border-style", on_klbui_static_border_style);

    // 边框的宽度 border-width
    KLBUI_static_bind("border-width", on_klbui_static_border_width);

    // 边框的颜色 border-color
    KLBUI_static_bind("border-color", on_klbui_static_border_color);

    // 圆角边框 border-radius
    KLBUI_static_bind("border-radius", on_klbui_static_border_radius);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_static_bind("title", on_klbui_static_title);
    KLBUI_static_bind("value", on_klbui_static_title);
}


klb_wnd_t* klbui_static_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_static_t));
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_static_destroy;
    p_wnd->vtable.on_control = klbui_static_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_static_on_set;
    p_wnd->vtable.on_get = klbui_static_on_get;

    p_wnd->p_gui = p_gui;


    // 初始化属性
    klbui_static_init_attribute(p_wnd, p_static, p_gui);

    // 初始化属性方法表
    klbui_static_init_func_map(p_wnd, p_static, p_gui);

    return p_wnd;
}
