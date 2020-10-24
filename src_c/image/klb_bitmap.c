#include "image/klb_bitmap.h"
#include "mem/klb_mem.h"
#include <stdio.h>
#include <assert.h>

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
    int32_t     biHeight;               ///< 图像宽
    uint16_t    biPlanes;               ///< 1
    uint16_t    biBitCount;             ///< 像素占用的比特位: 1(单色), 4(16色), 8(256色), 16(高彩色), 24(2真彩色), 32(增强型真彩色)
    uint32_t    biCompression;          ///< 压缩方式,0表示不压缩,1表示RLE8压缩,2表示RLE4压缩,3表示每个像素值由指定的掩码决定
    uint32_t    biSizeImage;            ///< BMP图像数据大小: 注意行夸距(biWidth*biBitCount/8)必须是4的整数倍
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

#define KLB_BITMAP_FILE     ((('M') << 8) | ('B'))

klb_canvas_t* klb_bitmap_read(const char* p_filename, int color_fmt)
{
    FILE* pf = fopen(p_filename, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    char* p_data = NULL;
    int stride = 0, bpp = 0, size = 0;

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };
    RGBQUAD palette[256] = { 0 };

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

    if ((8 != bih.biBitCount) && (16 != bih.biBitCount) &&
        (24 != bih.biBitCount) && (32 != bih.biBitCount))
    {
        // 比特位: 8, 16, 24, 32
        goto err_read;
    }

    //宽, 高
    if ((0 >= bih.biWidth) || (0 >= bih.biHeight))
    {
        goto err_read;
    }

    // 8位位图, 读取调色板
    if (8 == bih.biBitCount)
    {
        read_len = (0 == bih.biClrUsed) ? (sizeof(RGBQUAD) * 256) : (bih.biClrUsed * sizeof(RGBQUAD));
        if (1 != fread(palette, read_len, 1, pf))
        {
            goto err_read;
        }
    }

    // 行跨距
    bpp = bih.biBitCount >> 3;
    stride = bpp * bih.biWidth;
    stride = ((stride + 3) >> 2) << 2; // 4 字节对齐

    // 读取图像像素
    size = stride * bih.biHeight;
    assert(size == bih.biSizeImage);

    p_data = KLB_MALLOC(char, size, 0);
    assert(NULL != p_data);

    if (1 != fread(p_data, size, 1, pf))
    {
        goto err_read;
    }

    KLB_FREE(p_data);
    fclose(pf);
    return NULL;

err_read:
    fclose(pf);
    return NULL;
}

int klb_bitmap_write(const char* p_filename, const klb_canvas_t* p_canvas)
{
    FILE* pf = fopen(p_filename, "wb");
    if (NULL == pf)
    {
        return 1;
    }

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };

    int stride = ((p_canvas->stride + 3) >> 2) << 2;

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = p_canvas->rect.w;
    bih.biHeight = p_canvas->rect.h;
    bih.biPlanes = 1;
    bih.biBitCount = 32; // 颜色占用位数
    bih.biSizeImage = stride * bih.biHeight; // 图像数据大小

    bfh.bfType = KLB_BITMAP_FILE;
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bih.biSizeImage; // 完整文件大小
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // 图像数据偏移

    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pf);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pf);

    int zero_b[4] = { 0 };
    int zero_len = stride - p_canvas->stride;

    uint8_t* p_addr = p_canvas->p_addr + p_canvas->stride * (bih.biHeight - 1);
    for (int i = 0; i < bih.biHeight; i++)
    {
        fwrite(p_addr, p_canvas->stride, 1, pf);
        if (0 < zero_len)
        {
            fwrite(zero_b, zero_len, 1, pf);
        }

        p_addr -= p_canvas->stride;
    }

    fclose(pf);
    return 0;
err_write:
    fclose(pf);
    return 1;
}
