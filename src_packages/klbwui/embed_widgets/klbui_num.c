// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_num.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_num_t
/// @brief  数值编辑框: Lua 注册名 knum
typedef struct klbui_num_t_
{
    klbwnd_num_t            num;          ///< 必须首位, 保持内存一致

    klbwnd_num_css_t*       p_globalcss;    ///< 全局公共的 CSS
    klbwnd_num_css_t*       p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_num_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_num_cb)(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_num_destroy(klb_wnd_t* p_wnd)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    klbwnd_num_deinit(p_wnd);

    if (NULL != p_num->p_mycss)
    {
        klbwnd_num_css_deinit(p_num->p_mycss);
        KLB_FREE(p_num->p_mycss);
    }

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
// my css

static void new_my_css_klbui_num(klb_wnd_t* p_wnd)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    if (NULL == p_num->p_mycss)
    {
        p_num->p_mycss = KLB_MALLOCZ(klbwnd_num_css_t, 1, 0);

        klbwnd_num_css_copy(p_num->p_mycss, p_num->p_globalcss);
        klbwnd_num_set_css(p_wnd, p_num->p_mycss);
    }
}

static klbwnd_num_css_t* get_css_klbui_num(klb_wnd_t* p_wnd)
{
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    return (NULL != p_num->p_mycss) ? p_num->p_mycss : p_num->p_globalcss;
}

