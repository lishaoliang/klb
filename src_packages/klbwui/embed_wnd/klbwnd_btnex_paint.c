// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_btnex_paint.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_util.h"


/// @def   KLBWND_BTNEX_ON_PAINT
/// @brief 绘制通用流程
#define KLBWND_BTNEX_ON_PAINT(PAINT_STATUS) { \
    klbwnd_btnex_t* p_btn = (klbwnd_btnex_t*)p_wnd->ctrl; \
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas; \
\
    if (KLB_WND_STATUS_HIDE & p_wnd->state.status) \
    { \
        return 0; \
    } \
\
    klbwnd_btnex_css_t* p_css = p_btn->p_css; \
    if (NULL == p_css) \
    { \
        return 0; \
    } \
\
    klb_rect_t paint_rect = *p_rect; \
\
    paint_rect.x += p_css->margin.left; \
    paint_rect.y += p_css->margin.top; \
    paint_rect.w -= (p_css->margin.left + p_css->margin.right); \
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom); \
\
    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status) \
    { \
        (PAINT_STATUS)(p_wnd, p_btn, p_css, &p_css->disable, &paint_rect); \
    } \
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status) \
    { \
        (PAINT_STATUS)(p_wnd, p_btn, p_css, &p_css->focus, &paint_rect); \
    } \
    else \
    { \
        (PAINT_STATUS)(p_wnd, p_btn, p_css, &p_css->normal, &paint_rect); \
    } \
}


//////////////////////////////////////////////////////////////////////////
// 默认/未识别类型

static void on_paint_status_klbwnd_btnex_select(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    klbuicssex_draw_text(p_wnd, p_btn->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int on_paint_klbwnd_btnex_select(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_status_klbwnd_btnex_select);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_rectangle: 实体矩形

static void on_paint_rectangle_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_rectangle(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_rectangle_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_rectangle_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_triangle_left

static void on_paint_triangle_left_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_triangle_left(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_triangle_left_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_triangle_left_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_triangle_up

static void on_paint_triangle_up_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_triangle_up(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_triangle_up_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_triangle_up_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_triangle_right

static void on_paint_triangle_right_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_triangle_right(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_triangle_right_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_triangle_right_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_triangle_down

static void on_paint_triangle_down_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_triangle_down(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_triangle_down_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_triangle_down_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_plus

static void on_paint_line_plus_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_plus(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_plus_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_plus_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_minus

static void on_paint_line_minus_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_minus(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_minus_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_minus_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_x

static void on_paint_line_x_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_x(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_x_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_x_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_1

static void on_paint_line_1_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_1(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_1_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_1_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_2

static void on_paint_line_2_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_2(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_2_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_2_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_3

static void on_paint_line_3_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_3(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_3_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_3_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////
// KLBWND_BTNEX_line_4

static void on_paint_line_4_status_klbwnd_btnex(klb_wnd_t* p_wnd, klbwnd_btnex_t* p_btn, klbwnd_btnex_css_t* p_css, klbwnd_btnex_attributes_t* p_attr, klb_rect_t* p_rect)
{

    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    klb_rect_t content_rect = *p_rect;
    klbuiutil_remove_border(&content_rect, &p_attr->border);
    klbuiutil_remove_padding(&content_rect, &p_css->padding);

    klbuiutil_draw_line_4(p_wnd, &content_rect, p_attr->text.color);
}

static int on_paint_line_4_klbwnd_btnex(klb_wnd_t* p_wnd)
{
    KLBWND_BTNEX_ON_PAINT(on_paint_line_4_status_klbwnd_btnex);
    return 0;
}


//////////////////////////////////////////////////////////////////////////

int klbwnd_btnex_paint_select(klb_wnd_t* p_wnd, int type)
{
    switch (type)
    {
    case KLBWND_BTNEX_rectangle:
        klb_wnd_bind_paint(p_wnd, on_paint_rectangle_klbwnd_btnex);
        break;

    case KLBWND_BTNEX_triangle_left:
        klb_wnd_bind_paint(p_wnd, on_paint_triangle_left_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_triangle_up:
        klb_wnd_bind_paint(p_wnd, on_paint_triangle_up_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_triangle_right:
        klb_wnd_bind_paint(p_wnd, on_paint_triangle_right_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_triangle_down:
        klb_wnd_bind_paint(p_wnd, on_paint_triangle_down_klbwnd_btnex);
        break;

    case KLBWND_BTNEX_line_plus:
        klb_wnd_bind_paint(p_wnd, on_paint_line_plus_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_minus:
        klb_wnd_bind_paint(p_wnd, on_paint_line_minus_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_x:
        klb_wnd_bind_paint(p_wnd, on_paint_line_x_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_1:
        klb_wnd_bind_paint(p_wnd, on_paint_line_1_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_2:
        klb_wnd_bind_paint(p_wnd, on_paint_line_2_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_3:
        klb_wnd_bind_paint(p_wnd, on_paint_line_3_klbwnd_btnex);
        break;
    case KLBWND_BTNEX_line_4:
        klb_wnd_bind_paint(p_wnd, on_paint_line_4_klbwnd_btnex);
        break;

    default:
        klb_wnd_bind_paint(p_wnd, on_paint_klbwnd_btnex_select);
        break;
    }

    return 0;
}

// end
