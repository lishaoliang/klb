// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_list.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_list_t
/// @brief  简易列表框
typedef struct klbui_list_t_
{
    klbwnd_list_t       list;           ///< 必须首位, 保持内存一致

    klbwnd_list_css_t*  p_globalcss;    ///< 全局公共的CSS
    klbwnd_list_css_t*  p_mycss;        ///< 私有的CSS

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_list_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_list_cb)(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_list_destroy(klb_wnd_t* p_wnd)
{
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_list_quit(p_wnd);

    // 退出css
    if (NULL != p_list->p_mycss)
    {
        klbwnd_list_css_quit(p_list->p_mycss);
        KLB_FREE(p_list->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_list_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_list_cb func = (klbui_list_cb)klb_map_to_ptr(p_list->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_list, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_list_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_list_cb func = (klbui_list_cb)klb_map_to_ptr(p_list->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_list, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// my css

// 新建私有CSS
static void new_my_css_klbui_list(klb_wnd_t* p_wnd)
{
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;

    if (NULL == p_list->p_mycss)
    {
        p_list->p_mycss = KLB_MALLOCZ(klbwnd_list_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_list_css_copy(p_list->p_mycss, p_list->p_globalcss);
        klbwnd_list_set_css(p_wnd, p_list->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_list_css_t* get_css_klbui_list(klb_wnd_t* p_wnd)
{
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;
    return (NULL != p_list->p_mycss) ? p_list->p_mycss : p_list->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_list_css_t* check_css_klbui_list(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_list(p_wnd);
    }

    return get_css_klbui_list(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共CSS属性

static void globalcss_klbui_list_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_list_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_list(void* ptr)
{
    klbwnd_list_css_t* p_css = (klbwnd_list_css_t*)ptr;
    klbwnd_list_css_quit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_list_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_list_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBUI_klist);
    if (NULL != ptr)
    {
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_globalcss_map(p_gui, KLBUI_klist);

    // 添加全局 CSS 
    klbwnd_list_css_t* p_css = KLB_MALLOCZ(klbwnd_list_css_t, 1, 0);
    klbwnd_list_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBUI_klist, p_css, destroy_globalcss_klbwnd_list);

    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法 - 全局CSS属性

    // 外边距 margin
    KLBUI_GLOBAL_list_bind("margin", globalcss_klbui_list_margin);
    KLBUI_GLOBAL_list_bind("margin-top", globalcss_klbui_list_margin_top);
    KLBUI_GLOBAL_list_bind("margin-right", globalcss_klbui_list_margin_right);
    KLBUI_GLOBAL_list_bind("margin-bottom", globalcss_klbui_list_margin_bottom);
    KLBUI_GLOBAL_list_bind("margin-left", globalcss_klbui_list_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_list_bind("padding", globalcss_klbui_list_padding);
    KLBUI_GLOBAL_list_bind("padding-top", globalcss_klbui_list_padding_top);
    KLBUI_GLOBAL_list_bind("padding-right", globalcss_klbui_list_padding_right);
    KLBUI_GLOBAL_list_bind("padding-bottom", globalcss_klbui_list_padding_bottom);
    KLBUI_GLOBAL_list_bind("padding-left", globalcss_klbui_list_padding_left);

    // 文本颜色 color
    KLBUI_GLOBAL_list_bind("color", globalcss_klbui_list_text_color);
    KLBUI_GLOBAL_list_bind("color:focus", globalcss_klbui_list_text_color_focus);
    KLBUI_GLOBAL_list_bind("color:disable", globalcss_klbui_list_text_color_disable);

    // 文本对齐 text-align
    KLBUI_GLOBAL_list_bind("text-align", globalcss_klbui_list_text_align);
    KLBUI_GLOBAL_list_bind("text-align:focus", globalcss_klbui_list_text_align_focus);
    KLBUI_GLOBAL_list_bind("text-align:disable", globalcss_klbui_list_text_align_disable);

    // 字体大小 font-size
    KLBUI_GLOBAL_list_bind("font-size", globalcss_klbui_list_font_size);
    KLBUI_GLOBAL_list_bind("font-size:focus", globalcss_klbui_list_font_size_focus);
    KLBUI_GLOBAL_list_bind("font-size:disable", globalcss_klbui_list_font_size_disable);

    // 背景色 background-color
    KLBUI_GLOBAL_list_bind("background-color", globalcss_klbui_list_background_color);
    KLBUI_GLOBAL_list_bind("background-color:focus", globalcss_klbui_list_background_color_focus);
    KLBUI_GLOBAL_list_bind("background-color:disable", globalcss_klbui_list_background_color_disable);

    // 背景图片 background-image
    KLBUI_GLOBAL_list_bind("background-image", globalcss_klbui_list_background_image);
    KLBUI_GLOBAL_list_bind("background-image:focus", globalcss_klbui_list_background_image_focus);
    KLBUI_GLOBAL_list_bind("background-image:disable", globalcss_klbui_list_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_GLOBAL_list_bind("border-width", globalcss_klbui_list_border_width);
    KLBUI_GLOBAL_list_bind("border-width:focus", globalcss_klbui_list_border_width_focus);
    KLBUI_GLOBAL_list_bind("border-width:disable", globalcss_klbui_list_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_GLOBAL_list_bind("border-color", globalcss_klbui_list_border_color);
    KLBUI_GLOBAL_list_bind("border-color:focus", globalcss_klbui_list_border_color_focus);
    KLBUI_GLOBAL_list_bind("border-color:disable", globalcss_klbui_list_border_color_disable);

}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有CSS属性

static void on_klbui_list_margin(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_margin_top(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_margin_right(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_margin_bottom(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_margin_left(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_padding(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_padding_top(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_padding_right(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_padding_bottom(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_padding_left(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_color(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_color_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_color_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_align(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_align_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_text_align_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_font_size(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_font_size_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_font_size_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_color(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_color_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_color_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_image(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_image_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_background_image_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_width(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_width_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_width_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_color(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_color_focus(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_list_border_color_disable(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_list_css_t* p_css = check_css_klbui_list(p_wnd, method);
    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_list_append_column(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 添加列
    if (KLBUI_CSSEX_set == method)
    {
        /* eg.
        jq('list1')['append-column']({
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
                        klbwnd_list_append_column(p_wnd, width, klb_map_to_string(p_item_map, "title"));
                    }
                }
            }
        }
    }
}

static void on_klbui_list_append(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
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
            klb_map_t* p_data_map = klbwnd_list_get_data_map(p_wnd);

            int count = klb_map_array_size(p_in_array);
            for (int i = 0; i < count; i++)
            {
                klb_map_append_adt_clone(p_data_map, klb_map_idx_to_adt(p_in_array, i));
            }

            klbwnd_list_relayout(p_wnd); // 重新布局

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_list_clear(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 清空
    klbwnd_list_clear(p_wnd);
    klb_wnd_update(p_wnd);
}

static void on_klbui_list_clear_data(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 清空数据
    klbwnd_list_clear_data(p_wnd);
    klb_wnd_update(p_wnd);
}

static void on_klbui_list_value(klb_wnd_t* p_wnd, klbui_list_t* p_list, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 值
    if (KLBUI_CSSEX_get == method)
    {
        int sel = -1;
        klb_map_t* p_sel_map = klbwnd_list_get_sel(p_wnd, &sel);

        if (0 <= sel) sel += 1; // Lua从1开始

        klb_map_set_idx_int64(p_out, 0, sel);
        klb_map_set_idx_map_clone(p_out, 1, p_sel_map);
        
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;

        if (KLB_ADT_int64 == klb_map_array_type(p_in, start))
        {
            int sel = (int)klb_map_idx_to_int64(p_in, start);
            sel -= 1; // Lua从1开始

            klbwnd_list_set_sel(p_wnd, sel);
            klb_wnd_update(p_wnd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_list_bind(KEY_, FUNC_) { klb_map_set_ptr(p_list->p_func_map, (KEY_), (void*)(FUNC_), p_list); }

static void klbui_list_init_func_map(klb_wnd_t* p_wnd, klbui_list_t* p_list, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_klist);
    if (NULL != ptr)
    {
        p_list->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_klist);
    p_list->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_list_bind("margin", on_klbui_list_margin);
    KLBUI_list_bind("margin-top", on_klbui_list_margin_top);
    KLBUI_list_bind("margin-right", on_klbui_list_margin_right);
    KLBUI_list_bind("margin-bottom", on_klbui_list_margin_bottom);
    KLBUI_list_bind("margin-left", on_klbui_list_margin_left);

    // 内边距 padding
    KLBUI_list_bind("padding", on_klbui_list_padding);
    KLBUI_list_bind("padding-top", on_klbui_list_padding_top);
    KLBUI_list_bind("padding-right", on_klbui_list_padding_right);
    KLBUI_list_bind("padding-bottom", on_klbui_list_padding_bottom);
    KLBUI_list_bind("padding-left", on_klbui_list_padding_left);

    // 文本颜色 color
    KLBUI_list_bind("color", on_klbui_list_text_color);
    KLBUI_list_bind("color:focus", on_klbui_list_text_color_focus);
    KLBUI_list_bind("color:disable", on_klbui_list_text_color_disable);

    // 文本对齐 text-align
    KLBUI_list_bind("text-align", on_klbui_list_text_align);
    KLBUI_list_bind("text-align:focus", on_klbui_list_text_align_focus);
    KLBUI_list_bind("text-align:disable", on_klbui_list_text_align_disable);

    // 字体大小 font-size
    KLBUI_list_bind("font-size", on_klbui_list_font_size);
    KLBUI_list_bind("font-size:focus", on_klbui_list_font_size_focus);
    KLBUI_list_bind("font-size:disable", on_klbui_list_font_size_disable);

    // 背景色 background-color
    KLBUI_list_bind("background-color", on_klbui_list_background_color);
    KLBUI_list_bind("background-color:focus", on_klbui_list_background_color_focus);
    KLBUI_list_bind("background-color:disable", on_klbui_list_background_color_disable);

    // 背景图片 background-image
    KLBUI_list_bind("background-image", on_klbui_list_background_image);
    KLBUI_list_bind("background-image:focus", on_klbui_list_background_image_focus);
    KLBUI_list_bind("background-image:disable", on_klbui_list_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_list_bind("border-width", on_klbui_list_border_width);
    KLBUI_list_bind("border-width:focus", on_klbui_list_border_width_focus);
    KLBUI_list_bind("border-width:disable", on_klbui_list_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_list_bind("border-color", on_klbui_list_border_color);
    KLBUI_list_bind("border-color:focus", on_klbui_list_border_color_focus);
    KLBUI_list_bind("border-color:disable", on_klbui_list_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_list_bind("append_column", on_klbui_list_append_column);
    KLBUI_list_bind("append", on_klbui_list_append);
    KLBUI_list_bind("clear", on_klbui_list_clear);
    KLBUI_list_bind("clear_data", on_klbui_list_clear_data);

    KLBUI_list_bind("value", on_klbui_list_value);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_list_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_list_t));
    klbui_list_t* p_list = (klbui_list_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_list_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_list_destroy;
    p_wnd->vtable.on_set = klbui_list_on_set;
    p_wnd->vtable.on_get = klbui_list_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_list_init_func_map(p_wnd, p_list, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_list->p_globalcss = (klbwnd_list_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBUI_klist);
    klbwnd_list_set_css(p_wnd, p_list->p_globalcss);

    return p_wnd;
}

// 注册 "klist"
int klbui_register_klist(klb_gui_t* p_gui)
{
    klbui_list_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBUI_klist, klbui_list_create);
}
