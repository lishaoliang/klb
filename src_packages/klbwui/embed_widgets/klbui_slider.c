// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_slider.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_slider_t
/// @brief  水平滑动条: Lua 注册名 kslider
typedef struct klbui_slider_t_
{
    klbwnd_slider_t         slider;         ///< 必须首位, 保持内存一致

    klbwnd_slider_css_t*    p_globalcss;    ///< 全局公共的 CSS
    klbwnd_slider_css_t*    p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_slider_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_slider_cb)(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_slider_destroy(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    klbwnd_slider_deinit(p_wnd);

    if (NULL != p_slider->p_mycss)
    {
        klbwnd_slider_css_deinit(p_slider->p_mycss);
        KLB_FREE(p_slider->p_mycss);
    }

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
// my css

static void new_my_css_klbui_slider(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    if (NULL == p_slider->p_mycss)
    {
        p_slider->p_mycss = KLB_MALLOCZ(klbwnd_slider_css_t, 1, 0);

        klbwnd_slider_css_copy(p_slider->p_mycss, p_slider->p_globalcss);
        klbwnd_slider_set_css(p_wnd, p_slider->p_mycss);
    }
}

static klbwnd_slider_css_t* get_css_klbui_slider(klb_wnd_t* p_wnd)
{
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    return (NULL != p_slider->p_mycss) ? p_slider->p_mycss : p_slider->p_globalcss;
}

static klbwnd_slider_css_t* check_css_klbui_slider(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_slider(p_wnd);
    }

    return get_css_klbui_slider(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_slider_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_foreground_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_foreground_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_foreground_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_foreground_height(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->foreground_h), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.pos_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.pos_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.pos_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->normal.pos_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->focus.pos_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->disable.pos_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->pos_w), NULL, method, p_in, p_out);
}

