#include "klbutil/klb_canvas.h"
#include "klbutil/klb_canvas_in.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define KLB_CANVAS_DRAW_BEGIN(CANVAS_)          \
{                                               \
    if ((CANVAS_)->vtable.draw_begin)           \
    {                                           \
        (CANVAS_)->vtable.draw_begin(CANVAS_);  \
    }                                           \
}

#define KLB_CANVAS_DRAW_END(CANVAS_)            \
{                                               \
    if ((CANVAS_)->vtable.draw_end)             \
    {                                           \
        (CANVAS_)->vtable.draw_end(CANVAS_);    \
    }                                           \
}


static void klb_canvas_free(klb_canvas_t* p_canvas)
{
    assert(NULL != p_canvas);
    KLB_FREE(p_canvas->p_addr);
    KLB_FREE(p_canvas);
}

klb_canvas_t* klb_canvas_create(int w, int h, int color_fmt)
{
    klb_canvas_t* p_canvas = KLB_MALLOC(klb_canvas_t, 1, 0);
    KLB_MEMSET(p_canvas, 0, sizeof(klb_canvas_t));

    p_canvas->rect.w = w;
    p_canvas->rect.h = h;
    p_canvas->color_fmt = color_fmt;

    p_canvas->vtable.free = klb_canvas_free;

    switch (color_fmt)
    {
    case KLB_COLOR_FMT_ARGB8888:
        {
            p_canvas->stride = (int64_t)w * 4;
            p_canvas->mem_len = p_canvas->stride * h;
            p_canvas->p_addr = KLB_MALLOC(uint8_t, p_canvas->mem_len, 0);
        }
        break;
    default:
        assert(false);
        break;
    }

    return p_canvas;
}

void klb_canvas_destroy(klb_canvas_t* p_canvas)
{
    assert(NULL != p_canvas);
    assert(NULL != p_canvas->vtable.free);

    p_canvas->vtable.free(p_canvas);
}

int klb_canvas_draw_clear(klb_canvas_t* p_canvas, int x, int y, int w, int h)
{
    uint32_t color = 0;
    return klb_canvas_draw_fill(p_canvas, x, y, w, h, color);
}

int klb_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y, uint32_t color)
{
    assert(NULL != p_canvas);
    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);

    int bpp = klb_color_bpp(p_canvas->color_fmt);
    uint8_t* p_addr = p_canvas->p_addr + (p_canvas->stride * y) + (bpp * x);

    memcpy(p_addr, &c, bpp);

    return 0;
}

int klb_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2, uint32_t color, int width)
{
    assert(NULL != p_canvas);
    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);

    if (x1 == x2)
    {
        //横线
        int y = 0, len = 0;

        if (y1 > y2)
        {
            len = y1 - y2;
            y = y2;
        }
        else
        {
            len = y2 - y1;
            y = y1;
        }

        klb_canvas_draw_fill(p_canvas, x1, y, width, len, c);
    }
    else if (y1 == y2)
    {
        //竖线
        long x = 0, len = 0;

        if (x1 > x2)
        {
            len = x1 - x2;
            x = x2;
        }
        else
        {
            len = x2 - x1;
            x = x1;
        }

        klb_canvas_draw_fill(p_canvas, x1, y1, len, width, c);
    }
    else
    {
        assert(false);
    }

    return 0;
}

int klb_canvas_draw_fill(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color)
{
    assert(NULL != p_canvas);

    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);

    // 由硬件接口实现
    if (p_canvas->vtable.draw_fill)
    {
        int ret = 0;
        KLB_CANVAS_DRAW_BEGIN(p_canvas);
        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, w, h, c);
        KLB_CANVAS_DRAW_END(p_canvas);
        return ret;
    }

    // 软实现
    int bpp = klb_color_bpp(p_canvas->color_fmt);
    uint8_t* p_addr = p_canvas->p_addr + (p_canvas->stride * y) + (bpp * x);
    uint8_t* ptr = p_addr;

    // 绘制一行后, 再复制
    for (int i = 0; i < w; i++)
    {
        memcpy(ptr, &c, bpp);
        ptr += bpp;
    }

    ptr = p_addr + p_canvas->stride;
    int cp_w = bpp * w;

    for (int i = 1; i < h; i++)
    {
        memcpy(ptr, p_addr, cp_w);
        ptr += p_canvas->stride;
    }

    return 0;
}

int klb_canvas_draw_rect(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color, int depth)
{
    assert(NULL != p_canvas);

    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);

    if (p_canvas->vtable.draw_fill)
    {
        int ret = 0;
        KLB_CANVAS_DRAW_BEGIN(p_canvas);
        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, w, depth, c); // 上
        ret = p_canvas->vtable.draw_fill(p_canvas, x + w - depth, y, depth, h, c); // 右
        ret = p_canvas->vtable.draw_fill(p_canvas, x, y + h - depth, w, depth, c); // 下
        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, depth, h, c); // 左
        KLB_CANVAS_DRAW_END(p_canvas);
        return ret;
    }

    klb_canvas_draw_fill(p_canvas, x, y, w, depth, c); // 上
    klb_canvas_draw_fill(p_canvas, x + w - depth, y, depth, h, c); // 右
    klb_canvas_draw_fill(p_canvas, x, y + h - depth, w, depth, c); // 下
    klb_canvas_draw_fill(p_canvas, x, y, depth, h, c); // 左

    return 0;
}

int klb_canvas_draw_canvas(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, int src_x, int src_y, int src_w, int src_h)
{
    assert(NULL != p_canvas);

    // 由硬件接口实现
    if (p_canvas->vtable.draw_canvas)
    {
        int ret = 0;
        KLB_CANVAS_DRAW_BEGIN(p_canvas);
        ret = p_canvas->vtable.draw_canvas(p_canvas, x, y, p_src_canvas, src_x, src_y, src_w, src_h);
        KLB_CANVAS_DRAW_END(p_canvas);
        return ret;
    }

    // 软实现
    if (p_canvas->color_fmt == p_src_canvas->color_fmt)
    {
        // todo. 裁剪
        int bpp = klb_color_bpp(p_canvas->color_fmt);
        uint8_t* p_dst_addr = p_canvas->p_addr + (p_canvas->stride * y) + (bpp * x);
        uint8_t* p_src_addr = p_src_canvas->p_addr + (p_src_canvas->stride * src_y) + (bpp * src_x);

        int width = bpp * src_w;
        for (int i = 0; i < src_h; i++)
        {
            memcpy(p_dst_addr, p_src_addr, width);

            p_dst_addr += p_canvas->stride;
            p_src_addr += p_src_canvas->stride;
        }
    }
    else
    {
        assert(false);
    }

    return 0;
}

int klb_canvas_draw_text(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color, int font, const char* p_utf8, int utf8_len)
{
    assert(NULL != p_canvas);

    return 0;
}
