// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_color.h"


/// @def   KLBUI_BTN_get
/// @brief 从控件中获取函数
#define KLBUI_BTN_get       0


/// @def   KLBUI_BTN_set
/// @brief 设置控件函数
#define KLBUI_BTN_set       1


/// @struct klbui_button_status_t
/// @brief  按钮 可变状态的 CSS属性: "normal", "focus", "disable"
typedef struct klbui_button_status_t_
{
    klbuicss_text_t             text;           ///< 文本属性
    klbuicss_font_t             font;           ///< 字体属性
    klbuicss_background_t       background;     ///< 背景属性
    klbuicss_border_t           border;         ///< 边框属性
}klbui_button_status_t;


/// @struct klbui_button_t
/// @brief  常规按钮
typedef struct klbui_button_t_
{
    sds                         title;          ///< 标题

    // normal
    klbuicss_margin_t           margin;         ///< 外边框
    klbuicss_padding_t          padding;        ///< 内边框

    klbui_button_status_t       normal;         ///< normal 常规状态参数
    klbui_button_status_t       focus;          ///< focus 聚焦状态参数
    klbui_button_status_t       disable;        ///< disable 不使能状态参数

    klb_map_t                   map_func;       ///< 支持的 方法 集合  
}klbui_button_t;


typedef void(*klbui_button_cb)(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out);
static void klbui_button_quit_attribute(klbui_button_t* p_btn);

static void klbui_button_destroy(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    // 退出方法集
    klb_map_quit(&p_btn->map_func);
    
    // 清理属性
    klbui_button_quit_attribute(p_btn);

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

    // button的绘图区域
    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_btn->margin.left;
    paint_rect.y += p_btn->margin.top;
    paint_rect.w -= (p_btn->margin.left + p_btn->margin.right);
    paint_rect.h -= (p_btn->margin.top + p_btn->margin.bottom);

    uint32_t text_color = p_btn->normal.text.color;
    int font_size = p_btn->normal.font.size;

    if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        // 聚焦状态
        if (0 < sdslen(p_btn->focus.background.image))
        {
            klb_wnd_draw_image(p_wnd, &paint_rect, p_btn->focus.background.image, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect2(p_wnd, &paint_rect, p_btn->focus.background.color);
        }

        text_color = p_btn->focus.text.color;
        font_size = p_btn->focus.font.size;
    }
    else
    {
        // 普通状态
        if (0 < sdslen(p_btn->normal.background.image))
        {
            klb_wnd_draw_image(p_wnd, &paint_rect, p_btn->normal.background.image, NULL);
        }
        else
        {
            klb_wnd_draw_fill_rect2(p_wnd, &paint_rect, p_btn->normal.background.color);

            // border
            klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_btn->normal.border.width.top };
            klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_btn->normal.border.color.top);

            klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_btn->normal.border.width.right, paint_rect.y, p_btn->normal.border.width.right, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_btn->normal.border.color.right);

            klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_btn->normal.border.width.bottom, paint_rect.w, p_btn->normal.border.width.bottom };
            klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_btn->normal.border.color.bottom);

            klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_btn->normal.border.width.left, paint_rect.h };
            klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_btn->normal.border.color.left);
        }

        text_color = p_btn->normal.text.color;
        font_size = p_btn->normal.font.size;
    }

    if (0 < sdslen(p_btn->title))
    {
        int x = p_rect->x + 2, y = p_rect->y + 2, w = p_rect->w - 4, h = p_rect->h - 4 - 2;

        klb_rect_t r = { x, y, w, h };
        klb_wnd_draw_text2(p_wnd, &r, p_btn->title, sdslen(p_btn->title), text_color, font_size);
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

static void on_klbui_button_visibility(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        if (p_wnd->state.status & KLB_WND_STATUS_HIDE)
        {
            klb_map_set_idx_string(p_out, 0, "hidden");
        }
        else
        {
            klb_map_set_idx_string(p_out, 0, "visible");
        }
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);

        switch (type)
        {
        case KLB_ADT_bool:
            {
                klb_wnd_show(p_wnd, klb_map_idx_to_bool(p_in, start));
            }
            break;
        case KLB_ADT_string:
            {
                const char* p_value = (const char*)klb_map_idx_to_string(p_in, start);
                if (0 == strcmp(p_value, "visible"))
                {
                    klb_wnd_show(p_wnd, true);
                }
                else if (0 == strcmp(p_value, "hidden"))
                {
                    klb_wnd_show(p_wnd, false);
                }
            }
            break;
        case KLB_ADT_int64:
            {
                klb_wnd_show(p_wnd, (0 == klb_map_idx_to_int64(p_in, start)) ? false : true);
            }
            break;
        case KLB_ADT_uint64:
            {
                klb_wnd_show(p_wnd, (0 == klb_map_idx_to_uint64(p_in, start)) ? false : true);
            }
            break;
        default:
            break;
        }
    }
}

static void on_klbui_button_text_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->normal.text.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->normal.text.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_text_color_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->focus.text.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->focus.text.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_text_align(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
    }
}

static void on_klbui_button_text_align_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
    }
}

