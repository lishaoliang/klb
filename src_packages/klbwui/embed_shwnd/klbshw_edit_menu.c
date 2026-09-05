// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbshw_edit_menu.h"
#include "klbwui/embed_shwnd/klbwnd_edit_menu.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klbshw_edit_menu_t
/// @brief  共享使用的 edit 输入面板
typedef struct klbshw_edit_menu_t_
{
    klbwnd_edit_menu_t      edit_menu;      ///< 必须首位, 保持内存一致

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbshw_edit_menu_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbshw_edit_menu_css_cb)(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_edit_menu_destroy(klb_wnd_t* p_wnd)
{
    klbshw_edit_menu_t* p_menu = (klbshw_edit_menu_t*)p_wnd->ctrl;

    klbwnd_edit_menu_deinit(p_wnd);
    klbwnd_edit_menu_css_deinit(&p_menu->edit_menu);

    KLB_FREE(p_wnd);
}

static int klbshw_edit_menu_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_edit_menu_t* p_menu = (klbshw_edit_menu_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_edit_menu_css_cb func = (klbshw_edit_menu_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbshw_edit_menu_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_edit_menu_t* p_menu = (klbshw_edit_menu_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_edit_menu_css_cb func = (klbshw_edit_menu_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 根

static void on_klbshw_edit_menu_visibility(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_margin(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->edit_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_margin_top(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->edit_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_margin_right(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->edit_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_margin_bottom(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->edit_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_margin_left(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->edit_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_padding(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->edit_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_padding_top(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->edit_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_padding_right(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->edit_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_padding_bottom(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->edit_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_padding_left(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->edit_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_text_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->edit_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_text_align(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->edit_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_font_size(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->edit_menu.css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_background_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->edit_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_background_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->edit_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_border_width(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->edit_menu.css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_border_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->edit_menu.css.normal.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part string

static void on_klbshw_edit_menu_string_background_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->edit_menu.css.string_background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_string_background_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->edit_menu.css.string_background), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part button

static void on_klbshw_edit_menu_button_margin(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->edit_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_margin_top(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->edit_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_margin_right(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->edit_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_margin_bottom(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->edit_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_margin_left(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->edit_menu.css.btn_css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_padding(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->edit_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_padding_top(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->edit_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_padding_right(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->edit_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_padding_bottom(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->edit_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_padding_left(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->edit_menu.css.btn_css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->edit_menu.css.btn_css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_color_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->edit_menu.css.btn_css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_color_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->edit_menu.css.btn_css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_align(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->edit_menu.css.btn_css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_align_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->edit_menu.css.btn_css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_text_align_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->edit_menu.css.btn_css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_font_size(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->edit_menu.css.btn_css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_font_size_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->edit_menu.css.btn_css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_font_size_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->edit_menu.css.btn_css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->edit_menu.css.btn_css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_color_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->edit_menu.css.btn_css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_color_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->edit_menu.css.btn_css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->edit_menu.css.btn_css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->edit_menu.css.btn_css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_background_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->edit_menu.css.btn_css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_width(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->edit_menu.css.btn_css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_width_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->edit_menu.css.btn_css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_width_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->edit_menu.css.btn_css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_color(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->edit_menu.css.btn_css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_color_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->edit_menu.css.btn_css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_border_color_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->edit_menu.css.btn_css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// stretch-image

static void apply_stretch_klbshw_edit_menu(sds* p_attr_image, sds* p_bg_image, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(p_attr_image, p_wnd, method, p_in, p_out);

    if (KLBUI_CSSEX_set == method)
    {
        *p_bg_image = klb_sds_assign(*p_bg_image, *p_attr_image);
    }
}

static void on_klbshw_edit_menu_button_stretch_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.normal_stretch_image, &p_menu->edit_menu.css.btn_css.normal.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_stretch_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.focus_stretch_image, &p_menu->edit_menu.css.btn_css.focus.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_stretch_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.disable_stretch_image, &p_menu->edit_menu.css.btn_css.disable.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_check_stretch_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.check_normal_stretch_image, &p_menu->edit_menu.css.btn_css.check_normal.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_check_stretch_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.check_focus_stretch_image, &p_menu->edit_menu.css.btn_css.check_focus.background.image, p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_check_stretch_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    apply_stretch_klbshw_edit_menu(&p_menu->edit_menu.css.btn_attr.check_disable_stretch_image, &p_menu->edit_menu.css.btn_css.check_disable.background.image, p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// foreground-image

static void on_klbshw_edit_menu_button_moveleft_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.normal_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_moveleft_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.focus_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_moveleft_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.disable_foreground.move_left_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_moveright_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.normal_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_moveright_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.focus_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_moveright_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.disable_foreground.move_right_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_backspace_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.normal_foreground.backspace_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_backspace_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.focus_foreground.backspace_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_backspace_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.disable_foreground.backspace_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_global_image(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.normal_foreground.global_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_global_image_focus(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.focus_foreground.global_image), p_wnd, method, p_in, p_out);
}

static void on_klbshw_edit_menu_button_global_image_disable(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_image(&(p_menu->edit_menu.css.btn_attr.disable_foreground.global_image), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBSHW_edit_menu_bind(KEY_, FUNC_) { klb_map_set_ptr(p_menu->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbshw_edit_menu_init_func_map(klb_wnd_t* p_wnd, klbshw_edit_menu_t* p_menu, klb_gui_t* p_gui)
{

    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_edit_menu);

    if (NULL != ptr)
    {
        p_menu->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBSHW_edit_menu);
    p_menu->p_func_map = ptr;

    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 根
    KLBSHW_edit_menu_bind("visibility", on_klbshw_edit_menu_visibility);

    KLBSHW_edit_menu_bind("margin", on_klbshw_edit_menu_margin);
    KLBSHW_edit_menu_bind("margin-top", on_klbshw_edit_menu_margin_top);
    KLBSHW_edit_menu_bind("margin-right", on_klbshw_edit_menu_margin_right);
    KLBSHW_edit_menu_bind("margin-bottom", on_klbshw_edit_menu_margin_bottom);
    KLBSHW_edit_menu_bind("margin-left", on_klbshw_edit_menu_margin_left);

    KLBSHW_edit_menu_bind("padding", on_klbshw_edit_menu_padding);
    KLBSHW_edit_menu_bind("padding-top", on_klbshw_edit_menu_padding_top);
    KLBSHW_edit_menu_bind("padding-right", on_klbshw_edit_menu_padding_right);
    KLBSHW_edit_menu_bind("padding-bottom", on_klbshw_edit_menu_padding_bottom);
    KLBSHW_edit_menu_bind("padding-left", on_klbshw_edit_menu_padding_left);

    KLBSHW_edit_menu_bind("color", on_klbshw_edit_menu_text_color);
    KLBSHW_edit_menu_bind("text-align", on_klbshw_edit_menu_text_align);
    KLBSHW_edit_menu_bind("font-size", on_klbshw_edit_menu_font_size);
    KLBSHW_edit_menu_bind("background-color", on_klbshw_edit_menu_background_color);
    KLBSHW_edit_menu_bind("background-image", on_klbshw_edit_menu_background_image);
    KLBSHW_edit_menu_bind("border-width", on_klbshw_edit_menu_border_width);
    KLBSHW_edit_menu_bind("border-color", on_klbshw_edit_menu_border_color);

    // part string
    KLBSHW_edit_menu_bind("string.background-color", on_klbshw_edit_menu_string_background_color);
    KLBSHW_edit_menu_bind("string.background-image", on_klbshw_edit_menu_string_background_image);

    // part button
    KLBSHW_edit_menu_bind("button.margin", on_klbshw_edit_menu_button_margin);
    KLBSHW_edit_menu_bind("button.margin-top", on_klbshw_edit_menu_button_margin_top);
    KLBSHW_edit_menu_bind("button.margin-right", on_klbshw_edit_menu_button_margin_right);
    KLBSHW_edit_menu_bind("button.margin-bottom", on_klbshw_edit_menu_button_margin_bottom);
    KLBSHW_edit_menu_bind("button.margin-left", on_klbshw_edit_menu_button_margin_left);

    KLBSHW_edit_menu_bind("button.padding", on_klbshw_edit_menu_button_padding);
    KLBSHW_edit_menu_bind("button.padding-top", on_klbshw_edit_menu_button_padding_top);
    KLBSHW_edit_menu_bind("button.padding-right", on_klbshw_edit_menu_button_padding_right);
    KLBSHW_edit_menu_bind("button.padding-bottom", on_klbshw_edit_menu_button_padding_bottom);
    KLBSHW_edit_menu_bind("button.padding-left", on_klbshw_edit_menu_button_padding_left);

    KLBSHW_edit_menu_bind("button.color", on_klbshw_edit_menu_button_text_color);
    KLBSHW_edit_menu_bind("button.color:focus", on_klbshw_edit_menu_button_text_color_focus);
    KLBSHW_edit_menu_bind("button.color:disabled", on_klbshw_edit_menu_button_text_color_disable);

    KLBSHW_edit_menu_bind("button.text-align", on_klbshw_edit_menu_button_text_align);
    KLBSHW_edit_menu_bind("button.text-align:focus", on_klbshw_edit_menu_button_text_align_focus);
    KLBSHW_edit_menu_bind("button.text-align:disabled", on_klbshw_edit_menu_button_text_align_disable);

    KLBSHW_edit_menu_bind("button.font-size", on_klbshw_edit_menu_button_font_size);
    KLBSHW_edit_menu_bind("button.font-size:focus", on_klbshw_edit_menu_button_font_size_focus);
    KLBSHW_edit_menu_bind("button.font-size:disabled", on_klbshw_edit_menu_button_font_size_disable);

    KLBSHW_edit_menu_bind("button.background-color", on_klbshw_edit_menu_button_background_color);
    KLBSHW_edit_menu_bind("button.background-color:focus", on_klbshw_edit_menu_button_background_color_focus);
    KLBSHW_edit_menu_bind("button.background-color:disabled", on_klbshw_edit_menu_button_background_color_disable);

    KLBSHW_edit_menu_bind("button.background-image", on_klbshw_edit_menu_button_background_image);
    KLBSHW_edit_menu_bind("button.background-image:focus", on_klbshw_edit_menu_button_background_image_focus);
    KLBSHW_edit_menu_bind("button.background-image:disabled", on_klbshw_edit_menu_button_background_image_disable);

    KLBSHW_edit_menu_bind("button.border-width", on_klbshw_edit_menu_button_border_width);
    KLBSHW_edit_menu_bind("button.border-width:focus", on_klbshw_edit_menu_button_border_width_focus);
    KLBSHW_edit_menu_bind("button.border-width:disabled", on_klbshw_edit_menu_button_border_width_disable);

    KLBSHW_edit_menu_bind("button.border-color", on_klbshw_edit_menu_button_border_color);
    KLBSHW_edit_menu_bind("button.border-color:focus", on_klbshw_edit_menu_button_border_color_focus);
    KLBSHW_edit_menu_bind("button.border-color:disabled", on_klbshw_edit_menu_button_border_color_disable);

    // part button stretch-image
    KLBSHW_edit_menu_bind("button.stretch-image", on_klbshw_edit_menu_button_stretch_image);
    KLBSHW_edit_menu_bind("button.stretch-image:focus", on_klbshw_edit_menu_button_stretch_image_focus);
    KLBSHW_edit_menu_bind("button.stretch-image:disabled", on_klbshw_edit_menu_button_stretch_image_disable);

    KLBSHW_edit_menu_bind("button.stretch-image:checked", on_klbshw_edit_menu_button_check_stretch_image);
    KLBSHW_edit_menu_bind("button.stretch-image:checked:focus", on_klbshw_edit_menu_button_check_stretch_image_focus);
    KLBSHW_edit_menu_bind("button.stretch-image:checked:disabled", on_klbshw_edit_menu_button_check_stretch_image_disable);

    // part button foreground-image
    KLBSHW_edit_menu_bind("button.moveleft-image", on_klbshw_edit_menu_button_moveleft_image);
    KLBSHW_edit_menu_bind("button.moveleft-image:focus", on_klbshw_edit_menu_button_moveleft_image_focus);
    KLBSHW_edit_menu_bind("button.moveleft-image:disabled", on_klbshw_edit_menu_button_moveleft_image_disable);

    KLBSHW_edit_menu_bind("button.moveright-image", on_klbshw_edit_menu_button_moveright_image);
    KLBSHW_edit_menu_bind("button.moveright-image:focus", on_klbshw_edit_menu_button_moveright_image_focus);
    KLBSHW_edit_menu_bind("button.moveright-image:disabled", on_klbshw_edit_menu_button_moveright_image_disable);

    KLBSHW_edit_menu_bind("button.backspace-image", on_klbshw_edit_menu_button_backspace_image);
    KLBSHW_edit_menu_bind("button.backspace-image:focus", on_klbshw_edit_menu_button_backspace_image_focus);
    KLBSHW_edit_menu_bind("button.backspace-image:disabled", on_klbshw_edit_menu_button_backspace_image_disable);

    KLBSHW_edit_menu_bind("button.global-image", on_klbshw_edit_menu_button_global_image);
    KLBSHW_edit_menu_bind("button.global-image:focus", on_klbshw_edit_menu_button_global_image_focus);
    KLBSHW_edit_menu_bind("button.global-image:disabled", on_klbshw_edit_menu_button_global_image_disable);
}


//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_edit_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_edit_menu_t));
    klbshw_edit_menu_t* p_menu = (klbshw_edit_menu_t*)p_wnd->ctrl;

    klbwnd_edit_menu_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbshw_edit_menu_destroy;
    p_wnd->vtable.on_set = klbshw_edit_menu_on_set;
    p_wnd->vtable.on_get = klbshw_edit_menu_on_get;

    klbwnd_edit_menu_css_init(&p_menu->edit_menu, p_gui);
    klbwnd_edit_menu_set_css(p_wnd, &p_menu->edit_menu);

    klbshw_edit_menu_init_func_map(p_wnd, p_menu, p_gui);

    return p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// 导出公共函数

klb_wnd_t* klbui_shwnd_get_edit_menu(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_edit_menu);

    if (NULL == p_wnd)
    {
        p_wnd = klbshw_edit_menu_create(p_gui, 0, 0, 622, 262);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_edit_menu, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

int klbshw_edit_menu_bind(klb_wnd_t* p_wnd, klbshw_edit_menu_cb cb_result, void* ptr)
{
    return klbwnd_edit_menu_bind(p_wnd, cb_result, ptr);
}

int klbshw_edit_menu_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h)
{
    return klbwnd_edit_menu_wh(p_wnd, p_out_w, p_out_h);
}

void klbshw_edit_menu_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_edit_menu_set_value(p_wnd, p_value);
}

const sds klbshw_edit_menu_get_value(klb_wnd_t* p_wnd)
{
    return klbwnd_edit_menu_get_value(p_wnd);
}

void klbshw_edit_menu_layout(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_layout(p_wnd);
}

void klbshw_edit_menu_set_hide_letter(klb_wnd_t* p_wnd, bool is_hide_letter)
{
    klbwnd_edit_menu_set_hide_letter(p_wnd, is_hide_letter);
}

void klbshw_edit_menu_set_page_letter(klb_wnd_t* p_wnd, int page)
{
    klbwnd_edit_menu_set_page_letter(p_wnd, page);
}

// end
