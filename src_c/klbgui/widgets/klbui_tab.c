// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_tab.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_tab_t
/// @brief  Tab切换页
typedef struct klbui_tab_t_
{
    klbwnd_tab_t        tab;        ///< 必须首位, 保持内存一致
    klbwnd_tab_css_t    css;        ///< 样式

    klb_map_t*          p_func_map; ///< CSS属性函数表
}klbui_tab_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_tab_cb)(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_tab_destroy(klb_wnd_t* p_wnd)
{
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_tab_quit(p_wnd);

    // 退出css
    klbwnd_tab_css_quit(&p_tab->css);

    KLB_FREE(p_wnd);
}

static int klbui_tab_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_tab_cb func = (klbui_tab_cb)klb_map_to_ptr(p_tab->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_tab, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_tab_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_tab_cb func = (klbui_tab_cb)klb_map_to_ptr(p_tab->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_tab, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_tab_visibility(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_tab->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_top(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_tab->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_right(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_tab->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_bottom(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_tab->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_left(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_tab->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_tab->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_top(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_tab->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_right(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_tab->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_bottom(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_tab->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_left(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_tab->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_text_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_tab->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_text_align(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_tab->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_font_size(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_tab->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_background_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_tab->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_background_image(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_tab->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_border_width(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_tab->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_border_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_tab->css.normal.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_tab_title(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_tab->tab.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_value(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_tab->tab.value), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_tab_bind(KEY_, FUNC_) { klb_map_set_ptr(p_tab->p_func_map, (KEY_), (void*)(FUNC_), p_tab); }

static void klbui_tab_init_func_map(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_ktab);
    if (NULL != ptr)
    {
        p_tab->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_ktab);
    p_tab->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_tab_bind("visibility", on_klbui_tab_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_tab_bind("margin", on_klbui_tab_margin);
    KLBUI_tab_bind("margin-top", on_klbui_tab_margin_top);
    KLBUI_tab_bind("margin-right", on_klbui_tab_margin_right);
    KLBUI_tab_bind("margin-bottom", on_klbui_tab_margin_bottom);
    KLBUI_tab_bind("margin-left", on_klbui_tab_margin_left);

    // 内边距 padding
    KLBUI_tab_bind("padding", on_klbui_tab_padding);
    KLBUI_tab_bind("padding-top", on_klbui_tab_padding_top);
    KLBUI_tab_bind("padding-right", on_klbui_tab_padding_right);
    KLBUI_tab_bind("padding-bottom", on_klbui_tab_padding_bottom);
    KLBUI_tab_bind("padding-left", on_klbui_tab_padding_left);

    // 文本颜色 color
    KLBUI_tab_bind("color", on_klbui_tab_text_color);

    // 文本对齐 text-align
    KLBUI_tab_bind("text-align", on_klbui_tab_text_align);

    // 字体大小 font-size
    KLBUI_tab_bind("font-size", on_klbui_tab_font_size);

    // 背景色 background-color
    KLBUI_tab_bind("background-color", on_klbui_tab_background_color);

    // 背景图片 background-image
    KLBUI_tab_bind("background-image", on_klbui_tab_background_image);

    // 边框的宽度 border-width
    KLBUI_tab_bind("border-width", on_klbui_tab_border_width);

    // 边框的颜色 border-color
    KLBUI_tab_bind("border-color", on_klbui_tab_border_color);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_tab_bind("title", on_klbui_tab_title);
    KLBUI_tab_bind("value", on_klbui_tab_value);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_tab_t));
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_tab_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_tab_destroy;
    p_wnd->vtable.on_set = klbui_tab_on_set;
    p_wnd->vtable.on_get = klbui_tab_on_get;

    // css
    klbwnd_tab_css_init(&p_tab->css, p_gui);
    klbwnd_tab_set_css(p_wnd, &p_tab->css);

    // 初始化CSS 支持的方法
    klbui_tab_init_func_map(p_wnd, p_tab, p_gui);

    return p_wnd;
}
