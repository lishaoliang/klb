// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_qrcode.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_qrcode_t
/// @brief  二维码 (Lua type: kqrcode)
typedef struct klbui_qrcode_t_
{
    klbwnd_qrcode_t             qrcode;         ///< 必须首位, 保持内存一致

    klbwnd_qrcode_css_t*        p_globalcss;    ///< 全局公共的 CSS
    klbwnd_qrcode_css_t*        p_mycss;        ///< 私有的 CSS

    klb_map_t*                  p_func_map;     ///< CSS 属性函数表
} klbui_qrcode_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_qrcode_cb)(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_qrcode_destroy(klb_wnd_t* p_wnd)
{
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;

    klbwnd_qrcode_deinit(p_wnd);

    if (NULL != p_qr->p_mycss)
    {
        klbwnd_qrcode_css_deinit(p_qr->p_mycss);
        KLB_FREE(p_qr->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_qrcode_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_qrcode_cb func = (klbui_qrcode_cb)klb_map_to_ptr(p_qr->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_qr, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_qrcode_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_qrcode_cb func = (klbui_qrcode_cb)klb_map_to_ptr(p_qr->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_qr, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_qrcode(klb_wnd_t* p_wnd)
{
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;

    if (NULL == p_qr->p_mycss)
    {
        p_qr->p_mycss = KLB_MALLOCZ(klbwnd_qrcode_css_t, 1, 0);

        klbwnd_qrcode_css_copy(p_qr->p_mycss, p_qr->p_globalcss);
        klbwnd_qrcode_set_css(p_wnd, p_qr->p_mycss);
    }
}

static klbwnd_qrcode_css_t* get_css_klbui_qrcode(klb_wnd_t* p_wnd)
{
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;

    return (NULL != p_qr->p_mycss) ? p_qr->p_mycss : p_qr->p_globalcss;
}

static klbwnd_qrcode_css_t* check_css_klbui_qrcode(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_qrcode(p_wnd);
    }

    return get_css_klbui_qrcode(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS

static void globalcss_klbui_qrcode_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(((klbwnd_qrcode_css_t*)ptr)->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_qrcode_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(((klbwnd_qrcode_css_t*)ptr)->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_qrcode_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(((klbwnd_qrcode_css_t*)ptr)->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_qrcode_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(((klbwnd_qrcode_css_t*)ptr)->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_qrcode_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(((klbwnd_qrcode_css_t*)ptr)->normal.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_qrcode(void* ptr)
{
    klbwnd_qrcode_css_t* p_css = (klbwnd_qrcode_css_t*)ptr;

    klbwnd_qrcode_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_qrcode_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_qrcode_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kqrcode);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kqrcode);

    klbwnd_qrcode_css_t* p_css = KLB_MALLOCZ(klbwnd_qrcode_css_t, 1, 0);
    klbwnd_qrcode_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kqrcode, p_css, destroy_globalcss_klbwnd_qrcode);

    KLBUI_GLOBAL_qrcode_bind("margin", globalcss_klbui_qrcode_margin);
    KLBUI_GLOBAL_qrcode_bind("padding", globalcss_klbui_qrcode_padding);
    KLBUI_GLOBAL_qrcode_bind("background-color", globalcss_klbui_qrcode_background_color);
    KLBUI_GLOBAL_qrcode_bind("border-width", globalcss_klbui_qrcode_border_width);
    KLBUI_GLOBAL_qrcode_bind("border-color", globalcss_klbui_qrcode_border_color);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_qrcode_margin(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_qr;
    klbuicssex_margin(&(check_css_klbui_qrcode(p_wnd, method)->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_padding(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_qr;
    klbuicssex_padding(&(check_css_klbui_qrcode(p_wnd, method)->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_background_color(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_qr;
    klbuicssex_background_color(&(check_css_klbui_qrcode(p_wnd, method)->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_width(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_qr;
    klbuicssex_border_width(&(check_css_klbui_qrcode(p_wnd, method)->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_border_color(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_qr;
    klbuicssex_border_color(&(check_css_klbui_qrcode(p_wnd, method)->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_title(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_qr->qrcode.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_qrcode_value(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_set == method)
    {
        const char* p_value = klb_map_idx_to_string(p_in, 1);

        klbwnd_qrcode_set_value(p_wnd, p_value);
    }
    else
    {
        klbuicssex_attribute_sds(&(p_qr->qrcode.value), p_wnd, method, p_in, p_out);
    }
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_qrcode_bind(KEY_, FUNC_) { klb_map_set_ptr(p_qr->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_qrcode_init_func_map(klb_wnd_t* p_wnd, klbui_qrcode_t* p_qr, klb_gui_t* p_gui)
{
    (void)p_wnd;

    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kqrcode);

    if (NULL != ptr)
    {
        p_qr->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kqrcode);
    p_qr->p_func_map = ptr;

    KLBUI_qrcode_bind("margin", on_klbui_qrcode_margin);
    KLBUI_qrcode_bind("padding", on_klbui_qrcode_padding);
    KLBUI_qrcode_bind("background-color", on_klbui_qrcode_background_color);
    KLBUI_qrcode_bind("border-width", on_klbui_qrcode_border_width);
    KLBUI_qrcode_bind("border-color", on_klbui_qrcode_border_color);
    KLBUI_qrcode_bind("title", on_klbui_qrcode_title);
    KLBUI_qrcode_bind("value", on_klbui_qrcode_value);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_qrcode_t));
    klbui_qrcode_t* p_qr = (klbui_qrcode_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_qrcode_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_qrcode_destroy;
    p_wnd->vtable.on_set = klbui_qrcode_on_set;
    p_wnd->vtable.on_get = klbui_qrcode_on_get;

    // step4. 初始化 CSS 支持的方法
    klbui_qrcode_init_func_map(p_wnd, p_qr, p_gui);

    // step5. 默认使用公用全局 CSS 属性
    p_qr->p_globalcss = (klbwnd_qrcode_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kqrcode);
    klbwnd_qrcode_set_css(p_wnd, p_qr->p_globalcss);

    return p_wnd;
}

int klbui_register_kqrcode(klb_gui_t* p_gui)
{
    klbui_qrcode_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kqrcode, klbui_qrcode_create);
}

// end
