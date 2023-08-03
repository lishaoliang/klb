// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/shwnd/klbshw_combomenu.h"
#include "klbgui/subviews/klbwnd_combomenu.h"
#include "klbgui/klbui_widgets.h"
#include "klbgui/klbui_shwnd.h"
#include "klbmem/klb_mem.h"


/// @struct klbshw_combomenu_t
/// @brief  共享使用的 kcombo 控件菜单
typedef struct klbshw_combomenu_t_
{
    klbwnd_combomenu_t     combomenu;       ///< 必须首位, 保持内存一致
    klbwnd_combomenu_css_t css;             ///< 样式

    klb_map_t*             p_func_map;      ///< CSS属性函数表
}klbshw_combomenu_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbshw_combomenu_cb)(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_combomenu_destroy(klb_wnd_t* p_wnd)
{
    klbshw_combomenu_t* p_btn = (klbshw_combomenu_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_combomenu_quit(p_wnd);

    // 退出css
    klbwnd_combomenu_css_quit(&p_btn->css);

    KLB_FREE(p_wnd);
}

static int klbshw_combomenu_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_combomenu_t* p_btn = (klbshw_combomenu_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_combomenu_cb func = (klbshw_combomenu_cb)klb_map_to_ptr(p_btn->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btn, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbshw_combomenu_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_combomenu_t* p_btn = (klbshw_combomenu_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_combomenu_cb func = (klbshw_combomenu_cb)klb_map_to_ptr(p_btn->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btn, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbshw_combomenu_visibility(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_margin(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_btn->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_margin_top(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_btn->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_margin_right(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_btn->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_margin_bottom(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_btn->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_margin_left(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_btn->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_padding(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_btn->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_padding_top(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_btn->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_padding_right(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_btn->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_padding_bottom(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_btn->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_padding_left(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_btn->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_text_color(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_btn->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_text_align(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_btn->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_font_size(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_btn->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_background_color(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_btn->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_background_image(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_btn->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_border_width(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_btn->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_combomenu_border_color(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_btn->css.normal.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性



//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_btn_bind(KEY_, FUNC_) { klb_map_set_ptr(p_btn->p_func_map, (KEY_), (void*)(FUNC_), p_btn); }

static void klbshw_combomenu_init_func_map(klb_wnd_t* p_wnd, klbshw_combomenu_t* p_btn, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_combomenu);
    if (NULL != ptr)
    {
        p_btn->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBSHW_combomenu);
    p_btn->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_btn_bind("visibility", on_klbshw_combomenu_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_btn_bind("margin", on_klbshw_combomenu_margin);
    KLBUI_btn_bind("margin-top", on_klbshw_combomenu_margin_top);
    KLBUI_btn_bind("margin-right", on_klbshw_combomenu_margin_right);
    KLBUI_btn_bind("margin-bottom", on_klbshw_combomenu_margin_bottom);
    KLBUI_btn_bind("margin-left", on_klbshw_combomenu_margin_left);

    // 内边距 padding
    KLBUI_btn_bind("padding", on_klbshw_combomenu_padding);
    KLBUI_btn_bind("padding-top", on_klbshw_combomenu_padding_top);
    KLBUI_btn_bind("padding-right", on_klbshw_combomenu_padding_right);
    KLBUI_btn_bind("padding-bottom", on_klbshw_combomenu_padding_bottom);
    KLBUI_btn_bind("padding-left", on_klbshw_combomenu_padding_left);

    // 文本颜色 color
    KLBUI_btn_bind("color", on_klbshw_combomenu_text_color);

    // 文本对齐 text-align
    KLBUI_btn_bind("text-align", on_klbshw_combomenu_text_align);

    // 字体大小 font-size
    KLBUI_btn_bind("font-size", on_klbshw_combomenu_font_size);

    // 背景色 background-color
    KLBUI_btn_bind("background-color", on_klbshw_combomenu_background_color);

    // 背景图片 background-image
    KLBUI_btn_bind("background-image", on_klbshw_combomenu_background_image);

    // 边框的宽度 border-width
    KLBUI_btn_bind("border-width", on_klbshw_combomenu_border_width);

    // 边框的颜色 border-color
    KLBUI_btn_bind("border-color", on_klbshw_combomenu_border_color);

    //////////////////////////////////////////////
    // 自定义方法


}

//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_combomenu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_combomenu_t));
    klbshw_combomenu_t* p_btn = (klbshw_combomenu_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_combomenu_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbshw_combomenu_destroy;
    p_wnd->vtable.on_set = klbshw_combomenu_on_set;
    p_wnd->vtable.on_get = klbshw_combomenu_on_get;

    // css
    klbwnd_combomenu_css_init(&p_btn->css, p_gui);
    klbwnd_combomenu_set_css(p_wnd, &p_btn->css);

    // 初始化CSS 支持的方法
    klbshw_combomenu_init_func_map(p_wnd, p_btn, p_gui);

    return p_wnd;
}

//////////////////////////////////////////////////////////////////////////
// 导出公共函数 

/// @brief 获取"kcombo"控件的专用弹出(菜单)
klb_wnd_t* klbui_shwnd_get_combomenu(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_combomenu);

    if (NULL == p_wnd)
    {
        int w = 120, h = 240;

        p_wnd = klbshw_combomenu_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_combomenu, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

int klbshw_combomenu_bind(klb_wnd_t* p_wnd, klbshw_combomenu_result_cb cb_result, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h)
{
    return klbwnd_combomenu_bind(p_wnd, cb_result, ptr, p_data_array, p_out_w, p_out_h);
}
