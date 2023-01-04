// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_color.h"


#define KLBUI_BTN_get       0
#define KLBUI_BTN_set       1


/// @struct klbui_button_t
/// @brief  常规按钮
typedef struct klbui_button_t_
{
    sds         title;      // 标题

    sds         normal;     // 普通状态图
    sds         focus;      // 聚焦状态图
    sds         disable;    // 不使能状态图

    // normal
    klbuicss_margin_t       margin;
    klbuicss_border_t       border;
    klbuicss_padding_t      padding;

    klbuicss_text_t         text;
    klbuicss_font_t         font;
    klbuicss_background_t   bg;

    klb_map_t               map_func;     ///< 支持的 方法 集合  
}klbui_button_t;


typedef void(*klbui_button_cb)(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out);


static void klbui_button_destroy(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    klb_map_quit(&p_btn->map_func);

    KLB_FREE_BY(p_btn->title, sdsfree);

    KLB_FREE_BY(p_btn->normal, sdsfree);
    KLB_FREE_BY(p_btn->focus, sdsfree);
    KLB_FREE_BY(p_btn->disable, sdsfree);

    KLB_FREE(p_wnd);
}

static int klbui_button_on_paint(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        if (0 < sdslen(p_btn->normal))
        {
            klb_wnd_draw_image(p_wnd, p_rect, p_btn->focus, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 61, 71, 51));
            klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 180, 180, 180));
        }
    }
    else
    {
        if (0 < sdslen(p_btn->normal))
        {
            klb_wnd_draw_image(p_wnd, p_rect, p_btn->normal, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_btn->bg.color/*KLB_ARGB8888(255, 30, 30, 30)*/);
            klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 120, 120, 120));
        }
    }

    if (0 < sdslen(p_btn->title))
    {
        int x = p_rect->x + 2, y = p_rect->y + 2, w = p_rect->w - 4, h = p_rect->h - 4 - 2;

        klb_rect_t r = { x, y, w, h };
        klb_wnd_draw_text2(p_wnd, &r, p_btn->title, sdslen(p_btn->title), p_btn->text.color/*KLB_ARGB8888(255, 255, 128, 0)*/, h - 2);
    }

    return 0;
}

static int klbui_button_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_GUI_PAINT:
        return klbui_button_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

static int klbui_button_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        //KLB_LOG("klb_button_on_command\n");
        break;
    default:
        break;
    }

    return 0;
}

static int klbui_button_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_button_cb func = (klbui_button_cb)klb_map_to_ptr(&p_btn->map_func, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btn, KLBUI_BTN_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_button_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_button_cb func = (klbui_button_cb)klb_map_to_ptr(&p_btn->map_func, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_btn, KLBUI_BTN_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////


static void on_klbui_button_title(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, p_btn->title);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = (const char*)klb_map_idx_to_string(p_in, start);
        p_btn->title = sdscpy(p_btn->title, p_value);

        klb_wnd_update(p_wnd);
    }
}

static void on_klbui_button_text_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->text.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->text.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->bg.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->bg.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_image(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, p_btn->bg.image);
    } 
    else if(KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);
        p_btn->bg.image = sdscpy(p_btn->bg.image, p_value);

        klb_wnd_update(p_wnd);
    }
}

//////////////////////////////////////////////////////////////////////////

// 初始化属性默认值
static void klbui_button_init_attribute(klb_wnd_t* p_wnd, klbui_button_t* p_btn)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_btn->title = sdsempty();

    p_btn->normal = sdsempty();
    p_btn->focus = sdsempty();
    p_btn->disable = sdsempty();

    p_btn->text.color = p_default->text_color;

    p_btn->bg.color = p_default->background_color;
    p_btn->bg.image = sdsempty();
}

#define KLBUI_btn_bind(KEY_, FUNC_) { klb_map_set_ptr(&p_btn->map_func, (KEY_), (FUNC_), p_btn); }

klb_wnd_t* klbui_button_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_button_t));
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbui_button_destroy;
    p_wnd->vtable.on_control = klbui_button_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = klbui_button_on_set;
    p_wnd->vtable.on_get = klbui_button_on_get;

    p_wnd->p_gui = p_gui;

    // 初始化默认值
    klbui_button_init_attribute(p_wnd, p_btn);

    // 初始化 支持的方法
    klb_map_init(&p_btn->map_func);

    //
    KLBUI_btn_bind("title", on_klbui_button_title);

    // text
    KLBUI_btn_bind("color", on_klbui_button_text_color);

    // background
    KLBUI_btn_bind("background-color", on_klbui_button_background_color)
    KLBUI_btn_bind("background-image", on_klbui_button_background_image);

    return p_wnd;
}
