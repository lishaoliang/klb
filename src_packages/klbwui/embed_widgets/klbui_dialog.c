// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/embed_wnd/klbwnd_dialog.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_dialog_t
/// @brief  对话框 (Lua type: kdialog)
typedef struct klbui_dialog_t_
{
    klbwnd_dialog_t             dialog;         ///< 必须首位, 保持内存一致

    klbwnd_dialog_css_t*        p_globalcss;    ///< 全局公共的 CSS
    klbwnd_dialog_css_t*        p_mycss;        ///< 私有的 CSS

    klb_map_t*                  p_func_map;     ///< CSS 属性函数表
} klbui_dialog_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbui_dialog_cb)(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_dialog_destroy(klb_wnd_t* p_wnd)
{
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;

    klbwnd_dialog_deinit(p_wnd);

    if (NULL != p_dlg->p_mycss)
    {
        klbwnd_dialog_css_deinit(p_dlg->p_mycss);
        KLB_FREE(p_dlg->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_dialog_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_dialog_cb func = (klbui_dialog_cb)klb_map_to_ptr(p_dlg->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_dlg, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_dialog_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_dialog_cb func = (klbui_dialog_cb)klb_map_to_ptr(p_dlg->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_dlg, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

static void new_my_css_klbui_dialog(klb_wnd_t* p_wnd)
{
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;

    if (NULL == p_dlg->p_mycss)
    {
        p_dlg->p_mycss = KLB_MALLOCZ(klbwnd_dialog_css_t, 1, 0);

        klbwnd_dialog_css_copy(p_dlg->p_mycss, p_dlg->p_globalcss);
        klbwnd_dialog_set_css(p_wnd, p_dlg->p_mycss);
    }
}

static klbwnd_dialog_css_t* get_css_klbui_dialog(klb_wnd_t* p_wnd)
{
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;

    return (NULL != p_dlg->p_mycss) ? p_dlg->p_mycss : p_dlg->p_globalcss;
}

static klbwnd_dialog_css_t* check_css_klbui_dialog(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        new_my_css_klbui_dialog(p_wnd);
    }

    return get_css_klbui_dialog(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS

static void globalcss_klbui_dialog_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(((klbwnd_dialog_css_t*)ptr)->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_dialog_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(((klbwnd_dialog_css_t*)ptr)->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_dialog_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(((klbwnd_dialog_css_t*)ptr)->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_dialog_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(((klbwnd_dialog_css_t*)ptr)->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_dialog_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(((klbwnd_dialog_css_t*)ptr)->normal.border), NULL, method, p_in, p_out);
}

static void destroy_globalcss_klbwnd_dialog(void* ptr)
{
    klbwnd_dialog_css_t* p_css = (klbwnd_dialog_css_t*)ptr;

    klbwnd_dialog_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_dlg_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_dialog_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_kdialog);

    if (NULL != ptr)
    {
        return;
    }

    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_kdialog);

    klbwnd_dialog_css_t* p_css = KLB_MALLOCZ(klbwnd_dialog_css_t, 1, 0);
    klbwnd_dialog_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_kdialog, p_css, destroy_globalcss_klbwnd_dialog);

    KLBUI_GLOBAL_dlg_bind("margin", globalcss_klbui_dialog_margin);
    KLBUI_GLOBAL_dlg_bind("padding", globalcss_klbui_dialog_padding);
    KLBUI_GLOBAL_dlg_bind("background-color", globalcss_klbui_dialog_background_color);
    KLBUI_GLOBAL_dlg_bind("border-width", globalcss_klbui_dialog_border_width);
    KLBUI_GLOBAL_dlg_bind("border-color", globalcss_klbui_dialog_border_color);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_dialog_margin(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_dlg;
    klbuicssex_margin(&(check_css_klbui_dialog(p_wnd, method)->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_padding(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_dlg;
    klbuicssex_padding(&(check_css_klbui_dialog(p_wnd, method)->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_background_color(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_dlg;
    klbuicssex_background_color(&(check_css_klbui_dialog(p_wnd, method)->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_border_width(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_dlg;
    klbuicssex_border_width(&(check_css_klbui_dialog(p_wnd, method)->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_border_color(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_dlg;
    klbuicssex_border_color(&(check_css_klbui_dialog(p_wnd, method)->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_title(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_dlg->dialog.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_dialog_value(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_dlg->dialog.value), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_dlg_bind(KEY_, FUNC_) { klb_map_set_ptr(p_dlg->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_dialog_init_func_map(klb_wnd_t* p_wnd, klbui_dialog_t* p_dlg, klb_gui_t* p_gui)
{
    (void)p_wnd;

    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_kdialog);

    if (NULL != ptr)
    {
        p_dlg->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBWUI_kdialog);
    p_dlg->p_func_map = ptr;

    KLBUI_dlg_bind("margin", on_klbui_dialog_margin);
    KLBUI_dlg_bind("padding", on_klbui_dialog_padding);
    KLBUI_dlg_bind("background-color", on_klbui_dialog_background_color);
    KLBUI_dlg_bind("border-width", on_klbui_dialog_border_width);
    KLBUI_dlg_bind("border-color", on_klbui_dialog_border_color);
    KLBUI_dlg_bind("title", on_klbui_dialog_title);
    KLBUI_dlg_bind("value", on_klbui_dialog_value);
}


//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_dialog_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_dialog_t));
    klbui_dialog_t* p_dlg = (klbui_dialog_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_dialog_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_dialog_destroy;
    p_wnd->vtable.on_set = klbui_dialog_on_set;
    p_wnd->vtable.on_get = klbui_dialog_on_get;

    // step4. 初始化 CSS 支持的方法
    klbui_dialog_init_func_map(p_wnd, p_dlg, p_gui);

    // step5. 默认使用公用全局 CSS 属性
    p_dlg->p_globalcss = (klbwnd_dialog_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_kdialog);
    klbwnd_dialog_set_css(p_wnd, p_dlg->p_globalcss);

    return p_wnd;
}

int klbui_register_kdialog(klb_gui_t* p_gui)
{
    klbui_dialog_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBWUI_kdialog, klbui_dialog_create);
}

// end
