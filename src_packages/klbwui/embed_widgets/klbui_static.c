// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_static.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_static_t
/// @brief  静态文本框: 类似 wnd 的 Lua 注册名 kstatic
typedef struct klbui_static_t_
{
    klbwnd_static_t         sta;            ///< 必须首位, 保持内存一致

    klbwnd_static_css_t*    p_globalcss;    ///< 全局公共的 CSS
    klbwnd_static_css_t*    p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_static_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_static_cb)(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_static_destroy(klb_wnd_t* p_wnd)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    klbwnd_static_deinit(p_wnd);

    if (NULL != p_static->p_mycss)
    {
        klbwnd_static_css_deinit(p_static->p_mycss);
        KLB_FREE(p_static->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_static_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_static_cb func = (klbui_static_cb)klb_map_to_ptr(p_static->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_static, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_static_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_static_cb func = (klbui_static_cb)klb_map_to_ptr(p_static->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_static, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_static(klb_wnd_t* p_wnd)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    if (NULL == p_static->p_mycss)
    {
        p_static->p_mycss = KLB_MALLOCZ(klbwnd_static_css_t, 1, 0);

        klbwnd_static_css_copy(p_static->p_mycss, p_static->p_globalcss);
        klbwnd_static_set_css(p_wnd, p_static->p_mycss);
    }
}

static klbwnd_static_css_t* get_css_klbui_static(klb_wnd_t* p_wnd)
{
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    return (NULL != p_static->p_mycss) ? p_static->p_mycss : p_static->p_globalcss;
}

static klbwnd_static_css_t* check_css_klbui_static(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_static(p_wnd);
    }

    return get_css_klbui_static(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_static_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_static_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_static(void* ptr)
{
    klbwnd_static_css_t* p_css = (klbwnd_static_css_t*)ptr;

    klbwnd_static_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_static_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_static_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kstatic);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kstatic);

    klbwnd_static_css_t* p_css = KLB_MALLOCZ(klbwnd_static_css_t, 1, 0);
    klbwnd_static_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kstatic, p_css, destroy_globalcss_klbwnd_static);

    KLBUI_GLOBAL_static_bind("margin", globalcss_klbui_static_margin);
    KLBUI_GLOBAL_static_bind("margin-top", globalcss_klbui_static_margin_top);
    KLBUI_GLOBAL_static_bind("margin-right", globalcss_klbui_static_margin_right);
    KLBUI_GLOBAL_static_bind("margin-bottom", globalcss_klbui_static_margin_bottom);
    KLBUI_GLOBAL_static_bind("margin-left", globalcss_klbui_static_margin_left);

    KLBUI_GLOBAL_static_bind("padding", globalcss_klbui_static_padding);
    KLBUI_GLOBAL_static_bind("padding-top", globalcss_klbui_static_padding_top);
    KLBUI_GLOBAL_static_bind("padding-right", globalcss_klbui_static_padding_right);
    KLBUI_GLOBAL_static_bind("padding-bottom", globalcss_klbui_static_padding_bottom);
    KLBUI_GLOBAL_static_bind("padding-left", globalcss_klbui_static_padding_left);

    KLBUI_GLOBAL_static_bind("color", globalcss_klbui_static_text_color);
    KLBUI_GLOBAL_static_bind("text-align", globalcss_klbui_static_text_align);
    KLBUI_GLOBAL_static_bind("font-size", globalcss_klbui_static_font_size);
    KLBUI_GLOBAL_static_bind("background-color", globalcss_klbui_static_background_color);
    KLBUI_GLOBAL_static_bind("background-image", globalcss_klbui_static_background_image);
    KLBUI_GLOBAL_static_bind("background-image-mode", globalcss_klbui_static_background_image_mode);
    KLBUI_GLOBAL_static_bind("background-image-color-key", globalcss_klbui_static_background_image_color_key);
    KLBUI_GLOBAL_static_bind("border-width", globalcss_klbui_static_border_width);
    KLBUI_GLOBAL_static_bind("border-color", globalcss_klbui_static_border_color);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_static_margin(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_top(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_right(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_bottom(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_margin_left(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_top(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_right(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_bottom(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_padding_left(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_text_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_text_align(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_font_size(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_image(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_image_mode(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_background_image_color_key(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_width(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_border_color(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_static_css_t* p_css = check_css_klbui_static(p_wnd, method);

    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbui_static_tip(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
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


static void on_klbui_static_dynamic_tip(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    bool use = klb_wnd_is_dyntip(p_wnd);

    klbuicssex_attribute_bool(&use, p_wnd, method, p_in, p_out);

    if (KLBUI_CSSEX_set == method)
    {
        klb_wnd_dyntip(p_wnd, use);
    }
}

static void on_klbui_static_index(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_static->sta.index), p_wnd, method, p_in, p_out);
}

static void on_klbui_static_title(klb_wnd_t* p_wnd, klbui_static_t* p_static, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_static->sta.title), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_static_bind(KEY_, FUNC_) { klb_map_set_ptr(p_static->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_static_init_func_map(klb_wnd_t* p_wnd, klbui_static_t* p_static, klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kstatic);

    if (NULL != ptr)
    {
        p_static->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kstatic);
    p_static->p_func_map = ptr;

    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    KLBUI_static_bind("margin", on_klbui_static_margin);
    KLBUI_static_bind("margin-top", on_klbui_static_margin_top);
    KLBUI_static_bind("margin-right", on_klbui_static_margin_right);
    KLBUI_static_bind("margin-bottom", on_klbui_static_margin_bottom);
    KLBUI_static_bind("margin-left", on_klbui_static_margin_left);

    KLBUI_static_bind("padding", on_klbui_static_padding);
    KLBUI_static_bind("padding-top", on_klbui_static_padding_top);
    KLBUI_static_bind("padding-right", on_klbui_static_padding_right);
    KLBUI_static_bind("padding-bottom", on_klbui_static_padding_bottom);
    KLBUI_static_bind("padding-left", on_klbui_static_padding_left);

    KLBUI_static_bind("color", on_klbui_static_text_color);
    KLBUI_static_bind("text-align", on_klbui_static_text_align);
    KLBUI_static_bind("font-size", on_klbui_static_font_size);
    KLBUI_static_bind("background-color", on_klbui_static_background_color);
    KLBUI_static_bind("background-image", on_klbui_static_background_image);
    KLBUI_static_bind("background-image-mode", on_klbui_static_background_image_mode);
    KLBUI_static_bind("background-image-color-key", on_klbui_static_background_image_color_key);
    KLBUI_static_bind("border-width", on_klbui_static_border_width);
    KLBUI_static_bind("border-color", on_klbui_static_border_color);

    KLBUI_static_bind("tip", on_klbui_static_tip);
    KLBUI_static_bind("dynamic-tip", on_klbui_static_dynamic_tip);

    KLBUI_static_bind("index", on_klbui_static_index);
    KLBUI_static_bind("title", on_klbui_static_title);
    KLBUI_static_bind("value", on_klbui_static_title);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_static_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_static_t));
    klbui_static_t* p_static = (klbui_static_t*)p_wnd->ctrl;

    // step2. 初始化基础部分    
    klbwnd_static_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_static_destroy;
    p_wnd->vtable.on_set = klbui_static_on_set;
    p_wnd->vtable.on_get = klbui_static_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_static_init_func_map(p_wnd, p_static, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_static->p_globalcss = (klbwnd_static_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kstatic);
    klbwnd_static_set_css(p_wnd, p_static->p_globalcss);

    return p_wnd;
}

int klbui_register_kstatic(klb_gui_t* p_gui)
{
    klbui_static_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kstatic, klbui_static_create);
}

// end
