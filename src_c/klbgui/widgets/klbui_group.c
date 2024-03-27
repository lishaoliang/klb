// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_group.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_group_t
/// @brief  分组框
typedef struct klbui_group_t_
{
    klbwnd_group_t      group;          ///< 必须首位, 保持内存一致

    klbwnd_group_css_t* p_globalcss;    ///< 全局公共的CSS
    klbwnd_group_css_t* p_mycss;        ///< 私有的CSS

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_group_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_group_cb)(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_group_destroy(klb_wnd_t* p_wnd)
{
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_group_quit(p_wnd);

    // 退出css
    if (NULL != p_group->p_mycss)
    {
        klbwnd_group_css_quit(p_group->p_mycss);
        KLB_FREE(p_group->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_group_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_group_cb func = (klbui_group_cb)klb_map_to_ptr(p_group->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_group, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_group_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_group_cb func = (klbui_group_cb)klb_map_to_ptr(p_group->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_group, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// my css

// 新建私有CSS
static void new_my_css_klbui_group(klb_wnd_t* p_wnd)
{
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;

    if (NULL == p_group->p_mycss)
    {
        p_group->p_mycss = KLB_MALLOCZ(klbwnd_group_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_group_css_copy(p_group->p_mycss, p_group->p_globalcss);
        klbwnd_group_set_css(p_wnd, p_group->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_group_css_t* get_css_klbui_group(klb_wnd_t* p_wnd)
{
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;
    return (NULL != p_group->p_mycss) ? p_group->p_mycss : p_group->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_group_css_t* check_css_klbui_group(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_group(p_wnd);
    }

    return get_css_klbui_group(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共CSS属性

static void globalcss_klbui_group_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_group_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_group(void* ptr)
{
    klbwnd_group_css_t* p_css = (klbwnd_group_css_t*)ptr;
    klbwnd_group_css_quit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_group_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_group_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBUI_kgroup);
    if (NULL != ptr)
    {
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_globalcss_map(p_gui, KLBUI_kgroup);

    // 添加全局 CSS 
    klbwnd_group_css_t* p_css = KLB_MALLOCZ(klbwnd_group_css_t, 1, 0);
    klbwnd_group_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBUI_kgroup, p_css, destroy_globalcss_klbwnd_group);

    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法 - 全局CSS属性

    // 外边距 margin
    KLBUI_GLOBAL_group_bind("margin", globalcss_klbui_group_margin);
    KLBUI_GLOBAL_group_bind("margin-top", globalcss_klbui_group_margin_top);
    KLBUI_GLOBAL_group_bind("margin-right", globalcss_klbui_group_margin_right);
    KLBUI_GLOBAL_group_bind("margin-bottom", globalcss_klbui_group_margin_bottom);
    KLBUI_GLOBAL_group_bind("margin-left", globalcss_klbui_group_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_group_bind("padding", globalcss_klbui_group_padding);
    KLBUI_GLOBAL_group_bind("padding-top", globalcss_klbui_group_padding_top);
    KLBUI_GLOBAL_group_bind("padding-right", globalcss_klbui_group_padding_right);
    KLBUI_GLOBAL_group_bind("padding-bottom", globalcss_klbui_group_padding_bottom);
    KLBUI_GLOBAL_group_bind("padding-left", globalcss_klbui_group_padding_left);

    // 文本颜色 color
    KLBUI_GLOBAL_group_bind("color", globalcss_klbui_group_text_color);
    KLBUI_GLOBAL_group_bind("color:focus", globalcss_klbui_group_text_color_focus);
    KLBUI_GLOBAL_group_bind("color:disable", globalcss_klbui_group_text_color_disable);

    // 文本对齐 text-align
    KLBUI_GLOBAL_group_bind("text-align", globalcss_klbui_group_text_align);
    KLBUI_GLOBAL_group_bind("text-align:focus", globalcss_klbui_group_text_align_focus);
    KLBUI_GLOBAL_group_bind("text-align:disable", globalcss_klbui_group_text_align_disable);

    // 字体大小 font-size
    KLBUI_GLOBAL_group_bind("font-size", globalcss_klbui_group_font_size);
    KLBUI_GLOBAL_group_bind("font-size:focus", globalcss_klbui_group_font_size_focus);
    KLBUI_GLOBAL_group_bind("font-size:disable", globalcss_klbui_group_font_size_disable);

    // 背景色 background-color
    KLBUI_GLOBAL_group_bind("background-color", globalcss_klbui_group_background_color);
    KLBUI_GLOBAL_group_bind("background-color:focus", globalcss_klbui_group_background_color_focus);
    KLBUI_GLOBAL_group_bind("background-color:disable", globalcss_klbui_group_background_color_disable);

    // 背景图片 background-image
    KLBUI_GLOBAL_group_bind("background-image", globalcss_klbui_group_background_image);
    KLBUI_GLOBAL_group_bind("background-image:focus", globalcss_klbui_group_background_image_focus);
    KLBUI_GLOBAL_group_bind("background-image:disable", globalcss_klbui_group_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_GLOBAL_group_bind("border-width", globalcss_klbui_group_border_width);
    KLBUI_GLOBAL_group_bind("border-width:focus", globalcss_klbui_group_border_width_focus);
    KLBUI_GLOBAL_group_bind("border-width:disable", globalcss_klbui_group_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_GLOBAL_group_bind("border-color", globalcss_klbui_group_border_color);
    KLBUI_GLOBAL_group_bind("border-color:focus", globalcss_klbui_group_border_color_focus);
    KLBUI_GLOBAL_group_bind("border-color:disable", globalcss_klbui_group_border_color_disable);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有CSS属性

static void on_klbui_group_margin(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_margin_top(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_margin_right(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_margin_bottom(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_margin_left(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_padding(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_padding_top(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_padding_right(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_padding_bottom(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_padding_left(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_color(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_color_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_color_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_align(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_align_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_text_align_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_font_size(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_font_size_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_font_size_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_color(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_color_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_color_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_image(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_image_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_background_image_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_width(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_width_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_width_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_color(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_color_focus(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_border_color_disable(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_group_css_t* p_css = check_css_klbui_group(p_wnd, method);
    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_group_title(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_group->group.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_group_value(klb_wnd_t* p_wnd, klbui_group_t* p_group, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_group->group.value), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_group_bind(KEY_, FUNC_) { klb_map_set_ptr(p_group->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_group_init_func_map(klb_wnd_t* p_wnd, klbui_group_t* p_group, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kgroup);
    if (NULL != ptr)
    {
        p_group->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kgroup);
    p_group->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_group_bind("margin", on_klbui_group_margin);
    KLBUI_group_bind("margin-top", on_klbui_group_margin_top);
    KLBUI_group_bind("margin-right", on_klbui_group_margin_right);
    KLBUI_group_bind("margin-bottom", on_klbui_group_margin_bottom);
    KLBUI_group_bind("margin-left", on_klbui_group_margin_left);

    // 内边距 padding
    KLBUI_group_bind("padding", on_klbui_group_padding);
    KLBUI_group_bind("padding-top", on_klbui_group_padding_top);
    KLBUI_group_bind("padding-right", on_klbui_group_padding_right);
    KLBUI_group_bind("padding-bottom", on_klbui_group_padding_bottom);
    KLBUI_group_bind("padding-left", on_klbui_group_padding_left);

    // 文本颜色 color
    KLBUI_group_bind("color", on_klbui_group_text_color);
    KLBUI_group_bind("color:focus", on_klbui_group_text_color_focus);
    KLBUI_group_bind("color:disable", on_klbui_group_text_color_disable);

    // 文本对齐 text-align
    KLBUI_group_bind("text-align", on_klbui_group_text_align);
    KLBUI_group_bind("text-align:focus", on_klbui_group_text_align_focus);
    KLBUI_group_bind("text-align:disable", on_klbui_group_text_align_disable);

    // 字体大小 font-size
    KLBUI_group_bind("font-size", on_klbui_group_font_size);
    KLBUI_group_bind("font-size:focus", on_klbui_group_font_size_focus);
    KLBUI_group_bind("font-size:disable", on_klbui_group_font_size_disable);

    // 背景色 background-color
    KLBUI_group_bind("background-color", on_klbui_group_background_color);
    KLBUI_group_bind("background-color:focus", on_klbui_group_background_color_focus);
    KLBUI_group_bind("background-color:disable", on_klbui_group_background_color_disable);

    // 背景图片 background-image
    KLBUI_group_bind("background-image", on_klbui_group_background_image);
    KLBUI_group_bind("background-image:focus", on_klbui_group_background_image_focus);
    KLBUI_group_bind("background-image:disable", on_klbui_group_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_group_bind("border-width", on_klbui_group_border_width);
    KLBUI_group_bind("border-width:focus", on_klbui_group_border_width_focus);
    KLBUI_group_bind("border-width:disable", on_klbui_group_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_group_bind("border-color", on_klbui_group_border_color);
    KLBUI_group_bind("border-color:focus", on_klbui_group_border_color_focus);
    KLBUI_group_bind("border-color:disable", on_klbui_group_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_group_bind("title", on_klbui_group_title);
    KLBUI_group_bind("value", on_klbui_group_value);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_group_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_group_t));
    klbui_group_t* p_group = (klbui_group_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_group_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_group_destroy;
    p_wnd->vtable.on_set = klbui_group_on_set;
    p_wnd->vtable.on_get = klbui_group_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_group_init_func_map(p_wnd, p_group, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_group->p_globalcss = (klbwnd_group_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBUI_kgroup);
    klbwnd_group_set_css(p_wnd, p_group->p_globalcss);

    return p_wnd;
}

// 注册 "kgroup"
int klbui_register_kgroup(klb_gui_t* p_gui)
{
    klbui_group_init_globalcss(p_gui); // 初始化全局CSS, 需要注册时初始化, 否则全局属性无法生效
    return klb_gui_register(p_gui, KLBUI_kgroup, klbui_group_create);
}
