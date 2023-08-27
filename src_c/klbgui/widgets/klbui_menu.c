// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_menu.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_menu_t
/// @brief  菜单
typedef struct klbui_menu_t_
{
    klbwnd_menu_t       menu;           ///< 必须首位, 保持内存一致
    klbwnd_menu_css_t   css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_menu_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_menu_cb)(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_menu_destroy(klb_wnd_t* p_wnd)
{
    klbui_menu_t* p_menu = (klbui_menu_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_menu_quit(p_wnd);

    // 退出css
    klbwnd_menu_css_quit(&p_menu->css);

    KLB_FREE(p_wnd);
}

static int klbui_menu_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_menu_t* p_menu = (klbui_menu_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_menu_cb func = (klbui_menu_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_menu_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_menu_t* p_menu = (klbui_menu_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_menu_cb func = (klbui_menu_cb)klb_map_to_ptr(p_menu->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_menu, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_menu_margin(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_menu->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_margin_top(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_menu->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_margin_right(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_menu->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_margin_bottom(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_menu->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_margin_left(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_menu->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_padding(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_menu->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_padding_top(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_menu->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_padding_right(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_menu->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_padding_bottom(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_menu->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_padding_left(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_menu->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_text_color(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_text_align(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_font_size(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_background_color(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_background_image(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_menu->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_border_width(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_menu->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_border_color(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_menu->css.normal.border), p_wnd, method, p_in, p_out);
}

////////////////////////////////////////
// 子项

static void on_klbui_menu_item_text_color(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->css.css_item.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_text_color_focus(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_menu->css.css_item.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_text_align(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->css.css_item.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_text_align_focus(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_menu->css.css_item.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_font_size(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->css.css_item.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_font_size_focus(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_menu->css.css_item.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_background_color(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->css.css_item.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_item_background_color_focus(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_menu->css.css_item.focus.background), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_menu_value(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_menu->menu.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_menu_append(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_set == method)
    {
        /* eg.
            jq('menu1').append({
                {['1'] = '项目1'},
                {['2'] = '项目2'},
                {['3'] = '项目3'},
                {['4'] = '项目4'},
            })

            or, kgui.set('/xxx/.../combo1', 'append', {
                {['1'] = '项目1'},
                {['2'] = '项目2'},
                {['3'] = '项目3'},
                {['4'] = '项目4'},
            })
        */
        int start = 1;
        klb_map_t* p_in_array = (klb_map_t*)klb_map_idx_to_map(p_in, start);
        if (NULL != p_in_array)
        {
            klbwnd_menu_append(p_wnd, p_in_array);

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_menu_append_2rd(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_set == method)
    {
        /* eg.
            jq('menu1').append({
                ['1'] = {
                    {['1-1'] = '子项目1-1'},
                    {['1-2'] = '子项目1-2'},
                    {['1-3'] = '子项目1-3'},
                    {['1-4'] = '子项目1-4'},
                },
                ['2'] = {
                    {['2-1'] = '子项目2-1'},
                    {['2-2'] = '子项目2-2'},
                    {['2-3'] = '子项目2-3'},
                    {['2-4'] = '子项目2-4'},
                },
            })
        */
        int start = 1;
        klb_map_t* p_in_2rd = (klb_map_t*)klb_map_idx_to_map(p_in, start);
        if (NULL != p_in_2rd)
        {
            klbwnd_menu_append_2rd(p_wnd, p_in_2rd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_menu_bind(KEY_, FUNC_) { klb_map_set_ptr(p_menu->p_func_map, (KEY_), (void*)(FUNC_), p_menu); }

static void klbui_menu_init_func_map(klb_wnd_t* p_wnd, klbui_menu_t* p_menu, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kmenu);
    if (NULL != ptr)
    {
        p_menu->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kmenu);
    p_menu->p_func_map = ptr;
    klb_gui_css_map_append_std_function(ptr, p_menu); // 添加公共方法


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_menu_bind("margin", on_klbui_menu_margin);
    KLBUI_menu_bind("margin-top", on_klbui_menu_margin_top);
    KLBUI_menu_bind("margin-right", on_klbui_menu_margin_right);
    KLBUI_menu_bind("margin-bottom", on_klbui_menu_margin_bottom);
    KLBUI_menu_bind("margin-left", on_klbui_menu_margin_left);

    // 内边距 padding
    KLBUI_menu_bind("padding", on_klbui_menu_padding);
    KLBUI_menu_bind("padding-top", on_klbui_menu_padding_top);
    KLBUI_menu_bind("padding-right", on_klbui_menu_padding_right);
    KLBUI_menu_bind("padding-bottom", on_klbui_menu_padding_bottom);
    KLBUI_menu_bind("padding-left", on_klbui_menu_padding_left);

    // 文本颜色 color
    KLBUI_menu_bind("color", on_klbui_menu_text_color);

    // 文本对齐 text-align
    KLBUI_menu_bind("text-align", on_klbui_menu_text_align);

    // 字体大小 font-size
    KLBUI_menu_bind("font-size", on_klbui_menu_font_size);

    // 背景色 background-color
    KLBUI_menu_bind("background-color", on_klbui_menu_background_color);

    // 背景图片 background-image
    KLBUI_menu_bind("background-image", on_klbui_menu_background_image);

    // 边框的宽度 border-width
    KLBUI_menu_bind("border-width", on_klbui_menu_border_width);

    // 边框的颜色 border-color
    KLBUI_menu_bind("border-color", on_klbui_menu_border_color);

    //////////////////////////////////////////////
    // 子项

    // 子项.文本颜色 color
    KLBUI_menu_bind("item.color", on_klbui_menu_item_text_color);
    KLBUI_menu_bind("item.color:focus", on_klbui_menu_item_text_color_focus);

    // 子项.文本对齐 text-align
    KLBUI_menu_bind("item.text-align", on_klbui_menu_item_text_align);
    KLBUI_menu_bind("item.text-align:focus", on_klbui_menu_item_text_align_focus);

    // 子项.字体大小 font-size
    KLBUI_menu_bind("item.font-size", on_klbui_menu_item_font_size);
    KLBUI_menu_bind("item.font-size:focus", on_klbui_menu_item_font_size_focus);

    // 子项.背景色 background-color
    KLBUI_menu_bind("item.background-color", on_klbui_menu_item_background_color);
    KLBUI_menu_bind("item.background-color:focus", on_klbui_menu_item_background_color_focus);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_menu_bind("value", on_klbui_menu_value);

    KLBUI_menu_bind("append", on_klbui_menu_append);
    KLBUI_menu_bind("append_2rd", on_klbui_menu_append_2rd);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_menu_t));
    klbui_menu_t* p_menu = (klbui_menu_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_menu_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_menu_destroy;
    p_wnd->vtable.on_set = klbui_menu_on_set;
    p_wnd->vtable.on_get = klbui_menu_on_get;

    // css
    klbwnd_menu_css_init(&p_menu->css, p_gui);
    klbwnd_menu_set_css(p_wnd, &p_menu->css);

    // 初始化CSS 支持的方法
    klbui_menu_init_func_map(p_wnd, p_menu, p_gui);

    return p_wnd;
}
