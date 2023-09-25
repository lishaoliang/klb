// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/shwnd/klbshw_messagebox.h"
#include "klbgui/subviews/klbwnd_messagebox.h"
#include "klbmem/klb_mem.h"


/// @struct klbshw_messagebox_t
/// @brief  消息框
typedef struct klbshw_messagebox_t_
{
    klbwnd_messagebox_t     msgbox;         ///< 必须首位, 保持内存一致
    klbwnd_messagebox_css_t css;            ///< 样式

    klb_map_t*              p_func_map;     ///< CSS属性函数表
}klbshw_messagebox_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbshw_messagebox_cb)(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_messagebox_destroy(klb_wnd_t* p_wnd)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_messagebox_quit(p_wnd);

    // 退出css
    klbwnd_messagebox_css_quit(&p_msgbox->css);

    KLB_FREE(p_wnd);
}

static int klbshw_messagebox_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_messagebox_cb func = (klbshw_messagebox_cb)klb_map_to_ptr(p_msgbox->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_msgbox, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbshw_messagebox_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbshw_messagebox_cb func = (klbshw_messagebox_cb)klb_map_to_ptr(p_msgbox->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_msgbox, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbshw_messagebox_visibility(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_margin(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_margin_top(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_margin_right(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_margin_bottom(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_margin_left(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding_top(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding_right(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding_bottom(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding_left(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_color(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_msgbox->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_color_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_msgbox->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_color_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_msgbox->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_align(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_msgbox->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_align_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_msgbox->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_text_align_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_msgbox->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_font_size(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_msgbox->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_font_size_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_msgbox->css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_font_size_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_msgbox->css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_color(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_msgbox->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_color_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_msgbox->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_color_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_msgbox->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_image(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_msgbox->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_image_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_msgbox->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_image_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_msgbox->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_width(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_msgbox->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_width_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_msgbox->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_width_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_msgbox->css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_color(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_msgbox->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_color_focus(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_msgbox->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_color_disable(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_msgbox->css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbshw_messagebox_title(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_msgbox->msgbox.title), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_value(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        int value = klbwnd_messagebox_get_value(p_wnd);

        if (KLBWND_MESSAGEBOX_ok == value)
        {
            klb_map_set_idx_string(p_out, 0, "ok");
        }
        else if(KLBWND_MESSAGEBOX_cancel == value)
        {
            klb_map_set_idx_string(p_out, 0, "cancel");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_msgbox_bind(KEY_, FUNC_) { klb_map_set_ptr(p_msgbox->p_func_map, (KEY_), (void*)(FUNC_), p_msgbox); }

static void klbshw_messagebox_init_func_map(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_messagebox);
    if (NULL != ptr)
    {
        p_msgbox->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBSHW_messagebox);
    p_msgbox->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_msgbox_bind("margin", on_klbshw_messagebox_margin);
    KLBUI_msgbox_bind("margin-top", on_klbshw_messagebox_margin_top);
    KLBUI_msgbox_bind("margin-right", on_klbshw_messagebox_margin_right);
    KLBUI_msgbox_bind("margin-bottom", on_klbshw_messagebox_margin_bottom);
    KLBUI_msgbox_bind("margin-left", on_klbshw_messagebox_margin_left);

    // 内边距 padding
    KLBUI_msgbox_bind("padding", on_klbshw_messagebox_padding);
    KLBUI_msgbox_bind("padding-top", on_klbshw_messagebox_padding_top);
    KLBUI_msgbox_bind("padding-right", on_klbshw_messagebox_padding_right);
    KLBUI_msgbox_bind("padding-bottom", on_klbshw_messagebox_padding_bottom);
    KLBUI_msgbox_bind("padding-left", on_klbshw_messagebox_padding_left);

    // 文本颜色 color
    KLBUI_msgbox_bind("color", on_klbshw_messagebox_text_color);
    KLBUI_msgbox_bind("color:focus", on_klbshw_messagebox_text_color_focus);
    KLBUI_msgbox_bind("color:disable", on_klbshw_messagebox_text_color_disable);

    // 文本对齐 text-align
    KLBUI_msgbox_bind("text-align", on_klbshw_messagebox_text_align);
    KLBUI_msgbox_bind("text-align:focus", on_klbshw_messagebox_text_align_focus);
    KLBUI_msgbox_bind("text-align:disable", on_klbshw_messagebox_text_align_disable);

    // 字体大小 font-size
    KLBUI_msgbox_bind("font-size", on_klbshw_messagebox_font_size);
    KLBUI_msgbox_bind("font-size:focus", on_klbshw_messagebox_font_size_focus);
    KLBUI_msgbox_bind("font-size:disable", on_klbshw_messagebox_font_size_disable);

    // 背景色 background-color
    KLBUI_msgbox_bind("background-color", on_klbshw_messagebox_background_color);
    KLBUI_msgbox_bind("background-color:focus", on_klbshw_messagebox_background_color_focus);
    KLBUI_msgbox_bind("background-color:disable", on_klbshw_messagebox_background_color_disable);

    // 背景图片 background-image
    KLBUI_msgbox_bind("background-image", on_klbshw_messagebox_background_image);
    KLBUI_msgbox_bind("background-image:focus", on_klbshw_messagebox_background_image_focus);
    KLBUI_msgbox_bind("background-image:disable", on_klbshw_messagebox_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_msgbox_bind("border-width", on_klbshw_messagebox_border_width);
    KLBUI_msgbox_bind("border-width:focus", on_klbshw_messagebox_border_width_focus);
    KLBUI_msgbox_bind("border-width:disable", on_klbshw_messagebox_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_msgbox_bind("border-color", on_klbshw_messagebox_border_color);
    KLBUI_msgbox_bind("border-color:focus", on_klbshw_messagebox_border_color_focus);
    KLBUI_msgbox_bind("border-color:disable", on_klbshw_messagebox_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_msgbox_bind("title", on_klbshw_messagebox_title);
    KLBUI_msgbox_bind("value", on_klbshw_messagebox_value);
}

//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_messagebox_t));
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_messagebox_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbshw_messagebox_destroy;
    p_wnd->vtable.on_set = klbshw_messagebox_on_set;
    p_wnd->vtable.on_get = klbshw_messagebox_on_get;

    // css
    klbwnd_messagebox_css_init(&p_msgbox->css, p_gui);
    klbwnd_messagebox_set_css(p_wnd, &p_msgbox->css);

    // 初始化CSS 支持的方法
    klbshw_messagebox_init_func_map(p_wnd, p_msgbox, p_gui);

    return p_wnd;
}

//////////////////////////////////////////////////////////////////////////
// 导出公共函数 


/// @brief 获取"messagebox"弹出对话框
klb_wnd_t* klbui_shwnd_get_messagebox(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_messagebox);

    if (NULL == p_wnd)
    {
        int w = 480, h = 240;
        klbshw_messagebox_wh(p_gui, &w, &h);

        p_wnd = klbshw_messagebox_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_messagebox, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

/// @brief 获取消息框所需要的宽高
int klbshw_messagebox_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{
    if (NULL != p_out_w)
    {
        *p_out_w = 480;
    }

    if(NULL != p_out_h)
    {
        *p_out_h = 240;
    }

    return 0;
}

void klbshw_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_set_title(p_wnd, p_title);
}

void klbshw_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_set_body_text(p_wnd, p_text);
}

int klbshw_messagebox_get_value(klb_wnd_t* p_wnd)
{
    return klbwnd_messagebox_get_value(p_wnd);
}
