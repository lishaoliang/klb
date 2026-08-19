// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbgui/klb_wnd_ex.h"


static int clamp_count_wndex(int count, int max_count)
{
    if (count < 0)
    {
        return 0;
    }

    if (count > max_count)
    {
        return max_count;
    }

    return count;
}

static void fill_color_wndex(uint32_t* p_colors, int count, uint32_t color)
{
    int i = 0;

    for (i = 0; i < count; i++)
    {
        p_colors[i] = color;
    }
}

int klb_wndex_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color, int thick)
{
    return klb_wnd_draw_opt6(p_wnd, KLB_CANVAS_DRAW_OPT_LINE, &x1, &y1, &x2, &y2, &color, &thick);
}

int klb_wndex_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_pt1, const klb_point_t* p_pt2, const uint32_t* p_color, int count, int thick)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_LINES_MAX);

    return klb_wnd_draw_opt5(p_wnd, KLB_CANVAS_DRAW_OPT_LINES, p_pt1, p_pt2, p_color, &n, &thick);
}

int klb_wndex_draw_lines2(klb_wnd_t* p_wnd, const klb_point_t* p_pt1, const klb_point_t* p_pt2, uint32_t color, int count, int thick)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_LINES_MAX);

    if (0 >= n)
    {
        return 0;
    }

    uint32_t colors[KLB_CANVAS_DRAW_LINES_MAX];

    fill_color_wndex(colors, n, color);

    return klb_wnd_draw_opt5(p_wnd, KLB_CANVAS_DRAW_OPT_LINES, p_pt1, p_pt2, colors, &n, &thick);
}

int klb_wndex_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color, int thick)
{
    return klb_wnd_draw_opt3(p_wnd, KLB_CANVAS_DRAW_OPT_RECT, p_rect, &color, &thick);
}

int klb_wndex_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, const uint32_t* p_color, int count, int thick)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_RECTS_MAX);

    return klb_wnd_draw_opt4(p_wnd, KLB_CANVAS_DRAW_OPT_RECTS, p_rects, p_color, &n, &thick);
}

int klb_wndex_draw_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, uint32_t color, int count, int thick)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_RECTS_MAX);

    if (0 >= n)
    {
        return 0;
    }

    uint32_t colors[KLB_CANVAS_DRAW_RECTS_MAX];

    fill_color_wndex(colors, n, color);

    return klb_wnd_draw_opt4(p_wnd, KLB_CANVAS_DRAW_OPT_RECTS, p_rects, colors, &n, &thick);
}

int klb_wndex_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, const uint32_t* p_color, int count)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_FILL_RECTS_MAX);

    return klb_wnd_draw_opt3(p_wnd, KLB_CANVAS_DRAW_OPT_FILL_RECTS, p_rects, p_color, &n);
}

int klb_wndex_draw_fill_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, uint32_t color, int count)
{
    int n = clamp_count_wndex(count, KLB_CANVAS_DRAW_FILL_RECTS_MAX);

    if (0 >= n)
    {
        return 0;
    }

    uint32_t colors[KLB_CANVAS_DRAW_FILL_RECTS_MAX];

    fill_color_wndex(colors, n, color);

    return klb_wnd_draw_opt3(p_wnd, KLB_CANVAS_DRAW_OPT_FILL_RECTS, p_rects, colors, &n);
}

int klb_wndex_draw_image_resize(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_image_path)
{
    return klb_wnd_draw_opt2(p_wnd, KLB_CANVAS_DRAW_OPT_IMAGE_RESIZE, p_dst_rect, p_image_path);
}

int klb_wndex_draw_image_color_key(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path)
{
    return klb_wnd_draw_opt2(p_wnd, KLB_CANVAS_DRAW_OPT_IMAGE_COLOR_KEY, p_rect, p_image_path);
}

int klb_wndex_draw_image_scale9(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path)
{
    return klb_wnd_draw_opt2(p_wnd, KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9, p_rect, p_image_path);
}

int klb_wndex_draw_image_scale9_color_key(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_image_path)
{
    return klb_wnd_draw_opt2(p_wnd, KLB_CANVAS_DRAW_OPT_IMAGE_SCALE9_COLOR_KEY, p_rect, p_image_path);
}

int klb_wndex_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int* p_out_w, int* p_out_h, bool* p_out_all)
{
    int len = utf8_len;
    int fh = font_h;

    return klb_wnd_draw_opt8(p_wnd, KLB_CANVAS_DRAW_OPT_TEXT, p_rect, p_utf8, &len, &color, &fh, p_out_w, p_out_h, p_out_all);
}

int klb_wndex_draw_text_lines(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h, int line_spacing)
{
    int len = utf8_len;
    int fh = font_h;
    int spacing = line_spacing;

    return klb_wnd_draw_opt6(p_wnd, KLB_CANVAS_DRAW_OPT_TEXT_LINES, p_rect, p_utf8, &len, &color, &fh, &spacing);
}

// end