static klbwnd_num_css_t* check_css_klbui_num(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_num(p_wnd);
    }

    return get_css_klbui_num(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_num_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_mode_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_mode_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_color_key_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_background_image_color_key_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_num_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;
    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_num(void* ptr)
{
    klbwnd_num_css_t* p_css = (klbwnd_num_css_t*)ptr;

    klbwnd_num_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_num_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_num_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_knum);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_knum);

    klbwnd_num_css_t* p_css = KLB_MALLOCZ(klbwnd_num_css_t, 1, 0);
    klbwnd_num_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_knum, p_css, destroy_globalcss_klbwnd_num);

    KLBUI_GLOBAL_num_bind("margin", globalcss_klbui_num_margin);
    KLBUI_GLOBAL_num_bind("margin-top", globalcss_klbui_num_margin_top);
    KLBUI_GLOBAL_num_bind("margin-right", globalcss_klbui_num_margin_right);
    KLBUI_GLOBAL_num_bind("margin-bottom", globalcss_klbui_num_margin_bottom);
    KLBUI_GLOBAL_num_bind("margin-left", globalcss_klbui_num_margin_left);
    KLBUI_GLOBAL_num_bind("padding", globalcss_klbui_num_padding);
    KLBUI_GLOBAL_num_bind("padding-top", globalcss_klbui_num_padding_top);
    KLBUI_GLOBAL_num_bind("padding-right", globalcss_klbui_num_padding_right);
    KLBUI_GLOBAL_num_bind("padding-bottom", globalcss_klbui_num_padding_bottom);
    KLBUI_GLOBAL_num_bind("padding-left", globalcss_klbui_num_padding_left);

    KLBUI_GLOBAL_num_bind("color", globalcss_klbui_num_color);
    KLBUI_GLOBAL_num_bind("color:focus", globalcss_klbui_num_color_focus);
    KLBUI_GLOBAL_num_bind("color:disabled", globalcss_klbui_num_color_disable);

    KLBUI_GLOBAL_num_bind("text-align", globalcss_klbui_num_text_align);
    KLBUI_GLOBAL_num_bind("text-align:focus", globalcss_klbui_num_text_align_focus);
    KLBUI_GLOBAL_num_bind("text-align:disabled", globalcss_klbui_num_text_align_disable);

    KLBUI_GLOBAL_num_bind("font-size", globalcss_klbui_num_font_size);
    KLBUI_GLOBAL_num_bind("font-size:focus", globalcss_klbui_num_font_size_focus);
    KLBUI_GLOBAL_num_bind("font-size:disabled", globalcss_klbui_num_font_size_disable);

    KLBUI_GLOBAL_num_bind("background-color", globalcss_klbui_num_background_color);
    KLBUI_GLOBAL_num_bind("background-color:focus", globalcss_klbui_num_background_color_focus);
    KLBUI_GLOBAL_num_bind("background-color:disabled", globalcss_klbui_num_background_color_disable);

    KLBUI_GLOBAL_num_bind("background-image", globalcss_klbui_num_background_image);
    KLBUI_GLOBAL_num_bind("background-image:focus", globalcss_klbui_num_background_image_focus);
    KLBUI_GLOBAL_num_bind("background-image:disabled", globalcss_klbui_num_background_image_disable);

    KLBUI_GLOBAL_num_bind("background-image-mode", globalcss_klbui_num_background_image_mode);
    KLBUI_GLOBAL_num_bind("background-image-mode:focus", globalcss_klbui_num_background_image_mode_focus);
    KLBUI_GLOBAL_num_bind("background-image-mode:disabled", globalcss_klbui_num_background_image_mode_disable);

    KLBUI_GLOBAL_num_bind("background-image-color-key", globalcss_klbui_num_background_image_color_key);
    KLBUI_GLOBAL_num_bind("background-image-color-key:focus", globalcss_klbui_num_background_image_color_key_focus);
    KLBUI_GLOBAL_num_bind("background-image-color-key:disabled", globalcss_klbui_num_background_image_color_key_disable);

    KLBUI_GLOBAL_num_bind("border-width", globalcss_klbui_num_border_width);
    KLBUI_GLOBAL_num_bind("border-width:focus", globalcss_klbui_num_border_width_focus);
    KLBUI_GLOBAL_num_bind("border-width:disabled", globalcss_klbui_num_border_width_disable);

    KLBUI_GLOBAL_num_bind("border-color", globalcss_klbui_num_border_color);
    KLBUI_GLOBAL_num_bind("border-color:focus", globalcss_klbui_num_border_color_focus);
    KLBUI_GLOBAL_num_bind("border-color:disabled", globalcss_klbui_num_border_color_disable);

}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_num_margin(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_margin_top(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_margin_right(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_margin_bottom(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_margin_left(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_padding(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_padding_top(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_padding_right(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_padding_bottom(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_padding_left(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_text_align(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_text_align_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_text_align_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_font_size(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_font_size_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_font_size_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_mode(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_mode_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_mode_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_color_key(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_color_key_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_background_image_color_key_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_width(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_width_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_width_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_color(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_color_focus(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}

static void on_klbui_num_border_color_disable(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_num_css_t* p_css = check_css_klbui_num(p_wnd, method);

    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
    (void)p_num;
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbui_num_value(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_min(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.min), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_max(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_num->num.max), p_wnd, method, p_in, p_out);
}

static void on_klbui_num_ranges(klb_wnd_t* p_wnd, klbui_num_t* p_num, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t* p_ranges = klb_map_create();

        klb_map_set_int64(p_ranges, "min", p_num->num.min);
        klb_map_set_int64(p_ranges, "max", p_num->num.max);

        klb_map_set_idx_map(p_out, 0, p_ranges);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int t = klb_map_array_type(p_in, start);
        if (KLB_ADT_map == t)
        {
            klb_map_t* p_in_ranges = klb_map_idx_to_map(p_in, start);

            int min = 0;
            int max = 0;
            if (0 < klb_map_array_size(p_in_ranges))
            {
                min = (int)klb_map_idx_to_int64(p_in_ranges, 0);
                max = (int)klb_map_idx_to_int64(p_in_ranges, 1);
            }
            else
            {
                min = (int)klb_map_to_int64(p_in_ranges, "min");
                max = (int)klb_map_to_int64(p_in_ranges, "max");
            }

            klbwnd_num_set_ranges(p_wnd, min, max);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_num_bind(KEY_, FUNC_) { klb_map_set_ptr(p_num->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_num_init_func_map(klb_wnd_t* p_wnd, klbui_num_t* p_num, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_knum);
    if (NULL != ptr)
    {
        p_num->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBWUI_knum);
    p_num->p_func_map = ptr;

    KLBUI_num_bind("margin", on_klbui_num_margin);
    KLBUI_num_bind("margin-top", on_klbui_num_margin_top);
    KLBUI_num_bind("margin-right", on_klbui_num_margin_right);
    KLBUI_num_bind("margin-bottom", on_klbui_num_margin_bottom);
    KLBUI_num_bind("margin-left", on_klbui_num_margin_left);
    KLBUI_num_bind("padding", on_klbui_num_padding);
    KLBUI_num_bind("padding-top", on_klbui_num_padding_top);
    KLBUI_num_bind("padding-right", on_klbui_num_padding_right);
    KLBUI_num_bind("padding-bottom", on_klbui_num_padding_bottom);
    KLBUI_num_bind("padding-left", on_klbui_num_padding_left);

    KLBUI_num_bind("color", on_klbui_num_color);
    KLBUI_num_bind("color:focus", on_klbui_num_color_focus);
    KLBUI_num_bind("color:disabled", on_klbui_num_color_disable);

    KLBUI_num_bind("text-align", on_klbui_num_text_align);
    KLBUI_num_bind("text-align:focus", on_klbui_num_text_align_focus);
    KLBUI_num_bind("text-align:disabled", on_klbui_num_text_align_disable);

    KLBUI_num_bind("font-size", on_klbui_num_font_size);
    KLBUI_num_bind("font-size:focus", on_klbui_num_font_size_focus);
    KLBUI_num_bind("font-size:disabled", on_klbui_num_font_size_disable);

    KLBUI_num_bind("background-color", on_klbui_num_background_color);
    KLBUI_num_bind("background-color:focus", on_klbui_num_background_color_focus);
    KLBUI_num_bind("background-color:disabled", on_klbui_num_background_color_disable);

    KLBUI_num_bind("background-image", on_klbui_num_background_image);
    KLBUI_num_bind("background-image:focus", on_klbui_num_background_image_focus);
    KLBUI_num_bind("background-image:disabled", on_klbui_num_background_image_disable);

    KLBUI_num_bind("background-image-mode", on_klbui_num_background_image_mode);
    KLBUI_num_bind("background-image-mode:focus", on_klbui_num_background_image_mode_focus);
    KLBUI_num_bind("background-image-mode:disabled", on_klbui_num_background_image_mode_disable);

    KLBUI_num_bind("background-image-color-key", on_klbui_num_background_image_color_key);
    KLBUI_num_bind("background-image-color-key:focus", on_klbui_num_background_image_color_key_focus);
    KLBUI_num_bind("background-image-color-key:disabled", on_klbui_num_background_image_color_key_disable);

    KLBUI_num_bind("border-width", on_klbui_num_border_width);
    KLBUI_num_bind("border-width:focus", on_klbui_num_border_width_focus);
    KLBUI_num_bind("border-width:disabled", on_klbui_num_border_width_disable);

    KLBUI_num_bind("border-color", on_klbui_num_border_color);
    KLBUI_num_bind("border-color:focus", on_klbui_num_border_color_focus);
    KLBUI_num_bind("border-color:disabled", on_klbui_num_border_color_disable);

    KLBUI_num_bind("value", on_klbui_num_value);
    KLBUI_num_bind("min", on_klbui_num_min);
    KLBUI_num_bind("max", on_klbui_num_max);
    KLBUI_num_bind("ranges", on_klbui_num_ranges);

    (void)p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_num_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_num_t));
    klbui_num_t* p_num = (klbui_num_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_num_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_num_destroy;
    p_wnd->vtable.on_set = klbui_num_on_set;
    p_wnd->vtable.on_get = klbui_num_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_num_init_func_map(p_wnd, p_num, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_num->p_globalcss = (klbwnd_num_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_knum);
    klbwnd_num_set_css(p_wnd, p_num->p_globalcss);

    return p_wnd;
}

int klbui_register_knum(klb_gui_t* p_gui)
{
    klbui_num_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_knum, klbui_num_create);
}

// end
