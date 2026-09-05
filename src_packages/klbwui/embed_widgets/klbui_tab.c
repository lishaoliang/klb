// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/core/klbwui_widgets.h"
#include "klbwui/core/klbwui_css.h"
#include "klbwui/embed_wnd/klbwnd_tab.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_tab_t
/// @brief  Tab切换页
typedef struct klbui_tab_t_
{
    klbwnd_tab_t        tab;            ///< 必须首位, 保持内存一致

    klbwnd_tab_css_t*   p_globalcss;    ///< 全局公共的CSS
    klbwnd_tab_css_t*   p_mycss;        ///< 私有的CSS

    klb_map_t*          p_func_map;     ///< CSS属性函数表
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
    klbwnd_tab_deinit(p_wnd);

    // 退出css
    if (NULL != p_tab->p_mycss)
    {
        klbwnd_tab_css_deinit(p_tab->p_mycss);
        KLB_FREE(p_tab->p_mycss);
    }

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
// my css

// 新建私有CSS
static void new_my_css_klbui_tab(klb_wnd_t* p_wnd)
{
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;

    if (NULL == p_tab->p_mycss)
    {
        p_tab->p_mycss = KLB_MALLOCZ(klbwnd_tab_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_tab_css_copy(p_tab->p_mycss, p_tab->p_globalcss);
        klbwnd_tab_set_css(p_wnd, p_tab->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_tab_css_t* get_css_klbui_tab(klb_wnd_t* p_wnd)
{
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;
    return (NULL != p_tab->p_mycss) ? p_tab->p_mycss : p_tab->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_tab_css_t* check_css_klbui_tab(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_tab(p_wnd);
    }

    return get_css_klbui_tab(p_wnd);
}


//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共 CSS 属性

static void globalcss_klbui_tab_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_text_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_text_color(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_text_align(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_text_align(&(p_css->normal.text), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_font_size(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_font_size(&(p_css->normal.font), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_tab_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_tab(void* ptr)
{
    klbwnd_tab_css_t* p_css = (klbwnd_tab_css_t*)ptr;
    klbwnd_tab_css_deinit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_tab_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_tab_init_globalcss(klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBWUI_ktab);
    if (NULL != ptr)
    {
        return;
    }

    // step2. 新添加 解析map, 及公共 CSS 对象
    ptr = klb_gui_new_globalcss_map(p_gui, KLBWUI_ktab);

    // 添加全局 CSS 
    klbwnd_tab_css_t* p_css = KLB_MALLOCZ(klbwnd_tab_css_t, 1, 0);
    klbwnd_tab_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBWUI_ktab, p_css, destroy_globalcss_klbwnd_tab);


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

    // 外边距 margin
    KLBUI_GLOBAL_tab_bind("margin", globalcss_klbui_tab_margin);
    KLBUI_GLOBAL_tab_bind("margin-top", globalcss_klbui_tab_margin_top);
    KLBUI_GLOBAL_tab_bind("margin-right", globalcss_klbui_tab_margin_right);
    KLBUI_GLOBAL_tab_bind("margin-bottom", globalcss_klbui_tab_margin_bottom);
    KLBUI_GLOBAL_tab_bind("margin-left", globalcss_klbui_tab_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_tab_bind("padding", globalcss_klbui_tab_padding);
    KLBUI_GLOBAL_tab_bind("padding-top", globalcss_klbui_tab_padding_top);
    KLBUI_GLOBAL_tab_bind("padding-right", globalcss_klbui_tab_padding_right);
    KLBUI_GLOBAL_tab_bind("padding-bottom", globalcss_klbui_tab_padding_bottom);
    KLBUI_GLOBAL_tab_bind("padding-left", globalcss_klbui_tab_padding_left);

    // 文本颜色 color
    KLBUI_GLOBAL_tab_bind("color", globalcss_klbui_tab_text_color);

    // 文本对齐 text-align
    KLBUI_GLOBAL_tab_bind("text-align", globalcss_klbui_tab_text_align);

    // 字体大小 font-size
    KLBUI_GLOBAL_tab_bind("font-size", globalcss_klbui_tab_font_size);

    // 背景色 background-color
    KLBUI_GLOBAL_tab_bind("background-color", globalcss_klbui_tab_background_color);

    // 背景图片 background-image
    KLBUI_GLOBAL_tab_bind("background-image", globalcss_klbui_tab_background_image);

    // 边框的宽度 border-width
    KLBUI_GLOBAL_tab_bind("border-width", globalcss_klbui_tab_border_width);

    // 边框的颜色 border-color
    KLBUI_GLOBAL_tab_bind("border-color", globalcss_klbui_tab_border_color);
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有 CSS 属性

static void on_klbui_tab_margin(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_top(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_right(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_bottom(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_margin_left(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_top(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_right(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_bottom(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_padding_left(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_text_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_text_color(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_text_align(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_text_align(&(p_css->normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_font_size(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_font_size(&(p_css->normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_background_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_background_image(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_border_width(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_tab_border_color(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_tab_css_t* p_css = check_css_klbui_tab(p_wnd, method);
    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 私有自定义

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

#define KLBUI_tab_bind(KEY_, FUNC_) { klb_map_set_ptr(p_tab->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbui_tab_init_func_map(klb_wnd_t* p_wnd, klbui_tab_t* p_tab, klb_gui_t* p_gui)
{
    // step1. 获取CSS 支持的方法
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBWUI_ktab);
    if (NULL != ptr)
    {
        p_tab->p_func_map = ptr;
        return;
    }

    // step2. 新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBWUI_ktab);
    p_tab->p_func_map = ptr;


    //////////////////////////////////////////////
    // step3. 绑定CSS 支持的方法

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

    // 私有自定义

    KLBUI_tab_bind("title", on_klbui_tab_title);
    KLBUI_tab_bind("value", on_klbui_tab_value);
}

//////////////////////////////////////////////////////////////////////////
// create, register

static klb_wnd_t* klbui_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_tab_t));
    klbui_tab_t* p_tab = (klbui_tab_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_tab_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_tab_destroy;
    p_wnd->vtable.on_set = klbui_tab_on_set;
    p_wnd->vtable.on_get = klbui_tab_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_tab_init_func_map(p_wnd, p_tab, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_tab->p_globalcss = (klbwnd_tab_css_t*)klb_gui_globalcss_get_ptr(p_gui, KLBWUI_ktab);
    klbwnd_tab_set_css(p_wnd, p_tab->p_globalcss);

    return p_wnd;
}

// 注册 "ktab"
int klbui_register_ktab(klb_gui_t* p_gui)
{
    klbui_tab_init_globalcss(p_gui); // 初始化全局CSS, 需要注册时初始化, 否则全局属性无法生效
    return klb_gui_register(p_gui, KLBWUI_ktab, klbui_tab_create);
}

// end
