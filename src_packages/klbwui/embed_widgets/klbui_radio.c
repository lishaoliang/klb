// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_radio.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_radio_t
/// @brief  单选框: Lua 注册名 kradio
typedef struct klbui_radio_t_
{
    klbwnd_radio_t         radio;          ///< 必须首位, 保持内存一致

    klbwnd_radio_css_t*    p_globalcss;    ///< 全局公共的 CSS
    klbwnd_radio_css_t*    p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_radio_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_radio_cb)(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_radio_destroy(klb_wnd_t* p_wnd)
{
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;

    klbwnd_radio_deinit(p_wnd);

    if (NULL != p_radio->p_mycss)
    {
        klbwnd_radio_css_deinit(p_radio->p_mycss);
        KLB_FREE(p_radio->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_radio_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_radio_cb func = (klbui_radio_cb)klb_map_to_ptr(p_radio->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_radio, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_radio_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_radio_cb func = (klbui_radio_cb)klb_map_to_ptr(p_radio->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_radio, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_radio(klb_wnd_t* p_wnd)
{
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;

    if (NULL == p_radio->p_mycss)
    {
        p_radio->p_mycss = KLB_MALLOCZ(klbwnd_radio_css_t, 1, 0);

        klbwnd_radio_css_copy(p_radio->p_mycss, p_radio->p_globalcss);
        klbwnd_radio_set_css(p_wnd, p_radio->p_mycss);
    }
}

static klbwnd_radio_css_t* get_css_klbui_radio(klb_wnd_t* p_wnd)
{
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;

    return (NULL != p_radio->p_mycss) ? p_radio->p_mycss : p_radio->p_globalcss;
}

static klbwnd_radio_css_t* check_css_klbui_radio(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_radio(p_wnd);
    }

    return get_css_klbui_radio(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_radio_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_mode_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_mode_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_color_key_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_background_image_color_key_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->check_normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->check_focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_color(&(p_css->check_disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->check_normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->check_focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_text_align(&(p_css->check_disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->check_normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->check_focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_font_size(&(p_css->check_disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->check_normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->check_focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_color(&(p_css->check_disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->check_normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->check_focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_background_image(&(p_css->check_disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->check_normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_mode_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->check_focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_mode_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->check_disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->check_normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_color_key_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->check_focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_background_image_color_key_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->check_disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->check_normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->check_focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_width(&(p_css->check_disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->check_normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->check_focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_radio_check_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbuicssex_border_color(&(p_css->check_disable.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_radio(void* ptr)
{
    klbwnd_radio_css_t* p_css = (klbwnd_radio_css_t*)ptr;

    klbwnd_radio_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_radio_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_radio_init_globalcss(klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kradio);

    if (NULL != ptr)
    {
        return;
    }

    // step2. 新添加 解析map, 及公共 CSS 对象
    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kradio);

    klbwnd_radio_css_t* p_css = KLB_MALLOCZ(klbwnd_radio_css_t, 1, 0);
    klbwnd_radio_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kradio, p_css, destroy_globalcss_klbwnd_radio);


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_GLOBAL_radio_bind("margin", globalcss_klbui_radio_margin);
    KLBUI_GLOBAL_radio_bind("margin-top", globalcss_klbui_radio_margin_top);
    KLBUI_GLOBAL_radio_bind("margin-right", globalcss_klbui_radio_margin_right);
    KLBUI_GLOBAL_radio_bind("margin-bottom", globalcss_klbui_radio_margin_bottom);
    KLBUI_GLOBAL_radio_bind("margin-left", globalcss_klbui_radio_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_radio_bind("padding", globalcss_klbui_radio_padding);
    KLBUI_GLOBAL_radio_bind("padding-top", globalcss_klbui_radio_padding_top);
    KLBUI_GLOBAL_radio_bind("padding-right", globalcss_klbui_radio_padding_right);
    KLBUI_GLOBAL_radio_bind("padding-bottom", globalcss_klbui_radio_padding_bottom);
    KLBUI_GLOBAL_radio_bind("padding-left", globalcss_klbui_radio_padding_left);

    // 文本颜色 color
    KLBUI_GLOBAL_radio_bind("color", globalcss_klbui_radio_text_color);
    KLBUI_GLOBAL_radio_bind("color:focus", globalcss_klbui_radio_text_color_focus);
    KLBUI_GLOBAL_radio_bind("color:disabled", globalcss_klbui_radio_text_color_disable);
    KLBUI_GLOBAL_radio_bind("color:checked", globalcss_klbui_radio_check_text_color);
    KLBUI_GLOBAL_radio_bind("color:checked:focus", globalcss_klbui_radio_check_text_color_focus);
    KLBUI_GLOBAL_radio_bind("color:checked:disabled", globalcss_klbui_radio_check_text_color_disable);

    // 文本对齐 text-align
    KLBUI_GLOBAL_radio_bind("text-align", globalcss_klbui_radio_text_align);
    KLBUI_GLOBAL_radio_bind("text-align:focus", globalcss_klbui_radio_text_align_focus);
    KLBUI_GLOBAL_radio_bind("text-align:disabled", globalcss_klbui_radio_text_align_disable);
    KLBUI_GLOBAL_radio_bind("text-align:checked", globalcss_klbui_radio_check_text_align);
    KLBUI_GLOBAL_radio_bind("text-align:checked:focus", globalcss_klbui_radio_check_text_align_focus);
    KLBUI_GLOBAL_radio_bind("text-align:checked:disabled", globalcss_klbui_radio_check_text_align_disable);

    // 字体大小 font-size
    KLBUI_GLOBAL_radio_bind("font-size", globalcss_klbui_radio_font_size);
    KLBUI_GLOBAL_radio_bind("font-size:focus", globalcss_klbui_radio_font_size_focus);
    KLBUI_GLOBAL_radio_bind("font-size:disabled", globalcss_klbui_radio_font_size_disable);
    KLBUI_GLOBAL_radio_bind("font-size:checked", globalcss_klbui_radio_check_font_size);
    KLBUI_GLOBAL_radio_bind("font-size:checked:focus", globalcss_klbui_radio_check_font_size_focus);
    KLBUI_GLOBAL_radio_bind("font-size:checked:disabled", globalcss_klbui_radio_check_font_size_disable);

    // 背景色 background-color
    KLBUI_GLOBAL_radio_bind("background-color", globalcss_klbui_radio_background_color);
    KLBUI_GLOBAL_radio_bind("background-color:focus", globalcss_klbui_radio_background_color_focus);
    KLBUI_GLOBAL_radio_bind("background-color:disabled", globalcss_klbui_radio_background_color_disable);
    KLBUI_GLOBAL_radio_bind("background-color:checked", globalcss_klbui_radio_check_background_color);
    KLBUI_GLOBAL_radio_bind("background-color:checked:focus", globalcss_klbui_radio_check_background_color_focus);
    KLBUI_GLOBAL_radio_bind("background-color:checked:disabled", globalcss_klbui_radio_check_background_color_disable);

    // 背景图片 background-image
    KLBUI_GLOBAL_radio_bind("background-image", globalcss_klbui_radio_background_image);
    KLBUI_GLOBAL_radio_bind("background-image:focus", globalcss_klbui_radio_background_image_focus);
    KLBUI_GLOBAL_radio_bind("background-image:disabled", globalcss_klbui_radio_background_image_disable);
    KLBUI_GLOBAL_radio_bind("background-image:checked", globalcss_klbui_radio_check_background_image);
    KLBUI_GLOBAL_radio_bind("background-image:checked:focus", globalcss_klbui_radio_check_background_image_focus);
    KLBUI_GLOBAL_radio_bind("background-image:checked:disabled", globalcss_klbui_radio_check_background_image_disable);

    KLBUI_GLOBAL_radio_bind("background-image-mode", globalcss_klbui_radio_background_image_mode);
    KLBUI_GLOBAL_radio_bind("background-image-mode:focus", globalcss_klbui_radio_background_image_mode_focus);
    KLBUI_GLOBAL_radio_bind("background-image-mode:disabled", globalcss_klbui_radio_background_image_mode_disable);
    KLBUI_GLOBAL_radio_bind("background-image-mode:checked", globalcss_klbui_radio_check_background_image_mode);
    KLBUI_GLOBAL_radio_bind("background-image-mode:checked:focus", globalcss_klbui_radio_check_background_image_mode_focus);
    KLBUI_GLOBAL_radio_bind("background-image-mode:checked:disabled", globalcss_klbui_radio_check_background_image_mode_disable);

    KLBUI_GLOBAL_radio_bind("background-image-color-key", globalcss_klbui_radio_background_image_color_key);
    KLBUI_GLOBAL_radio_bind("background-image-color-key:focus", globalcss_klbui_radio_background_image_color_key_focus);
    KLBUI_GLOBAL_radio_bind("background-image-color-key:disabled", globalcss_klbui_radio_background_image_color_key_disable);
    KLBUI_GLOBAL_radio_bind("background-image-color-key:checked", globalcss_klbui_radio_check_background_image_color_key);
    KLBUI_GLOBAL_radio_bind("background-image-color-key:checked:focus", globalcss_klbui_radio_check_background_image_color_key_focus);
    KLBUI_GLOBAL_radio_bind("background-image-color-key:checked:disabled", globalcss_klbui_radio_check_background_image_color_key_disable);

    // 边框宽度 border
    KLBUI_GLOBAL_radio_bind("border-width", globalcss_klbui_radio_border_width);
    KLBUI_GLOBAL_radio_bind("border-width:focus", globalcss_klbui_radio_border_width_focus);
    KLBUI_GLOBAL_radio_bind("border-width:disabled", globalcss_klbui_radio_border_width_disable);
    KLBUI_GLOBAL_radio_bind("border-width:checked", globalcss_klbui_radio_check_border_width);
    KLBUI_GLOBAL_radio_bind("border-width:checked:focus", globalcss_klbui_radio_check_border_width_focus);
    KLBUI_GLOBAL_radio_bind("border-width:checked:disabled", globalcss_klbui_radio_check_border_width_disable);

    KLBUI_GLOBAL_radio_bind("border-color", globalcss_klbui_radio_border_color);
    KLBUI_GLOBAL_radio_bind("border-color:focus", globalcss_klbui_radio_border_color_focus);
    KLBUI_GLOBAL_radio_bind("border-color:disabled", globalcss_klbui_radio_border_color_disable);
    KLBUI_GLOBAL_radio_bind("border-color:checked", globalcss_klbui_radio_check_border_color);
    KLBUI_GLOBAL_radio_bind("border-color:checked:focus", globalcss_klbui_radio_check_border_color_focus);
    KLBUI_GLOBAL_radio_bind("border-color:checked:disabled", globalcss_klbui_radio_check_border_color_disable);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_radio_margin(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_margin_top(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_margin_right(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_margin_bottom(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_margin_left(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_padding(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_padding_top(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_padding_right(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_padding_bottom(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_padding_left(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->check_normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->check_focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_color(&(p_css->check_disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_align(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_align_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_text_align_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_align(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->check_normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_align_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->check_focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_text_align_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_text_align(&(p_css->check_disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_font_size(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_font_size_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_font_size_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_font_size(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->check_normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_font_size_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->check_focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_font_size_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_font_size(&(p_css->check_disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->check_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->check_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_color(&(p_css->check_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->check_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->check_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_background_image(&(p_css->check_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_mode(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_mode_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_mode_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_mode(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->check_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_mode_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->check_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_mode_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->check_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_color_key(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_color_key_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_background_image_color_key_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_color_key(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->check_normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_color_key_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->check_focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_background_image_color_key_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->check_disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_width(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_width_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_width_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_width(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->check_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_width_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->check_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_width_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_width(&(p_css->check_disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_border_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_color(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->check_normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_color_focus(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->check_focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_check_border_color_disable(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_radio_css_t* p_css = check_css_klbui_radio(p_wnd, method);

    klbuicssex_border_color(&(p_css->check_disable.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 命令键

static void on_klbui_radio_tip(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, klb_wnd_get_tip(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        const char* p_tip = klb_map_idx_to_string(p_in, start);

        klb_wnd_set_tip(p_wnd, p_tip);
        klb_wnd_dyntip(p_wnd, false);
    }
}

static void on_klbui_radio_dynamic_tip(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    bool use = klb_wnd_is_dyntip(p_wnd);

    klbuicssex_attribute_bool(&use, p_wnd, method, p_in, p_out);

    if (KLBUI_CSSEX_set == method)
    {
        klb_wnd_dyntip(p_wnd, use);
    }
}

// 私有自定义

static void on_klbui_radio_index(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_radio->radio.index), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_title(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_radio->radio.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_radio_value(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_check(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        if (KLB_ADT_bool == klb_map_array_type(p_in, start))
        {
            bool check = klb_map_idx_to_bool(p_in, start);
            klb_wnd_check(p_wnd, check);
            klb_wnd_update(p_wnd);
        }
    }

}
//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_radio_bind(KEY_, FUNC_) { klb_map_set_ptr(p_radio->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_radio_init_func_map(klb_wnd_t* p_wnd, klbui_radio_t* p_radio, klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kradio);
    if (NULL != ptr)
    {
        p_radio->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kradio);
    p_radio->p_func_map = ptr;


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_radio_bind("margin", on_klbui_radio_margin);
    KLBUI_radio_bind("margin-top", on_klbui_radio_margin_top);
    KLBUI_radio_bind("margin-right", on_klbui_radio_margin_right);
    KLBUI_radio_bind("margin-bottom", on_klbui_radio_margin_bottom);
    KLBUI_radio_bind("margin-left", on_klbui_radio_margin_left);

    // 内边距 padding
    KLBUI_radio_bind("padding", on_klbui_radio_padding);
    KLBUI_radio_bind("padding-top", on_klbui_radio_padding_top);
    KLBUI_radio_bind("padding-right", on_klbui_radio_padding_right);
    KLBUI_radio_bind("padding-bottom", on_klbui_radio_padding_bottom);
    KLBUI_radio_bind("padding-left", on_klbui_radio_padding_left);

    // 文本颜色 color
    KLBUI_radio_bind("color", on_klbui_radio_text_color);
    KLBUI_radio_bind("color:focus", on_klbui_radio_text_color_focus);
    KLBUI_radio_bind("color:disabled", on_klbui_radio_text_color_disable);
    KLBUI_radio_bind("color:checked", on_klbui_radio_check_text_color);
    KLBUI_radio_bind("color:checked:focus", on_klbui_radio_check_text_color_focus);
    KLBUI_radio_bind("color:checked:disabled", on_klbui_radio_check_text_color_disable);

    // 文本对齐 text-align
    KLBUI_radio_bind("text-align", on_klbui_radio_text_align);
    KLBUI_radio_bind("text-align:focus", on_klbui_radio_text_align_focus);
    KLBUI_radio_bind("text-align:disabled", on_klbui_radio_text_align_disable);
    KLBUI_radio_bind("text-align:checked", on_klbui_radio_check_text_align);
    KLBUI_radio_bind("text-align:checked:focus", on_klbui_radio_check_text_align_focus);
    KLBUI_radio_bind("text-align:checked:disabled", on_klbui_radio_check_text_align_disable);

    // 字体大小 font-size
    KLBUI_radio_bind("font-size", on_klbui_radio_font_size);
    KLBUI_radio_bind("font-size:focus", on_klbui_radio_font_size_focus);
    KLBUI_radio_bind("font-size:disabled", on_klbui_radio_font_size_disable);
    KLBUI_radio_bind("font-size:checked", on_klbui_radio_check_font_size);
    KLBUI_radio_bind("font-size:checked:focus", on_klbui_radio_check_font_size_focus);
    KLBUI_radio_bind("font-size:checked:disabled", on_klbui_radio_check_font_size_disable);

    // 背景色 background-color
    KLBUI_radio_bind("background-color", on_klbui_radio_background_color);
    KLBUI_radio_bind("background-color:focus", on_klbui_radio_background_color_focus);
    KLBUI_radio_bind("background-color:disabled", on_klbui_radio_background_color_disable);
    KLBUI_radio_bind("background-color:checked", on_klbui_radio_check_background_color);
    KLBUI_radio_bind("background-color:checked:focus", on_klbui_radio_check_background_color_focus);
    KLBUI_radio_bind("background-color:checked:disabled", on_klbui_radio_check_background_color_disable);

    // 背景图片 background-image
    KLBUI_radio_bind("background-image", on_klbui_radio_background_image);
    KLBUI_radio_bind("background-image:focus", on_klbui_radio_background_image_focus);
    KLBUI_radio_bind("background-image:disabled", on_klbui_radio_background_image_disable);
    KLBUI_radio_bind("background-image:checked", on_klbui_radio_check_background_image);
    KLBUI_radio_bind("background-image:checked:focus", on_klbui_radio_check_background_image_focus);
    KLBUI_radio_bind("background-image:checked:disabled", on_klbui_radio_check_background_image_disable);

    KLBUI_radio_bind("background-image-mode", on_klbui_radio_background_image_mode);
    KLBUI_radio_bind("background-image-mode:focus", on_klbui_radio_background_image_mode_focus);
    KLBUI_radio_bind("background-image-mode:disabled", on_klbui_radio_background_image_mode_disable);
    KLBUI_radio_bind("background-image-mode:checked", on_klbui_radio_check_background_image_mode);
    KLBUI_radio_bind("background-image-mode:checked:focus", on_klbui_radio_check_background_image_mode_focus);
    KLBUI_radio_bind("background-image-mode:checked:disabled", on_klbui_radio_check_background_image_mode_disable);

    KLBUI_radio_bind("background-image-color-key", on_klbui_radio_background_image_color_key);
    KLBUI_radio_bind("background-image-color-key:focus", on_klbui_radio_background_image_color_key_focus);
    KLBUI_radio_bind("background-image-color-key:disabled", on_klbui_radio_background_image_color_key_disable);
    KLBUI_radio_bind("background-image-color-key:checked", on_klbui_radio_check_background_image_color_key);
    KLBUI_radio_bind("background-image-color-key:checked:focus", on_klbui_radio_check_background_image_color_key_focus);
    KLBUI_radio_bind("background-image-color-key:checked:disabled", on_klbui_radio_check_background_image_color_key_disable);

    // 边框宽度 border
    KLBUI_radio_bind("border-width", on_klbui_radio_border_width);
    KLBUI_radio_bind("border-width:focus", on_klbui_radio_border_width_focus);
    KLBUI_radio_bind("border-width:disabled", on_klbui_radio_border_width_disable);
    KLBUI_radio_bind("border-width:checked", on_klbui_radio_check_border_width);
    KLBUI_radio_bind("border-width:checked:focus", on_klbui_radio_check_border_width_focus);
    KLBUI_radio_bind("border-width:checked:disabled", on_klbui_radio_check_border_width_disable);

    KLBUI_radio_bind("border-color", on_klbui_radio_border_color);
    KLBUI_radio_bind("border-color:focus", on_klbui_radio_border_color_focus);
    KLBUI_radio_bind("border-color:disabled", on_klbui_radio_border_color_disable);
    KLBUI_radio_bind("border-color:checked", on_klbui_radio_check_border_color);
    KLBUI_radio_bind("border-color:checked:focus", on_klbui_radio_check_border_color_focus);
    KLBUI_radio_bind("border-color:checked:disabled", on_klbui_radio_check_border_color_disable);

    // 命令键

    // 提示 tip
    KLBUI_radio_bind("tip", on_klbui_radio_tip);
    KLBUI_radio_bind("dynamic-tip", on_klbui_radio_dynamic_tip);

    // 私有自定义

    KLBUI_radio_bind("index", on_klbui_radio_index);
    KLBUI_radio_bind("title", on_klbui_radio_title);
    KLBUI_radio_bind("value", on_klbui_radio_value);
    KLBUI_radio_bind("check", on_klbui_radio_value);
    KLBUI_radio_bind("checked", on_klbui_radio_value);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_radio_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_radio_t));
    klbui_radio_t* p_radio = (klbui_radio_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_radio_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_radio_destroy;
    p_wnd->vtable.on_set = klbui_radio_on_set;
    p_wnd->vtable.on_get = klbui_radio_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_radio_init_func_map(p_wnd, p_radio, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_radio->p_globalcss = (klbwnd_radio_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kradio);
    klbwnd_radio_set_css(p_wnd, p_radio->p_globalcss);

    return p_wnd;
}

int klbui_register_kradio(klb_gui_t* p_gui)
{
    klbui_radio_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kradio, klbui_radio_create);
}

// end
