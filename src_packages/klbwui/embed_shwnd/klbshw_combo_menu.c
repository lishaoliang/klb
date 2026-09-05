// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbshw_combo_menu.h"
#include "klbwui/embed_shwnd/klbwnd_combo_menu.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klbshw_combo_menu_t
/// @brief  共享使用的 combo 弹出菜单
typedef struct klbshw_combo_menu_t_
{
    klbwnd_combo_menu_t      combo_menu;      ///< 必须首位, 保持内存一致

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbshw_combo_menu_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbshw_combo_menu_css_cb)(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_combo_menu_destroy(klb_wnd_t* p_wnd)
{
    klbshw_combo_menu_t* p_menu = (klbshw_combo_menu_t*)p_wnd->ctrl;

    klbwnd_combo_menu_deinit(p_wnd);
    klbwnd_combo_menu_css_deinit(&p_menu->combo_menu);

    KLB_FREE(p_wnd);
}

static int klbshw_combo_menu_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_combo_menu_t* p_menu = (klbshw_combo_menu_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_combo_menu_css_cb func = (klbshw_combo_menu_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbshw_combo_menu_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_combo_menu_t* p_menu = (klbshw_combo_menu_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_combo_menu_css_cb func = (klbshw_combo_menu_css_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 根

static void on_klbshw_combo_menu_visibility(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_margin(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->combo_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_margin_top(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->combo_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_margin_right(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->combo_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_margin_bottom(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->combo_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_margin_left(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->combo_menu.css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_padding(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->combo_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_padding_top(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->combo_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_padding_right(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->combo_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_padding_bottom(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->combo_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_padding_left(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->combo_menu.css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_text_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->combo_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_text_align(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->combo_menu.css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_font_size(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->combo_menu.css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_background_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_background_image(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_border_width(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_border_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbshw_combo_menu_item_h(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_menu->combo_menu.css.item_h), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vscrollbar_w(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_menu->combo_menu.css.vscrollbar_w), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_height_max(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_menu->combo_menu.css.height_max), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part item

static void on_klbshw_combo_menu_item_margin(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->combo_menu.css.css_item.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_margin_top(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->combo_menu.css.css_item.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_margin_right(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->combo_menu.css.css_item.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_margin_bottom(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->combo_menu.css.css_item.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_margin_left(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->combo_menu.css.css_item.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_padding(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->combo_menu.css.css_item.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_padding_top(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->combo_menu.css.css_item.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_padding_right(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->combo_menu.css.css_item.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_padding_bottom(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->combo_menu.css.css_item.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_padding_left(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->combo_menu.css.css_item.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->combo_menu.css.css_item.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_color_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->combo_menu.css.css_item.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_color_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->combo_menu.css.css_item.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_align(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->combo_menu.css.css_item.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_align_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->combo_menu.css.css_item.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_text_align_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->combo_menu.css.css_item.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_font_size(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->combo_menu.css.css_item.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_font_size_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->combo_menu.css.css_item.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_font_size_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->combo_menu.css.css_item.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_item.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_color_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_item.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_color_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_item.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_image(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_item.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_image_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_item.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_background_image_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_item.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_width(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.css_item.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_width_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.css_item.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_width_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.css_item.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.css_item.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_color_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.css_item.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_item_border_color_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.css_item.disable.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part vscrollbar

static void on_klbshw_combo_menu_vs_background_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_background_color_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_background_color_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_background_image(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_background_image_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_background_image_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_border_width(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.css_vscrollbar.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_border_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.css_vscrollbar.normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- part vscrollbar-button

static void on_klbshw_combo_menu_vs_btn_background_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_background_color_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_background_color_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_background_image(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_background_image_focus(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_background_image_disable(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_border_width(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_border_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combo_menu_vs_btn_text_color(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->combo_menu.css.css_vscrollbar.css_btn.normal.text), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBSHW_combo_menu_bind(KEY_, FUNC_) { klb_map_set_ptr(p_menu->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbshw_combo_menu_init_func_map(klb_wnd_t* p_wnd, klbshw_combo_menu_t* p_menu, klb_gui_t* p_gui)
{
    (void)p_wnd;

    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_combo_menu);
    if (NULL != ptr)
    {
        p_menu->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBSHW_combo_menu);
    p_menu->p_func_map = ptr;

    // 根
    KLBSHW_combo_menu_bind("visibility", on_klbshw_combo_menu_visibility);

    KLBSHW_combo_menu_bind("margin", on_klbshw_combo_menu_margin);
    KLBSHW_combo_menu_bind("margin-top", on_klbshw_combo_menu_margin_top);
    KLBSHW_combo_menu_bind("margin-right", on_klbshw_combo_menu_margin_right);
    KLBSHW_combo_menu_bind("margin-bottom", on_klbshw_combo_menu_margin_bottom);
    KLBSHW_combo_menu_bind("margin-left", on_klbshw_combo_menu_margin_left);

    KLBSHW_combo_menu_bind("padding", on_klbshw_combo_menu_padding);
    KLBSHW_combo_menu_bind("padding-top", on_klbshw_combo_menu_padding_top);
    KLBSHW_combo_menu_bind("padding-right", on_klbshw_combo_menu_padding_right);
    KLBSHW_combo_menu_bind("padding-bottom", on_klbshw_combo_menu_padding_bottom);
    KLBSHW_combo_menu_bind("padding-left", on_klbshw_combo_menu_padding_left);

    KLBSHW_combo_menu_bind("color", on_klbshw_combo_menu_text_color);
    KLBSHW_combo_menu_bind("text-align", on_klbshw_combo_menu_text_align);
    KLBSHW_combo_menu_bind("font-size", on_klbshw_combo_menu_font_size);
    KLBSHW_combo_menu_bind("background-color", on_klbshw_combo_menu_background_color);
    KLBSHW_combo_menu_bind("background-image", on_klbshw_combo_menu_background_image);
    KLBSHW_combo_menu_bind("border-width", on_klbshw_combo_menu_border_width);
    KLBSHW_combo_menu_bind("border-color", on_klbshw_combo_menu_border_color);

    // 自定义属性
    KLBSHW_combo_menu_bind("item_h", on_klbshw_combo_menu_item_h);
    KLBSHW_combo_menu_bind("vscrollbar_w", on_klbshw_combo_menu_vscrollbar_w);
    KLBSHW_combo_menu_bind("height_max", on_klbshw_combo_menu_height_max);

    // part item
    KLBSHW_combo_menu_bind("item.margin", on_klbshw_combo_menu_item_margin);
    KLBSHW_combo_menu_bind("item.margin-top", on_klbshw_combo_menu_item_margin_top);
    KLBSHW_combo_menu_bind("item.margin-right", on_klbshw_combo_menu_item_margin_right);
    KLBSHW_combo_menu_bind("item.margin-bottom", on_klbshw_combo_menu_item_margin_bottom);
    KLBSHW_combo_menu_bind("item.margin-left", on_klbshw_combo_menu_item_margin_left);

    KLBSHW_combo_menu_bind("item.padding", on_klbshw_combo_menu_item_padding);
    KLBSHW_combo_menu_bind("item.padding-top", on_klbshw_combo_menu_item_padding_top);
    KLBSHW_combo_menu_bind("item.padding-right", on_klbshw_combo_menu_item_padding_right);
    KLBSHW_combo_menu_bind("item.padding-bottom", on_klbshw_combo_menu_item_padding_bottom);
    KLBSHW_combo_menu_bind("item.padding-left", on_klbshw_combo_menu_item_padding_left);

    KLBSHW_combo_menu_bind("item.color", on_klbshw_combo_menu_item_text_color);
    KLBSHW_combo_menu_bind("item.color:focus", on_klbshw_combo_menu_item_text_color_focus);
    KLBSHW_combo_menu_bind("item.color:disabled", on_klbshw_combo_menu_item_text_color_disable);

    KLBSHW_combo_menu_bind("item.text-align", on_klbshw_combo_menu_item_text_align);
    KLBSHW_combo_menu_bind("item.text-align:focus", on_klbshw_combo_menu_item_text_align_focus);
    KLBSHW_combo_menu_bind("item.text-align:disabled", on_klbshw_combo_menu_item_text_align_disable);

    KLBSHW_combo_menu_bind("item.font-size", on_klbshw_combo_menu_item_font_size);
    KLBSHW_combo_menu_bind("item.font-size:focus", on_klbshw_combo_menu_item_font_size_focus);
    KLBSHW_combo_menu_bind("item.font-size:disabled", on_klbshw_combo_menu_item_font_size_disable);

    KLBSHW_combo_menu_bind("item.background-color", on_klbshw_combo_menu_item_background_color);
    KLBSHW_combo_menu_bind("item.background-color:focus", on_klbshw_combo_menu_item_background_color_focus);
    KLBSHW_combo_menu_bind("item.background-color:disabled", on_klbshw_combo_menu_item_background_color_disable);

    KLBSHW_combo_menu_bind("item.background-image", on_klbshw_combo_menu_item_background_image);
    KLBSHW_combo_menu_bind("item.background-image:focus", on_klbshw_combo_menu_item_background_image_focus);
    KLBSHW_combo_menu_bind("item.background-image:disabled", on_klbshw_combo_menu_item_background_image_disable);

    KLBSHW_combo_menu_bind("item.border-width", on_klbshw_combo_menu_item_border_width);
    KLBSHW_combo_menu_bind("item.border-width:focus", on_klbshw_combo_menu_item_border_width_focus);
    KLBSHW_combo_menu_bind("item.border-width:disabled", on_klbshw_combo_menu_item_border_width_disable);

    KLBSHW_combo_menu_bind("item.border-color", on_klbshw_combo_menu_item_border_color);
    KLBSHW_combo_menu_bind("item.border-color:focus", on_klbshw_combo_menu_item_border_color_focus);
    KLBSHW_combo_menu_bind("item.border-color:disabled", on_klbshw_combo_menu_item_border_color_disable);

    // part vscrollbar
    KLBSHW_combo_menu_bind("vscrollbar.background-color", on_klbshw_combo_menu_vs_background_color);
    KLBSHW_combo_menu_bind("vscrollbar.background-color:focus", on_klbshw_combo_menu_vs_background_color_focus);
    KLBSHW_combo_menu_bind("vscrollbar.background-color:disabled", on_klbshw_combo_menu_vs_background_color_disable);

    KLBSHW_combo_menu_bind("vscrollbar.background-image", on_klbshw_combo_menu_vs_background_image);
    KLBSHW_combo_menu_bind("vscrollbar.background-image:focus", on_klbshw_combo_menu_vs_background_image_focus);
    KLBSHW_combo_menu_bind("vscrollbar.background-image:disabled", on_klbshw_combo_menu_vs_background_image_disable);

    KLBSHW_combo_menu_bind("vscrollbar.border-width", on_klbshw_combo_menu_vs_border_width);
    KLBSHW_combo_menu_bind("vscrollbar.border-color", on_klbshw_combo_menu_vs_border_color);

    // part vscrollbar-button
    KLBSHW_combo_menu_bind("vscrollbar-button.color", on_klbshw_combo_menu_vs_btn_text_color);
    KLBSHW_combo_menu_bind("vscrollbar-button.background-color", on_klbshw_combo_menu_vs_btn_background_color);
    KLBSHW_combo_menu_bind("vscrollbar-button.background-color:focus", on_klbshw_combo_menu_vs_btn_background_color_focus);
    KLBSHW_combo_menu_bind("vscrollbar-button.background-color:disabled", on_klbshw_combo_menu_vs_btn_background_color_disable);

    KLBSHW_combo_menu_bind("vscrollbar-button.background-image", on_klbshw_combo_menu_vs_btn_background_image);
    KLBSHW_combo_menu_bind("vscrollbar-button.background-image:focus", on_klbshw_combo_menu_vs_btn_background_image_focus);
    KLBSHW_combo_menu_bind("vscrollbar-button.background-image:disabled", on_klbshw_combo_menu_vs_btn_background_image_disable);

    KLBSHW_combo_menu_bind("vscrollbar-button.border-width", on_klbshw_combo_menu_vs_btn_border_width);
    KLBSHW_combo_menu_bind("vscrollbar-button.border-color", on_klbshw_combo_menu_vs_btn_border_color);
}


//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_combo_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_combo_menu_t));
    klbshw_combo_menu_t* p_menu = (klbshw_combo_menu_t*)p_wnd->ctrl;

    klbwnd_combo_menu_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbshw_combo_menu_destroy;
    p_wnd->vtable.on_set = klbshw_combo_menu_on_set;
    p_wnd->vtable.on_get = klbshw_combo_menu_on_get;

    klbwnd_combo_menu_css_init(&p_menu->combo_menu, p_gui);
    klbwnd_combo_menu_set_css(p_wnd, &p_menu->combo_menu);

    klbshw_combo_menu_init_func_map(p_wnd, p_menu, p_gui);

    return p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// 导出公共函数

klb_wnd_t* klbui_shwnd_get_combo_menu(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_combo_menu);

    if (NULL == p_wnd)
    {
        p_wnd = klbshw_combo_menu_create(p_gui, 0, 0, 120, 240);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_combo_menu, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

int klbshw_combo_menu_bind(klb_wnd_t* p_wnd, klbshw_combo_menu_cb cb, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h, bool is_more_col, int menu_min_w)
{
    return klbwnd_combo_menu_bind(p_wnd, cb, ptr, p_data_array, p_out_w, p_out_h, is_more_col, menu_min_w);
}

void klbshw_combo_menu_layout(klb_wnd_t* p_wnd)
{
    klbwnd_combo_menu_layout(p_wnd);
}

// end
