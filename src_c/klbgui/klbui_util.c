// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_util.h"


//////////////////////////////////////////////////////////////////////////
// 一些函数

/// @brief 矩形, 移除 外边距/内边距/边框
void klbuiutil_remove_margin(klb_rect_t* p_rect, klbuicss_margin_t* p_margin)
{
    if (p_rect && p_margin)
    {
        p_rect->x += p_margin->left;
        p_rect->y += p_margin->top;
        p_rect->w -= (p_margin->left + p_margin->right);
        p_rect->h -= (p_margin->top + p_margin->bottom);
    }
}

void klbuiutil_remove_padding(klb_rect_t* p_rect, klbuicss_padding_t* p_padding)
{
    if (p_rect && p_padding)
    {
        p_rect->x += p_padding->left;
        p_rect->y += p_padding->top;
        p_rect->w -= (p_padding->left + p_padding->right);
        p_rect->h -= (p_padding->top + p_padding->bottom);
    }
}

void klbuiutil_remove_border(klb_rect_t* p_rect, klbuicss_border_t* p_border)
{
    if (p_rect && p_border)
    {
        p_rect->x += p_border->width.left;
        p_rect->y += p_border->width.top;
        p_rect->w -= (p_border->width.left + p_border->width.right);
        p_rect->h -= (p_border->width.top + p_border->width.bottom);
    }
}

//////////////////////////////////////
// 矩形

/// @brief 绘制实体矩形
void klbuiutil_draw_rectangle(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, color);
}

//////////////////////////////////////
// 三角形


// 绘制向左实体三角形
void klbuiutil_draw_triangle_left(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 向左 三角

    // 求取 r, 满足如下条件
    // 1. 高 = 宽 * 2              =>     r.h = r.w * 2
    // 2. 在原矩形之内的最大矩形   =>     r.h < p_rect->h && r.w < p_rect->w && (r.h,r.w)为最大值
    klb_rect_t src = *p_rect;
    klb_rect_t r = { 0 };

    if (src.w * 2 <= src.h)
    {
        r.w = src.w;
        r.h = r.w * 2;
    }
    else
    {
        r.h = (src.h / 2) * 2;
        r.w = r.h / 2;
    }

    r.x = src.x + (src.w - r.w) / 2;
    r.y = src.y + (src.h - r.h) / 2;

    // 向左 三角
    int x = r.x;
    int y = r.y + r.h / 2;
    int h = 1;

    klb_wnd_draw_point2(p_wnd, x, y, color);

    for (int i = 1; i < r.w; i++)
    {
        klb_wnd_draw_line2(p_wnd, x + i, y - h, x + i, y + h, color);
        h += 1;
    }
}

// 绘制向右实体三角形
void klbuiutil_draw_triangle_right(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 向右 三角

    // 求取 r, 满足如下条件
    // 1. 高 = 宽 * 2              =>     r.h = r.w * 2
    // 2. 在原矩形之内的最大矩形   =>     r.h < p_rect->h && r.w < p_rect->w && (r.h,r.w)为最大值
    klb_rect_t src = *p_rect;
    klb_rect_t r = { 0 };

    if (src.w * 2 <= src.h)
    {
        r.w = src.w;
        r.h = r.w * 2;
    }
    else
    {
        r.h = (src.h / 2) * 2;
        r.w = r.h / 2;
    }

    r.x = src.x + (src.w - r.w) / 2;
    r.y = src.y + (src.h - r.h) / 2;

    // 向右 三角
    int x = r.x + r.w - 1;
    int y = r.y + r.h / 2;
    int h = 1;

    klb_wnd_draw_point2(p_wnd, x, y, color);

    for (int i = 1; i < r.w; i++)
    {
        klb_wnd_draw_line2(p_wnd, x - i, y - h, x - i, y + h, color);
        h += 1;
    }
}

/// @brief 绘制向上实体三角形
void klbuiutil_draw_triangle_up(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 向上 三角

    // 求取 r, 满足如下条件
    // 1. 宽 = 高 * 2              =>     r.w = r.h * 2
    // 2. 在原矩形之内的最大矩形   =>     r.h < p_rect->h && r.w < p_rect->w && (r.h,r.w)为最大值
    klb_rect_t src = *p_rect;
    klb_rect_t r = { 0 };

    if (src.h * 2 <= src.w)
    {
        r.h = src.h;
        r.w = r.h * 2;
    }
    else
    {
        r.w = (src.w / 2) * 2;
        r.h = r.w / 2;
    }

    r.x = src.x + (src.w - r.w) / 2;
    r.y = src.y + (src.h - r.h) / 2;

    // 向上 三角
    int x = r.x + r.w / 2;
    int y = r.y;
    int w = 1;

    klb_wnd_draw_point2(p_wnd, x, y, color);

    for (int i = 1; i < r.h; i++)
    {
        klb_wnd_draw_line2(p_wnd, x - w, y + i, x + w, y + i, color);
        w += 1;
    }
}

/// @brief 绘制向下实体三角形
void klbuiutil_draw_triangle_down(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 向下 三角

    // 求取 r, 满足如下条件
    // 1. 宽 = 高 * 2              =>     r.w = r.h * 2
    // 2. 在原矩形之内的最大矩形   =>     r.h < p_rect->h && r.w < p_rect->w && (r.h,r.w)为最大值
    klb_rect_t src = *p_rect;
    klb_rect_t r = { 0 };

    if (src.h * 2 <= src.w)
    {
        r.h = src.h;
        r.w = r.h * 2;
    }
    else
    {
        r.w = (src.w / 2) * 2;
        r.h = r.w / 2;
    }

    r.x = src.x + (src.w - r.w) / 2;
    r.y = src.y + (src.h - r.h) / 2;

    // 向上 三角
    int x = r.x + r.w / 2;
    int y = r.y + r.h - 1;
    int w = 1;

    klb_wnd_draw_point2(p_wnd, x, y, color);

    for (int i = 1; i < r.h; i++)
    {
        klb_wnd_draw_line2(p_wnd, x - w, y - i, x + w, y - i, color);
        w += 1;
    }
}
