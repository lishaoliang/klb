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

/// @brief 绘制实体符号 "+"
void klbuiutil_draw_line_plus(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 求取矩形中的最大正方形, 且居中
    int w = MIN(p_rect->w, p_rect->h);

    klb_rect_t r = { 0 };
    r.x = p_rect->x + (p_rect->w - w) / 2;
    r.y = p_rect->y + (p_rect->h - w) / 2;
    r.w = w;
    r.h = w;

    // 在此正方形中绘制 "+"
    int line_w = MAX((w / 4), 1);

    // 绘制水平矩形
    {
        klb_rect_t rect1 = { 0 };
        rect1.x = r.x;
        rect1.y = r.y + (r.h - line_w) / 2;
        rect1.w = r.w;
        rect1.h = line_w;

        klb_wnd_draw_fill_rect2(p_wnd, &rect1, color);
    }

    // 绘制垂直矩形
    {
        klb_rect_t rect2 = { 0 };
        rect2.x = r.x + (r.w - line_w) / 2;
        rect2.y = r.y;
        rect2.w = line_w;
        rect2.h = r.h;

        klb_wnd_draw_fill_rect2(p_wnd, &rect2, color);
    }
}

/// @brief 绘制实体符号 "-"
void klbuiutil_draw_line_minus(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 求取矩形中的最大正方形, 且居中
    int w = MIN(p_rect->w, p_rect->h);

    klb_rect_t r = { 0 };
    r.x = p_rect->x + (p_rect->w - w) / 2;
    r.y = p_rect->y + (p_rect->h - w) / 2;
    r.w = w;
    r.h = w;

    // 在此正方形中绘制 "-"
    int line_w = MAX((w / 4), 1);

    // 绘制水平矩形
    {
        klb_rect_t rect1 = { 0 };
        rect1.x = r.x;
        rect1.y = r.y + (r.h - line_w) / 2;
        rect1.w = r.w;
        rect1.h = line_w;

        klb_wnd_draw_fill_rect2(p_wnd, &rect1, color);
    }
}

/// @brief 绘制实体符号 "X"
void klbuiutil_draw_line_x(klb_wnd_t* p_wnd, klb_rect_t* p_rect, uint32_t color)
{
    // 求取矩形中的最大正方形, 且居中
    int w = MIN(p_rect->w, p_rect->h);

    klb_rect_t r = { 0 };
    r.x = p_rect->x + (p_rect->w - w) / 2;
    r.y = p_rect->y + (p_rect->h - w) / 2;
    r.w = w;
    r.h = w;

    // "X" =>  两个正方形错开一定宽度
    int line_w = MAX((w / 8), 1);
    r.y += line_w / 2;
    r.h -= line_w;

    klb_rect_t r1 = r; // 偏左正方形
    r1.w = r.h;

    klb_rect_t r2 = r; // 偏右正方形
    r1.x += line_w;
    r2.w = r.h;

    int x1 = r1.x, y1 = r1.y;
    int x2 = r2.x, y2 = r2.y;

    int x3 = r1.x, y3 = r1.y + r1.h;
    int x4 = r2.x, y4 = r2.y + r2.h;

    for (int i = 0; i <= r1.w; i++)
    {
        klb_wnd_draw_line2(p_wnd, x1, y1, x2, y2, color);

        x1 += 1;    y1 += 1;
        x2 += 1;    y2 += 1;

        klb_wnd_draw_line2(p_wnd, x3, y3, x4, y4, color);

        x3 += 1;    y3 -= 1;
        x4 += 1;    y4 -= 1;
    }
}
