// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


/// @struct klbui_view_t
/// @brief  视图组件: 类似wnd的 替代名称
typedef struct klbui_view_t_
{
    sds                     title;          ///< 标题

    // normal
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数

    klb_map_t*              p_func_map;     ///< 属性函数表
}klbui_view_t;

//////////////////////////////////////////////////////////////////////////

static void klbui_view_quit_attribute(klbui_view_t* p_view);
typedef void(*klbui_view_cb)(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_view_destroy(klb_wnd_t* p_wnd)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    // 清理属性
    klbui_view_quit_attribute(p_view);
    p_view->p_func_map = NULL;

    KLB_FREE(p_wnd);
}

static void klbui_view_on_paint_status(klb_wnd_t* p_wnd, klbui_view_t* p_view, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (0 < sdslen(p_attr->background.image))
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
}

static int klbui_view_on_paint(klb_wnd_t* p_wnd)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_view->margin.left;
    paint_rect.y += p_view->margin.top;
    paint_rect.w -= (p_view->margin.left + p_view->margin.right);
    paint_rect.h -= (p_view->margin.top + p_view->margin.bottom);

    klbui_view_on_paint_status(p_wnd, p_view, &p_view->normal, &paint_rect);

    return 0;
}

static int klbui_view_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbui_view_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_view_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_view_cb func = (klbui_view_cb)klb_map_to_ptr(p_view->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_view, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_view_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_view_cb func = (klbui_view_cb)klb_map_to_ptr(p_view->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_view, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值
static void klbui_view_init_attribute(klb_wnd_t* p_wnd, klbui_view_t* p_view)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_view->title = sdsempty();

    klbuicssex_attributes_init(&p_view->normal, &p_default->normal);
}

static void klbui_view_quit_attribute(klbui_view_t* p_view)
{
    KLB_FREE_BY(p_view->title, sdsfree);

    klbuicssex_attributes_quit(&p_view->normal);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_view_visibility(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&p_view->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_top(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&p_view->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_right(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&p_view->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_bottom(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&p_view->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_left(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&p_view->margin, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&p_view->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_top(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&p_view->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_right(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&p_view->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_bottom(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&p_view->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_left(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&p_view->padding, p_wnd, method, p_in, p_out);
}

static void on_klbui_view_text_color(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_view->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_text_align(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_view->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_font_style(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_style(&(p_view->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_font_weight(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_weight(&(p_view->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_font_size(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_view->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_background_color(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_view->normal.background), p_wnd, method, p_in, p_out);
}


static void on_klbui_view_background_image(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_view->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_style(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_style(&(p_view->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_width(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_view->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_color(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_view->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_radius(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_radius(&(p_view->normal.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_view_title(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_view->title), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////

#define KLBUI_view_bind(KEY_, FUNC_) { klb_map_set_ptr(p_view->p_func_map, (KEY_), (void*)(FUNC_), p_view); }

static void klbui_view_init_func_map(klb_wnd_t* p_wnd, klbui_view_t* p_view, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kview);
    if (NULL != ptr)
    {
        p_view->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kview);
    p_view->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_view_bind("visibility", on_klbui_view_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_view_bind("margin", on_klbui_view_margin);
    KLBUI_view_bind("margin-top", on_klbui_view_margin_top);
    KLBUI_view_bind("margin-right", on_klbui_view_margin_right);
    KLBUI_view_bind("margin-bottom", on_klbui_view_margin_bottom);
    KLBUI_view_bind("margin-left", on_klbui_view_margin_left);

    // 内边距 padding
    KLBUI_view_bind("padding", on_klbui_view_padding);
    KLBUI_view_bind("padding-top", on_klbui_view_padding_top);
    KLBUI_view_bind("padding-right", on_klbui_view_padding_right);
    KLBUI_view_bind("padding-bottom", on_klbui_view_padding_bottom);
    KLBUI_view_bind("padding-left", on_klbui_view_padding_left);

    // 文本颜色 color
    //KLBUI_view_bind("color", on_klbui_view_text_color);

    // 文本对齐 text-align
    //KLBUI_view_bind("text-align", on_klbui_view_text_align);

    // 斜体 font-style
    //KLBUI_view_bind("font-style", on_klbui_view_font_style);

    // 字体粗细 font-weight
    //KLBUI_view_bind("font-weight", on_klbui_view_font_weight);

    // 字体大小 font-size
    //KLBUI_view_bind("font-size", on_klbui_view_font_size);

    // 背景色 background-color
    KLBUI_view_bind("background-color", on_klbui_view_background_color);

    // 背景图片 background-image
    KLBUI_view_bind("background-image", on_klbui_view_background_image);

    // 边框类型 border-style
    //KLBUI_view_bind("border-style", on_klbui_view_border_style);

    // 边框的宽度 border-width
    KLBUI_view_bind("border-width", on_klbui_view_border_width);

    // 边框的颜色 border-color
    KLBUI_view_bind("border-color", on_klbui_view_border_color);

    // 圆角边框 border-radius
    //KLBUI_view_bind("border-radius", on_klbui_view_border_radius);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_view_bind("title", on_klbui_view_title);
    KLBUI_view_bind("value", on_klbui_view_title);
}

klb_wnd_t* klbui_view_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_view_t));
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_view_destroy;
    p_wnd->vtable.on_control = klbui_view_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_view_on_set;
    p_wnd->vtable.on_get = klbui_view_on_get;

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 初始化默认值
    klbui_view_init_attribute(p_wnd, p_view);

    // 初始化 支持的方法
    klbui_view_init_func_map(p_wnd, p_view, p_gui);

    return p_wnd;
}
