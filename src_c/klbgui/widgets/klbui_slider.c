// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_slider.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_slider_t
/// @brief  滑动条
typedef struct klbui_slider_t_
{
    klbwnd_slider_t     slider;         ///< 必须首位, 保持内存一致
    klbwnd_slider_css_t css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_slider_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_slider_cb)(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_slider_destroy(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_slider_quit(p_wnd);

    // 退出css
    klbwnd_slider_css_quit(&p_slider->css);

    KLB_FREE(p_wnd);
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
// 仿 CSS 方法

static void on_klbui_slider_margin(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_slider->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_slider->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_slider->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_slider->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_margin_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_slider->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_slider->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_slider->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_slider->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_slider->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_padding_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_slider->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_slider->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_align(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_slider->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_align_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_slider->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_text_align_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_slider->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.normal.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.focus.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_background_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.disable.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.normal.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.focus.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.disable.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_foreground_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->css.foreground_h), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.normal.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.focus.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_slider->css.disable.pos_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->css.normal.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->css.focus.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_image_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_slider->css.disable.pos_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_width(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->css.pos_w), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_pos_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->css.pos_h), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性


static void on_klbui_slider_value(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->slider.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_min(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->slider.min), p_wnd, method, p_in, p_out);
}

static void on_klbui_slider_max(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_slider->slider.max), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// css func

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

    // 文本对齐 text-align
    KLBUI_slider_bind("text-align", on_klbui_slider_text_align);
    KLBUI_slider_bind("text-align:focus", on_klbui_slider_text_align_focus);
    KLBUI_slider_bind("text-align:disable", on_klbui_slider_text_align_disable);

    // 背景色 background-color
    KLBUI_slider_bind("background-color", on_klbui_slider_background_color);
    KLBUI_slider_bind("background-color:focus", on_klbui_slider_background_color_focus);
    KLBUI_slider_bind("background-color:disable", on_klbui_slider_background_color_disable);

    // 滑块区域前景色: foreground-color
    KLBUI_slider_bind("foreground-color", on_klbui_slider_foreground_color);
    KLBUI_slider_bind("foreground-color:focus", on_klbui_slider_foreground_color_focus);
    KLBUI_slider_bind("foreground-color:disable", on_klbui_slider_foreground_color_disable);

    //// 滑块区域前景高度: foreground-height
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

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_slider_t));
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_slider_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_slider_destroy;
    p_wnd->vtable.on_set = klbui_slider_on_set;
    p_wnd->vtable.on_get = klbui_slider_on_get;

    // css
    klbwnd_slider_css_init(&p_slider->css, p_gui);
    klbwnd_slider_set_css(p_wnd, &p_slider->css);

    // 初始化CSS 支持的方法
    klbui_slider_init_func_map(p_wnd, p_slider, p_gui);

    return p_wnd;
}
