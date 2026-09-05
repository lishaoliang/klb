// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_div.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_div_t
/// @brief  布局容器: Lua 注册名 kdiv
typedef struct klbui_div_t_
{
    klbwnd_div_t        div;            ///< 必须首位, 保持内存一致

    klbwnd_div_css_t*   p_globalcss;    ///< 全局公共的 CSS
    klbwnd_div_css_t*   p_mycss;        ///< 私有的 CSS

    klb_map_t*          p_func_map;     ///< CSS 属性函数表
} klbui_div_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_div_cb)(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_div_destroy(klb_wnd_t* p_wnd)
{
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;

    klbwnd_div_deinit(p_wnd);

    if (NULL != p_div->p_mycss)
    {
        klbwnd_div_css_deinit(p_div->p_mycss);
        KLB_FREE(p_div->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_div_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_div_cb func = (klbui_div_cb)klb_map_to_ptr(p_div->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_div, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_div_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_div_cb func = (klbui_div_cb)klb_map_to_ptr(p_div->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_div, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_div(klb_wnd_t* p_wnd)
{
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;

    if (NULL == p_div->p_mycss)
    {
        p_div->p_mycss = KLB_MALLOCZ(klbwnd_div_css_t, 1, 0);

        klbwnd_div_css_copy(p_div->p_mycss, p_div->p_globalcss);
        klbwnd_div_set_css(p_wnd, p_div->p_mycss);
    }
}

static klbwnd_div_css_t* get_css_klbui_div(klb_wnd_t* p_wnd)
{
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;

    return (NULL != p_div->p_mycss) ? p_div->p_mycss : p_div->p_globalcss;
}

static klbwnd_div_css_t* check_css_klbui_div(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_div(p_wnd);
    }

    return get_css_klbui_div(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_div_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_div_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_div(void* ptr)
{
    klbwnd_div_css_t* p_css = (klbwnd_div_css_t*)ptr;

    klbwnd_div_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_div_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_div_init_globalcss(klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kdiv);

    if (NULL != ptr)
    {
        return;
    }

    // step2. 新添加 解析map, 及公共 CSS 对象
    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kdiv);

    klbwnd_div_css_t* p_css = KLB_MALLOCZ(klbwnd_div_css_t, 1, 0);
    klbwnd_div_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kdiv, p_css, destroy_globalcss_klbwnd_div);


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_GLOBAL_div_bind("margin", globalcss_klbui_div_margin);
    KLBUI_GLOBAL_div_bind("margin-top", globalcss_klbui_div_margin_top);
    KLBUI_GLOBAL_div_bind("margin-right", globalcss_klbui_div_margin_right);
    KLBUI_GLOBAL_div_bind("margin-bottom", globalcss_klbui_div_margin_bottom);
    KLBUI_GLOBAL_div_bind("margin-left", globalcss_klbui_div_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_div_bind("padding", globalcss_klbui_div_padding);
    KLBUI_GLOBAL_div_bind("padding-top", globalcss_klbui_div_padding_top);
    KLBUI_GLOBAL_div_bind("padding-right", globalcss_klbui_div_padding_right);
    KLBUI_GLOBAL_div_bind("padding-bottom", globalcss_klbui_div_padding_bottom);
    KLBUI_GLOBAL_div_bind("padding-left", globalcss_klbui_div_padding_left);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_div_margin(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_margin_top(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_margin_right(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_margin_bottom(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_margin_left(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_padding(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_padding_top(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_padding_right(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_padding_bottom(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_padding_left(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_div_css_t* p_css = check_css_klbui_div(p_wnd, method);

    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// 私有自定义

static void on_klbui_div_index(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_div->div.index), p_wnd, method, p_in, p_out);
}

static void on_klbui_div_title(klb_wnd_t* p_wnd, klbui_div_t* p_div, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_div->div.title), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_div_bind(KEY_, FUNC_) { klb_map_set_ptr(p_div->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_div_init_func_map(klb_wnd_t* p_wnd, klbui_div_t* p_div, klb_gui_t* p_gui)
{

    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kdiv);

    if (NULL != ptr)
    {
        p_div->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kdiv);
    p_div->p_func_map = ptr;

    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_div_bind("margin", on_klbui_div_margin);
    KLBUI_div_bind("margin-top", on_klbui_div_margin_top);
    KLBUI_div_bind("margin-right", on_klbui_div_margin_right);
    KLBUI_div_bind("margin-bottom", on_klbui_div_margin_bottom);
    KLBUI_div_bind("margin-left", on_klbui_div_margin_left);

    // 内边距 padding
    KLBUI_div_bind("padding", on_klbui_div_padding);
    KLBUI_div_bind("padding-top", on_klbui_div_padding_top);
    KLBUI_div_bind("padding-right", on_klbui_div_padding_right);
    KLBUI_div_bind("padding-bottom", on_klbui_div_padding_bottom);
    KLBUI_div_bind("padding-left", on_klbui_div_padding_left);

    // 私有自定义

    KLBUI_div_bind("index", on_klbui_div_index);
    KLBUI_div_bind("title", on_klbui_div_title);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_div_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_div_t));
    klbui_div_t* p_div = (klbui_div_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_div_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_div_destroy;
    p_wnd->vtable.on_set = klbui_div_on_set;
    p_wnd->vtable.on_get = klbui_div_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_div_init_func_map(p_wnd, p_div, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_div->p_globalcss = (klbwnd_div_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kdiv);
    klbwnd_div_set_css(p_wnd, p_div->p_globalcss);

    return p_wnd;
}

int klbui_register_kdiv(klb_gui_t* p_gui)
{
    klbui_div_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kdiv, klbui_div_create);
}

// end
