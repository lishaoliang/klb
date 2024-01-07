// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
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
            p_canvas->pitch = (int64_t)w * 4;
            p_canvas->mem_len = p_canvas->pitch * h;
            p_canvas->p_addr = KLB_MALLOC(uint8_t, (size_t)p_canvas->mem_len, 0);
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

/// @brief 设置绘制颜色
int klb_canvas_set_draw_color(klb_canvas_t* p_canvas, uint32_t color)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.set_draw_color)
    {
        return p_canvas->vtable.set_draw_color(p_canvas, color);
    }

    return 0;
}

/// @brief 获取绘制颜色
uint32_t klb_canvas_get_draw_color(klb_canvas_t* p_canvas)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.get_draw_color)
    {
        return p_canvas->vtable.get_draw_color(p_canvas);
    }

    return 0;
}

int klb_canvas_set_font_height(klb_canvas_t* p_canvas, int h)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.set_font_height)
    {
        return p_canvas->vtable.set_font_height(p_canvas, h);
    }

    return 0;
}

int klb_canvas_get_font_height(klb_canvas_t* p_canvas)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.get_font_height)
    {
        return p_canvas->vtable.get_font_height(p_canvas);
    }

    return 0;
}

int klb_canvas_draw_clear(klb_canvas_t* p_canvas)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.draw_clear)
    {
        return p_canvas->vtable.draw_clear(p_canvas);
    }

    return 0;
}

int klb_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.draw_point)
    {
        return p_canvas->vtable.draw_point(p_canvas, x, y);
    }

    return 0;
}

int klb_canvas_draw_points(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (NULL != p_canvas->vtable.draw_points)
    {
        return p_canvas->vtable.draw_points(p_canvas, p_points, count);
    }

    return 0;
}

int klb_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_line)
    {
        return p_canvas->vtable.draw_line(p_canvas, x1, y1, x2, y2);
    }

    //assert(NULL != p_canvas);
    //uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);
    //if (x1 == x2)
    //{
    //    //横线
    //    int y = 0, len = 0;
    //    if (y1 > y2)
    //    {
    //        len = y1 - y2;
    //        y = y2;
    //    }
    //    else
    //    {
    //        len = y2 - y1;
    //        y = y1;
    //    }
    //    klb_canvas_draw_fill(p_canvas, x1, y, width, len, c);
    //}
    //else if (y1 == y2)
    //{
    //    //竖线
    //    long x = 0, len = 0;
    //    if (x1 > x2)
    //    {
    //        len = x1 - x2;
    //        x = x2;
    //    }
    //    else
    //    {
    //        len = x2 - x1;
    //        x = x1;
    //    }
    //    klb_canvas_draw_fill(p_canvas, x1, y1, len, width, c);
    //}
    //else
    //{
    //    assert(false);
    //}

    return 0;
}

int klb_canvas_draw_lines(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_lines)
    {
        return p_canvas->vtable.draw_lines(p_canvas, p_points, count);
    }

    return 0;
}

int klb_canvas_draw_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_rect)
    {
        return p_canvas->vtable.draw_rect(p_canvas, p_rect);
    }

    return 0;
}

int klb_canvas_draw_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_rects)
    {
        return p_canvas->vtable.draw_rects(p_canvas, p_rects, count);
    }

    return 0;
}

int klb_canvas_draw_fill_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_fill_rect)
    {
        return p_canvas->vtable.draw_fill_rect(p_canvas, p_rect);
    }

    return 0;
}

int klb_canvas_draw_fill_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_fill_rects)
    {
        return p_canvas->vtable.draw_fill_rects(p_canvas, p_rects, count);
    }

    return 0;
}

/// @brief 绘制文字
/// @param [in] *p_canvas       画布对象
/// @return int 0
int klb_canvas_draw_text(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_text)
    {
        return p_canvas->vtable.draw_text(p_canvas, p_rect, p_utf8, utf8_len);
    }

    return 0;
}

