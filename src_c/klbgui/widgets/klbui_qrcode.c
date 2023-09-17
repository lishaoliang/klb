// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_qrcode.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_qrcode_t
/// @brief  二维码
typedef struct klbui_qrcode_t_
{
    klbwnd_qrcode_t     qrcode;         ///< 必须首位, 保持内存一致
    klbwnd_qrcode_css_t css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_qrcode_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_qrcode_cb)(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_qrcode_destroy(klb_wnd_t* p_wnd)
{
    klbui_qrcode_t* p_qrcode = (klbui_qrcode_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_qrcode_quit(p_wnd);

    // 退出css
    klbwnd_qrcode_css_quit(&p_qrcode->css);

    KLB_FREE(p_wnd);
}

static int klbui_qrcode_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_qrcode_t* p_qrcode = (klbui_qrcode_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_qrcode_cb func = (klbui_qrcode_cb)klb_map_to_ptr(p_qrcode->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_qrcode, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_qrcode_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_qrcode_t* p_qrcode = (klbui_qrcode_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_qrcode_cb func = (klbui_qrcode_cb)klb_map_to_ptr(p_qrcode->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_qrcode, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_qrcode_margin(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_qrcode->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_margin_top(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_qrcode->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_margin_right(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_qrcode->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_margin_bottom(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_qrcode->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_margin_left(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_qrcode->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_qrcode->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding_top(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_qrcode->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding_right(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_qrcode->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding_bottom(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_qrcode->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding_left(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_qrcode->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_color(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_qrcode->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_color_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_qrcode->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_color_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_qrcode->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_align(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_qrcode->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_align_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_qrcode->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_text_align_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_qrcode->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_font_size(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_qrcode->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_font_size_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_qrcode->css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_font_size_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_qrcode->css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_color(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_qrcode->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_color_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_qrcode->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_color_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_qrcode->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_image(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_qrcode->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_image_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_qrcode->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_image_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_qrcode->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_width(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_qrcode->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_width_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_qrcode->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_width_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_qrcode->css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_color(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_qrcode->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_color_focus(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_qrcode->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_color_disable(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_qrcode->css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_qrcode_title(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_qrcode->qrcode.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_value(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, klbwnd_qrcode_get_value(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        if (KLB_ADT_string == klb_map_array_type(p_in, start))
        {
            const char* p_value = klb_map_idx_to_string(p_in, start);
            klbwnd_qrcode_set_value(p_wnd, p_value);

            klb_wnd_update(p_wnd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_qrcode_bind(KEY_, FUNC_) { klb_map_set_ptr(p_qrcode->p_func_map, (KEY_), (void*)(FUNC_), p_qrcode); }

static void klbui_qrcode_init_func_map(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qrcode, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kqrcode);
    if (NULL != ptr)
    {
        p_qrcode->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kqrcode);
    p_qrcode->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_qrcode_bind("margin", on_klbui_qrcode_margin);
    KLBUI_qrcode_bind("margin-top", on_klbui_qrcode_margin_top);
    KLBUI_qrcode_bind("margin-right", on_klbui_qrcode_margin_right);
    KLBUI_qrcode_bind("margin-bottom", on_klbui_qrcode_margin_bottom);
    KLBUI_qrcode_bind("margin-left", on_klbui_qrcode_margin_left);

    // 内边距 padding
    KLBUI_qrcode_bind("padding", on_klbui_qrcode_padding);
    KLBUI_qrcode_bind("padding-top", on_klbui_qrcode_padding_top);
    KLBUI_qrcode_bind("padding-right", on_klbui_qrcode_padding_right);
    KLBUI_qrcode_bind("padding-bottom", on_klbui_qrcode_padding_bottom);
    KLBUI_qrcode_bind("padding-left", on_klbui_qrcode_padding_left);

    // 文本颜色 color
    KLBUI_qrcode_bind("color", on_klbui_qrcode_text_color);
    KLBUI_qrcode_bind("color:focus", on_klbui_qrcode_text_color_focus);
    KLBUI_qrcode_bind("color:disable", on_klbui_qrcode_text_color_disable);

    // 文本对齐 text-align
    KLBUI_qrcode_bind("text-align", on_klbui_qrcode_text_align);
    KLBUI_qrcode_bind("text-align:focus", on_klbui_qrcode_text_align_focus);
    KLBUI_qrcode_bind("text-align:disable", on_klbui_qrcode_text_align_disable);

    // 字体大小 font-size
    KLBUI_qrcode_bind("font-size", on_klbui_qrcode_font_size);
    KLBUI_qrcode_bind("font-size:focus", on_klbui_qrcode_font_size_focus);
    KLBUI_qrcode_bind("font-size:disable", on_klbui_qrcode_font_size_disable);

    // 背景色 background-color
    KLBUI_qrcode_bind("background-color", on_klbui_qrcode_background_color);
    KLBUI_qrcode_bind("background-color:focus", on_klbui_qrcode_background_color_focus);
    KLBUI_qrcode_bind("background-color:disable", on_klbui_qrcode_background_color_disable);

    // 背景图片 background-image
    KLBUI_qrcode_bind("background-image", on_klbui_qrcode_background_image);
    KLBUI_qrcode_bind("background-image:focus", on_klbui_qrcode_background_image_focus);
    KLBUI_qrcode_bind("background-image:disable", on_klbui_qrcode_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_qrcode_bind("border-width", on_klbui_qrcode_border_width);
    KLBUI_qrcode_bind("border-width:focus", on_klbui_qrcode_border_width_focus);
    KLBUI_qrcode_bind("border-width:disable", on_klbui_qrcode_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_qrcode_bind("border-color", on_klbui_qrcode_border_color);
    KLBUI_qrcode_bind("border-color:focus", on_klbui_qrcode_border_color_focus);
    KLBUI_qrcode_bind("border-color:disable", on_klbui_qrcode_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_qrcode_bind("title", on_klbui_qrcode_title);
    KLBUI_qrcode_bind("value", on_klbui_qrcode_value);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_qrcode_t));
    klbui_qrcode_t* p_qrcode = (klbui_qrcode_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_qrcode_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_qrcode_destroy;
    p_wnd->vtable.on_set = klbui_qrcode_on_set;
    p_wnd->vtable.on_get = klbui_qrcode_on_get;

    // css
    klbwnd_qrcode_css_init(&p_qrcode->css, p_gui);
    klbwnd_qrcode_set_css(p_wnd, &p_qrcode->css);

    // 初始化CSS 支持的方法
    klbui_qrcode_init_func_map(p_wnd, p_qrcode, p_gui);

    return p_wnd;
}
