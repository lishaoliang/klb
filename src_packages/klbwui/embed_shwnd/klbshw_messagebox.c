// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbshw_messagebox.h"
#include "klbwui/embed_shwnd/klbwnd_messagebox.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klbshw_messagebox_t
/// @brief  消息框共享窗口
typedef struct klbshw_messagebox_t_
{
    klbwnd_messagebox_t         msgbox;         ///< 必须首位, 保持内存一致
    klbwnd_messagebox_css_t     css;            ///< 样式

    klb_map_t*                  p_func_map;     ///< CSS 属性函数表
} klbshw_messagebox_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

typedef void(*klbshw_messagebox_cb)(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_messagebox_destroy(klb_wnd_t* p_wnd)
{
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_deinit(p_wnd);
    klbwnd_messagebox_css_deinit(&p_msgbox->css);

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
// 仿 CSS 方法 -- 根

static void on_klbshw_messagebox_margin(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_msgbox->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_padding(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_msgbox->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_background_color(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_msgbox->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_width(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_msgbox->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbshw_messagebox_border_color(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_msgbox->css.normal.border), p_wnd, method, p_in, p_out);
}


//////////////////////////////////////////////////////////////////////////
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
        else if (KLBWND_MESSAGEBOX_cancel == value)
        {
            klb_map_set_idx_string(p_out, 0, "cancel");
        }
        else
        {
            klb_map_set_idx_string(p_out, 0, "close");
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// css func

#define KLBSHW_messagebox_bind(KEY_, FUNC_) { klb_map_set_ptr(p_msgbox->p_func_map, (KEY_), (void*)(FUNC_), NULL); }

static void klbshw_messagebox_init_func_map(klb_wnd_t* p_wnd, klbshw_messagebox_t* p_msgbox, klb_gui_t* p_gui)
{
    (void)p_wnd;

    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBSHW_messagebox);

    if (NULL != ptr)
    {
        p_msgbox->p_func_map = ptr;
        return;
    }

    ptr = klb_gui_new_css_map(p_gui, KLBSHW_messagebox);
    p_msgbox->p_func_map = ptr;

    // 根
    KLBSHW_messagebox_bind("margin", on_klbshw_messagebox_margin);
    KLBSHW_messagebox_bind("padding", on_klbshw_messagebox_padding);
    KLBSHW_messagebox_bind("background-color", on_klbshw_messagebox_background_color);
    KLBSHW_messagebox_bind("border-width", on_klbshw_messagebox_border_width);
    KLBSHW_messagebox_bind("border-color", on_klbshw_messagebox_border_color);

    // 自定义属性
    KLBSHW_messagebox_bind("title", on_klbshw_messagebox_title);
    KLBSHW_messagebox_bind("value", on_klbshw_messagebox_value);
}


//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_messagebox_t));
    klbshw_messagebox_t* p_msgbox = (klbshw_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbshw_messagebox_destroy;
    p_wnd->vtable.on_set = klbshw_messagebox_on_set;
    p_wnd->vtable.on_get = klbshw_messagebox_on_get;

    klbwnd_messagebox_css_init(&p_msgbox->css, p_gui);
    klbwnd_messagebox_set_css(p_wnd, &p_msgbox->css);

    klbshw_messagebox_init_func_map(p_wnd, p_msgbox, p_gui);

    return p_wnd;
}


//////////////////////////////////////////////////////////////////////////
// export 导出

klb_wnd_t* klbui_shwnd_get_messagebox(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_messagebox);

    if (NULL == p_wnd)
    {
        int w = 480;
        int h = 240;

        klbshw_messagebox_wh(p_gui, &w, &h);

        p_wnd = klbshw_messagebox_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_messagebox, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

int klbshw_messagebox_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{

    if (NULL != p_out_w)
    {
        *p_out_w = 480;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = 240;
    }

    return 0;
}

void klbshw_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_messagebox_set_title(p_wnd, p_title);
}

void klbshw_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text)
{
    klbwnd_messagebox_set_body_text(p_wnd, p_text);
}

int klbshw_messagebox_get_value(klb_wnd_t* p_wnd)
{
    return klbwnd_messagebox_get_value(p_wnd);
}

// end