static void on_klbui_button_font_size(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->normal.font.size);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        p_btn->normal.font.size = (int)klb_map_idx_to_int64(p_in, start);

        klb_wnd_update(p_wnd);
    }
}

static void on_klbui_button_font_size_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->focus.font.size);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        p_btn->focus.font.size = (int)klb_map_idx_to_int64(p_in, start);

        klb_wnd_update(p_wnd);
    }
}

static void on_klbui_button_background_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->normal.background.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->normal.background.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_color_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_btn->focus.background.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_btn->focus.background.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_image(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, p_btn->normal.background.image);
    } 
    else if(KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);
        p_btn->normal.background.image = sdscpy(p_btn->normal.background.image, p_value);

        klb_wnd_update(p_wnd);
    }
}

static void on_klbui_button_background_image_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, p_btn->focus.background.image);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);
        p_btn->focus.background.image = sdscpy(p_btn->focus.background.image, p_value);

        klb_wnd_update(p_wnd);
    }
}

//////////////////////////////////////////////////////////////////////////


static void klbui_button_status_init()
{

}

// 初始化属性默认值
static void klbui_button_init_attribute(klb_wnd_t* p_wnd, klbui_button_t* p_btn)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_btn->title = sdsempty();

    // normal 常规状态参数
    p_btn->normal.text.color = p_default->text_color;
    p_btn->normal.font.size = p_default->font_size;
    p_btn->normal.background.color = p_default->background_color;
    p_btn->normal.background.image = sdsempty();

    p_btn->normal.border.width.top = p_default->border_width;
    p_btn->normal.border.width.right = p_default->border_width;
    p_btn->normal.border.width.bottom = p_default->border_width;
    p_btn->normal.border.width.left = p_default->border_width;
    p_btn->normal.border.color.top = p_default->border_color;
    p_btn->normal.border.color.right = p_default->border_color;
    p_btn->normal.border.color.bottom = p_default->border_color;
    p_btn->normal.border.color.left = p_default->border_color;
    p_btn->normal.border.radius = p_default->border_radius;

    // focus 聚焦状态参数
    p_btn->focus.text.color = p_default->text_color;
    p_btn->focus.font.size = p_default->font_size;
    p_btn->focus.background.color = p_default->background_color;
    p_btn->focus.background.image = sdsempty();

    p_btn->focus.border.width.top = p_default->border_width;
    p_btn->focus.border.width.right = p_default->border_width;
    p_btn->focus.border.width.bottom = p_default->border_width;
    p_btn->focus.border.width.left = p_default->border_width;
    p_btn->focus.border.color.top = p_default->border_color;
    p_btn->focus.border.color.right = p_default->border_color;
    p_btn->focus.border.color.bottom = p_default->border_color;
    p_btn->focus.border.color.left = p_default->border_color;
    p_btn->focus.border.radius = p_default->border_radius;

    // disable 不使能状态参数
    p_btn->disable.text.color = p_default->text_color;
    p_btn->disable.font.size = p_default->font_size;
    p_btn->disable.background.color = p_default->background_color;
    p_btn->disable.background.image = sdsempty();

    p_btn->disable.border.width.top = p_default->border_width;
    p_btn->disable.border.width.right = p_default->border_width;
    p_btn->disable.border.width.bottom = p_default->border_width;
    p_btn->disable.border.width.left = p_default->border_width;
    p_btn->disable.border.color.top = p_default->border_color;
    p_btn->disable.border.color.right = p_default->border_color;
    p_btn->disable.border.color.bottom = p_default->border_color;
    p_btn->disable.border.color.left = p_default->border_color;
    p_btn->disable.border.radius = p_default->border_radius;

}

static void klbui_button_quit_attribute(klbui_button_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE_BY(p_btn->normal.background.image, sdsfree);
    KLB_FREE_BY(p_btn->focus.background.image, sdsfree);
    KLB_FREE_BY(p_btn->disable.background.image, sdsfree);
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


    //////////////////////////////////////////////
    // 仿 CSS 方法

    //
    KLBUI_btn_bind("visibility", on_klbui_button_visibility);

    // text
    KLBUI_btn_bind("color", on_klbui_button_text_color);
    KLBUI_btn_bind("color:focus", on_klbui_button_text_color_focus);

    KLBUI_btn_bind("text-align", on_klbui_button_text_align);
    KLBUI_btn_bind("text-align:focus", on_klbui_button_text_align_focus);

    // font
    KLBUI_btn_bind("font-size", on_klbui_button_font_size);
    KLBUI_btn_bind("font-size:focus", on_klbui_button_font_size_focus);

    // background
    KLBUI_btn_bind("background-color", on_klbui_button_background_color);
    KLBUI_btn_bind("background-color:focus", on_klbui_button_background_color_focus);
    KLBUI_btn_bind("background-image", on_klbui_button_background_image);
    KLBUI_btn_bind("background-image:focus", on_klbui_button_background_image_focus);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_btn_bind("title", on_klbui_button_title);
    KLBUI_btn_bind("value", on_klbui_button_title);

    return p_wnd;
}
