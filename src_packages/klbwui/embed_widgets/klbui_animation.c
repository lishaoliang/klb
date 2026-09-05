// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_animation.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_animation_t
/// @brief  动画 (Lua type: kanimation)
typedef struct klbui_animation_t_
{
    klbwnd_animation_t          animation;      ///< 必须首位, 保持内存一致

    klbwnd_animation_css_t*     p_globalcss;    ///< 全局公共的 CSS
    klbwnd_animation_css_t*     p_mycss;        ///< 私有的 CSS

    klb_map_t*                  p_func_map;     ///< CSS 属性函数表
} klbui_animation_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_animation_cb)(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_animation_destroy(klb_wnd_t* p_wnd)
{
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;

    klbwnd_animation_deinit(p_wnd);

    if (NULL != p_ani->p_mycss)
    {
        klbwnd_animation_css_deinit(p_ani->p_mycss);
        KLB_FREE(p_ani->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_animation_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_animation_cb func = (klbui_animation_cb)klb_map_to_ptr(p_ani->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_ani, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_animation_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_animation_cb func = (klbui_animation_cb)klb_map_to_ptr(p_ani->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_ani, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_animation(klb_wnd_t* p_wnd)
{
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;

    if (NULL == p_ani->p_mycss)
    {
        p_ani->p_mycss = KLB_MALLOCZ(klbwnd_animation_css_t, 1, 0);

        klbwnd_animation_css_copy(p_ani->p_mycss, p_ani->p_globalcss);
        klbwnd_animation_set_css(p_wnd, p_ani->p_mycss);
    }
}

static klbwnd_animation_css_t* get_css_klbui_animation(klb_wnd_t* p_wnd)
{
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;

    return (NULL != p_ani->p_mycss) ? p_ani->p_mycss : p_ani->p_globalcss;
}

static klbwnd_animation_css_t* check_css_klbui_animation(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_animation(p_wnd);
    }

    return get_css_klbui_animation(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_animation_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = (klbwnd_animation_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_animation_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = (klbwnd_animation_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_animation_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = (klbwnd_animation_css_t*)ptr;

    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_animation_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = (klbwnd_animation_css_t*)ptr;

    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_animation(void* ptr)
{
    klbwnd_animation_css_t* p_css = (klbwnd_animation_css_t*)ptr;

    klbwnd_animation_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_ani_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_animation_init_globalcss(klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kanimation);

    if (NULL != ptr)
    {
        return;
    }

    // step2. 新添加 解析map, 及公共 CSS 对象
    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kanimation);

    klbwnd_animation_css_t* p_css = KLB_MALLOCZ(klbwnd_animation_css_t, 1, 0);
    klbwnd_animation_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kanimation, p_css, destroy_globalcss_klbwnd_animation);


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_GLOBAL_ani_bind("margin", globalcss_klbui_animation_margin);

    // 内边距 padding
    KLBUI_GLOBAL_ani_bind("padding", globalcss_klbui_animation_padding);

    // 背景色 background-color
    KLBUI_GLOBAL_ani_bind("background-color", globalcss_klbui_animation_background_color);
    // 背景图片 background-image
    KLBUI_GLOBAL_ani_bind("background-image", globalcss_klbui_animation_background_image);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_animation_margin(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = check_css_klbui_animation(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_animation_padding(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = check_css_klbui_animation(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_animation_background_color(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = check_css_klbui_animation(p_wnd, method);

    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_animation_background_image(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_animation_css_t* p_css = check_css_klbui_animation(p_wnd, method);

    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 命令键

static void on_klbui_animation_ticker(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klbwnd_animation_get_enable(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool is_enable = klb_map_idx_to_bool(p_in, start);

        klbwnd_animation_set_enable(p_wnd, is_enable);
    }
}

static void on_klbui_animation_ticker_interval(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klbwnd_animation_get_interval(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int64_t interval = klb_map_idx_to_int64(p_in, start);

        klbwnd_animation_set_interval(p_wnd, interval);
    }
}

// 私有自定义

static void on_klbui_animation_title(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_ani->animation.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_animation_value(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_ani->animation.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_animation_index(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_ani->animation.index), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_ani_bind(KEY_, FUNC_) { klb_map_set_ptr(p_ani->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_animation_init_func_map(klb_wnd_t* p_wnd, klbui_animation_t* p_ani, klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kanimation);

    if (NULL != ptr)
    {
        p_ani->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kanimation);
    p_ani->p_func_map = ptr;


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_ani_bind("margin", on_klbui_animation_margin);

    // 内边距 padding
    KLBUI_ani_bind("padding", on_klbui_animation_padding);

    // 背景色 background-color
    KLBUI_ani_bind("background-color", on_klbui_animation_background_color);
    // 背景图片 background-image
    KLBUI_ani_bind("background-image", on_klbui_animation_background_image);

    // 命令键

    KLBUI_ani_bind("ticker", on_klbui_animation_ticker);
    KLBUI_ani_bind("ticker-interval", on_klbui_animation_ticker_interval);

    // 私有自定义

    KLBUI_ani_bind("title", on_klbui_animation_title);
    KLBUI_ani_bind("value", on_klbui_animation_value);
    KLBUI_ani_bind("index", on_klbui_animation_index);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_animation_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_animation_t));
    klbui_animation_t* p_ani = (klbui_animation_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_animation_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_animation_destroy;
    p_wnd->vtable.on_set = klbui_animation_on_set;
    p_wnd->vtable.on_get = klbui_animation_on_get;

    // step4. 初始化 CSS 支持的方法
    klbui_animation_init_func_map(p_wnd, p_ani, p_gui);

    // step5. 默认使用公用全局 CSS 属性
    p_ani->p_globalcss = (klbwnd_animation_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kanimation);
    klbwnd_animation_set_css(p_wnd, p_ani->p_globalcss);

    return p_wnd;
}

int klbui_register_kanimation(klb_gui_t* p_gui)
{
    klbui_animation_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kanimation, klbui_animation_create);
}

// end
