// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_widgets.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klbui_css.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_hmap.h"


/// @def   KLBUI_BTN_get
/// @brief 从控件中获取函数
#define KLBUI_BTN_get       0


/// @def   KLBUI_BTN_set
/// @brief 设置控件函数
#define KLBUI_BTN_set       1


/// @struct klbui_button_status_t
/// @brief  按钮 可变状态的 CSS属性: "normal", "focus", "disable"
typedef struct klbui_button_attribute_t_
{
    klbuicss_text_t             text;           ///< 文本属性
    klbuicss_font_t             font;           ///< 字体属性
    klbuicss_background_t       background;     ///< 背景属性
    klbuicss_border_t           border;         ///< 边框属性
}klbui_button_attribute_t;


/// @struct klbui_button_t
/// @brief  常规按钮
typedef struct klbui_button_t_
{
    sds                         title;          ///< 标题

    // normal
    klbuicss_margin_t           margin;         ///< 外边框
    klbuicss_padding_t          padding;        ///< 内边框

    klbui_button_attribute_t    normal;         ///< normal 常规状态参数
    klbui_button_attribute_t    focus;          ///< focus 聚焦状态参数
    klbui_button_attribute_t    disable;        ///< disable 不使能状态参数

    klb_map_t                   map_func;       ///< 支持的 方法 集合  
}klbui_button_t;

//////////////////////////////////////////////////////////////////////////

static void klbui_button_quit_attribute(klbui_button_t* p_btn);

typedef void(*klbui_button_cb)(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out);

static void klbui_button_destroy(klb_wnd_t* p_wnd)
{
    klbui_button_t* p_btn = (klbui_button_t*)p_wnd->ctrl;

    // 退出方法集
    klb_map_quit(&p_btn->map_func);
    
    // 清理属性
    klbui_button_quit_attribute(p_btn);

    KLB_FREE(p_wnd);
}

static void klbui_button_on_paint_status(klb_wnd_t* p_wnd, klbui_button_t* p_btn, klbui_button_attribute_t* p_attr, klb_rect_t* p_rect)
{
    // 普通状态
    if (0 < sdslen(p_attr->background.image))
    {
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }
    else
    {
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

        klb_rect_t paint_rect = *p_rect;

        // border
        klb_rect_t border_top = { paint_rect.x, paint_rect.y, paint_rect.w, p_attr->border.width.top };
        klb_wnd_draw_fill_rect2(p_wnd, &border_top, p_attr->border.color.top);

        klb_rect_t border_right = { paint_rect.x + paint_rect.w - p_attr->border.width.right, paint_rect.y, p_attr->border.width.right, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_right, p_attr->border.color.right);

        klb_rect_t border_bottom = { paint_rect.x, paint_rect.y + paint_rect.h - p_attr->border.width.bottom, paint_rect.w, p_attr->border.width.bottom };
        klb_wnd_draw_fill_rect2(p_wnd, &border_bottom, p_attr->border.color.bottom);

        klb_rect_t border_left = { paint_rect.x, paint_rect.y, p_attr->border.width.left, paint_rect.h };
        klb_wnd_draw_fill_rect2(p_wnd, &border_left, p_attr->border.color.left);
    }

    if (0 < sdslen(p_btn->title))
    {
        klb_rect_t text_rect = *p_rect;

        // 移除边框
        text_rect.x += p_attr->border.width.left;
        text_rect.y += p_attr->border.width.top;
        text_rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
        text_rect.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        // 移除内边距
        text_rect.x += p_btn->padding.left;
        text_rect.y += p_btn->padding.top;
        text_rect.w -= (p_btn->padding.left + p_btn->padding.right);
        text_rect.h -= (p_btn->padding.top + p_btn->padding.bottom);

        klb_wnd_draw_text2(p_wnd, &text_rect, p_btn->title, sdslen(p_btn->title), p_attr->text.color, p_attr->font.size);
    }
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

    // 移除外边框
    paint_rect.x += p_btn->margin.left;
    paint_rect.y += p_btn->margin.top;
    paint_rect.w -= (p_btn->margin.left + p_btn->margin.right);
    paint_rect.h -= (p_btn->margin.top + p_btn->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbui_button_on_paint_status(p_wnd, p_btn, &p_btn->disable, &paint_rect);
    }
    else if(KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbui_button_on_paint_status(p_wnd, p_btn, &p_btn->focus, &paint_rect);
    }
    else
    {
        klbui_button_on_paint_status(p_wnd, p_btn, &p_btn->normal, &paint_rect);
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
// 仿 CSS 方法

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

static void on_klbui_button_attribute_int(int* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, *p_attr);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_int64:
            {
                *p_attr = (int)klb_map_idx_to_int64(p_in, start);
            }
            break;
        case KLB_ADT_uint64:
            {
                *p_attr = (int)klb_map_idx_to_uint64(p_in, start);
            }
            break;
        default:
            break;
        }
    }
}

