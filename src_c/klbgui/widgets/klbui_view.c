// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_view.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_view_t
/// @brief  视图组件: 类似wnd的 替代名称
typedef struct klbui_view_t_
{
    klbwnd_view_t       view;           ///< 必须首位, 保持内存一致

    klbwnd_view_css_t*  p_globalcss;    ///< 全局公共的CSS
    klbwnd_view_css_t*  p_mycss;        ///< 私有的CSS

    klb_map_t*          p_func_map;     ///< CSS属性函数表
}klbui_view_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_view_cb)(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_view_destroy(klb_wnd_t* p_wnd)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_view_quit(p_wnd);

    // 退出css
    if (NULL != p_view->p_mycss)
    {
        klbwnd_view_css_quit(p_view->p_mycss);
        KLB_FREE(p_view->p_mycss);
    }

    KLB_FREE(p_wnd);
}

static int klbui_view_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_view_cb func = (klbui_view_cb)klb_map_to_ptr(p_view->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_view, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_view_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_view_cb func = (klbui_view_cb)klb_map_to_ptr(p_view->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_view, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}


//////////////////////////////////////////////////////////////////////////
// my css

// 新建私有CSS
static void new_my_css_klbui_view(klb_wnd_t* p_wnd)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    if (NULL == p_view->p_mycss)
    {
        p_view->p_mycss = KLB_MALLOCZ(klbwnd_view_css_t, 1, 0);

        // 从新建时, 复制公共参数; 后面分开
        klbwnd_view_css_copy(p_view->p_mycss, p_view->p_globalcss);
        klbwnd_view_set_css(p_wnd, p_view->p_mycss);
    }
}

// 获取当前 CSS
static klbwnd_view_css_t* get_css_klbui_view(klb_wnd_t* p_wnd)
{
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;
    return (NULL != p_view->p_mycss) ? p_view->p_mycss : p_view->p_globalcss;
}

