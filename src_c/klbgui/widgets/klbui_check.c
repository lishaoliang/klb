// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_check.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_check_t
/// @brief  选择框
///   参考: https://www.w3school.com.cn/jsref/dom_obj_checkbox.asp
typedef struct klbui_check_t_
{
    klbwnd_check_t      check;          ///< 必须首位, 保持内存一致
    klbwnd_check_css_t  css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_check_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_check_cb)(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_check_destroy(klb_wnd_t* p_wnd)
{
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_check_quit(p_wnd);

    // 退出css
    klbwnd_check_css_quit(&p_check->css);

    KLB_FREE(p_wnd);
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
// 仿 CSS 方法

static void on_klbui_check_margin(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_check->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_top(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_check->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_right(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_check->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_bottom(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_check->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_margin_left(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_check->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_check->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_top(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_check->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_right(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_check->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_bottom(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_check->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_padding_left(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_check->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.off_normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.off_focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_text_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.off_disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_text_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.on_normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_text_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.on_focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_text_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_check->css.on_disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.off_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.off_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.off_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.on_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.on_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_check->css.on_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.off_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.off_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_background_image_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.off_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_image(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.on_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_image_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.on_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_background_image_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_check->css.on_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.off_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.off_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_width_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.off_disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_width(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.on_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_width_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.on_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_width_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_check->css.on_disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.off_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.off_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_border_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.off_disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_color(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.on_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_color_focus(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.on_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_check_on_border_color_disable(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_check->css.on_disable.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////
// 自定义属性

static void on_klbui_check_value(klb_wnd_t* p_wnd, klbui_check_t* p_check, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_check(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_bool:
            {
                bool check = klb_map_idx_to_bool(p_in, start);
                klb_wnd_check(p_wnd, check);

                klb_wnd_update(p_wnd);
            }
            break;
        default:
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

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
    klb_gui_css_map_append_std_function(ptr, p_check); // 添加公共方法


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

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
    KLBUI_check_bind("check.color", on_klbui_check_on_text_color);
    KLBUI_check_bind("check.color:focus", on_klbui_check_on_text_color_focus);
    KLBUI_check_bind("check.color:disable", on_klbui_check_on_text_color_disable);

    // 背景色 background-color
    KLBUI_check_bind("background-color", on_klbui_check_background_color);
    KLBUI_check_bind("background-color:focus", on_klbui_check_background_color_focus);
    KLBUI_check_bind("background-color:disable", on_klbui_check_background_color_disable);
    KLBUI_check_bind("check.background-color", on_klbui_check_on_background_color);
    KLBUI_check_bind("check.background-color:focus", on_klbui_check_on_background_color_focus);
    KLBUI_check_bind("check.background-color:disable", on_klbui_check_on_background_color_disable);

    // 背景图片 background-image
    KLBUI_check_bind("background-image", on_klbui_check_background_image);
    KLBUI_check_bind("background-image:focus", on_klbui_check_background_image_focus);
    KLBUI_check_bind("background-image:disable", on_klbui_check_background_image_disable);
    KLBUI_check_bind("check.background-image", on_klbui_check_on_background_image);
    KLBUI_check_bind("check.background-image:focus", on_klbui_check_on_background_image_focus);
    KLBUI_check_bind("check.background-image:disable", on_klbui_check_on_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_check_bind("border-width", on_klbui_check_border_width);
    KLBUI_check_bind("border-width:focus", on_klbui_check_border_width_focus);
    KLBUI_check_bind("border-width:disable", on_klbui_check_border_width_disable);
    KLBUI_check_bind("check.border-width", on_klbui_check_on_border_width);
    KLBUI_check_bind("check.border-width:focus", on_klbui_check_on_border_width_focus);
    KLBUI_check_bind("check.border-width:disable", on_klbui_check_on_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_check_bind("border-color", on_klbui_check_border_color);
    KLBUI_check_bind("border-color:focus", on_klbui_check_border_color_focus);
    KLBUI_check_bind("border-color:disable", on_klbui_check_border_color_disable);
    KLBUI_check_bind("check.border-color", on_klbui_check_on_border_color);
    KLBUI_check_bind("check.border-color:focus", on_klbui_check_on_border_color_focus);
    KLBUI_check_bind("check.border-color:disable", on_klbui_check_on_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_check_bind("value", on_klbui_check_value);
    KLBUI_check_bind("check", on_klbui_check_value);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_check_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_check_t));
    klbui_check_t* p_check = (klbui_check_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_check_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_check_destroy;
    p_wnd->vtable.on_set = klbui_check_on_set;
    p_wnd->vtable.on_get = klbui_check_on_get;

    // css
    klbwnd_check_css_init(&p_check->css, p_gui);
    klbwnd_check_set_css(p_wnd, &p_check->css);

    // 初始化CSS 支持的方法
    klbui_check_init_func_map(p_wnd, p_check, p_gui);

    return p_wnd;
}
