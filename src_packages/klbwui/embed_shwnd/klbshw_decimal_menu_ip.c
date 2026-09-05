// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbshw_decimal_menu_ip.h"
#include "klbwui/embed_shwnd/klbwnd_decimal_menu_ip.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klbshw_decimal_menu_ip_t
/// @brief  共享使用的 10 进制输入面板
typedef struct klbshw_decimal_menu_ip_t_
{
    klbwnd_decimal_menu_ip_t   decimal_menu;   ///< 必须首位, 保持内存一致

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbshw_decimal_menu_ip_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbshw_decimal_menu_ip_css_cb)(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_decimal_menu_ip_destroy(klb_wnd_t* p_wnd)
{
    klbshw_decimal_menu_ip_t* p_menu = (klbshw_decimal_menu_ip_t*)p_wnd->ctrl;

    klbwnd_decimal_menu_ip_deinit(p_wnd);
    klbwnd_decimal_menu_ip_css_deinit(&p_menu->decimal_menu);

    KLB_FREE(p_wnd);
}

static int klbshw_decimal_menu_ip_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_decimal_menu_ip_t* p_menu = (klbshw_decimal_menu_ip_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_decimal_menu_ip_css_cb func = (klbshw_decimal_menu_ip_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbshw_decimal_menu_ip_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_decimal_menu_ip_t* p_menu = (klbshw_decimal_menu_ip_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_decimal_menu_ip_css_cb func = (klbshw_decimal_menu_ip_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 根

static void on_klbshw_decimal_menu_ip_visibility(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_margin(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->decimal_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_margin_top(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->decimal_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_margin_right(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->decimal_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_margin_bottom(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->decimal_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_margin_left(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->decimal_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_padding(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->decimal_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_padding_top(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->decimal_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_padding_right(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->decimal_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_padding_bottom(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->decimal_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_padding_left(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->decimal_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_text_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->decimal_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_text_align(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->decimal_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_font_size(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->decimal_menu.css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_background_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->decimal_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_background_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->decimal_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_border_width(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->decimal_menu.css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_border_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->decimal_menu.css.normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part string

static void on_klbshw_decimal_menu_ip_string_background_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->decimal_menu.css.string_background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_string_background_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->decimal_menu.css.string_background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_cursor_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_color(&(p_menu->decimal_menu.cursor_color), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part button

static void on_klbshw_decimal_menu_ip_button_margin(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->decimal_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_margin_top(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->decimal_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_margin_right(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->decimal_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_margin_bottom(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->decimal_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_margin_left(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->decimal_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_padding(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->decimal_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_padding_top(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->decimal_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_padding_right(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->decimal_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_padding_bottom(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->decimal_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_padding_left(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->decimal_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->decimal_menu.css.btn_css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_color_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->decimal_menu.css.btn_css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_color_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->decimal_menu.css.btn_css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_align(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->decimal_menu.css.btn_css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_align_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->decimal_menu.css.btn_css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_text_align_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->decimal_menu.css.btn_css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_font_size(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->decimal_menu.css.btn_css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_font_size_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->decimal_menu.css.btn_css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_font_size_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->decimal_menu.css.btn_css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->decimal_menu.css.btn_css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_color_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->decimal_menu.css.btn_css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_color_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->decimal_menu.css.btn_css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->decimal_menu.css.btn_css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->decimal_menu.css.btn_css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_background_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->decimal_menu.css.btn_css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_width(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->decimal_menu.css.btn_css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_width_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->decimal_menu.css.btn_css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_width_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->decimal_menu.css.btn_css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_color(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->decimal_menu.css.btn_css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_color_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->decimal_menu.css.btn_css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_border_color_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->decimal_menu.css.btn_css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// stretch-image

static void apply_stretch_klbshw_decimal_menu_ip(sds* p_attr_image, sds* p_bg_image, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(p_attr_image, p_wnd, method, p_in, p_out);

    if (KLBUI_CSSEX_set == method)
    {
        *p_bg_image = klb_sds_assign(*p_bg_image, *p_attr_image);
    }
}

static void on_klbshw_decimal_menu_ip_button_stretch_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.normal_stretch_image, &p_menu->decimal_menu.css.btn_css.normal.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_stretch_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.focus_stretch_image, &p_menu->decimal_menu.css.btn_css.focus.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_stretch_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.disable_stretch_image, &p_menu->decimal_menu.css.btn_css.disable.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_check_stretch_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.check_normal_stretch_image, &p_menu->decimal_menu.css.btn_css.check_normal.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_check_stretch_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.check_focus_stretch_image, &p_menu->decimal_menu.css.btn_css.check_focus.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_check_stretch_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_decimal_menu_ip(&p_menu->decimal_menu.css.btn_attr.check_disable_stretch_image, &p_menu->decimal_menu.css.btn_css.check_disable.background.image, p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// foreground-image

static void on_klbshw_decimal_menu_ip_button_moveleft_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.normal_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_moveright_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.normal_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_backspace_image(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.normal_foreground.backspace_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_moveleft_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.focus_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_moveright_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.focus_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_backspace_image_focus(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.focus_foreground.backspace_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_moveleft_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.disable_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_moveright_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.disable_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_decimal_menu_ip_button_backspace_image_disable(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->decimal_menu.css.btn_attr.disable_foreground.backspace_image), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBSHW_decimal_menu_ip_bind(KEY_, FUNC_) { klb_map_set_ptr(p_menu->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbshw_decimal_menu_ip_init_func_map(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_t* p_menu, klb_gui_t* p_gui)
{
    (void)p_wnd;

    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_decimal_menu_ip);
    if (NULL != ptr)
    {
        p_menu->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBSHW_decimal_menu_ip);
    p_menu->p_func_map = ptr;

    // 根
    KLBSHW_decimal_menu_ip_bind("visibility", on_klbshw_decimal_menu_ip_visibility);

    KLBSHW_decimal_menu_ip_bind("margin", on_klbshw_decimal_menu_ip_margin);
    KLBSHW_decimal_menu_ip_bind("margin-top", on_klbshw_decimal_menu_ip_margin_top);
    KLBSHW_decimal_menu_ip_bind("margin-right", on_klbshw_decimal_menu_ip_margin_right);
    KLBSHW_decimal_menu_ip_bind("margin-bottom", on_klbshw_decimal_menu_ip_margin_bottom);
    KLBSHW_decimal_menu_ip_bind("margin-left", on_klbshw_decimal_menu_ip_margin_left);

    KLBSHW_decimal_menu_ip_bind("padding", on_klbshw_decimal_menu_ip_padding);
    KLBSHW_decimal_menu_ip_bind("padding-top", on_klbshw_decimal_menu_ip_padding_top);
    KLBSHW_decimal_menu_ip_bind("padding-right", on_klbshw_decimal_menu_ip_padding_right);
    KLBSHW_decimal_menu_ip_bind("padding-bottom", on_klbshw_decimal_menu_ip_padding_bottom);
    KLBSHW_decimal_menu_ip_bind("padding-left", on_klbshw_decimal_menu_ip_padding_left);

    KLBSHW_decimal_menu_ip_bind("color", on_klbshw_decimal_menu_ip_text_color);
    KLBSHW_decimal_menu_ip_bind("text-align", on_klbshw_decimal_menu_ip_text_align);
    KLBSHW_decimal_menu_ip_bind("font-size", on_klbshw_decimal_menu_ip_font_size);
    KLBSHW_decimal_menu_ip_bind("background-color", on_klbshw_decimal_menu_ip_background_color);
    KLBSHW_decimal_menu_ip_bind("background-image", on_klbshw_decimal_menu_ip_background_image);
    KLBSHW_decimal_menu_ip_bind("border-width", on_klbshw_decimal_menu_ip_border_width);
    KLBSHW_decimal_menu_ip_bind("border-color", on_klbshw_decimal_menu_ip_border_color);

    // part string
    KLBSHW_decimal_menu_ip_bind("string.background-color", on_klbshw_decimal_menu_ip_string_background_color);
    KLBSHW_decimal_menu_ip_bind("string.background-image", on_klbshw_decimal_menu_ip_string_background_image);
    KLBSHW_decimal_menu_ip_bind("cursor-color", on_klbshw_decimal_menu_ip_cursor_color);

    // part button
    KLBSHW_decimal_menu_ip_bind("button.margin", on_klbshw_decimal_menu_ip_button_margin);
    KLBSHW_decimal_menu_ip_bind("button.margin-top", on_klbshw_decimal_menu_ip_button_margin_top);
    KLBSHW_decimal_menu_ip_bind("button.margin-right", on_klbshw_decimal_menu_ip_button_margin_right);
    KLBSHW_decimal_menu_ip_bind("button.margin-bottom", on_klbshw_decimal_menu_ip_button_margin_bottom);
    KLBSHW_decimal_menu_ip_bind("button.margin-left", on_klbshw_decimal_menu_ip_button_margin_left);

    KLBSHW_decimal_menu_ip_bind("button.padding", on_klbshw_decimal_menu_ip_button_padding);
    KLBSHW_decimal_menu_ip_bind("button.padding-top", on_klbshw_decimal_menu_ip_button_padding_top);
    KLBSHW_decimal_menu_ip_bind("button.padding-right", on_klbshw_decimal_menu_ip_button_padding_right);
    KLBSHW_decimal_menu_ip_bind("button.padding-bottom", on_klbshw_decimal_menu_ip_button_padding_bottom);
    KLBSHW_decimal_menu_ip_bind("button.padding-left", on_klbshw_decimal_menu_ip_button_padding_left);

    KLBSHW_decimal_menu_ip_bind("button.color", on_klbshw_decimal_menu_ip_button_text_color);
    KLBSHW_decimal_menu_ip_bind("button.color:focus", on_klbshw_decimal_menu_ip_button_text_color_focus);
    KLBSHW_decimal_menu_ip_bind("button.color:disabled", on_klbshw_decimal_menu_ip_button_text_color_disable);

    KLBSHW_decimal_menu_ip_bind("button.text-align", on_klbshw_decimal_menu_ip_button_text_align);
    KLBSHW_decimal_menu_ip_bind("button.text-align:focus", on_klbshw_decimal_menu_ip_button_text_align_focus);
    KLBSHW_decimal_menu_ip_bind("button.text-align:disabled", on_klbshw_decimal_menu_ip_button_text_align_disable);

    KLBSHW_decimal_menu_ip_bind("button.font-size", on_klbshw_decimal_menu_ip_button_font_size);
    KLBSHW_decimal_menu_ip_bind("button.font-size:focus", on_klbshw_decimal_menu_ip_button_font_size_focus);
    KLBSHW_decimal_menu_ip_bind("button.font-size:disabled", on_klbshw_decimal_menu_ip_button_font_size_disable);

    KLBSHW_decimal_menu_ip_bind("button.background-color", on_klbshw_decimal_menu_ip_button_background_color);
    KLBSHW_decimal_menu_ip_bind("button.background-color:focus", on_klbshw_decimal_menu_ip_button_background_color_focus);
    KLBSHW_decimal_menu_ip_bind("button.background-color:disabled", on_klbshw_decimal_menu_ip_button_background_color_disable);

    KLBSHW_decimal_menu_ip_bind("button.background-image", on_klbshw_decimal_menu_ip_button_background_image);
    KLBSHW_decimal_menu_ip_bind("button.background-image:focus", on_klbshw_decimal_menu_ip_button_background_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.background-image:disabled", on_klbshw_decimal_menu_ip_button_background_image_disable);

    KLBSHW_decimal_menu_ip_bind("button.border-width", on_klbshw_decimal_menu_ip_button_border_width);
    KLBSHW_decimal_menu_ip_bind("button.border-width:focus", on_klbshw_decimal_menu_ip_button_border_width_focus);
    KLBSHW_decimal_menu_ip_bind("button.border-width:disabled", on_klbshw_decimal_menu_ip_button_border_width_disable);

    KLBSHW_decimal_menu_ip_bind("button.border-color", on_klbshw_decimal_menu_ip_button_border_color);
    KLBSHW_decimal_menu_ip_bind("button.border-color:focus", on_klbshw_decimal_menu_ip_button_border_color_focus);
    KLBSHW_decimal_menu_ip_bind("button.border-color:disabled", on_klbshw_decimal_menu_ip_button_border_color_disable);

    // part button stretch-image
    KLBSHW_decimal_menu_ip_bind("button.stretch-image", on_klbshw_decimal_menu_ip_button_stretch_image);
    KLBSHW_decimal_menu_ip_bind("button.stretch-image:focus", on_klbshw_decimal_menu_ip_button_stretch_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.stretch-image:disabled", on_klbshw_decimal_menu_ip_button_stretch_image_disable);

    KLBSHW_decimal_menu_ip_bind("button.stretch-image:checked", on_klbshw_decimal_menu_ip_button_check_stretch_image);
    KLBSHW_decimal_menu_ip_bind("button.stretch-image:checked:focus", on_klbshw_decimal_menu_ip_button_check_stretch_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.stretch-image:checked:disabled", on_klbshw_decimal_menu_ip_button_check_stretch_image_disable);

    // part button foreground-image
    KLBSHW_decimal_menu_ip_bind("button.moveleft-image", on_klbshw_decimal_menu_ip_button_moveleft_image);
    KLBSHW_decimal_menu_ip_bind("button.moveleft-image:focus", on_klbshw_decimal_menu_ip_button_moveleft_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.moveleft-image:disabled", on_klbshw_decimal_menu_ip_button_moveleft_image_disable);

    KLBSHW_decimal_menu_ip_bind("button.moveright-image", on_klbshw_decimal_menu_ip_button_moveright_image);
    KLBSHW_decimal_menu_ip_bind("button.moveright-image:focus", on_klbshw_decimal_menu_ip_button_moveright_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.moveright-image:disabled", on_klbshw_decimal_menu_ip_button_moveright_image_disable);

    KLBSHW_decimal_menu_ip_bind("button.backspace-image", on_klbshw_decimal_menu_ip_button_backspace_image);
    KLBSHW_decimal_menu_ip_bind("button.backspace-image:focus", on_klbshw_decimal_menu_ip_button_backspace_image_focus);
    KLBSHW_decimal_menu_ip_bind("button.backspace-image:disabled", on_klbshw_decimal_menu_ip_button_backspace_image_disable);
}


//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_decimal_menu_ip_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_decimal_menu_ip_t));
    klbshw_decimal_menu_ip_t* p_menu = (klbshw_decimal_menu_ip_t*)p_wnd->ctrl;

    klbwnd_decimal_menu_ip_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbshw_decimal_menu_ip_destroy;
    p_wnd->vtable.on_set = klbshw_decimal_menu_ip_on_set;
    p_wnd->vtable.on_get = klbshw_decimal_menu_ip_on_get;

    klbwnd_decimal_menu_ip_css_init(&p_menu->decimal_menu, p_gui);
    klbwnd_decimal_menu_ip_set_css(p_wnd, &p_menu->decimal_menu);

    klbshw_decimal_menu_ip_init_func_map(p_wnd, p_menu, p_gui);

    return p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// 导出公共函数

klb_wnd_t* klbui_shwnd_get_decimal_menu_ip(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_decimal_menu_ip);

    if (NULL == p_wnd)
    {
        p_wnd = klbshw_decimal_menu_ip_create(p_gui, 0, 0, 374, 262);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_decimal_menu_ip, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

int klbshw_decimal_menu_ip_bind(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb_result, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind(p_wnd, cb_result, ptr);
}

int klbshw_decimal_menu_ip_bind_click_point(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_point(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_click_number(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_number(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_click_moveleft(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_moveleft(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_click_moveright(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_moveright(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_click_backspace(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_backspace(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_click_clear(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_click_clear(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_get_cursor_pos(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_get_cursor_pos(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_bind_set_show_cursor(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    return klbwnd_decimal_menu_ip_bind_set_show_cursor(p_wnd, cb, ptr);
}

int klbshw_decimal_menu_ip_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h)
{
    return klbwnd_decimal_menu_ip_wh(p_wnd, p_out_w, p_out_h);
}

void klbshw_decimal_menu_ip_set_ranges(klb_wnd_t* p_wnd, int max_len)
{
    klbwnd_decimal_menu_ip_set_ranges(p_wnd, max_len);
}

void klbshw_decimal_menu_ip_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_decimal_menu_ip_set_value(p_wnd, value);
}

int klbshw_decimal_menu_ip_get_value(klb_wnd_t* p_wnd)
{
    return klbwnd_decimal_menu_ip_get_value(p_wnd);
}

void klbshw_decimal_menu_ip_layout(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_layout(p_wnd);
}

// end