// 检查方法, 并获取当前的CSS
static klbwnd_view_css_t* check_css_klbui_view(klb_wnd_t* p_wnd, int method)
{
    if (KLBUI_CSSEX_set == method)
    {
        // 只要是 set-css 相关方法, 立即转换为使用私有CSS
        new_my_css_klbui_view(p_wnd);
    }

    return get_css_klbui_view(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// global CSS 方法 - 公共CSS属性

static void globalcss_klbui_view_margin(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_margin(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_margin_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_margin_top(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_margin_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_margin_right(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_margin_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_margin_bottom(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_margin_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_margin_left(&(p_css->margin), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_padding(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_padding(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_padding_top(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_padding_top(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_padding_right(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_padding_right(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_padding_bottom(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_padding_bottom(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_padding_left(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_padding_left(&(p_css->padding), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_background_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_background_color(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_background_image(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_background_image(&(p_css->normal.background), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_border_width(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_border_width(&(p_css->normal.border), NULL, method, p_in, p_out);
}

static void globalcss_klbui_view_border_color(void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbuicssex_border_color(&(p_css->normal.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////

/// @brief 销毁 全局公共CSS属性
static void destroy_globalcss_klbwnd_view(void* ptr)
{
    klbwnd_view_css_t* p_css = (klbwnd_view_css_t*)ptr;
    klbwnd_view_css_quit(p_css);
    KLB_FREE(p_css);
}

#define KLBUI_GLOBAL_view_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (void*)(FUNC_), NULL); }

void klbui_view_init_globalcss(klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_globalcss_map(p_gui, KLBUI_kview);
    if (NULL != ptr)
    {
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_globalcss_map(p_gui, KLBUI_kview);

    // 添加全局 CSS 
    klbwnd_view_css_t* p_css = KLB_MALLOCZ(klbwnd_view_css_t, 1, 0);
    klbwnd_view_css_init(p_css, p_gui);
    klb_gui_globalcss_set_ptr(p_gui, KLBUI_kview, p_css, destroy_globalcss_klbwnd_view);

    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法 - 全局CSS属性

    // 外边距 margin
    KLBUI_GLOBAL_view_bind("margin", globalcss_klbui_view_margin);
    KLBUI_GLOBAL_view_bind("margin-top", globalcss_klbui_view_margin_top);
    KLBUI_GLOBAL_view_bind("margin-right", globalcss_klbui_view_margin_right);
    KLBUI_GLOBAL_view_bind("margin-bottom", globalcss_klbui_view_margin_bottom);
    KLBUI_GLOBAL_view_bind("margin-left", globalcss_klbui_view_margin_left);

    // 内边距 padding
    KLBUI_GLOBAL_view_bind("padding", globalcss_klbui_view_padding);
    KLBUI_GLOBAL_view_bind("padding-top", globalcss_klbui_view_padding_top);
    KLBUI_GLOBAL_view_bind("padding-right", globalcss_klbui_view_padding_right);
    KLBUI_GLOBAL_view_bind("padding-bottom", globalcss_klbui_view_padding_bottom);
    KLBUI_GLOBAL_view_bind("padding-left", globalcss_klbui_view_padding_left);

    // 背景色 background-color
    KLBUI_GLOBAL_view_bind("background-color", globalcss_klbui_view_background_color);

    // 背景图片 background-image
    KLBUI_GLOBAL_view_bind("background-image", globalcss_klbui_view_background_image);

    // 边框的宽度 border-width
    KLBUI_GLOBAL_view_bind("border-width", globalcss_klbui_view_border_width);

    // 边框的颜色 border-color
    KLBUI_GLOBAL_view_bind("border-color", globalcss_klbui_view_border_color);
}


//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法 -- 私有CSS属性

static void on_klbui_view_margin(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_margin(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_top(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_margin_top(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_right(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_margin_right(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_bottom(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_margin_bottom(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_margin_left(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_margin_left(&(p_css->margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_padding(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_top(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_padding_top(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_right(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_padding_right(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_bottom(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_padding_bottom(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_padding_left(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_padding_left(&(p_css->padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_background_color(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_background_color(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_background_image(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_background_image(&(p_css->normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_width(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_border_width(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_view_border_color(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbwnd_view_css_t* p_css = check_css_klbui_view(p_wnd, method);
    klbuicssex_border_color(&(p_css->normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////
// 自定义属性

static void on_klbui_view_title(klb_wnd_t* p_wnd, klbui_view_t* p_view, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_attribute_sds(&(p_view->view.title), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_view_bind(KEY_, FUNC_) { klb_map_set_ptr(p_view->p_func_map, (KEY_), (void*)(FUNC_), p_view); }

static void klbui_view_init_func_map(klb_wnd_t* p_wnd, klbui_view_t* p_view, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kview);
    if (NULL != ptr)
    {
        p_view->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kview);
    p_view->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 外边距 margin
    KLBUI_view_bind("margin", on_klbui_view_margin);
    KLBUI_view_bind("margin-top", on_klbui_view_margin_top);
    KLBUI_view_bind("margin-right", on_klbui_view_margin_right);
    KLBUI_view_bind("margin-bottom", on_klbui_view_margin_bottom);
    KLBUI_view_bind("margin-left", on_klbui_view_margin_left);

    // 内边距 padding
    KLBUI_view_bind("padding", on_klbui_view_padding);
    KLBUI_view_bind("padding-top", on_klbui_view_padding_top);
    KLBUI_view_bind("padding-right", on_klbui_view_padding_right);
    KLBUI_view_bind("padding-bottom", on_klbui_view_padding_bottom);
    KLBUI_view_bind("padding-left", on_klbui_view_padding_left);

    // 背景色 background-color
    KLBUI_view_bind("background-color", on_klbui_view_background_color);

    // 背景图片 background-image
    KLBUI_view_bind("background-image", on_klbui_view_background_image);

    // 边框的宽度 border-width
    KLBUI_view_bind("border-width", on_klbui_view_border_width);

    // 边框的颜色 border-color
    KLBUI_view_bind("border-color", on_klbui_view_border_color);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_view_bind("title", on_klbui_view_title);
    KLBUI_view_bind("value", on_klbui_view_title);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_view_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_view_t));
    klbui_view_t* p_view = (klbui_view_t*)p_wnd->ctrl;

    // step2. 初始化基础部分
    klbwnd_view_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbui_view_destroy;
    p_wnd->vtable.on_set = klbui_view_on_set;
    p_wnd->vtable.on_get = klbui_view_on_get;

    // step4. 初始化CSS 支持的方法
    klbui_view_init_func_map(p_wnd, p_view, p_gui);

    // step5. 默认使用公用全局CSS属性
    p_view->p_globalcss = klb_gui_globalcss_get_ptr(p_gui, KLBUI_kview);
    klbwnd_view_set_css(p_wnd, p_view->p_globalcss);

    return p_wnd;
}