static void on_klbui_button_margin_top(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->margin.top, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_margin_right(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->margin.right, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_margin_bottom(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->margin.bottom, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_margin_left(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->margin.left, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_padding_top(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->padding.top, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_padding_right(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->padding.right, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_padding_bottom(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->padding.bottom, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_padding_left(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_int(&p_btn->padding.left, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_text_color(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_attr->text.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_attr->text.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_text_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_color(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_text_color_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_color(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_text_color_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_color(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_text_align(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        switch (p_attr->text.align)
        {       
        case KLBUICSS_text1_left:
            {
                klb_map_set_idx_string(p_out, 0, "left");
            }
            break;
        case KLBUICSS_text1_right:
            {
                klb_map_set_idx_string(p_out, 0, "right");
            }
            break;
        case KLBUICSS_text1_center:
        default:
            {
                klb_map_set_idx_string(p_out, 0, "center");
            }
            break;
        }
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "center"))
        {
            p_attr->text.align = KLBUICSS_text1_center;
            klb_wnd_update(p_wnd);
        }
        else if(0 == strcmp(p_value, "left"))
        {
            p_attr->text.align = KLBUICSS_text1_left;
            klb_wnd_update(p_wnd);
        }
        else if(0 == strcmp(p_value, "right"))
        {
            p_attr->text.align = KLBUICSS_text1_right;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_text_align(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_align(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_text_align_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_align(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_text_align_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_text_align(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_font_style(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        switch (p_attr->font.style)
        {
        case KLBUICSS_font1_normal:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        case KLBUICSS_font1_italic:
            {
                klb_map_set_idx_string(p_out, 0, "italic");
            }
            break;
        default:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        }
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "normal"))
        {
            p_attr->font.style = KLBUICSS_font1_normal;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "italic"))
        {
            p_attr->font.style = KLBUICSS_font1_italic;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_font_style(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_style(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_style_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_style(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_style_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_style(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_font_weight(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        switch (p_attr->font.weight)
        {
        case KLBUICSS_font2_normal:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        case KLBUICSS_font2_bold:
            {
                klb_map_set_idx_string(p_out, 0, "bold");
            }
            break;
        default:
            {
                klb_map_set_idx_string(p_out, 0, "normal");
            }
            break;
        }
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "normal"))
        {
            p_attr->font.weight = KLBUICSS_font2_normal;
            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "bold"))
        {
            p_attr->font.weight = KLBUICSS_font2_bold;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_font_weight(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_weight(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_weight_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_weight(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_weight_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_weight(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_font_size(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_attr->font.size);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        int type = klb_map_array_type(p_in, start);
        if (KLB_ADT_int64 == type)
        {
            p_attr->font.size = (int)klb_map_idx_to_int64(p_in, start);
            klb_wnd_update(p_wnd);
        }
        else if(KLB_ADT_uint64 == type)
        {
            p_attr->font.size = (int)klb_map_idx_to_uint64(p_in, start);
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_font_size(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_size(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_size_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_size(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_font_size_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_font_size(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_background_color(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_attr->background.color);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_attr->background.color = color;
            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_color(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_background_color_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_color(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_background_color_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_color(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_background_image(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, p_attr->background.image);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;

        if (KLB_ADT_string == klb_map_array_type(p_in, start))
        {
            const char* p_value = klb_map_idx_to_string(p_in, start);
            p_attr->background.image = sdscpy(p_attr->background.image, p_value);

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_background_image(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_image(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_background_image_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_image(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_background_image_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_background_image(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_border_style(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_string(p_out, 0, "normal");
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        const char* p_value = klb_map_idx_to_string(p_in, start);

        if (0 == strcmp(p_value, "none"))
        {
            p_attr->border.style.top = KLBUICSS_border1_none;
            p_attr->border.style.right = KLBUICSS_border1_none;
            p_attr->border.style.bottom = KLBUICSS_border1_none;
            p_attr->border.style.left = KLBUICSS_border1_none;

            klb_wnd_update(p_wnd);
        }
        else if (0 == strcmp(p_value, "dotted"))
        {
            p_attr->border.style.top = KLBUICSS_border1_dotted;
            p_attr->border.style.right = KLBUICSS_border1_dotted;
            p_attr->border.style.bottom = KLBUICSS_border1_dotted;
            p_attr->border.style.left = KLBUICSS_border1_dotted;

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_border_style(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_style(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_style_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_style(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_style_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_style(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_border_width(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, 1);
        klb_map_set_idx_int64(p_out, 1, 1);
        klb_map_set_idx_int64(p_out, 2, 1);
        klb_map_set_idx_int64(p_out, 3, 1);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        if (KLB_ADT_map == klb_map_array_type(p_in, start))
        {
            klb_map_t* ptr = klb_map_idx_to_map(p_in, start);
            p_attr->border.width.top = klb_map_idx_to_int64(ptr, 0);
            p_attr->border.width.right = klb_map_idx_to_int64(ptr, 1);
            p_attr->border.width.bottom = klb_map_idx_to_int64(ptr, 2);
            p_attr->border.width.left = klb_map_idx_to_int64(ptr, 3);

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_border_width(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_width(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_width_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_width(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_width_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_width(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_attribute_border_color(klbui_button_attribute_t* p_attr, klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_BTN_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, p_attr->border.color.top);
    }
    else if (KLBUI_BTN_set == method)
    {
        int start = 1;
        uint32_t color = 0;
        if (klb_gui_check_color(p_wnd->p_gui, p_in, start, &color))
        {
            p_attr->border.color.top = color;
            p_attr->border.color.right = color;
            p_attr->border.color.bottom = color;
            p_attr->border.color.left = color;

            klb_wnd_update(p_wnd);
        }
    }
}

static void on_klbui_button_border_color(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_color(&p_btn->normal, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_color_focus(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_color(&p_btn->focus, p_wnd, p_btn, method, p_in, p_out);
}

static void on_klbui_button_border_color_disable(klb_wnd_t* p_wnd, klbui_button_t* p_btn, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbui_button_attribute_border_color(&p_btn->disable, p_wnd, p_btn, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

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

//////////////////////////////////////////////////////////////////////////

static void klbui_button_status_init(klbui_button_attribute_t* p_status, const klbui_default_t* p_default)
{
    p_status->text.color = p_default->text_color;
    p_status->text.align = KLBUICSS_text1_center;

    p_status->font.style = KLBUICSS_font1_normal;
    p_status->font.weight = KLBUICSS_font2_normal;
    p_status->font.size = p_default->font_size;

    p_status->background.color = p_default->background_color;
    p_status->background.image = sdsempty();

    p_status->border.width.top = p_default->border_width;
    p_status->border.width.right = p_default->border_width;
    p_status->border.width.bottom = p_default->border_width;
    p_status->border.width.left = p_default->border_width;
    p_status->border.color.top = p_default->border_color;
    p_status->border.color.right = p_default->border_color;
    p_status->border.color.bottom = p_default->border_color;
    p_status->border.color.left = p_default->border_color;
    p_status->border.radius = p_default->border_radius;
}

// 初始化属性默认值
static void klbui_button_init_attribute(klb_wnd_t* p_wnd, klbui_button_t* p_btn)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

    p_btn->title = sdsempty();

    klbui_button_status_init(&p_btn->normal, p_default);
    klbui_button_status_init(&p_btn->focus, p_default);
    klbui_button_status_init(&p_btn->disable, p_default);
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

    // 显隐
    KLBUI_btn_bind("visibility", on_klbui_button_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_btn_bind("margin-top", on_klbui_button_margin_top);
    KLBUI_btn_bind("margin-right", on_klbui_button_margin_right);
    KLBUI_btn_bind("margin-bottom", on_klbui_button_margin_bottom);
    KLBUI_btn_bind("margin-left", on_klbui_button_margin_left);

    // 内边距 padding
    KLBUI_btn_bind("padding-top", on_klbui_button_padding_top);
    KLBUI_btn_bind("padding-right", on_klbui_button_padding_right);
    KLBUI_btn_bind("padding-bottom", on_klbui_button_padding_bottom);
    KLBUI_btn_bind("padding-left", on_klbui_button_padding_left);

    // 文本颜色 color
    KLBUI_btn_bind("color", on_klbui_button_text_color);
    KLBUI_btn_bind("color:focus", on_klbui_button_text_color_focus);
    KLBUI_btn_bind("color:disable", on_klbui_button_text_color_disable);

    // 文本对齐 text-align
    KLBUI_btn_bind("text-align", on_klbui_button_text_align);
    KLBUI_btn_bind("text-align:focus", on_klbui_button_text_align_focus);
    KLBUI_btn_bind("text-align:disable", on_klbui_button_text_align_disable);

    // 斜体 font-style
    KLBUI_btn_bind("font-style", on_klbui_button_font_style);
    KLBUI_btn_bind("font-style:focus", on_klbui_button_font_style_focus);
    KLBUI_btn_bind("font-style:disable", on_klbui_button_font_style_disable);

    // 字体粗细 font-weight
    KLBUI_btn_bind("font-weight", on_klbui_button_font_weight);
    KLBUI_btn_bind("font-weight:focus", on_klbui_button_font_weight_focus);
    KLBUI_btn_bind("font-weight:disable", on_klbui_button_font_weight_disable);

    // 字体大小 font-size
    KLBUI_btn_bind("font-size", on_klbui_button_font_size);
    KLBUI_btn_bind("font-size:focus", on_klbui_button_font_size_focus);
    KLBUI_btn_bind("font-size:disable", on_klbui_button_font_size_disable);

    // 背景色 background-color
    KLBUI_btn_bind("background-color", on_klbui_button_background_color);
    KLBUI_btn_bind("background-color:focus", on_klbui_button_background_color_focus);
    KLBUI_btn_bind("background-color:disable", on_klbui_button_background_color_disable);

    // 背景图片 background-image
    KLBUI_btn_bind("background-image", on_klbui_button_background_image);
    KLBUI_btn_bind("background-image:focus", on_klbui_button_background_image_focus);
    KLBUI_btn_bind("background-image:disable", on_klbui_button_background_image_disable);

    // 边框类型 border-style
    KLBUI_btn_bind("border-style", on_klbui_button_border_style);
    KLBUI_btn_bind("border-style:focus", on_klbui_button_border_style_focus);
    KLBUI_btn_bind("border-style:disable", on_klbui_button_border_style_disable);

    // 边框的宽度 border-width
    KLBUI_btn_bind("border-width", on_klbui_button_border_width);
    KLBUI_btn_bind("border-width:focus", on_klbui_button_border_width_focus);
    KLBUI_btn_bind("border-width:disable", on_klbui_button_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_btn_bind("border-color", on_klbui_button_border_color);
    KLBUI_btn_bind("border-color:focus", on_klbui_button_border_color_focus);
    KLBUI_btn_bind("border-color:disable", on_klbui_button_border_color_disable);

    // 圆角边框 border-radius
    //KLBUI_btn_bind("border-radius", on_klbui_button_border_radius);
    //KLBUI_btn_bind("border-radius:focus", on_klbui_button_border_radius_focus);
    //KLBUI_btn_bind("border-radius:disable", on_klbui_button_border_radius_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_btn_bind("title", on_klbui_button_title);
    KLBUI_btn_bind("value", on_klbui_button_title);

    return p_wnd;
}
