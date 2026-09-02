// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbformat/klb_bitmap.h"
#include "klbutil/klb_color.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#else
#pragma pack(1)

typedef struct tagBITMAPFILEHEADER
{
    uint16_t    bfType;                 ///< KLB_BITMAP_FILE: "BM"
    uint32_t    bfSize;                 ///< 整个文件大小
    uint16_t    bfReserved1;            ///< 0
    uint16_t    bfReserved2;            ///< 0
    uint32_t    bfOffBits;              ///< 图像数据区偏移
}BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    uint32_t    biSize;                 ///< sizeof(BITMAPINFOHEADER)
    int32_t     biWidth;                ///< 图像宽
    int32_t     biHeight;               ///< 图像高
    uint16_t    biPlanes;               ///< 1
    uint16_t    biBitCount;             ///< 像素占用的比特位: 1(单色), 4(16色), 8(256色), 16(高彩色), 24(2真彩色), 32(增强型真彩色)
    uint32_t    biCompression;          ///< 压缩方式,0表示不压缩,1表示RLE8压缩,2表示RLE4压缩,3表示每个像素值由指定的掩码决定
    uint32_t    biSizeImage;            ///< BMP图像数据大小: 注意行跨距(biWidth*biBitCount/8)必须是4的整数倍
    int32_t     biXPelsPerMeter;
    int32_t     biYPelsPerMeter;
    uint32_t    biClrUsed;
    uint32_t    biClrImportant;
}BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    uint8_t     rgbBlue;
    uint8_t     rgbGreen;
    uint8_t     rgbRed;
    uint8_t     rgbReserved;
}RGBQUAD;
#pragma pack()
#endif

#define KLB_BITMAP_FILE         ((('M') << 8) | ('B'))
#define KLB_BITMAP_BI_RGB       0
#define KLB_BITMAP_BI_BITFIELDS 3
#define KLB_BITMAP_DIB_MAX      256
#define KLB_BITMAP_V4_MASKS_MIN 56
#define KLB_BITMAP_MASK_R       0x00FF0000u
#define KLB_BITMAP_MASK_G       0x0000FF00u
#define KLB_BITMAP_MASK_B       0x000000FFu
#define KLB_BITMAP_MASK_A       0xFF000000u

static uint32_t pixel_to_argb8888_klb_bitmap(const uint8_t* p_px, int bi_bit_count, const RGBQUAD* p_palette, int palette_count, bool keep_alpha)
{
    if (32 == bi_bit_count)
    {
        uint8_t b = p_px[0], g = p_px[1], r = p_px[2], a = p_px[3];
        if (!keep_alpha && 0 == a)
        {
            a = 0xFF;
        }
        return KLB_ARGB8888(a, r, g, b);
    }
    else if (24 == bi_bit_count)
    {
        return KLB_ARGB8888(0xFF, p_px[2], p_px[1], p_px[0]);
    }
    else if (16 == bi_bit_count)
    {
        uint16_t v = (uint16_t)p_px[0] | ((uint16_t)p_px[1] << 8);
        int r5 = (v >> 10) & 0x1F, g5 = (v >> 5) & 0x1F, b5 = v & 0x1F;
        return KLB_ARGB8888(0xFF, (r5 << 3) | (r5 >> 2), (g5 << 3) | (g5 >> 2), (b5 << 3) | (b5 >> 2));
    }
    else if (8 == bi_bit_count)
    {
        int idx = (int)p_px[0];
        if (idx < 0 || palette_count <= idx)
        {
            idx = 0; // 像素索引越界, 回退调色板首项
        }
        if (palette_count <= 0)
        {
            return 0;
        }
        const RGBQUAD* p_q = p_palette + idx;
        return KLB_ARGB8888(0xFF, p_q->rgbRed, p_q->rgbGreen, p_q->rgbBlue);
    }

    return 0;
}

static void fill_canvas_klb_bitmap(klb_canvas_t* p_canvas, const char* p_data, int stride, int w, int h, int bi_bit_count, const RGBQUAD* p_palette, int palette_count, int color_fmt, bool top_down, bool keep_alpha)
{
    int src_bpp = (8 == bi_bit_count) ? 1 : (bi_bit_count >> 3);

    for (int y = 0; y < h; y++)
    {
        int64_t src_y = 0;
        if (top_down)
        {
            src_y = (int64_t)y;
        }
        else
        {
            src_y = (int64_t)(h - 1 - y);
        }

        const uint8_t* p_src_row = (const uint8_t*)p_data + src_y * stride;
        uint32_t* p_dst_row = (uint32_t*)(p_canvas->p_addr + p_canvas->pitch * y);

        for (int x = 0; x < w; x++)
        {
            uint32_t argb8888 = pixel_to_argb8888_klb_bitmap(p_src_row + x * src_bpp, bi_bit_count, p_palette, palette_count, keep_alpha);
            p_dst_row[x] = klb_color_argb8888_to(argb8888, color_fmt);
        }
    }
}

