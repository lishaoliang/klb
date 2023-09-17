// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_listex.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_listex_t
/// @brief  扩展列表框
typedef struct klbui_listex_t_
{
    klbwnd_listex_t     listex;         ///< 必须首位, 保持内存一致
    klbwnd_listex_css_t css;            ///< 样式

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_listex_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_listex_cb)(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_listex_destroy(klb_wnd_t* p_wnd)
{
    klbui_listex_t* p_list = (klbui_listex_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_listex_quit(p_wnd);

    // 退出css
    klbwnd_listex_css_quit(&p_list->css);

    KLB_FREE(p_wnd);
}

static int klbui_listex_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_listex_t* p_list = (klbui_listex_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_listex_cb func = (klbui_listex_cb)klb_map_to_ptr(p_list->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_list, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_listex_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_listex_t* p_list = (klbui_listex_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_listex_cb func = (klbui_listex_cb)klb_map_to_ptr(p_list->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_list, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_listex_margin(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_list->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_margin_top(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_list->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_margin_right(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_list->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_margin_bottom(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_list->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_margin_left(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_list->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_padding(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_list->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_padding_top(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_list->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_padding_right(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_list->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_padding_bottom(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_list->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_padding_left(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_list->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_color(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_list->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_color_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_list->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_color_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_list->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_align(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_list->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_align_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_list->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_text_align_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_list->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_font_size(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_list->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_font_size_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_list->css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_font_size_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_list->css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_color(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_list->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_color_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_list->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_color_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_list->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_image(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_list->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_image_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_list->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_background_image_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_list->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_width(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_list->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_width_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_list->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_width_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_list->css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_color(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_list->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_color_focus(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_list->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_listex_border_color_disable(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_list->css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性


static void on_klbui_listex_append_column(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 添加列
    if (KLBUI_CSSEX_set == method)
    {
        /* eg.
            jq('list1')['append_column']({
                {['width']=128,['title']='列1'},
                {['width']=128,['title']='列2'},
                {['width']=128,['title']='列3'},
            })
        */
        int start = 1;
        klb_map_t* p_in_array = (klb_map_t*)klb_map_idx_to_map(p_in, start);
        if (NULL != p_in_array)
        {
            int count = klb_map_array_size(p_in_array);
            for (int i = 0; i < count; i++)
            {
                klb_map_t* p_item_map = klb_map_idx_to_map(p_in_array, i);
                if (NULL != p_item_map)
                {
                    int width = 0;
                    width = (int)klb_map_to_int64(p_item_map, "width");
                    if (width <= 0)
                    {
                        width = (int)klb_map_to_uint64(p_item_map, "width");
                    }

                    // 宽度必须大于0, 才认可有效列
                    if (0 < width)
                    {
                        klbwnd_listex_append_column(p_wnd, width, klb_map_to_string(p_item_map, "title"), klb_map_to_map(p_item_map, "child"));
                    }
                }
            }
        }
    }
}

static void on_klbui_listex_append(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 添加数据
    if (KLBUI_CSSEX_set == method)
    {
        /* eg.
        jq('list1').append({
            {{['11']='行11'}, {['12']='行12'}, {['13']='行13'}},
            {{['21']='行21'}, {['22']='行12'}, {['23']='行23'}},
            {{['31']='行31'}, {['32']='行12'}, {['33']='行33'}},
        })

        或

        jq('list1').append({
            {'行11', '行12', '行13'},
            {'行21', '行22', '行23'},
            {'行31', '行32', '行33'},
        })
        */
        int start = 1;
        klb_map_t* p_in_array = (klb_map_t*)klb_map_idx_to_map(p_in, start);
        if (NULL != p_in_array)
        {
            klb_map_t* p_data_map = klbwnd_listex_get_data_map(p_wnd);

            int count = klb_map_array_size(p_in_array);
            for (int i = 0; i < count; i++)
            {
                klb_map_append_adt_clone(p_data_map, klb_map_idx_to_adt(p_in_array, i));
            }

            klbwnd_listex_relayout(p_wnd); // 重新布局

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_listex_clear(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 清空
    klbwnd_listex_clear(p_wnd);
    klb_wnd_update(p_wnd);
}

static void on_klbui_listex_clear_data(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 清空
    klbwnd_listex_clear_data(p_wnd);
    klb_wnd_update(p_wnd);
}

static void on_klbui_listex_value(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 值
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t* p_checks_map = klb_map_create();
        klb_map_t* p_data_map = klbwnd_listex_get_data_map(p_wnd);

        int array_size = klb_map_array_size(p_data_map);
        for (int i = 0; i < array_size; i++)
        {
            klb_map_t* p_row_map = klb_map_idx_to_map(p_data_map, i);
            if (NULL != p_row_map && klb_map_to_bool(p_row_map, "check"))
            {
                klb_map_append_map_clone(p_checks_map, p_row_map);
            }   
        }

        klb_map_set_idx_map(p_out, 0, p_checks_map);
    }
    else if (KLBUI_CSSEX_set == method)
    {

    }
}

static void on_klbui_listex_event_wnd(klb_wnd_t* p_wnd, klbui_listex_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_wnd_t* p_event_wnd =  klbwnd_listex_get_event_wnd(p_wnd);
    if (NULL == p_event_wnd)
    {
        return;
    }

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t param_in = { 0 };
        klb_map_init(&param_in);

        klb_map_set_idx_adt_clone(&param_in, 0, klb_map_idx_to_adt(p_in, 1));

        klb_map_t* ptr = klb_wnd_get(p_event_wnd, &param_in);

        klb_map_copy(p_out, ptr);

        klb_map_quit(&param_in);
        KLB_FREE_BY(ptr, klb_map_destroy);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        klb_map_t param_in = { 0 };
        klb_map_init(&param_in);

        klb_map_set_idx_adt_clone(&param_in, 0, klb_map_idx_to_adt(p_in, 1));
        klb_map_set_idx_adt_clone(&param_in, 1, klb_map_idx_to_adt(p_in, 2));

        klb_wnd_set(p_event_wnd, &param_in);

        klb_map_quit(&param_in);
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_listex_bind(KEY_, FUNC_) { klb_map_set_ptr(p_list->p_func_map, (KEY_), (void*)(FUNC_), p_list); }

static void klbui_listex_init_func_map(klb_wnd_t* p_wnd, klbui_listex_t* p_list, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_klistex);
    if (NULL != ptr)
    {
        p_list->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_klistex);
    p_list->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_listex_bind("margin", on_klbui_listex_margin);
    KLBUI_listex_bind("margin-top", on_klbui_listex_margin_top);
    KLBUI_listex_bind("margin-right", on_klbui_listex_margin_right);
    KLBUI_listex_bind("margin-bottom", on_klbui_listex_margin_bottom);
    KLBUI_listex_bind("margin-left", on_klbui_listex_margin_left);

    // 内边距 padding
    KLBUI_listex_bind("padding", on_klbui_listex_padding);
    KLBUI_listex_bind("padding-top", on_klbui_listex_padding_top);
    KLBUI_listex_bind("padding-right", on_klbui_listex_padding_right);
    KLBUI_listex_bind("padding-bottom", on_klbui_listex_padding_bottom);
    KLBUI_listex_bind("padding-left", on_klbui_listex_padding_left);

    // 文本颜色 color
    KLBUI_listex_bind("color", on_klbui_listex_text_color);
    KLBUI_listex_bind("color:focus", on_klbui_listex_text_color_focus);
    KLBUI_listex_bind("color:disable", on_klbui_listex_text_color_disable);

    // 文本对齐 text-align
    KLBUI_listex_bind("text-align", on_klbui_listex_text_align);
    KLBUI_listex_bind("text-align:focus", on_klbui_listex_text_align_focus);
    KLBUI_listex_bind("text-align:disable", on_klbui_listex_text_align_disable);

    // 字体大小 font-size
    KLBUI_listex_bind("font-size", on_klbui_listex_font_size);
    KLBUI_listex_bind("font-size:focus", on_klbui_listex_font_size_focus);
    KLBUI_listex_bind("font-size:disable", on_klbui_listex_font_size_disable);

    // 背景色 background-color
    KLBUI_listex_bind("background-color", on_klbui_listex_background_color);
    KLBUI_listex_bind("background-color:focus", on_klbui_listex_background_color_focus);
    KLBUI_listex_bind("background-color:disable", on_klbui_listex_background_color_disable);

    // 背景图片 background-image
    KLBUI_listex_bind("background-image", on_klbui_listex_background_image);
    KLBUI_listex_bind("background-image:focus", on_klbui_listex_background_image_focus);
    KLBUI_listex_bind("background-image:disable", on_klbui_listex_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_listex_bind("border-width", on_klbui_listex_border_width);
    KLBUI_listex_bind("border-width:focus", on_klbui_listex_border_width_focus);
    KLBUI_listex_bind("border-width:disable", on_klbui_listex_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_listex_bind("border-color", on_klbui_listex_border_color);
    KLBUI_listex_bind("border-color:focus", on_klbui_listex_border_color_focus);
    KLBUI_listex_bind("border-color:disable", on_klbui_listex_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_listex_bind("append_column", on_klbui_listex_append_column);
    KLBUI_listex_bind("append", on_klbui_listex_append);
    KLBUI_listex_bind("clear", on_klbui_listex_clear);
    KLBUI_listex_bind("clear_data", on_klbui_listex_clear_data);

    KLBUI_listex_bind("value", on_klbui_listex_value);

    KLBUI_listex_bind("event_wnd", on_klbui_listex_event_wnd);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_listex_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_listex_t));
    klbui_listex_t* p_list = (klbui_listex_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_listex_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_listex_destroy;
    p_wnd->vtable.on_set = klbui_listex_on_set;
    p_wnd->vtable.on_get = klbui_listex_on_get;

    // css
    klbwnd_listex_css_init(&p_list->css, p_gui);
    klbwnd_listex_set_css(p_wnd, &p_list->css);

    // 初始化CSS 支持的方法
    klbui_listex_init_func_map(p_wnd, p_list, p_gui);

    return p_wnd;
}
