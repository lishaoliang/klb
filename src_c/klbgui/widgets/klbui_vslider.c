// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_vslider.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_vslider_t
/// @brief  垂直滑动条
typedef struct klbui_vslider_t_
{
    klbwnd_vslider_t        vslider;        ///< 必须首位, 保持内存一致

    klbwnd_vslider_css_t*   p_globalcss;    ///< 全局公共的CSS
    klbwnd_vslider_css_t*   p_mycss;        ///< 私有的CSS

    klb_map_t*              p_func_map;     ///< CSS属性函数表
}klbui_vslider_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_vslider_cb)(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_vslider_destroy(klb_wnd_t* p_wnd)
{
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_vslider_quit(p_wnd);

    // 退出css
    if (NULL != p_vslider->p_mycss)
    {
        klbwnd_vslider_css_quit(p_vslider->p_mycss);
        KLB_FREE(p_vslider->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_vslider_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_vslider_cb func = (klbui_vslider_cb)klb_map_to_ptr(p_vslider->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_vslider, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_vslider_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_vslider_cb func = (klbui_vslider_cb)klb_map_to_ptr(p_vslider->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_vslider, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// my css

// 新建私有CSS
static void new_my_css_klbui_vslider(klb_wnd_t* p_wnd)
{
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;

    if (NULL == p_vslider->p_mycss)
    {
        p_vslider->p_mycss = KLB_MALLOCZ(klbwnd_vslider_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_vslider_css_copy(p_vslider->p_mycss, p_vslider->p_globalcss);
        klbwnd_vslider_set_css(p_wnd, p_vslider->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_vslider_css_t* get_css_klbui_vslider(klb_wnd_t* p_wnd)
{
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;
    return (NULL != p_vslider->p_mycss) ? p_vslider->p_mycss : p_vslider->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_vslider_css_t* check_css_klbui_vslider(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_vslider(p_wnd);
    }

    return get_css_klbui_vslider(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共CSS属性

static void globalcss_klbui_vslider_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_text_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_text_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_background_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_background_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.background_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_foreground_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_foreground_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_foreground_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.foreground_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_foreground_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->foreground_width), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->normal.thumb_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_color_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->focus.thumb_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_color_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_color(&(p_css->disable.thumb_color), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->normal.thumb_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_image_focus(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->focus.thumb_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_image_disable(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_image(&(p_css->disable.thumb_image), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->thumb_width), NULL, method, p_in, p_out);
}

static void globalcss_klbui_vslider_thumb_height(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbuicssex_attribute_int(&(p_css->thumb_height), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_vslider(void* ptr)
{
    klbwnd_vslider_css_t* p_css = (klbwnd_vslider_css_t*)ptr;
    klbwnd_vslider_css_quit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_vslider_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_vslider_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBUI_kvslider);
    if (NULL != ptr)
    {
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_globalcss_map(p_gui, KLBUI_kvslider);

    // 添加全局 CSS 
    klbwnd_vslider_css_t* p_css = KLB_MALLOCZ(klbwnd_vslider_css_t, 1, 0);
    klbwnd_vslider_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBUI_kvslider, p_css, destroy_globalcss_klbwnd_vslider);

    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法 - 全局CSS属性

    // 外边距 margin
    KLBUI_GLOBAL_vslider_bind("margin", globalcss_klbui_vslider_margin);
    KLBUI_GLOBAL_vslider_bind("margin-top", globalcss_klbui_vslider_margin_top);
    KLBUI_GLOBAL_vslider_bind("margin-right", globalcss_klbui_vslider_margin_right);
    KLBUI_GLOBAL_vslider_bind("margin-bottom", globalcss_klbui_vslider_margin_bottom);
    KLBUI_GLOBAL_vslider_bind("margin-left", globalcss_klbui_vslider_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_vslider_bind("padding", globalcss_klbui_vslider_padding);
    KLBUI_GLOBAL_vslider_bind("padding-top", globalcss_klbui_vslider_padding_top);
    KLBUI_GLOBAL_vslider_bind("padding-right", globalcss_klbui_vslider_padding_right);
    KLBUI_GLOBAL_vslider_bind("padding-bottom", globalcss_klbui_vslider_padding_bottom);
    KLBUI_GLOBAL_vslider_bind("padding-left", globalcss_klbui_vslider_padding_left);

    // 滑块颜色 color
    KLBUI_GLOBAL_vslider_bind("color", globalcss_klbui_vslider_text_color);
    KLBUI_GLOBAL_vslider_bind("color:focus", globalcss_klbui_vslider_text_color_focus);
    KLBUI_GLOBAL_vslider_bind("color:disable", globalcss_klbui_vslider_text_color_disable);

    // 背景色 background-color
    KLBUI_GLOBAL_vslider_bind("background-color", globalcss_klbui_vslider_background_color);
    KLBUI_GLOBAL_vslider_bind("background-color:focus", globalcss_klbui_vslider_background_color_focus);
    KLBUI_GLOBAL_vslider_bind("background-color:disable", globalcss_klbui_vslider_background_color_disable);

    // 滑块区域前景色: foreground-color
    KLBUI_GLOBAL_vslider_bind("foreground-color", globalcss_klbui_vslider_foreground_color);
    KLBUI_GLOBAL_vslider_bind("foreground-color:focus", globalcss_klbui_vslider_foreground_color_focus);
    KLBUI_GLOBAL_vslider_bind("foreground-color:disable", globalcss_klbui_vslider_foreground_color_disable);

    // 滑块区域前景宽度: foreground-width
    KLBUI_GLOBAL_vslider_bind("foreground-width", globalcss_klbui_vslider_foreground_width);

    // 子区域: 滑块 - color
    KLBUI_GLOBAL_vslider_bind("thumb.color", globalcss_klbui_vslider_thumb_color);
    KLBUI_GLOBAL_vslider_bind("thumb.color:focus", globalcss_klbui_vslider_thumb_color_focus);
    KLBUI_GLOBAL_vslider_bind("thumb.color:disable", globalcss_klbui_vslider_thumb_color_disable);

    // 子区域: 滑块 - 图片
    KLBUI_GLOBAL_vslider_bind("thumb.image", globalcss_klbui_vslider_thumb_image);
    KLBUI_GLOBAL_vslider_bind("thumb.image:focus", globalcss_klbui_vslider_thumb_image_focus);
    KLBUI_GLOBAL_vslider_bind("thumb.image:disable", globalcss_klbui_vslider_thumb_image_disable);

    // 子区域: 滑块 - 宽度 / 高度
    KLBUI_GLOBAL_vslider_bind("thumb.width", globalcss_klbui_vslider_thumb_width);
    KLBUI_GLOBAL_vslider_bind("thumb.height", globalcss_klbui_vslider_thumb_height);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有CSS属性

static void on_klbui_vslider_margin(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_margin_top(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_margin_right(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_margin_bottom(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_margin_left(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_padding(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_padding_top(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_padding_right(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_padding_bottom(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_padding_left(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_text_color(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_text_color_focus(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_text_color_disable(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_background_color(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_background_color_focus(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_background_color_disable(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.background_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_foreground_color(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_foreground_color_focus(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_foreground_color_disable(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.foreground_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_foreground_width(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->foreground_width), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_color(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->normal.thumb_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_color_focus(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->focus.thumb_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_color_disable(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_color(&(p_css->disable.thumb_color), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_image(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->normal.thumb_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_image_focus(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->focus.thumb_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_image_disable(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_image(&(p_css->disable.thumb_image), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_width(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->thumb_width), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_thumb_height(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_vslider_css_t* p_css = check_css_klbui_vslider(p_wnd, method);
    klbuicssex_attribute_int(&(p_css->thumb_height), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_vslider_value(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klbwnd_vslider_get_value(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                klbwnd_vslider_set_value(p_wnd, (int)klb_map_idx_to_int64(p_in, start));

                if (NULL != p_wnd)
                {
                    klb_wnd_update(p_wnd);
                }
            }
            break;
        case KLB_ADT_uint64:
            {
                klbwnd_vslider_set_value(p_wnd, (int)klb_map_idx_to_uint64(p_in, start));

                if (NULL != p_wnd)
                {
                    klb_wnd_update(p_wnd);
                }
            }
            break;
        default:
            break;
        }
    }
}

static void on_klbui_vslider_min(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_vslider->vslider.min), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_max(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_vslider->vslider.max), p_wnd, method, p_in, p_out);
}

static void on_klbui_vslider_step(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_int(&(p_vslider->vslider.step), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_vslider_bind(KEY_, FUNC_) { klb_map_set_ptr(p_vslider->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_vslider_init_func_map(klb_wnd_t* p_wnd, klbui_vslider_t* p_vslider, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kvslider);
    if (NULL != ptr)
    {
        p_vslider->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kvslider);
    p_vslider->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_vslider_bind("margin", on_klbui_vslider_margin);
    KLBUI_vslider_bind("margin-top", on_klbui_vslider_margin_top);
    KLBUI_vslider_bind("margin-right", on_klbui_vslider_margin_right);
    KLBUI_vslider_bind("margin-bottom", on_klbui_vslider_margin_bottom);
    KLBUI_vslider_bind("margin-left", on_klbui_vslider_margin_left);

    // 内边距 padding
    KLBUI_vslider_bind("padding", on_klbui_vslider_padding);
    KLBUI_vslider_bind("padding-top", on_klbui_vslider_padding_top);
    KLBUI_vslider_bind("padding-right", on_klbui_vslider_padding_right);
    KLBUI_vslider_bind("padding-bottom", on_klbui_vslider_padding_bottom);
    KLBUI_vslider_bind("padding-left", on_klbui_vslider_padding_left);

    // 滑块颜色 color
    KLBUI_vslider_bind("color", on_klbui_vslider_text_color);
    KLBUI_vslider_bind("color:focus", on_klbui_vslider_text_color_focus);
    KLBUI_vslider_bind("color:disable", on_klbui_vslider_text_color_disable);

    // 背景色 background-color
    KLBUI_vslider_bind("background-color", on_klbui_vslider_background_color);
    KLBUI_vslider_bind("background-color:focus", on_klbui_vslider_background_color_focus);
    KLBUI_vslider_bind("background-color:disable", on_klbui_vslider_background_color_disable);

    // 滑块区域前景色: foreground-color
    KLBUI_vslider_bind("foreground-color", on_klbui_vslider_foreground_color);
    KLBUI_vslider_bind("foreground-color:focus", on_klbui_vslider_foreground_color_focus);
    KLBUI_vslider_bind("foreground-color:disable", on_klbui_vslider_foreground_color_disable);

    // 滑块区域前景宽度: foreground-width
    KLBUI_vslider_bind("foreground-width", on_klbui_vslider_foreground_width);

    // 子区域: 滑块 - color
    KLBUI_vslider_bind("thumb.color", on_klbui_vslider_thumb_color);
    KLBUI_vslider_bind("thumb.color:focus", on_klbui_vslider_thumb_color_focus);
    KLBUI_vslider_bind("thumb.color:disable", on_klbui_vslider_thumb_color_disable);

    // 子区域: 滑块 - 图片
    KLBUI_vslider_bind("thumb.image", on_klbui_vslider_thumb_image);
    KLBUI_vslider_bind("thumb.image:focus", on_klbui_vslider_thumb_image_focus);
    KLBUI_vslider_bind("thumb.image:disable", on_klbui_vslider_thumb_image_disable);

    // 子区域: 滑块 - 宽度 / 高度
    KLBUI_vslider_bind("thumb.width", on_klbui_vslider_thumb_width);
    KLBUI_vslider_bind("thumb.height", on_klbui_vslider_thumb_height);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_vslider_bind("value", on_klbui_vslider_value);
    KLBUI_vslider_bind("min", on_klbui_vslider_min);
    KLBUI_vslider_bind("max", on_klbui_vslider_max);
    KLBUI_vslider_bind("step", on_klbui_vslider_step);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_vslider_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_vslider_t));
    klbui_vslider_t* p_vslider = (klbui_vslider_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_vslider_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_vslider_destroy;
    p_wnd->vtable.on_set = klbui_vslider_on_set;
    p_wnd->vtable.on_get = klbui_vslider_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_vslider_init_func_map(p_wnd, p_vslider, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_vslider->p_globalcss = (klbwnd_vslider_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBUI_kvslider);
    klbwnd_vslider_set_css(p_wnd, p_vslider->p_globalcss);

    return p_wnd;
}

// 注册 "kvslider"
int klbui_register_kvslider(klb_gui_t* p_gui)
{
    klbui_vslider_init_globalcss(p_gui);
    return klb_gui_register(p_gui, KLBUI_kvslider, klbui_vslider_create);
}
