// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_btnex.h"
#include "klbwui/embed_wnd/klbwnd_btnex_paint.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"

#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_btnex_deinit_attribute(klbwnd_btnex_t* p_btn);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_btnex_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_btnex_on_paint_status(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    // 有背景图时图内自带边框; 纯色才画 border
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    klbuicssex_draw_text(p_wnd, p_btn->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_btnex_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_btnex_css_t* p_css = p_btn->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_btnex_on_paint_status(p_wnd, p_btn, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_btnex_on_paint_status(p_wnd, p_btn, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_btnex_on_paint_status(p_wnd, p_btn, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int klbwnd_btnex_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_btnex_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_btnex_set_css(klb_wnd_t* p_wnd, klbwnd_btnex_css_t* p_css)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->p_css = p_css;
}

void klbwnd_btnex_set_type(klb_wnd_t* p_wnd, int type)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->type = type;

    // 设置内置绘图函数
    if (type <= KLBWND_BTNEX_klbuimax)
    {
        klbwnd_btnex_paint_select(p_wnd, type);
    }
}

int klbwnd_btnex_get_type(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->type;
}

void klbwnd_btnex_set_type_str(klb_wnd_t* p_wnd, const char* p_type)
{
    int type = KLBWND_BTNEX_normal;

    if (NULL == p_type)
    {
        klbwnd_btnex_set_type(p_wnd, type);
        return;
    }

    if (0 == strcmp(p_type, "rectangle"))
    {
        type = KLBWND_BTNEX_rectangle;
    }
    else if (0 == strcmp(p_type, "triangle-left"))
    {
        type = KLBWND_BTNEX_triangle_left;
    }
    else if (0 == strcmp(p_type, "triangle-up"))
    {
        type = KLBWND_BTNEX_triangle_up;
    }
    else if (0 == strcmp(p_type, "triangle-right"))
    {
        type = KLBWND_BTNEX_triangle_right;
    }
    else if (0 == strcmp(p_type, "triangle-down"))
    {
        type = KLBWND_BTNEX_triangle_down;
    }
    else if (0 == strcmp(p_type, "line-plus"))
    {
        type = KLBWND_BTNEX_line_plus;
    }
    else if (0 == strcmp(p_type, "line-minus"))
    {
        type = KLBWND_BTNEX_line_minus;
    }
    else if (0 == strcmp(p_type, "line-x"))
    {
        type = KLBWND_BTNEX_line_x;
    }
    else if (0 == strcmp(p_type, "line-1"))
    {
        type = KLBWND_BTNEX_line_1;
    }
    else if (0 == strcmp(p_type, "line-2"))
    {
        type = KLBWND_BTNEX_line_2;
    }
    else if (0 == strcmp(p_type, "line-3"))
    {
        type = KLBWND_BTNEX_line_3;
    }
    else if (0 == strcmp(p_type, "line-4"))
    {
        type = KLBWND_BTNEX_line_4;
    }

    klbwnd_btnex_set_type(p_wnd, type);
}

const char* klbwnd_btnex_get_type_str(klb_wnd_t* p_wnd)
{
    int type = klbwnd_btnex_get_type(p_wnd);

    switch (type)
    {
    case KLBWND_BTNEX_rectangle:
        return "rectangle";
    case KLBWND_BTNEX_triangle_left:
        return "triangle-left";
    case KLBWND_BTNEX_triangle_up:
        return "triangle-up";
    case KLBWND_BTNEX_triangle_right:
        return "triangle-right";
    case KLBWND_BTNEX_triangle_down:
        return "triangle-down";
    case KLBWND_BTNEX_line_plus:
        return "line-plus";
    case KLBWND_BTNEX_line_minus:
        return "line-minus";
    case KLBWND_BTNEX_line_x:
        return "line-x";
    case KLBWND_BTNEX_line_1:
        return "line-1";
    case KLBWND_BTNEX_line_2:
        return "line-2";
    case KLBWND_BTNEX_line_3:
        return "line-3";
    case KLBWND_BTNEX_line_4:
        return "line-4";
    default:
        break;
    }

    return "normal";
}

void klbwnd_btnex_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->index = index;
}

int klbwnd_btnex_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->index;
}

void klbwnd_btnex_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->title = klb_sds_assign(p_btn->title, p_title);
}

const sds klbwnd_btnex_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->title;
}

void klbwnd_btnex_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->value = klb_sds_assign(p_btn->value, p_value);
}

const sds klbwnd_btnex_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->value;
}

void klbwnd_btnex_set_value_int(klb_wnd_t* p_wnd, int value)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->i_value = value;
}

int klbwnd_btnex_get_value_int(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->i_value;
}

void klbwnd_btnex_set_value_bool(klb_wnd_t* p_wnd, bool value)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_btn->b_value = value;
}

bool klbwnd_btnex_get_value_bool(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    return p_btn->b_value;
}

klb_map_t* klbwnd_btnex_get_map(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;
    klb_map_t* p_map = p_btn->p_map;

    // 使用时再初始化
    if (NULL == p_map)
    {
        p_map = klb_map_create();
        p_btn->p_map = p_map;
    }

    return p_map;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_btnex_init_attribute(klbwnd_btnex_t* p_btn)
{
    p_btn->title = NULL;
    p_btn->value = NULL;

    p_btn->index = 0;
    p_btn->type = KLBWND_BTNEX_normal;
    p_btn->i_value = 0;
    p_btn->b_value = false;

    p_btn->p_map = NULL;
}

static void klbwnd_btnex_deinit_attribute(klbwnd_btnex_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE_BY(p_btn->value, sdsfree);

    KLB_FREE_BY(p_btn->p_map, klb_map_destroy);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_btnex_attributes_init(klbwnd_btnex_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_btnex_attributes_deinit(klbwnd_btnex_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_btnex_attributes_copy(klbwnd_btnex_attributes_t* p_dst, const klbwnd_btnex_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_btnex_css_init(klbwnd_btnex_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_btnex_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_btnex_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_btnex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_btnex_css_deinit(klbwnd_btnex_css_t* p_css)
{
    klbwnd_btnex_attributes_deinit(&p_css->normal);
    klbwnd_btnex_attributes_deinit(&p_css->focus);
    klbwnd_btnex_attributes_deinit(&p_css->disable);
}

void klbwnd_btnex_css_copy(klbwnd_btnex_css_t* p_dst, klbwnd_btnex_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_btnex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_btnex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_btnex_attributes_copy(&p_dst->disable, &p_src->disable);
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_btnex_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_btnex_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = 0x0;

    klbwnd_btnex_init_attribute(p_btn);
}

void klbwnd_btnex_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl;

    klbwnd_btnex_deinit_attribute(p_btn);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_btnex_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_btnex_t));

    klbwnd_btnex_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_btnex_destroy;

    return p_wnd;
}

// end