int klb_canvas_text_size(klb_canvas_t* p_canvas, const char* p_utf8, int utf8_len, int* p_out_w, int* p_out_h)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.text_size)
    {
        return p_canvas->vtable.text_size(p_canvas, p_utf8, utf8_len, p_out_w, p_out_h);
    }

    return 0;
}

int klb_canvas_image_size(klb_canvas_t* p_canvas, const char* p_path, int* p_out_w, int* p_out_h)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.image_size)
    {
        return p_canvas->vtable.image_size(p_canvas, p_path, p_out_w, p_out_h);
    }

    return 0;
}

int klb_canvas_draw_image(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_image)
    {
        return p_canvas->vtable.draw_image(p_canvas, p_dst_rect, p_path, NULL);
    }

    return 0;
}

int klb_canvas_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.refresh_rect)
    {
        return p_canvas->vtable.refresh_rect(p_canvas, p_rect);
    }

    return 0;
}

int klb_canvas_refresh_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.refresh_rects)
    {
        return p_canvas->vtable.refresh_rects(p_canvas, p_rects, count);
    }

    return 0;
}

int klb_canvas_refresh(klb_canvas_t* p_canvas,                                  // 主画布
                        const klb_rect_t dst[KLB_CANVAS_LAYER_max],             // 目标主显存对应区域
                        klb_canvas_t* p_src_canvas[KLB_CANVAS_LAYER_max],       // 待刷新的源画布
                        const klb_rect_t src[KLB_CANVAS_LAYER_max],             // 源区域
                        int layer_count)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.refresh)
    {
        return p_canvas->vtable.refresh(p_canvas, dst, p_src_canvas, src, layer_count);
    }

    return 0;
}

klb_canvas_t* klb_canvas_malloc(klb_canvas_t* p_canvas, int w, int h, int layer_type)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.malloc)
    {
        return p_canvas->vtable.malloc(p_canvas, w, h, layer_type);
    }

    return 0;
}

int klb_canvas_draw_opt1(klb_canvas_t* p_canvas, int opt, const void* ptr1)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt1)
    {
        return p_canvas->vtable.draw_opt1(p_canvas, opt, ptr1);
    }

    return 0;
}

int klb_canvas_draw_opt2(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt2)
    {
        return p_canvas->vtable.draw_opt2(p_canvas, opt, ptr1, ptr2);
    }

    return 0;
}

int klb_canvas_draw_opt3(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt3)
    {
        return p_canvas->vtable.draw_opt3(p_canvas, opt, ptr1, ptr2, ptr3);
    }

    return 0;
}

int klb_canvas_draw_opt4(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt4)
    {
        return p_canvas->vtable.draw_opt4(p_canvas, opt, ptr1, ptr2, ptr3, ptr4);
    }

    return 0;
}

int klb_canvas_draw_opt5(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt5)
    {
        return p_canvas->vtable.draw_opt5(p_canvas, opt, ptr1, ptr2, ptr3, ptr4, ptr5);
    }

    return 0;
}

int klb_canvas_draw_opt6(klb_canvas_t* p_canvas, int opt, const void* ptr1, const void* ptr2, const void* ptr3, const void* ptr4, const void* ptr5, const void* ptr6)
{
    if (NULL == p_canvas)
    {
        return 0;
    }

    if (p_canvas->vtable.draw_opt6)
    {
        return p_canvas->vtable.draw_opt6(p_canvas, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6);
    }

    return 0;
}

