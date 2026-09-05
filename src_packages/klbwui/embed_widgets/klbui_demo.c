// Doc Encode : UTF-8 BOM, Unix(LF)
/// @brief  kdemo 学习用最小控件 (Lua type: kdemo; 固定显示 "kdemo")
///         ② widgets 层样板: 注册 + 基础 CSS set/get
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_demo.h"
#include "klbmem/klb_mem.h"


typedef struct klbui_demo_t_
{
    klbwnd_demo_t           demo;           ///< 必须首位

    klbwnd_demo_css_t*      p_globalcss;
    klbwnd_demo_css_t*      p_mycss;

    klb_map_t*              p_func_map;
} klbui_demo_t;


typedef void(*klbui_demo_cb)(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_demo_destroy(klb_wnd_t* p_wnd)
{
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;

    klbwnd_demo_deinit(p_wnd);

    if (NULL != p_demo->p_mycss)
    {
        klbwnd_demo_css_deinit(p_demo->p_mycss);
        KLB_FREE(p_demo->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_demo_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_demo_cb func = (klbui_demo_cb)klb_map_to_ptr(p_demo->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_demo, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_demo_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_demo_cb func = (klbui_demo_cb)klb_map_to_ptr(p_demo->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_demo, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_demo(klb_wnd_t* p_wnd)
{
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;

    if (NULL == p_demo->p_mycss)
    {
        p_demo->p_mycss = KLB_MALLOCZ(klbwnd_demo_css_t, 1, 0);

        klbwnd_demo_css_copy(p_demo->p_mycss, p_demo->p_globalcss);
        klbwnd_demo_set_css(p_wnd, p_demo->p_mycss);
    }
}

static klbwnd_demo_css_t* get_css_klbui_demo(klb_wnd_t* p_wnd)
{
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;

    return (NULL != p_demo->p_mycss) ? p_demo->p_mycss : p_demo->p_globalcss;
}

static klbwnd_demo_css_t* check_css_klbui_demo(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_demo(p_wnd);
    }

    return get_css_klbui_demo(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_demo_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_background_image_mode(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbwuicss_background_image_mode(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_background_image_color_key(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbwuicss_background_image_color_key(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_demo_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_demo(void* ptr)
{
    klbwnd_demo_css_t* p_css = (klbwnd_demo_css_t*)ptr;

    klbwnd_demo_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_demo_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_demo_init_globalcss(klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kdemo);

    if (NULL != ptr)
    {
        return;
    }

    // step2. 新添加 解析map, 及公共 CSS 对象
    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kdemo);

    klbwnd_demo_css_t* p_css = KLB_MALLOCZ(klbwnd_demo_css_t, 1, 0);
    klbwnd_demo_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kdemo, p_css, destroy_globalcss_klbwnd_demo);


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_GLOBAL_demo_bind("margin", globalcss_klbui_demo_margin);
    KLBUI_GLOBAL_demo_bind("margin-top", globalcss_klbui_demo_margin_top);
    KLBUI_GLOBAL_demo_bind("margin-right", globalcss_klbui_demo_margin_right);
    KLBUI_GLOBAL_demo_bind("margin-bottom", globalcss_klbui_demo_margin_bottom);
    KLBUI_GLOBAL_demo_bind("margin-left", globalcss_klbui_demo_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_demo_bind("padding", globalcss_klbui_demo_padding);
    KLBUI_GLOBAL_demo_bind("padding-top", globalcss_klbui_demo_padding_top);
    KLBUI_GLOBAL_demo_bind("padding-right", globalcss_klbui_demo_padding_right);
    KLBUI_GLOBAL_demo_bind("padding-bottom", globalcss_klbui_demo_padding_bottom);
    KLBUI_GLOBAL_demo_bind("padding-left", globalcss_klbui_demo_padding_left);

    // 背景色 background-color
    KLBUI_GLOBAL_demo_bind("background-color", globalcss_klbui_demo_background_color);

    // 背景图片 background-image
    KLBUI_GLOBAL_demo_bind("background-image", globalcss_klbui_demo_background_image);
    KLBUI_GLOBAL_demo_bind("background-image-mode", globalcss_klbui_demo_background_image_mode);
    KLBUI_GLOBAL_demo_bind("background-image-color-key", globalcss_klbui_demo_background_image_color_key);

    // 边框宽度 border
    KLBUI_GLOBAL_demo_bind("border-width", globalcss_klbui_demo_border_width);
    KLBUI_GLOBAL_demo_bind("border-color", globalcss_klbui_demo_border_color);

    // 文本颜色 color
    KLBUI_GLOBAL_demo_bind("color", globalcss_klbui_demo_color);

    // 字体大小 font-size
    KLBUI_GLOBAL_demo_bind("font-size", globalcss_klbui_demo_font_size);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 实例 CSS 属性

static void on_klbui_demo_margin(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_margin_top(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_margin_right(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_margin_bottom(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_margin_left(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_padding(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_padding_top(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_padding_right(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_padding_bottom(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_padding_left(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_background_color(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_background_image(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_background_image_mode(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbwuicss_background_image_mode(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_background_image_color_key(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbwuicss_background_image_color_key(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_border_width(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_border_color(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_color(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_demo_font_size(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_demo_css_t* p_css = check_css_klbui_demo(p_wnd, method);

    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_demo_bind(KEY_, FUNC_) { klb_map_set_ptr(p_demo->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_demo_init_func_map(klb_wnd_t* p_wnd, klbui_demo_t* p_demo, klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kdemo);

    if (NULL != ptr)
    {
        p_demo->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kdemo);
    p_demo->p_func_map = ptr;

    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_demo_bind("margin", on_klbui_demo_margin);
    KLBUI_demo_bind("margin-top", on_klbui_demo_margin_top);
    KLBUI_demo_bind("margin-right", on_klbui_demo_margin_right);
    KLBUI_demo_bind("margin-bottom", on_klbui_demo_margin_bottom);
    KLBUI_demo_bind("margin-left", on_klbui_demo_margin_left);

    // 内边距 padding
    KLBUI_demo_bind("padding", on_klbui_demo_padding);
    KLBUI_demo_bind("padding-top", on_klbui_demo_padding_top);
    KLBUI_demo_bind("padding-right", on_klbui_demo_padding_right);
    KLBUI_demo_bind("padding-bottom", on_klbui_demo_padding_bottom);
    KLBUI_demo_bind("padding-left", on_klbui_demo_padding_left);

    // 背景色 background-color
    KLBUI_demo_bind("background-color", on_klbui_demo_background_color);

    // 背景图片 background-image
    KLBUI_demo_bind("background-image", on_klbui_demo_background_image);
    KLBUI_demo_bind("background-image-mode", on_klbui_demo_background_image_mode);
    KLBUI_demo_bind("background-image-color-key", on_klbui_demo_background_image_color_key);

    // 边框宽度 border
    KLBUI_demo_bind("border-width", on_klbui_demo_border_width);
    KLBUI_demo_bind("border-color", on_klbui_demo_border_color);

    // 文本颜色 color
    KLBUI_demo_bind("color", on_klbui_demo_color);

    // 字体大小 font-size
    KLBUI_demo_bind("font-size", on_klbui_demo_font_size);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_demo_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_demo_t));
    klbui_demo_t* p_demo = (klbui_demo_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_demo_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_demo_destroy;
    p_wnd->vtable.on_set = klbui_demo_on_set;
    p_wnd->vtable.on_get = klbui_demo_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_demo_init_func_map(p_wnd, p_demo, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_demo->p_globalcss = (klbwnd_demo_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kdemo);
    klbwnd_demo_set_css(p_wnd, p_demo->p_globalcss);

    return p_wnd;
}

int klbui_register_kdemo(klb_gui_t* p_gui)
{
    klbui_demo_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kdemo, klbui_demo_create);
}

// end
