// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_hscrollbar.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_hscrollbar_t
/// @brief  水平滚动条: Lua 注册名 khscrollbar
typedef struct klbui_hscrollbar_t_
{
    klbwnd_hscrollbar_t       hsc;           ///< 必须首位, 保持内存一致

    klbwnd_hscrollbar_css_t*       p_globalcss;    ///< 全局公共的 CSS
    klbwnd_hscrollbar_css_t*       p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_hscrollbar_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_hscrollbar_cb)(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_hscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;

    klbwnd_hscrollbar_deinit(p_wnd);

    if (NULL != p_hsc->p_mycss)
    {
        klbwnd_hscrollbar_css_deinit(p_hsc->p_mycss);
        KLB_FREE(p_hsc->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_hscrollbar_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_hscrollbar_cb func = (klbui_hscrollbar_cb)klb_map_to_ptr(p_hsc->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_hsc, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_hscrollbar_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_hscrollbar_cb func = (klbui_hscrollbar_cb)klb_map_to_ptr(p_hsc->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_hsc, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_hscrollbar(klb_wnd_t* p_wnd)
{
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;

    if (NULL == p_hsc->p_mycss)
    {
        p_hsc->p_mycss = KLB_MALLOCZ(klbwnd_hscrollbar_css_t, 1, 0);

        klbwnd_hscrollbar_css_copy(p_hsc->p_mycss, p_hsc->p_globalcss);
        klbwnd_hscrollbar_set_css(p_wnd, p_hsc->p_mycss);
    }
}

static klbwnd_hscrollbar_css_t* get_css_klbui_hscrollbar(klb_wnd_t* p_wnd)
{
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;

    return (NULL != p_hsc->p_mycss) ? p_hsc->p_mycss : p_hsc->p_globalcss;
}

static klbwnd_hscrollbar_css_t* check_css_klbui_hscrollbar(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_hscrollbar(p_wnd);
    }

    return get_css_klbui_hscrollbar(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_hscrollbar_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_mode_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_mode_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_mode(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_color_key_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_background_image_color_key_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbwuicss_background_image_color_key(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_hscrollbar_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;
    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_hscrollbar(void* ptr)
{
    klbwnd_hscrollbar_css_t* p_css = (klbwnd_hscrollbar_css_t*)ptr;

    klbwnd_hscrollbar_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_hscrollbar_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_hscrollbar_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_khscrollbar);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_khscrollbar);

    klbwnd_hscrollbar_css_t* p_css = KLB_MALLOCZ(klbwnd_hscrollbar_css_t, 1, 0);
    klbwnd_hscrollbar_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_khscrollbar, p_css, destroy_globalcss_klbwnd_hscrollbar);

    KLBUI_GLOBAL_hscrollbar_bind("margin", globalcss_klbui_hscrollbar_margin);
    KLBUI_GLOBAL_hscrollbar_bind("margin-top", globalcss_klbui_hscrollbar_margin_top);
    KLBUI_GLOBAL_hscrollbar_bind("margin-right", globalcss_klbui_hscrollbar_margin_right);
    KLBUI_GLOBAL_hscrollbar_bind("margin-bottom", globalcss_klbui_hscrollbar_margin_bottom);
    KLBUI_GLOBAL_hscrollbar_bind("margin-left", globalcss_klbui_hscrollbar_margin_left);
    KLBUI_GLOBAL_hscrollbar_bind("padding", globalcss_klbui_hscrollbar_padding);
    KLBUI_GLOBAL_hscrollbar_bind("padding-top", globalcss_klbui_hscrollbar_padding_top);
    KLBUI_GLOBAL_hscrollbar_bind("padding-right", globalcss_klbui_hscrollbar_padding_right);
    KLBUI_GLOBAL_hscrollbar_bind("padding-bottom", globalcss_klbui_hscrollbar_padding_bottom);
    KLBUI_GLOBAL_hscrollbar_bind("padding-left", globalcss_klbui_hscrollbar_padding_left);

    KLBUI_GLOBAL_hscrollbar_bind("color", globalcss_klbui_hscrollbar_color);
    KLBUI_GLOBAL_hscrollbar_bind("color:focus", globalcss_klbui_hscrollbar_color_focus);
    KLBUI_GLOBAL_hscrollbar_bind("color:disabled", globalcss_klbui_hscrollbar_color_disable);

    KLBUI_GLOBAL_hscrollbar_bind("text-align", globalcss_klbui_hscrollbar_text_align);
    KLBUI_GLOBAL_hscrollbar_bind("text-align:focus", globalcss_klbui_hscrollbar_text_align_focus);
    KLBUI_GLOBAL_hscrollbar_bind("text-align:disabled", globalcss_klbui_hscrollbar_text_align_disable);

    KLBUI_GLOBAL_hscrollbar_bind("font-size", globalcss_klbui_hscrollbar_font_size);
    KLBUI_GLOBAL_hscrollbar_bind("font-size:focus", globalcss_klbui_hscrollbar_font_size_focus);
    KLBUI_GLOBAL_hscrollbar_bind("font-size:disabled", globalcss_klbui_hscrollbar_font_size_disable);

    KLBUI_GLOBAL_hscrollbar_bind("background-color", globalcss_klbui_hscrollbar_background_color);
    KLBUI_GLOBAL_hscrollbar_bind("background-color:focus", globalcss_klbui_hscrollbar_background_color_focus);
    KLBUI_GLOBAL_hscrollbar_bind("background-color:disabled", globalcss_klbui_hscrollbar_background_color_disable);

    KLBUI_GLOBAL_hscrollbar_bind("background-image", globalcss_klbui_hscrollbar_background_image);
    KLBUI_GLOBAL_hscrollbar_bind("background-image:focus", globalcss_klbui_hscrollbar_background_image_focus);
    KLBUI_GLOBAL_hscrollbar_bind("background-image:disabled", globalcss_klbui_hscrollbar_background_image_disable);

    KLBUI_GLOBAL_hscrollbar_bind("background-image-mode", globalcss_klbui_hscrollbar_background_image_mode);
    KLBUI_GLOBAL_hscrollbar_bind("background-image-mode:focus", globalcss_klbui_hscrollbar_background_image_mode_focus);
    KLBUI_GLOBAL_hscrollbar_bind("background-image-mode:disabled", globalcss_klbui_hscrollbar_background_image_mode_disable);

    KLBUI_GLOBAL_hscrollbar_bind("background-image-color-key", globalcss_klbui_hscrollbar_background_image_color_key);
    KLBUI_GLOBAL_hscrollbar_bind("background-image-color-key:focus", globalcss_klbui_hscrollbar_background_image_color_key_focus);
    KLBUI_GLOBAL_hscrollbar_bind("background-image-color-key:disabled", globalcss_klbui_hscrollbar_background_image_color_key_disable);

    KLBUI_GLOBAL_hscrollbar_bind("border-width", globalcss_klbui_hscrollbar_border_width);
    KLBUI_GLOBAL_hscrollbar_bind("border-width:focus", globalcss_klbui_hscrollbar_border_width_focus);
    KLBUI_GLOBAL_hscrollbar_bind("border-width:disabled", globalcss_klbui_hscrollbar_border_width_disable);

    KLBUI_GLOBAL_hscrollbar_bind("border-color", globalcss_klbui_hscrollbar_border_color);
    KLBUI_GLOBAL_hscrollbar_bind("border-color:focus", globalcss_klbui_hscrollbar_border_color_focus);
    KLBUI_GLOBAL_hscrollbar_bind("border-color:disabled", globalcss_klbui_hscrollbar_border_color_disable);

}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_hscrollbar_margin(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_margin_top(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_margin_right(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_margin_bottom(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_margin_left(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_padding(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_padding_top(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_padding_right(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_padding_bottom(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_padding_left(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_color(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_color_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_color_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_text_align(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_text_align_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_text_align_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_font_size(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_font_size_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_font_size_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_color(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_color_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_color_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_mode(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_mode_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_mode_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_color_key(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->normal.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_color_key_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->focus.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_background_image_color_key_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->disable.background), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_width(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_width_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_width_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_color(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_color_focus(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}

static void on_klbui_hscrollbar_border_color_disable(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_hscrollbar_css_t* p_css = check_css_klbui_hscrollbar(p_wnd, method);

    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
    (void)p_hsc;
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbui_hscrollbar_value(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klbwnd_hscrollbar_get_value(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);

        if (KLB_ADT_int64 == type)
        {
            klbwnd_hscrollbar_set_value(p_wnd, (int)klb_map_idx_to_int64(p_in, start));
        }
        else if (KLB_ADT_uint64 == type)
        {
            klbwnd_hscrollbar_set_value(p_wnd, (int)klb_map_idx_to_uint64(p_in, start));
        }
    }

    (void)p_hsc;
}

static void on_klbui_hscrollbar_ranges(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t* p_ranges = klb_map_create();

        klb_map_set_int64(p_ranges, "min", p_hsc->hsc.min);
        klb_map_set_int64(p_ranges, "max", p_hsc->hsc.max);
        klb_map_set_int64(p_ranges, "step", p_hsc->hsc.step);

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
            int step = 0;

            if (0 < klb_map_array_size(p_in_ranges))
            {
                min = (int)klb_map_idx_to_int64(p_in_ranges, 0);
                max = (int)klb_map_idx_to_int64(p_in_ranges, 1);
                step = (int)klb_map_idx_to_int64(p_in_ranges, 2);
            }
            else
            {
                min = (int)klb_map_to_int64(p_in_ranges, "min");
                max = (int)klb_map_to_int64(p_in_ranges, "max");
                step = (int)klb_map_to_int64(p_in_ranges, "step");
            }

            klbwnd_hscrollbar_set_ranges(p_wnd, min, max, step);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_hscrollbar_bind(KEY_, FUNC_) { klb_map_set_ptr(p_hsc->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_hscrollbar_init_func_map(klb_wnd_t* p_wnd, klbui_hscrollbar_t* p_hsc, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_khscrollbar);
    if (NULL != ptr)
    {
        p_hsc->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBWUI_khscrollbar);
    p_hsc->p_func_map = ptr;

    KLBUI_hscrollbar_bind("margin", on_klbui_hscrollbar_margin);
    KLBUI_hscrollbar_bind("margin-top", on_klbui_hscrollbar_margin_top);
    KLBUI_hscrollbar_bind("margin-right", on_klbui_hscrollbar_margin_right);
    KLBUI_hscrollbar_bind("margin-bottom", on_klbui_hscrollbar_margin_bottom);
    KLBUI_hscrollbar_bind("margin-left", on_klbui_hscrollbar_margin_left);
    KLBUI_hscrollbar_bind("padding", on_klbui_hscrollbar_padding);
    KLBUI_hscrollbar_bind("padding-top", on_klbui_hscrollbar_padding_top);
    KLBUI_hscrollbar_bind("padding-right", on_klbui_hscrollbar_padding_right);
    KLBUI_hscrollbar_bind("padding-bottom", on_klbui_hscrollbar_padding_bottom);
    KLBUI_hscrollbar_bind("padding-left", on_klbui_hscrollbar_padding_left);

    KLBUI_hscrollbar_bind("color", on_klbui_hscrollbar_color);
    KLBUI_hscrollbar_bind("color:focus", on_klbui_hscrollbar_color_focus);
    KLBUI_hscrollbar_bind("color:disabled", on_klbui_hscrollbar_color_disable);

    KLBUI_hscrollbar_bind("text-align", on_klbui_hscrollbar_text_align);
    KLBUI_hscrollbar_bind("text-align:focus", on_klbui_hscrollbar_text_align_focus);
    KLBUI_hscrollbar_bind("text-align:disabled", on_klbui_hscrollbar_text_align_disable);

    KLBUI_hscrollbar_bind("font-size", on_klbui_hscrollbar_font_size);
    KLBUI_hscrollbar_bind("font-size:focus", on_klbui_hscrollbar_font_size_focus);
    KLBUI_hscrollbar_bind("font-size:disabled", on_klbui_hscrollbar_font_size_disable);

    KLBUI_hscrollbar_bind("background-color", on_klbui_hscrollbar_background_color);
    KLBUI_hscrollbar_bind("background-color:focus", on_klbui_hscrollbar_background_color_focus);
    KLBUI_hscrollbar_bind("background-color:disabled", on_klbui_hscrollbar_background_color_disable);

    KLBUI_hscrollbar_bind("background-image", on_klbui_hscrollbar_background_image);
    KLBUI_hscrollbar_bind("background-image:focus", on_klbui_hscrollbar_background_image_focus);
    KLBUI_hscrollbar_bind("background-image:disabled", on_klbui_hscrollbar_background_image_disable);

    KLBUI_hscrollbar_bind("background-image-mode", on_klbui_hscrollbar_background_image_mode);
    KLBUI_hscrollbar_bind("background-image-mode:focus", on_klbui_hscrollbar_background_image_mode_focus);
    KLBUI_hscrollbar_bind("background-image-mode:disabled", on_klbui_hscrollbar_background_image_mode_disable);

    KLBUI_hscrollbar_bind("background-image-color-key", on_klbui_hscrollbar_background_image_color_key);
    KLBUI_hscrollbar_bind("background-image-color-key:focus", on_klbui_hscrollbar_background_image_color_key_focus);
    KLBUI_hscrollbar_bind("background-image-color-key:disabled", on_klbui_hscrollbar_background_image_color_key_disable);

    KLBUI_hscrollbar_bind("border-width", on_klbui_hscrollbar_border_width);
    KLBUI_hscrollbar_bind("border-width:focus", on_klbui_hscrollbar_border_width_focus);
    KLBUI_hscrollbar_bind("border-width:disabled", on_klbui_hscrollbar_border_width_disable);

    KLBUI_hscrollbar_bind("border-color", on_klbui_hscrollbar_border_color);
    KLBUI_hscrollbar_bind("border-color:focus", on_klbui_hscrollbar_border_color_focus);
    KLBUI_hscrollbar_bind("border-color:disabled", on_klbui_hscrollbar_border_color_disable);

    KLBUI_hscrollbar_bind("value", on_klbui_hscrollbar_value);
    KLBUI_hscrollbar_bind("ranges", on_klbui_hscrollbar_ranges);

    (void)p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_hscrollbar_t));
    klbui_hscrollbar_t* p_hsc = (klbui_hscrollbar_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_hscrollbar_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_hscrollbar_destroy;
    p_wnd->vtable.on_set = klbui_hscrollbar_on_set;
    p_wnd->vtable.on_get = klbui_hscrollbar_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_hscrollbar_init_func_map(p_wnd, p_hsc, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_hsc->p_globalcss = (klbwnd_hscrollbar_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_khscrollbar);
    klbwnd_hscrollbar_set_css(p_wnd, p_hsc->p_globalcss);

    return p_wnd;
}

int klbui_register_khscrollbar(klb_gui_t* p_gui)
{
    klbui_hscrollbar_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_khscrollbar, klbui_hscrollbar_create);
}

// end