//
//int klb_canvas_draw_fill(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color)
//{
//    assert(NULL != p_canvas);
//
//    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);
//
//    // 由硬件接口实现
//    if (p_canvas->vtable.draw_fill)
//    {
//        int ret = 0;
//        KLB_CANVAS_DRAW_BEGIN(p_canvas);
//        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, w, h, c);
//        KLB_CANVAS_DRAW_END(p_canvas);
//        return ret;
//    }
//
//    // 软实现
//    int bpp = klb_color_bpp(p_canvas->color_fmt);
//    uint8_t* p_addr = p_canvas->p_addr + (p_canvas->pitch * y) + (bpp * x);
//    uint8_t* ptr = p_addr;
//
//    // 绘制一行后, 再复制
//    for (int i = 0; i < w; i++)
//    {
//        memcpy(ptr, &c, bpp);
//        ptr += bpp;
//    }
//
//    ptr = p_addr + p_canvas->pitch;
//    int cp_w = bpp * w;
//
//    for (int i = 1; i < h; i++)
//    {
//        memcpy(ptr, p_addr, cp_w);
//        ptr += p_canvas->pitch;
//    }
//
//    return 0;
//}
//
//int klb_canvas_draw_rect(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color, int depth)
//{
//    assert(NULL != p_canvas);
//
//    uint32_t c = KLB_COLOR_TO(color, p_canvas->color_fmt);
//
//    if (p_canvas->vtable.draw_fill)
//    {
//        int ret = 0;
//        KLB_CANVAS_DRAW_BEGIN(p_canvas);
//        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, w, depth, c); // 上
//        ret = p_canvas->vtable.draw_fill(p_canvas, x + w - depth, y, depth, h, c); // 右
//        ret = p_canvas->vtable.draw_fill(p_canvas, x, y + h - depth, w, depth, c); // 下
//        ret = p_canvas->vtable.draw_fill(p_canvas, x, y, depth, h, c); // 左
//        KLB_CANVAS_DRAW_END(p_canvas);
//        return ret;
//    }
//
//    klb_canvas_draw_fill(p_canvas, x, y, w, depth, c); // 上
//    klb_canvas_draw_fill(p_canvas, x + w - depth, y, depth, h, c); // 右
//    klb_canvas_draw_fill(p_canvas, x, y + h - depth, w, depth, c); // 下
//    klb_canvas_draw_fill(p_canvas, x, y, depth, h, c); // 左
//
//    return 0;
//}
//
//int klb_canvas_draw_canvas(klb_canvas_t* p_canvas, int x, int y, const klb_canvas_t* p_src_canvas, int src_x, int src_y, int src_w, int src_h)
//{
//    assert(NULL != p_canvas);
//
//    // 由硬件接口实现
//    if (p_canvas->vtable.draw_canvas)
//    {
//        int ret = 0;
//        KLB_CANVAS_DRAW_BEGIN(p_canvas);
//        ret = p_canvas->vtable.draw_canvas(p_canvas, x, y, p_src_canvas, src_x, src_y, src_w, src_h);
//        KLB_CANVAS_DRAW_END(p_canvas);
//        return ret;
//    }
//
//    // 软实现
//    if (p_canvas->color_fmt == p_src_canvas->color_fmt)
//    {
//        // todo. 裁剪
//        int bpp = klb_color_bpp(p_canvas->color_fmt);
//        uint8_t* p_dst_addr = p_canvas->p_addr + (p_canvas->pitch * y) + (bpp * x);
//        uint8_t* p_src_addr = p_src_canvas->p_addr + (p_src_canvas->pitch * src_y) + (bpp * src_x);
//
//        int width = bpp * src_w;
//        for (int i = 0; i < src_h; i++)
//        {
//            memcpy(p_dst_addr, p_src_addr, width);
//
//            p_dst_addr += p_canvas->pitch;
//            p_src_addr += p_src_canvas->pitch;
//        }
//    }
//    else
//    {
//        assert(false);
//    }
//
//    return 0;
//}
//
//int klb_canvas_draw_text(klb_canvas_t* p_canvas, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
//{
//    assert(NULL != p_canvas);
//
//    if (p_canvas->vtable.draw_text)
//    {
//        int ret = 0;
//        KLB_CANVAS_DRAW_BEGIN(p_canvas);
//        ret = p_canvas->vtable.draw_text(p_canvas, x, y, w, h, p_utf8, utf8_len, color, font_h);
//        KLB_CANVAS_DRAW_END(p_canvas);
//
//        return ret;
//    }
//
//    return 0;
//}
