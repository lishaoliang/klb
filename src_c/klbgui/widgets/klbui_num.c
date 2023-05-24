// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_num.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_num_t
/// @brief  数值编辑框
typedef struct klbui_num_t_
{
    klbwnd_num_t        num;            ///< 必须首位, 保持内存一致
    klbwnd_num_css_t    css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_num_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_num_cb)(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_num_destroy(klb_wnd_t* p_wnd)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_num_quit(p_wnd);

    // 退出css
    klbwnd_num_css_quit(&p_num->css);

    KLB_FREE(p_wnd);
}

static int klbui_num_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_num_cb func = (klbui_num_cb)klb_map_to_ptr(p_num->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_num, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_num_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_num_cb func = (klbui_num_cb)klb_map_to_ptr(p_num->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_num, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_num_visibility(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_num_margin(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_num->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_margin_top(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_num->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_margin_right(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_num->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_margin_bottom(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_num->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_margin_left(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_num->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_padding(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_num->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_padding_top(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_num->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_padding_right(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_num->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_padding_bottom(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_num->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_padding_left(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_num->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_num->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_num->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_num->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_align(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_num->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_align_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_num->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_text_align_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_num->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_font_size(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_num->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_font_size_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_num->css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_font_size_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_num->css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_num->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_num->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_num->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_image(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_num->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_image_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_num->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_background_image_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_num->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_width(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_num->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_width_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_num->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_width_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_num->css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_num->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_num->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_border_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_num->css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_num_value(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_min(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.min), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_max(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.max), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_num_bind(KEY_, FUNC_) { klb_map_set_ptr(p_num->p_func_map, (KEY_), (void*)(FUNC_), p_num); }

static void klbui_num_init_func_map(klb_wnd_t* p_wnd, klbui_num_t* p_num, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_knum);
    if (NULL != ptr)
    {
        p_num->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_knum);
    p_num->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_num_bind("visibility", on_klbui_num_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_num_bind("margin", on_klbui_num_margin);
    KLBUI_num_bind("margin-top", on_klbui_num_margin_top);
    KLBUI_num_bind("margin-right", on_klbui_num_margin_right);
    KLBUI_num_bind("margin-bottom", on_klbui_num_margin_bottom);
    KLBUI_num_bind("margin-left", on_klbui_num_margin_left);

    // 内边距 padding
    KLBUI_num_bind("padding", on_klbui_num_padding);
    KLBUI_num_bind("padding-top", on_klbui_num_padding_top);
    KLBUI_num_bind("padding-right", on_klbui_num_padding_right);
    KLBUI_num_bind("padding-bottom", on_klbui_num_padding_bottom);
    KLBUI_num_bind("padding-left", on_klbui_num_padding_left);

    // 文本颜色 color
    KLBUI_num_bind("color", on_klbui_num_text_color);
    KLBUI_num_bind("color:focus", on_klbui_num_text_color_focus);
    KLBUI_num_bind("color:disable", on_klbui_num_text_color_disable);

    // 文本对齐 text-align
    KLBUI_num_bind("text-align", on_klbui_num_text_align);
    KLBUI_num_bind("text-align:focus", on_klbui_num_text_align_focus);
    KLBUI_num_bind("text-align:disable", on_klbui_num_text_align_disable);

    // 字体大小 font-size
    KLBUI_num_bind("font-size", on_klbui_num_font_size);
    KLBUI_num_bind("font-size:focus", on_klbui_num_font_size_focus);
    KLBUI_num_bind("font-size:disable", on_klbui_num_font_size_disable);

    // 背景色 background-color
    KLBUI_num_bind("background-color", on_klbui_num_background_color);
    KLBUI_num_bind("background-color:focus", on_klbui_num_background_color_focus);
    KLBUI_num_bind("background-color:disable", on_klbui_num_background_color_disable);

    // 背景图片 background-image
    KLBUI_num_bind("background-image", on_klbui_num_background_image);
    KLBUI_num_bind("background-image:focus", on_klbui_num_background_image_focus);
    KLBUI_num_bind("background-image:disable", on_klbui_num_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_num_bind("border-width", on_klbui_num_border_width);
    KLBUI_num_bind("border-width:focus", on_klbui_num_border_width_focus);
    KLBUI_num_bind("border-width:disable", on_klbui_num_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_num_bind("border-color", on_klbui_num_border_color);
    KLBUI_num_bind("border-color:focus", on_klbui_num_border_color_focus);
    KLBUI_num_bind("border-color:disable", on_klbui_num_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_num_bind("value", on_klbui_num_value);
    KLBUI_num_bind("min", on_klbui_slider_min);
    KLBUI_num_bind("max", on_klbui_slider_max);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_num_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_num_t));
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_num_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_num_destroy;
    p_wnd->vtable.on_set = klbui_num_on_set;
    p_wnd->vtable.on_get = klbui_num_on_get;

    // css
    klbwnd_num_css_init(&p_num->css, p_gui);
    klbwnd_num_set_css(p_wnd, &p_num->css);

    // 初始化CSS 支持的方法
    klbui_num_init_func_map(p_wnd, p_num, p_gui);

    return p_wnd;
}
