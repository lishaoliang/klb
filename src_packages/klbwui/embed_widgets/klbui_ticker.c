// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_ticker.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_ticker_t
/// @brief  定时器: Lua 注册名 kticker
typedef struct klbui_ticker_t_
{
    klbwnd_ticker_t         ticker;         ///< 必须首位, 保持内存一致

    klbwnd_ticker_css_t*    p_globalcss;    ///< 全局公共的 CSS
    klbwnd_ticker_css_t*    p_mycss;        ///< 私有的 CSS

    klb_map_t*              p_func_map;     ///< CSS 属性函数表
} klbui_ticker_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_ticker_cb)(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_ticker_destroy(klb_wnd_t* p_wnd)
{
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;

    klbwnd_ticker_deinit(p_wnd);

    if (NULL != p_ticker->p_mycss)
    {
        klbwnd_ticker_css_deinit(p_ticker->p_mycss);
        KLB_FREE(p_ticker->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_ticker_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_ticker_cb func = (klbui_ticker_cb)klb_map_to_ptr(p_ticker->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_ticker, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_ticker_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_ticker_cb func = (klbui_ticker_cb)klb_map_to_ptr(p_ticker->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_ticker, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_ticker(klb_wnd_t* p_wnd)
{
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;

    if (NULL == p_ticker->p_mycss)
    {
        p_ticker->p_mycss = KLB_MALLOCZ(klbwnd_ticker_css_t, 1, 0);

        klbwnd_ticker_css_copy(p_ticker->p_mycss, p_ticker->p_globalcss);
        klbwnd_ticker_set_css(p_wnd, p_ticker->p_mycss);
    }
}

static klbwnd_ticker_css_t* get_css_klbui_ticker(klb_wnd_t* p_wnd)
{
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;

    return (NULL != p_ticker->p_mycss) ? p_ticker->p_mycss : p_ticker->p_globalcss;
}

static klbwnd_ticker_css_t* check_css_klbui_ticker(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_ticker(p_wnd);
    }

    return get_css_klbui_ticker(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_ticker_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_ticker_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_ticker(void* ptr)
{
    klbwnd_ticker_css_t* p_css = (klbwnd_ticker_css_t*)ptr;

    klbwnd_ticker_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_ticker_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_ticker_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kticker);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kticker);

    klbwnd_ticker_css_t* p_css = KLB_MALLOCZ(klbwnd_ticker_css_t, 1, 0);
    klbwnd_ticker_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kticker, p_css, destroy_globalcss_klbwnd_ticker);

    KLBUI_GLOBAL_ticker_bind("margin", globalcss_klbui_ticker_margin);
    KLBUI_GLOBAL_ticker_bind("margin-top", globalcss_klbui_ticker_margin_top);
    KLBUI_GLOBAL_ticker_bind("margin-right", globalcss_klbui_ticker_margin_right);
    KLBUI_GLOBAL_ticker_bind("margin-bottom", globalcss_klbui_ticker_margin_bottom);
    KLBUI_GLOBAL_ticker_bind("margin-left", globalcss_klbui_ticker_margin_left);

    KLBUI_GLOBAL_ticker_bind("padding", globalcss_klbui_ticker_padding);
    KLBUI_GLOBAL_ticker_bind("padding-top", globalcss_klbui_ticker_padding_top);
    KLBUI_GLOBAL_ticker_bind("padding-right", globalcss_klbui_ticker_padding_right);
    KLBUI_GLOBAL_ticker_bind("padding-bottom", globalcss_klbui_ticker_padding_bottom);
    KLBUI_GLOBAL_ticker_bind("padding-left", globalcss_klbui_ticker_padding_left);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_ticker_margin(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_margin_top(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_margin_right(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_margin_bottom(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_margin_left(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_padding(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_padding_top(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_padding_right(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_padding_bottom(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_padding_left(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;
    klbwnd_ticker_css_t* p_css = check_css_klbui_ticker(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 自定义属性

static void on_klbui_ticker_index(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_ticker->ticker.index), p_wnd, method, p_in, p_out);
}

static void on_klbui_ticker_ticker(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klbwnd_ticker_get_enable(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool is_enable = klb_map_idx_to_bool(p_in, start);

        klbwnd_ticker_set_enable(p_wnd, is_enable);
    }
}

static void on_klbui_ticker_ticker_default(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klbwnd_ticker_get_enable_default(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool is_enable = klb_map_idx_to_bool(p_in, start);

        klbwnd_ticker_set_enable_default(p_wnd, is_enable);
    }
}

static void on_klbui_ticker_ticker_interval(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_ticker;

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klbwnd_ticker_get_interval(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int64_t interval = klb_map_idx_to_int64(p_in, start);

        klbwnd_ticker_set_interval(p_wnd, interval);
    }
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_ticker_bind(KEY_, FUNC_) { klb_map_set_ptr(p_ticker->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_ticker_init_func_map(klb_wnd_t* p_wnd, klbui_ticker_t* p_ticker, klb_gui_t* p_gui)
{
    (void)p_wnd;

    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kticker);

    if (NULL != ptr)
    {
        p_ticker->p_func_map = ptr;
        return;
    }

    // step2. 初始化CSS 支持的方法
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kticker);
    p_ticker->p_func_map = ptr;

    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    KLBUI_ticker_bind("margin", on_klbui_ticker_margin);
    KLBUI_ticker_bind("margin-top", on_klbui_ticker_margin_top);
    KLBUI_ticker_bind("margin-right", on_klbui_ticker_margin_right);
    KLBUI_ticker_bind("margin-bottom", on_klbui_ticker_margin_bottom);
    KLBUI_ticker_bind("margin-left", on_klbui_ticker_margin_left);

    KLBUI_ticker_bind("padding", on_klbui_ticker_padding);
    KLBUI_ticker_bind("padding-top", on_klbui_ticker_padding_top);
    KLBUI_ticker_bind("padding-right", on_klbui_ticker_padding_right);
    KLBUI_ticker_bind("padding-bottom", on_klbui_ticker_padding_bottom);
    KLBUI_ticker_bind("padding-left", on_klbui_ticker_padding_left);

    KLBUI_ticker_bind("index", on_klbui_ticker_index);
    KLBUI_ticker_bind("ticker", on_klbui_ticker_ticker);
    KLBUI_ticker_bind("ticker-default", on_klbui_ticker_ticker_default);
    KLBUI_ticker_bind("ticker-interval", on_klbui_ticker_ticker_interval);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_ticker_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_ticker_t));
    klbui_ticker_t* p_ticker = (klbui_ticker_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_ticker_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_ticker_destroy;
    p_wnd->vtable.on_set = klbui_ticker_on_set;
    p_wnd->vtable.on_get = klbui_ticker_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_ticker_init_func_map(p_wnd, p_ticker, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_ticker->p_globalcss = (klbwnd_ticker_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kticker);
    klbwnd_ticker_set_css(p_wnd, p_ticker->p_globalcss);

    return p_wnd;
}

int klbui_register_kticker(klb_gui_t* p_gui)
{
    klbui_ticker_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kticker, klbui_ticker_create);
}

// end
