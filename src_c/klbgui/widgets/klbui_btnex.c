// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_btnex.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_btnex_t
/// @brief  扩展按钮
typedef struct klbui_btnex_t_
{
    klbwnd_btnex_t      btnex;          ///< 必须首位, 保持内存一致

    klbwnd_btnex_css_t* p_globalcss;    ///< 全局公共的CSS
    klbwnd_btnex_css_t* p_mycss;        ///< 私有的CSS

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_btnex_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_btnex_cb)(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_btnex_destroy(klb_wnd_t* p_wnd)
{
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_btnex_quit(p_wnd);

    // 退出css
    if (NULL != p_btnex->p_mycss)
    {
        klbwnd_btnex_css_quit(p_btnex->p_mycss);
        KLB_FREE(p_btnex->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_btnex_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_btnex_cb func = (klbui_btnex_cb)klb_map_to_ptr(p_btnex->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btnex, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_btnex_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_btnex_cb func = (klbui_btnex_cb)klb_map_to_ptr(p_btnex->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btnex, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

// 新建私有CSS
static void new_my_css_klbui_btnex(klb_wnd_t* p_wnd)
{
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;

    if (NULL == p_btnex->p_mycss)
    {
        p_btnex->p_mycss = KLB_MALLOCZ(klbwnd_btnex_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_btnex_css_copy(p_btnex->p_mycss, p_btnex->p_globalcss);
        klbwnd_btnex_set_css(p_wnd, p_btnex->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_btnex_css_t* get_css_klbui_btnex(klb_wnd_t* p_wnd)
{
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;
    return (NULL != p_btnex->p_mycss) ? p_btnex->p_mycss : p_btnex->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_btnex_css_t* check_css_klbui_btnex(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_btnex(p_wnd);
    }

    return get_css_klbui_btnex(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共CSS属性

static void globalcss_klbui_btnex_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_color(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_color(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_align_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_align(&(p_css->focus.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_text_align_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_text_align(&(p_css->disable.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_font_size_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_font_size(&(p_css->focus.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_font_size_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_font_size(&(p_css->disable.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_color(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_color(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_image(&(p_css->focus.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_background_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_background_image(&(p_css->disable.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_width_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_width(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_width_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_width(&(p_css->disable.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_color(&(p_css->focus.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_btnex_border_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbuicssex_border_color(&(p_css->disable.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_btnex(void* ptr)
{
    klbwnd_btnex_css_t* p_css = (klbwnd_btnex_css_t*)ptr;
    klbwnd_btnex_css_quit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_btnex_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_btnex_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBUI_kbtnex);
    if (NULL != ptr)
    {
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_globalcss_map(p_gui, KLBUI_kbtnex);

    // 添加全局 CSS 
    klbwnd_btnex_css_t* p_css = KLB_MALLOCZ(klbwnd_btnex_css_t, 1, 0);
    klbwnd_btnex_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBUI_kbtnex, p_css, destroy_globalcss_klbwnd_btnex);

    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法 - 全局CSS属性

    // 外边距 margin
    KLBUI_GLOBAL_btnex_bind("margin", globalcss_klbui_btnex_margin);
    KLBUI_GLOBAL_btnex_bind("margin-top", globalcss_klbui_btnex_margin_top);
    KLBUI_GLOBAL_btnex_bind("margin-right", globalcss_klbui_btnex_margin_right);
    KLBUI_GLOBAL_btnex_bind("margin-bottom", globalcss_klbui_btnex_margin_bottom);
    KLBUI_GLOBAL_btnex_bind("margin-left", globalcss_klbui_btnex_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_btnex_bind("padding", globalcss_klbui_btnex_padding);
    KLBUI_GLOBAL_btnex_bind("padding-top", globalcss_klbui_btnex_padding_top);
    KLBUI_GLOBAL_btnex_bind("padding-right", globalcss_klbui_btnex_padding_right);
    KLBUI_GLOBAL_btnex_bind("padding-bottom", globalcss_klbui_btnex_padding_bottom);
    KLBUI_GLOBAL_btnex_bind("padding-left", globalcss_klbui_btnex_padding_left);

    // 文本颜色 color
    KLBUI_GLOBAL_btnex_bind("color", globalcss_klbui_btnex_text_color);
    KLBUI_GLOBAL_btnex_bind("color:focus", globalcss_klbui_btnex_text_color_focus);
    KLBUI_GLOBAL_btnex_bind("color:disable", globalcss_klbui_btnex_text_color_disable);

    // 文本对齐 text-align
    KLBUI_GLOBAL_btnex_bind("text-align", globalcss_klbui_btnex_text_align);
    KLBUI_GLOBAL_btnex_bind("text-align:focus", globalcss_klbui_btnex_text_align_focus);
    KLBUI_GLOBAL_btnex_bind("text-align:disable", globalcss_klbui_btnex_text_align_disable);

    // 字体大小 font-size
    KLBUI_GLOBAL_btnex_bind("font-size", globalcss_klbui_btnex_font_size);
    KLBUI_GLOBAL_btnex_bind("font-size:focus", globalcss_klbui_btnex_font_size_focus);
    KLBUI_GLOBAL_btnex_bind("font-size:disable", globalcss_klbui_btnex_font_size_disable);

    // 背景色 background-color
    KLBUI_GLOBAL_btnex_bind("background-color", globalcss_klbui_btnex_background_color);
    KLBUI_GLOBAL_btnex_bind("background-color:focus", globalcss_klbui_btnex_background_color_focus);
    KLBUI_GLOBAL_btnex_bind("background-color:disable", globalcss_klbui_btnex_background_color_disable);

    // 背景图片 background-image
    KLBUI_GLOBAL_btnex_bind("background-image", globalcss_klbui_btnex_background_image);
    KLBUI_GLOBAL_btnex_bind("background-image:focus", globalcss_klbui_btnex_background_image_focus);
    KLBUI_GLOBAL_btnex_bind("background-image:disable", globalcss_klbui_btnex_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_GLOBAL_btnex_bind("border-width", globalcss_klbui_btnex_border_width);
    KLBUI_GLOBAL_btnex_bind("border-width:focus", globalcss_klbui_btnex_border_width_focus);
    KLBUI_GLOBAL_btnex_bind("border-width:disable", globalcss_klbui_btnex_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_GLOBAL_btnex_bind("border-color", globalcss_klbui_btnex_border_color);
    KLBUI_GLOBAL_btnex_bind("border-color:focus", globalcss_klbui_btnex_border_color_focus);
    KLBUI_GLOBAL_btnex_bind("border-color:disable", globalcss_klbui_btnex_border_color_disable);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有CSS属性

static void on_klbui_btnex_margin(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_margin_top(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_margin_right(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_margin_bottom(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_margin_left(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_padding(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_padding_top(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_padding_right(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_padding_bottom(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_padding_left(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_color(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_color_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_color(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_color_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_color(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_align(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_align_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_align(&(p_css->focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_text_align_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_text_align(&(p_css->disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_font_size(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_font_size_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_font_size(&(p_css->focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_font_size_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_font_size(&(p_css->disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_color(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_color_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_color(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_color_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_color(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_image(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_image_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_image(&(p_css->focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_background_image_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_background_image(&(p_css->disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_width(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_width_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_width(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_width_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_width(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_color(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_color_focus(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_color(&(p_css->focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_border_color_disable(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_btnex_css_t* p_css = check_css_klbui_btnex(p_wnd, method);
    klbuicssex_border_color(&(p_css->disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_btnex_index(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_btnex->btnex.index), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_title(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_btnex->btnex.title), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_value_str(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_btnex->btnex.value), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_value_int(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_btnex->btnex.i_value), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_value_bool(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_bool(&(p_btnex->btnex.b_value), p_wnd, method, p_in, p_out);
}

static void on_klbui_btnex_graph(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // CSS 的 ["type"] 表示控件类型
    // 这里 用 ["graph"] 替代

    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_string(p_out, 0, klbwnd_btnex_get_type_str(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        if (KLB_ADT_string == klb_map_array_type(p_in, start))
        {
            const char* p_type = klb_map_idx_to_string(p_in, start);
            klbwnd_btnex_set_type_str(p_wnd, p_type);

            klb_wnd_update(p_wnd);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_btnex_bind(KEY_, FUNC_) { klb_map_set_ptr(p_btnex->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_btnex_init_func_map(klb_wnd_t* p_wnd, klbui_btnex_t* p_btnex, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kbtnex);
    if (NULL != ptr)
    {
        p_btnex->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kbtnex);
    p_btnex->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_btnex_bind("margin", on_klbui_btnex_margin);
    KLBUI_btnex_bind("margin-top", on_klbui_btnex_margin_top);
    KLBUI_btnex_bind("margin-right", on_klbui_btnex_margin_right);
    KLBUI_btnex_bind("margin-bottom", on_klbui_btnex_margin_bottom);
    KLBUI_btnex_bind("margin-left", on_klbui_btnex_margin_left);

    // 内边距 padding
    KLBUI_btnex_bind("padding", on_klbui_btnex_padding);
    KLBUI_btnex_bind("padding-top", on_klbui_btnex_padding_top);
    KLBUI_btnex_bind("padding-right", on_klbui_btnex_padding_right);
    KLBUI_btnex_bind("padding-bottom", on_klbui_btnex_padding_bottom);
    KLBUI_btnex_bind("padding-left", on_klbui_btnex_padding_left);

    // 文本颜色 color
    KLBUI_btnex_bind("color", on_klbui_btnex_text_color);
    KLBUI_btnex_bind("color:focus", on_klbui_btnex_text_color_focus);
    KLBUI_btnex_bind("color:disable", on_klbui_btnex_text_color_disable);

    // 文本对齐 text-align
    KLBUI_btnex_bind("text-align", on_klbui_btnex_text_align);
    KLBUI_btnex_bind("text-align:focus", on_klbui_btnex_text_align_focus);
    KLBUI_btnex_bind("text-align:disable", on_klbui_btnex_text_align_disable);

    // 字体大小 font-size
    KLBUI_btnex_bind("font-size", on_klbui_btnex_font_size);
    KLBUI_btnex_bind("font-size:focus", on_klbui_btnex_font_size_focus);
    KLBUI_btnex_bind("font-size:disable", on_klbui_btnex_font_size_disable);

    // 背景色 background-color
    KLBUI_btnex_bind("background-color", on_klbui_btnex_background_color);
    KLBUI_btnex_bind("background-color:focus", on_klbui_btnex_background_color_focus);
    KLBUI_btnex_bind("background-color:disable", on_klbui_btnex_background_color_disable);

    // 背景图片 background-image
    KLBUI_btnex_bind("background-image", on_klbui_btnex_background_image);
    KLBUI_btnex_bind("background-image:focus", on_klbui_btnex_background_image_focus);
    KLBUI_btnex_bind("background-image:disable", on_klbui_btnex_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_btnex_bind("border-width", on_klbui_btnex_border_width);
    KLBUI_btnex_bind("border-width:focus", on_klbui_btnex_border_width_focus);
    KLBUI_btnex_bind("border-width:disable", on_klbui_btnex_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_btnex_bind("border-color", on_klbui_btnex_border_color);
    KLBUI_btnex_bind("border-color:focus", on_klbui_btnex_border_color_focus);
    KLBUI_btnex_bind("border-color:disable", on_klbui_btnex_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_btnex_bind("index", on_klbui_btnex_index);
    KLBUI_btnex_bind("title", on_klbui_btnex_title);
    KLBUI_btnex_bind("value-str", on_klbui_btnex_value_str);
    KLBUI_btnex_bind("value-int", on_klbui_btnex_value_int);
    KLBUI_btnex_bind("value-bool", on_klbui_btnex_value_bool);

    KLBUI_btnex_bind("graph", on_klbui_btnex_graph)
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_btnex_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_btnex_t));
    klbui_btnex_t* p_btnex = (klbui_btnex_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_btnex_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_btnex_destroy;
    p_wnd->vtable.on_set = klbui_btnex_on_set;
    p_wnd->vtable.on_get = klbui_btnex_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_btnex_init_func_map(p_wnd, p_btnex, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_btnex->p_globalcss = (klbwnd_btnex_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBUI_kbtnex);
    klbwnd_btnex_set_css(p_wnd, p_btnex->p_globalcss);

    return p_wnd;
}

// 注册 "kbtnex"
int klbui_register_kbtnex(klb_gui_t* p_gui)
{
    klbui_btnex_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBUI_kbtnex, klbui_btnex_create);
}