static int read_bitfields_masks_klb_bitmap(FILE* pf, uint32_t bi_size, uint32_t* p_mask_extra, bool* p_keep_alpha)
{
    uint32_t mask_r = 0, mask_g = 0, mask_b = 0, mask_a = 0;

    *p_mask_extra = 0;
    *p_keep_alpha = false;

    if (sizeof(BITMAPINFOHEADER) == bi_size)
    {
        uint32_t masks[3] = { 0 };
        if (1 != fread(masks, sizeof(masks), 1, pf))
        {
            return 1;
        }
        mask_r = masks[0];
        mask_g = masks[1];
        mask_b = masks[2];
        *p_mask_extra = (uint32_t)sizeof(masks);
    }
    else if (KLB_BITMAP_V4_MASKS_MIN <= bi_size)
    {
        uint32_t masks[4] = { 0 };
        if (1 != fread(masks, sizeof(masks), 1, pf))
        {
            return 1;
        }
        mask_r = masks[0];
        mask_g = masks[1];
        mask_b = masks[2];
        mask_a = masks[3];
    }
    else
    {
        return 1;
    }

    if ((KLB_BITMAP_MASK_R != mask_r) || (KLB_BITMAP_MASK_G != mask_g) || (KLB_BITMAP_MASK_B != mask_b))
    {
        return 1;
    }

    if (0 != mask_a)
    {
        if (KLB_BITMAP_MASK_A != mask_a)
        {
            return 1;
        }
        *p_keep_alpha = true;
    }

    return 0;
}

klb_canvas_t* klb_bitmap_read(const char* p_filename, int color_fmt)
{
    if (NULL == p_filename)
    {
        return NULL;
    }

    FILE* pf = fopen(p_filename, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    char* p_data = NULL;
    klb_canvas_t* p_canvas = NULL;
    int stride = 0, bpp = 0, size = 0;

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };
    RGBQUAD palette[256] = { 0 };
    int palette_count = 0;

    // BITMAPFILEHEADER
    int read_len = sizeof(BITMAPFILEHEADER);
    if (1 != fread(&bfh, read_len, 1, pf))
    {
        goto err_read;
    }

    if (KLB_BITMAP_FILE != bfh.bfType)
    {
        goto err_read;
    }

    // BITMAPINFOHEADER
    read_len = sizeof(BITMAPINFOHEADER);
    if (1 != fread(&bih, read_len, 1, pf))
    {
        goto err_read;
    }

    if ((sizeof(BITMAPINFOHEADER) > bih.biSize) || (KLB_BITMAP_DIB_MAX < bih.biSize))
    {
        goto err_read;
    }

    if ((8 != bih.biBitCount) && (16 != bih.biBitCount) &&
        (24 != bih.biBitCount) && (32 != bih.biBitCount))
    {
        // 比特位: 8, 16, 24, 32
        goto err_read;
    }

    bool use_bitfields = false;
    if (KLB_BITMAP_BI_BITFIELDS == bih.biCompression)
    {
        use_bitfields = true;
    }
    else if (KLB_BITMAP_BI_RGB != bih.biCompression)
    {
        goto err_read;
    }

    if (use_bitfields && (32 != bih.biBitCount))
    {
        goto err_read;
    }

    if (1 != bih.biPlanes)
    {
        goto err_read;
    }

    // 宽, 高; 负高为顶向下
    if (0 >= bih.biWidth)
    {
        goto err_read;
    }

    bool top_down = false;
    int h = 0;
    if (0 > bih.biHeight)
    {
        if (INT32_MIN == bih.biHeight)
        {
            goto err_read;
        }
        h = (int)(-bih.biHeight);
        top_down = true;
    }
    else if (0 == bih.biHeight)
    {
        goto err_read;
    }
    else
    {
        h = (int)bih.biHeight;
    }

    if ((uint32_t)LONG_MAX < bfh.bfOffBits)
    {
        goto err_read;
    }

    bool keep_alpha = false;
    uint32_t mask_extra = 0;
    if (use_bitfields)
    {
        if (0 != read_bitfields_masks_klb_bitmap(pf, bih.biSize, &mask_extra, &keep_alpha))
        {
            goto err_read;
        }
    }

    uint32_t min_off = (uint32_t)sizeof(BITMAPFILEHEADER) + bih.biSize + mask_extra;

    // 8位位图, 读取调色板(最多256色); 调色板在完整 DIB 头之后
    if (8 == bih.biBitCount)
    {
        uint32_t clr_count = (0 == bih.biClrUsed) ? 256 : bih.biClrUsed;
        if (256 < clr_count)
        {
            goto err_read;
        }

        palette_count = (int)clr_count;
        min_off += clr_count * (uint32_t)sizeof(RGBQUAD);

        if (0 != fseek(pf, (long)((uint32_t)sizeof(BITMAPFILEHEADER) + bih.biSize), SEEK_SET))
        {
            goto err_read;
        }

        read_len = (int)(clr_count * sizeof(RGBQUAD));
        if (1 != fread(palette, (size_t)read_len, 1, pf))
        {
            goto err_read;
        }
    }

    if (bfh.bfOffBits < min_off)
    {
        goto err_read;
    }

    // 像素数据按 bfOffBits 定位, 跳过 DIB 扩展与空隙
    if (0 != fseek(pf, (long)bfh.bfOffBits, SEEK_SET))
    {
        goto err_read;
    }

    // 行跨距
    bpp = bih.biBitCount >> 3;
    {
        int64_t stride64 = (int64_t)bpp * (int64_t)bih.biWidth;
        if (stride64 <= 0 || (int64_t)INT_MAX < stride64)
        {
            goto err_read;
        }
        stride = (int)stride64;
        stride = ((stride + 3) >> 2) << 2; // 4 字节对齐
        if (stride <= 0)
        {
            goto err_read;
        }
    }

    // 读取图像像素
    {
        int64_t size64 = (int64_t)stride * (int64_t)h;
        if (size64 <= 0 || (int64_t)INT_MAX < size64)
        {
            goto err_read;
        }
        size = (int)size64;
    }

    p_data = KLB_MALLOC(char, size, 0);

    if (1 != fread(p_data, (size_t)size, 1, pf))
    {
        goto err_read;
    }

    p_canvas = klb_canvas_create(bih.biWidth, h, KLB_COLOR_FMT_ARGB8888);
    if (NULL == p_canvas)
    {
        goto err_read;
    }

    fill_canvas_klb_bitmap(p_canvas, p_data, stride, bih.biWidth, h, bih.biBitCount, palette, palette_count, color_fmt, top_down, keep_alpha);

    KLB_FREE(p_data);
    fclose(pf);
    return p_canvas;

err_read:
    KLB_FREE(p_data);
    fclose(pf);
    return NULL;
}