static void globalcss_klbui_slider_pos_height(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->pos_h), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_slider(void* ptr)
{
    klbwnd_slider_css_t* p_css = (klbwnd_slider_css_t*)ptr;

    klbwnd_slider_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_slider_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_slider_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kslider);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kslider);

    klbwnd_slider_css_t* p_css = KLB_MALLOCZ(klbwnd_slider_css_t, 1, 0);
    klbwnd_slider_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kslider, p_css, destroy_globalcss_klbwnd_slider);

    KLBUI_GLOBAL_slider_bind("margin", globalcss_klbui_slider_margin);
    KLBUI_GLOBAL_slider_bind("margin-top", globalcss_klbui_slider_margin_top);
    KLBUI_GLOBAL_slider_bind("margin-right", globalcss_klbui_slider_margin_right);
    KLBUI_GLOBAL_slider_bind("margin-bottom", globalcss_klbui_slider_margin_bottom);
    KLBUI_GLOBAL_slider_bind("margin-left", globalcss_klbui_slider_margin_left);

    KLBUI_GLOBAL_slider_bind("padding", globalcss_klbui_slider_padding);
    KLBUI_GLOBAL_slider_bind("padding-top", globalcss_klbui_slider_padding_top);
    KLBUI_GLOBAL_slider_bind("padding-right", globalcss_klbui_slider_padding_right);
    KLBUI_GLOBAL_slider_bind("padding-bottom", globalcss_klbui_slider_padding_bottom);
    KLBUI_GLOBAL_slider_bind("padding-left", globalcss_klbui_slider_padding_left);

    KLBUI_GLOBAL_slider_bind("color", globalcss_klbui_slider_text_color);
    KLBUI_GLOBAL_slider_bind("color:focus", globalcss_klbui_slider_text_color_focus);
    KLBUI_GLOBAL_slider_bind("color:disabled", globalcss_klbui_slider_text_color_disable);

    KLBUI_GLOBAL_slider_bind("text-align", globalcss_klbui_slider_text_align);
    KLBUI_GLOBAL_slider_bind("text-align:focus", globalcss_klbui_slider_text_align_focus);
    KLBUI_GLOBAL_slider_bind("text-align:disabled", globalcss_klbui_slider_text_align_disable);

    KLBUI_GLOBAL_slider_bind("background-color", globalcss_klbui_slider_background_color);
    KLBUI_GLOBAL_slider_bind("background-color:focus", globalcss_klbui_slider_background_color_focus);
    KLBUI_GLOBAL_slider_bind("background-color:disabled", globalcss_klbui_slider_background_color_disable);

    KLBUI_GLOBAL_slider_bind("foreground-color", globalcss_klbui_slider_foreground_color);
    KLBUI_GLOBAL_slider_bind("foreground-color:focus", globalcss_klbui_slider_foreground_color_focus);
    KLBUI_GLOBAL_slider_bind("foreground-color:disabled", globalcss_klbui_slider_foreground_color_disable);

    KLBUI_GLOBAL_slider_bind("foreground-height", globalcss_klbui_slider_foreground_height);

    KLBUI_GLOBAL_slider_bind("pos.color", globalcss_klbui_slider_pos_color);
    KLBUI_GLOBAL_slider_bind("pos.color:focus", globalcss_klbui_slider_pos_color_focus);
    KLBUI_GLOBAL_slider_bind("pos.color:disabled", globalcss_klbui_slider_pos_color_disable);

    KLBUI_GLOBAL_slider_bind("pos.image", globalcss_klbui_slider_pos_image);
    KLBUI_GLOBAL_slider_bind("pos.image:focus", globalcss_klbui_slider_pos_image_focus);
    KLBUI_GLOBAL_slider_bind("pos.image:disabled", globalcss_klbui_slider_pos_image_disable);

    KLBUI_GLOBAL_slider_bind("pos.width", globalcss_klbui_slider_pos_width);
    KLBUI_GLOBAL_slider_bind("pos.height", globalcss_klbui_slider_pos_height);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_slider_margin(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_margin_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_margin_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_margin_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_margin_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_padding(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_padding_top(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_padding_right(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_padding_bottom(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_padding_left(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_align(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_align_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_text_align_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_background_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.background_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_background_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.background_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_background_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.background_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_foreground_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.foreground_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_foreground_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.foreground_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_foreground_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.foreground_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_foreground_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->foreground_h), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_color(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.pos_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_color_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.pos_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_color_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.pos_color), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_image(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->normal.pos_image), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_image_focus(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->focus.pos_image), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_image_disable(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->disable.pos_image), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_width(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->pos_w), p_wnd, method, p_in, p_out);
    (void)p_slider;
}

static void on_klbui_slider_pos_height(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_slider_css_t* p_css = check_css_klbui_slider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->pos_h), p_wnd, method, p_in, p_out);
    (void)p_slider;
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbui_slider_title(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_slider->slider.title), p_wnd, method, p_in, p_out);
}

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

#define KLBUI_slider_bind(KEY_, FUNC_) { klb_map_set_ptr(p_slider->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_slider_init_func_map(klb_wnd_t* p_wnd, klbui_slider_t* p_slider, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kslider);
    if (NULL != ptr)
    {
        p_slider->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kslider);
    p_slider->p_func_map = ptr;

    KLBUI_slider_bind("margin", on_klbui_slider_margin);
    KLBUI_slider_bind("margin-top", on_klbui_slider_margin_top);
    KLBUI_slider_bind("margin-right", on_klbui_slider_margin_right);
    KLBUI_slider_bind("margin-bottom", on_klbui_slider_margin_bottom);
    KLBUI_slider_bind("margin-left", on_klbui_slider_margin_left);

    KLBUI_slider_bind("padding", on_klbui_slider_padding);
    KLBUI_slider_bind("padding-top", on_klbui_slider_padding_top);
    KLBUI_slider_bind("padding-right", on_klbui_slider_padding_right);
    KLBUI_slider_bind("padding-bottom", on_klbui_slider_padding_bottom);
    KLBUI_slider_bind("padding-left", on_klbui_slider_padding_left);

    KLBUI_slider_bind("color", on_klbui_slider_text_color);
    KLBUI_slider_bind("color:focus", on_klbui_slider_text_color_focus);
    KLBUI_slider_bind("color:disabled", on_klbui_slider_text_color_disable);

    KLBUI_slider_bind("text-align", on_klbui_slider_text_align);
    KLBUI_slider_bind("text-align:focus", on_klbui_slider_text_align_focus);
    KLBUI_slider_bind("text-align:disabled", on_klbui_slider_text_align_disable);

    KLBUI_slider_bind("background-color", on_klbui_slider_background_color);
    KLBUI_slider_bind("background-color:focus", on_klbui_slider_background_color_focus);
    KLBUI_slider_bind("background-color:disabled", on_klbui_slider_background_color_disable);

    KLBUI_slider_bind("foreground-color", on_klbui_slider_foreground_color);
    KLBUI_slider_bind("foreground-color:focus", on_klbui_slider_foreground_color_focus);
    KLBUI_slider_bind("foreground-color:disabled", on_klbui_slider_foreground_color_disable);

    KLBUI_slider_bind("foreground-height", on_klbui_slider_foreground_height);

    KLBUI_slider_bind("pos.color", on_klbui_slider_pos_color);
    KLBUI_slider_bind("pos.color:focus", on_klbui_slider_pos_color_focus);
    KLBUI_slider_bind("pos.color:disabled", on_klbui_slider_pos_color_disable);

    KLBUI_slider_bind("pos.image", on_klbui_slider_pos_image);
    KLBUI_slider_bind("pos.image:focus", on_klbui_slider_pos_image_focus);
    KLBUI_slider_bind("pos.image:disabled", on_klbui_slider_pos_image_disable);

    KLBUI_slider_bind("pos.width", on_klbui_slider_pos_width);
    KLBUI_slider_bind("pos.height", on_klbui_slider_pos_height);

    KLBUI_slider_bind("title", on_klbui_slider_title);
    KLBUI_slider_bind("value", on_klbui_slider_value);
    KLBUI_slider_bind("min", on_klbui_slider_min);
    KLBUI_slider_bind("max", on_klbui_slider_max);

    (void)p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_slider_t));
    klbui_slider_t* p_slider = (klbui_slider_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_slider_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_slider_destroy;
    p_wnd->vtable.on_set = klbui_slider_on_set;
    p_wnd->vtable.on_get = klbui_slider_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_slider_init_func_map(p_wnd, p_slider, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_slider->p_globalcss = (klbwnd_slider_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kslider);
    klbwnd_slider_set_css(p_wnd, p_slider->p_globalcss);

    return p_wnd;
}

int klbui_register_kslider(klb_gui_t* p_gui)
{
    klbui_slider_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kslider, klbui_slider_create);
}

// end