int klb_bitmap_write(const char* p_filename, const klb_canvas_t* p_canvas)
{
    if (NULL == p_filename || NULL == p_canvas || NULL == p_canvas->p_addr)
    {
        return 1;
    }

    int w = p_canvas->rect.w;
    int h = p_canvas->rect.h;
    if (w <= 0 || h <= 0)
    {
        return 1;
    }

    if (KLB_COLOR_FMT_ARGB8888 != p_canvas->color_fmt)
    {
        return 1;
    }

    int64_t row64 = (int64_t)w * 4;
    if (row64 <= 0 || (int64_t)INT_MAX < row64 || p_canvas->pitch < row64)
    {
        return 1;
    }

    int row_bytes = (int)row64;
    int stride = ((row_bytes + 3) >> 2) << 2;
    int64_t img64 = (int64_t)stride * (int64_t)h;
    if (img64 <= 0 || (int64_t)INT_MAX < img64)
    {
        return 1;
    }

    FILE* pf = fopen(p_filename, "wb");
    if (NULL == pf)
    {
        return 1;
    }

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = h;
    bih.biPlanes = 1;
    bih.biBitCount = 32; // 颜色占用位数
    bih.biCompression = KLB_BITMAP_BI_RGB;
    bih.biSizeImage = (uint32_t)img64; // 图像数据大小

    bfh.bfType = KLB_BITMAP_FILE;
    bfh.bfSize = (uint32_t)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (size_t)img64); // 完整文件大小
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // 图像数据偏移

    if (1 != fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pf))
    {
        goto err_write;
    }

    if (1 != fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pf))
    {
        goto err_write;
    }

    uint8_t zero_b[4] = { 0 };
    int zero_len = stride - row_bytes;

    uint8_t* p_addr = p_canvas->p_addr + p_canvas->pitch * (h - 1);
    for (int i = 0; i < h; i++)
    {
        if (1 != fwrite(p_addr, (size_t)row_bytes, 1, pf))
        {
            goto err_write;
        }

        if (0 < zero_len)
        {
            if (1 != fwrite(zero_b, (size_t)zero_len, 1, pf))
            {
                goto err_write;
            }
        }

        p_addr -= p_canvas->pitch;
    }

    fclose(pf);
    return 0;

err_write:
    fclose(pf);
    return 1;
}

// end
