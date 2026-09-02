// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbformat/klb_png.h"
#include "klbutil/klb_color.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifndef __KLB_NO_ZLIB__
#include "klbthird/zlib.h"
#endif


#define KLB_PNG_SIG_LEN     8
#define KLB_PNG_IHDR_LEN    13
#define KLB_PNG_MAX_DIM     16384
#define KLB_PNG_MAX_FILE    (32 * 1024 * 1024)

#define KLB_PNG_IHDR        0x49484452u
#define KLB_PNG_PLTE        0x504C5445u
#define KLB_PNG_IDAT        0x49444154u
#define KLB_PNG_IEND        0x49454E44u
#define KLB_PNG_tRNS        0x74524E53u


#ifndef __KLB_NO_ZLIB__

static const uint8_t g_png_sig_klb_png[KLB_PNG_SIG_LEN] =
{
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};

typedef struct klb_png_info_t_
{
    int         w;
    int         h;
    int         bit_depth;
    int         color_type;
    uint8_t     palette[256][3];
    int         palette_count;
    uint8_t     trns[256];
    int         trns_count;
    bool        has_trns;
    uint8_t*    p_idat;
    int         idat_len;
}klb_png_info_t;


static uint32_t read_be32_klb_png(const uint8_t* p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}


static uint16_t read_be16_klb_png(const uint8_t* p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}


static void write_be32_klb_png(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8) & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}


static bool is_ancillary_klb_png(uint32_t type)
{
    uint8_t c = (uint8_t)((type >> 24) & 0xFF);
    return (0 != (c & 0x20));
}


static int channels_klb_png(int color_type)
{
    if (0 == color_type)
    {
        return 1;
    }
    else if (2 == color_type)
    {
        return 3;
    }
    else if (3 == color_type)
    {
        return 1;
    }
    else if (4 == color_type)
    {
        return 2;
    }
    else if (6 == color_type)
    {
        return 4;
    }

    return 0;
}


static int filter_bpp_klb_png(int color_type, int bit_depth)
{
    int ch = channels_klb_png(color_type);
    if (ch <= 0 || bit_depth <= 0)
    {
        return 0;
    }

    int bpp = (ch * bit_depth) / 8;
    if (bpp <= 0)
    {
        return 1;
    }

    return bpp;
}


static int row_bytes_klb_png(int w, int color_type, int bit_depth)
{
    int ch = channels_klb_png(color_type);
    if (w <= 0 || ch <= 0 || bit_depth <= 0)
    {
        return 0;
    }

    int64_t bits = (int64_t)w * (int64_t)ch * (int64_t)bit_depth;
    int64_t bytes = (bits + 7) / 8;
    if (bytes <= 0 || (int64_t)INT_MAX < bytes)
    {
        return 0;
    }

    return (int)bytes;
}


static bool depth_ok_klb_png(int color_type, int bit_depth)
{
    if (3 == color_type)
    {
        return (1 == bit_depth || 2 == bit_depth || 4 == bit_depth || 8 == bit_depth);
    }
    else if (0 == color_type || 2 == color_type || 4 == color_type || 6 == color_type)
    {
        return (8 == bit_depth || 16 == bit_depth);
    }

    return false;
}


static bool crc_ok_klb_png(const uint8_t* p_type, uint32_t chunk_len, uint32_t crc)
{
    uLong c = crc32(0L, Z_NULL, 0);
    uLong n = 4u + (uLong)chunk_len;
    if ((uLong)UINT_MAX < n)
    {
        return false;
    }

    c = crc32(c, p_type, (uInt)n);
    return ((uint32_t)c == crc);
}


static uint8_t paeth_klb_png(uint8_t a, uint8_t b, uint8_t c)
{
    int p = (int)a + (int)b - (int)c;
    int pa = p - (int)a;
    int pb = p - (int)b;
    int pc = p - (int)c;

    if (pa < 0)
    {
        pa = -pa;
    }

    if (pb < 0)
    {
        pb = -pb;
    }

    if (pc < 0)
    {
        pc = -pc;
    }

    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }

    return c;
}


static bool unfilter_klb_png(uint8_t* p_raw, int h, int row_bytes, int bpp)
{
    int64_t stride64 = 1 + (int64_t)row_bytes;
    if (row_bytes <= 0 || stride64 <= 0 || (int64_t)INT_MAX < stride64)
    {
        return false;
    }

    int stride = (int)stride64;

    for (int y = 0; y < h; y++)
    {
        uint8_t* p_row = p_raw + (int64_t)y * stride;
        uint8_t filter = p_row[0];
        uint8_t* p_data = p_row + 1;
        const uint8_t* p_prev = NULL;

        if (0 < y)
        {
            p_prev = p_raw + (int64_t)(y - 1) * stride + 1;
        }

        if (0 == filter)
        {
            // None
        }
        else if (1 == filter)
        {
            for (int i = 0; i < row_bytes; i++)
            {
                uint8_t left = (i < bpp) ? 0 : p_data[i - bpp];
                p_data[i] = (uint8_t)(p_data[i] + left);
            }
        }
        else if (2 == filter)
        {
            for (int i = 0; i < row_bytes; i++)
            {
                uint8_t up = (NULL == p_prev) ? 0 : p_prev[i];
                p_data[i] = (uint8_t)(p_data[i] + up);
            }
        }
        else if (3 == filter)
        {
            for (int i = 0; i < row_bytes; i++)
            {
                uint8_t left = (i < bpp) ? 0 : p_data[i - bpp];
                uint8_t up = (NULL == p_prev) ? 0 : p_prev[i];
                p_data[i] = (uint8_t)(p_data[i] + (uint8_t)(((int)left + (int)up) / 2));
            }
        }
        else if (4 == filter)
        {
            for (int i = 0; i < row_bytes; i++)
            {
                uint8_t left = (i < bpp) ? 0 : p_data[i - bpp];
                uint8_t up = (NULL == p_prev) ? 0 : p_prev[i];
                uint8_t up_left = (NULL == p_prev || i < bpp) ? 0 : p_prev[i - bpp];
                p_data[i] = (uint8_t)(p_data[i] + paeth_klb_png(left, up, up_left));
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}


static uint16_t channel_sample_klb_png(const uint8_t* p_px, int channel, int bit_depth)
{
    if (16 == bit_depth)
    {
        return read_be16_klb_png(p_px + channel * 2);
    }

    return (uint16_t)p_px[channel];
}


static uint8_t sample_to_u8_klb_png(uint16_t sample, int bit_depth)
{
    if (16 == bit_depth)
    {
        return (uint8_t)(((uint32_t)sample * 255u + 32895u) >> 16);
    }

    return (uint8_t)sample;
}


static uint16_t sample_as_trns16_klb_png(uint16_t sample, int bit_depth)
{
    if (16 == bit_depth)
    {
        return sample;
    }

    uint8_t s8 = (uint8_t)sample;
    return ((uint16_t)s8 << 8) | (uint16_t)s8;
}


static int packed_index_klb_png(const uint8_t* p_row, int x, int bit_depth)
{
    if (8 == bit_depth)
    {
        return (int)p_row[x];
    }
    else if (4 == bit_depth)
    {
        uint8_t b = p_row[x >> 1];
        if (0 == (x & 1))
        {
            return (int)(b >> 4);
        }

        return (int)(b & 0x0F);
    }
    else if (2 == bit_depth)
    {
        uint8_t b = p_row[x >> 2];
        int shift = 6 - 2 * (x & 3);
        return (int)((b >> shift) & 0x03);
    }
    else if (1 == bit_depth)
    {
        uint8_t b = p_row[x >> 3];
        int shift = 7 - (x & 7);
        return (int)((b >> shift) & 0x01);
    }

    return 0;
}


static uint32_t pixel_to_argb8888_klb_png(const uint8_t* p_px, const klb_png_info_t* p_info)
{
    int ct = p_info->color_type;
    int depth = p_info->bit_depth;

    if (0 == ct)
    {
        uint16_t gs = channel_sample_klb_png(p_px, 0, depth);
        uint8_t g = sample_to_u8_klb_png(gs, depth);
        uint8_t a = 0xFF;

        if (p_info->has_trns && 2 <= p_info->trns_count)
        {
            uint16_t tv = read_be16_klb_png(p_info->trns);
            if (tv == sample_as_trns16_klb_png(gs, depth))
            {
                a = 0;
            }
        }

        return KLB_ARGB8888(a, g, g, g);
    }
    else if (2 == ct)
    {
        uint16_t rs = channel_sample_klb_png(p_px, 0, depth);
        uint16_t gs = channel_sample_klb_png(p_px, 1, depth);
        uint16_t bs = channel_sample_klb_png(p_px, 2, depth);
        uint8_t r = sample_to_u8_klb_png(rs, depth);
        uint8_t g = sample_to_u8_klb_png(gs, depth);
        uint8_t b = sample_to_u8_klb_png(bs, depth);
        uint8_t a = 0xFF;

        if (p_info->has_trns && 6 <= p_info->trns_count)
        {
            uint16_t tr = read_be16_klb_png(p_info->trns);
            uint16_t tg = read_be16_klb_png(p_info->trns + 2);
            uint16_t tb = read_be16_klb_png(p_info->trns + 4);
            if (tr == sample_as_trns16_klb_png(rs, depth) &&
                tg == sample_as_trns16_klb_png(gs, depth) &&
                tb == sample_as_trns16_klb_png(bs, depth))
            {
                a = 0;
            }
        }

        return KLB_ARGB8888(a, r, g, b);
    }
    else if (3 == ct)
    {
        int idx = (int)p_px[0];
        if (idx < 0 || p_info->palette_count <= idx)
        {
            idx = 0;
        }

        uint8_t r = p_info->palette[idx][0];
        uint8_t g = p_info->palette[idx][1];
        uint8_t b = p_info->palette[idx][2];
        uint8_t a = 0xFF;

        if (p_info->has_trns && idx < p_info->trns_count)
        {
            a = p_info->trns[idx];
        }

        return KLB_ARGB8888(a, r, g, b);
    }
    else if (4 == ct)
    {
        uint8_t g = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 0, depth), depth);
        uint8_t a = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 1, depth), depth);
        return KLB_ARGB8888(a, g, g, g);
    }
    else if (6 == ct)
    {
        uint8_t r = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 0, depth), depth);
        uint8_t g = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 1, depth), depth);
        uint8_t b = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 2, depth), depth);
        uint8_t a = sample_to_u8_klb_png(channel_sample_klb_png(p_px, 3, depth), depth);
        return KLB_ARGB8888(a, r, g, b);
    }

    return 0;
}


static void fill_canvas_klb_png(klb_canvas_t* p_canvas, const uint8_t* p_raw, const klb_png_info_t* p_info, int color_fmt)
{
    int row_bytes = row_bytes_klb_png(p_info->w, p_info->color_type, p_info->bit_depth);
    int px_bytes = filter_bpp_klb_png(p_info->color_type, p_info->bit_depth);
    int stride = 1 + row_bytes;

    for (int y = 0; y < p_info->h; y++)
    {
        const uint8_t* p_src_row = p_raw + (int64_t)y * stride + 1;
        uint32_t* p_dst_row = (uint32_t*)(p_canvas->p_addr + p_canvas->pitch * y);

        for (int x = 0; x < p_info->w; x++)
        {
            uint32_t argb8888 = 0;

            if (3 == p_info->color_type)
            {
                uint8_t idx = (uint8_t)packed_index_klb_png(p_src_row, x, p_info->bit_depth);
                argb8888 = pixel_to_argb8888_klb_png(&idx, p_info);
            }
            else
            {
                argb8888 = pixel_to_argb8888_klb_png(p_src_row + x * px_bytes, p_info);
            }

            p_dst_row[x] = klb_color_argb8888_to(argb8888, color_fmt);
        }
    }
}


static bool append_idat_klb_png(klb_png_info_t* p_info, const uint8_t* p_data, uint32_t chunk_len)
{
    if (0 == chunk_len)
    {
        return true;
    }

    int64_t new_len64 = (int64_t)p_info->idat_len + (int64_t)chunk_len;
    if (new_len64 <= 0 || (int64_t)INT_MAX < new_len64)
    {
        return false;
    }

    int new_len = (int)new_len64;
    uint8_t* p_new = NULL;

    if (NULL == p_info->p_idat)
    {
        p_new = KLB_MALLOC(uint8_t, new_len, 0);
    }
    else
    {
        p_new = KLB_REALLOC(p_info->p_idat, uint8_t, new_len, 0);
        if (NULL == p_new)
        {
            return false;
        }
    }

    memcpy(p_new + p_info->idat_len, p_data, (size_t)chunk_len);
    p_info->p_idat = p_new;
    p_info->idat_len = new_len;
    return true;
}


static bool parse_ihdr_klb_png(klb_png_info_t* p_info, const uint8_t* p_data, uint32_t chunk_len)
{
    if (KLB_PNG_IHDR_LEN != chunk_len)
    {
        return false;
    }

    uint32_t w = read_be32_klb_png(p_data);
    uint32_t h = read_be32_klb_png(p_data + 4);
    if (0 == w || 0 == h || KLB_PNG_MAX_DIM < w || KLB_PNG_MAX_DIM < h)
    {
        return false;
    }

    int bit_depth = (int)p_data[8];
    int color_type = (int)p_data[9];
    int compression = (int)p_data[10];
    int filter = (int)p_data[11];
    int interlace = (int)p_data[12];

    if (0 != compression || 0 != filter || 0 != interlace)
    {
        return false;
    }

    if (!depth_ok_klb_png(color_type, bit_depth))
    {
        return false;
    }

    p_info->w = (int)w;
    p_info->h = (int)h;
    p_info->bit_depth = bit_depth;
    p_info->color_type = color_type;
    return true;
}


static bool parse_chunks_klb_png(klb_png_info_t* p_info, const uint8_t* p_file, int file_len)
{
    int off = KLB_PNG_SIG_LEN;
    bool seen_ihdr = false;
    bool seen_idat = false;
    bool idat_done = false;
    bool seen_iend = false;

    while (off < file_len)
    {
        if (file_len - off < 12)
        {
            return false;
        }

        uint32_t chunk_len = read_be32_klb_png(p_file + off);
        uint32_t type = read_be32_klb_png(p_file + off + 4);
        if ((uint32_t)INT_MAX < chunk_len)
        {
            return false;
        }

        int64_t need = 12 + (int64_t)chunk_len;
        if ((int64_t)INT_MAX < need || (int64_t)(file_len - off) < need)
        {
            return false;
        }

        const uint8_t* p_type = p_file + off + 4;
        const uint8_t* p_data = p_file + off + 8;
        uint32_t crc = read_be32_klb_png(p_file + off + 8 + (int)chunk_len);
        if (!crc_ok_klb_png(p_type, chunk_len, crc))
        {
            return false;
        }

        if (!seen_ihdr)
        {
            if (KLB_PNG_IHDR != type)
            {
                return false;
            }
        }

        if (KLB_PNG_IHDR == type)
        {
            if (seen_ihdr)
            {
                return false;
            }

            if (!parse_ihdr_klb_png(p_info, p_data, chunk_len))
            {
                return false;
            }

            seen_ihdr = true;
        }
        else if (KLB_PNG_PLTE == type)
        {
            if (!seen_ihdr || seen_idat || 0 != (chunk_len % 3) || 0 == chunk_len || 768 < chunk_len)
            {
                return false;
            }

            p_info->palette_count = (int)(chunk_len / 3);
            memcpy(p_info->palette, p_data, (size_t)chunk_len);
        }
        else if (KLB_PNG_tRNS == type)
        {
            if (!seen_ihdr || seen_idat || 0 == chunk_len || 256 < chunk_len)
            {
                return false;
            }

            memset(p_info->trns, 0xFF, sizeof(p_info->trns));
            memcpy(p_info->trns, p_data, (size_t)chunk_len);
            p_info->trns_count = (int)chunk_len;
            p_info->has_trns = true;
        }
        else if (KLB_PNG_IDAT == type)
        {
            if (!seen_ihdr || idat_done)
            {
                return false;
            }

            if (!append_idat_klb_png(p_info, p_data, chunk_len))
            {
                return false;
            }

            seen_idat = true;
        }
        else if (KLB_PNG_IEND == type)
        {
            if (0 != chunk_len || !seen_ihdr || !seen_idat)
            {
                return false;
            }

            seen_iend = true;
            break;
        }
        else if (!is_ancillary_klb_png(type))
        {
            return false;
        }

        if (seen_idat && KLB_PNG_IDAT != type)
        {
            idat_done = true;
        }

        off += (int)need;
    }

    if (!seen_iend || 0 == p_info->idat_len)
    {
        return false;
    }

    if (3 == p_info->color_type && p_info->palette_count <= 0)
    {
        return false;
    }

    return true;
}


static uint8_t* inflate_idat_klb_png(const uint8_t* p_idat, int idat_len, int raw_len)
{
    uint8_t* p_raw = KLB_MALLOC(uint8_t, raw_len, 0);
    z_stream strm;
    KLB_MEMSET(&strm, 0, sizeof(strm));

    int zret = inflateInit(&strm);
    if (Z_MEM_ERROR == zret)
    {
        assert(false);
    }

    if (Z_OK != zret)
    {
        KLB_FREE(p_raw);
        return NULL;
    }

    strm.next_in = (Bytef*)p_idat;
    strm.avail_in = (uInt)idat_len;
    strm.next_out = (Bytef*)p_raw;
    strm.avail_out = (uInt)raw_len;

    zret = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);

    if (Z_STREAM_END != zret || (uLong)raw_len != strm.total_out)
    {
        KLB_FREE(p_raw);
        return NULL;
    }

    return p_raw;
}


static uint8_t* load_file_klb_png(FILE* pf, int* p_len)
{
    if (0 != fseek(pf, 0, SEEK_END))
    {
        return NULL;
    }

    long file_len_l = ftell(pf);
    if (file_len_l < (long)KLB_PNG_SIG_LEN || (long)KLB_PNG_MAX_FILE < file_len_l)
    {
        return NULL;
    }

    if (0 != fseek(pf, 0, SEEK_SET))
    {
        return NULL;
    }

    int file_len = (int)file_len_l;
    uint8_t* p_file = KLB_MALLOC(uint8_t, file_len, 0);
    if (1 != fread(p_file, (size_t)file_len, 1, pf))
    {
        KLB_FREE(p_file);
        return NULL;
    }

    *p_len = file_len;
    return p_file;
}


static bool write_chunk_klb_png(FILE* pf, uint32_t type, const uint8_t* p_data, uint32_t chunk_len)
{
    uint8_t hdr[8];
    write_be32_klb_png(hdr, chunk_len);
    write_be32_klb_png(hdr + 4, type);

    if (1 != fwrite(hdr, 8, 1, pf))
    {
        return false;
    }

    uLong c = crc32(0L, Z_NULL, 0);
    c = crc32(c, hdr + 4, 4);

    if (0 < chunk_len)
    {
        if (NULL == p_data || (uLong)UINT_MAX < (uLong)chunk_len)
        {
            return false;
        }

        if (1 != fwrite(p_data, (size_t)chunk_len, 1, pf))
        {
            return false;
        }

        c = crc32(c, p_data, (uInt)chunk_len);
    }

    uint8_t crc_buf[4];
    write_be32_klb_png(crc_buf, (uint32_t)c);
    if (1 != fwrite(crc_buf, 4, 1, pf))
    {
        return false;
    }

    return true;
}


static uint8_t* pack_rgba_raw_klb_png(const klb_canvas_t* p_canvas, int* p_raw_len)
{
    int w = p_canvas->rect.w;
    int h = p_canvas->rect.h;
    int stride = 1 + w * 4;
    int64_t raw64 = (int64_t)h * (int64_t)stride;
    if (raw64 <= 0 || (int64_t)INT_MAX < raw64 || (int64_t)UINT_MAX < raw64)
    {
        return NULL;
    }

    int raw_len = (int)raw64;
    uint8_t* p_raw = KLB_MALLOC(uint8_t, raw_len, 0);

    for (int y = 0; y < h; y++)
    {
        uint8_t* p_row = p_raw + (int64_t)y * stride;
        p_row[0] = 0;
        const uint32_t* p_src = (const uint32_t*)(p_canvas->p_addr + p_canvas->pitch * y);
        uint8_t* p_dst = p_row + 1;

        for (int x = 0; x < w; x++)
        {
            uint32_t c = p_src[x];
            p_dst[x * 4 + 0] = (uint8_t)((c >> 16) & 0xFF);
            p_dst[x * 4 + 1] = (uint8_t)((c >> 8) & 0xFF);
            p_dst[x * 4 + 2] = (uint8_t)(c & 0xFF);
            p_dst[x * 4 + 3] = (uint8_t)((c >> 24) & 0xFF);
        }
    }

    *p_raw_len = raw_len;
    return p_raw;
}


static uint8_t* deflate_idat_klb_png(const uint8_t* p_raw, int raw_len, int* p_idat_len)
{
    uLong bound = compressBound((uLong)raw_len);
    if (0 == bound || (uLong)INT_MAX < bound)
    {
        return NULL;
    }

    int dest_cap = (int)bound;
    uint8_t* p_dest = KLB_MALLOC(uint8_t, dest_cap, 0);
    uLongf out_len = (uLongf)dest_cap;

    int zret = compress2((Bytef*)p_dest, &out_len, (const Bytef*)p_raw, (uLong)raw_len, Z_DEFAULT_COMPRESSION);
    if (Z_MEM_ERROR == zret)
    {
        assert(false);
    }

    if (Z_OK != zret || 0 == out_len || (uLong)INT_MAX < out_len)
    {
        KLB_FREE(p_dest);
        return NULL;
    }

    *p_idat_len = (int)out_len;
    return p_dest;
}

#endif // !__KLB_NO_ZLIB__


klb_canvas_t* klb_png_read(const char* p_filename, int color_fmt)
{
#ifdef __KLB_NO_ZLIB__
    (void)p_filename;
    (void)color_fmt;
    return NULL;
#else
    if (NULL == p_filename)
    {
        return NULL;
    }

    FILE* pf = fopen(p_filename, "rb");
    if (NULL == pf)
    {
        return NULL;
    }

    uint8_t* p_file = NULL;
    uint8_t* p_raw = NULL;
    klb_canvas_t* p_canvas = NULL;
    klb_png_info_t info;
    KLB_MEMSET(&info, 0, sizeof(info));

    // step1. 读入文件
    int file_len = 0;
    p_file = load_file_klb_png(pf, &file_len);
    fclose(pf);
    pf = NULL;
    if (NULL == p_file)
    {
        return NULL;
    }

    if (0 != memcmp(p_file, g_png_sig_klb_png, KLB_PNG_SIG_LEN))
    {
        goto err_read;
    }

    // step2. 解析 chunk (IHDR/PLTE/tRNS/IDAT/IEND)
    if (!parse_chunks_klb_png(&info, p_file, file_len))
    {
        goto err_read;
    }

    KLB_FREE(p_file);

    int row_bytes = row_bytes_klb_png(info.w, info.color_type, info.bit_depth);
    int bpp = filter_bpp_klb_png(info.color_type, info.bit_depth);
    if (0 == row_bytes || 0 == bpp)
    {
        goto err_read;
    }

    int64_t raw64 = (int64_t)info.h * (1 + (int64_t)row_bytes);
    if (raw64 <= 0 || (int64_t)INT_MAX < raw64 || (int64_t)UINT_MAX < raw64)
    {
        goto err_read;
    }

    if ((int64_t)UINT_MAX < (int64_t)info.idat_len)
    {
        goto err_read;
    }

    // step3. zlib inflate IDAT
    int raw_len = (int)raw64;
    p_raw = inflate_idat_klb_png(info.p_idat, info.idat_len, raw_len);
    KLB_FREE(info.p_idat);
    if (NULL == p_raw)
    {
        return NULL;
    }

    if (!unfilter_klb_png(p_raw, info.h, row_bytes, bpp))
    {
        KLB_FREE(p_raw);
        return NULL;
    }

    // step4. 填入画布
    p_canvas = klb_canvas_create(info.w, info.h, KLB_COLOR_FMT_ARGB8888);
    if (NULL == p_canvas)
    {
        KLB_FREE(p_raw);
        return NULL;
    }

    fill_canvas_klb_png(p_canvas, p_raw, &info, color_fmt);
    KLB_FREE(p_raw);
    return p_canvas;

err_read:
    KLB_FREE(p_file);
    KLB_FREE(info.p_idat);
    KLB_FREE(p_raw);
    return NULL;
#endif
}


int klb_png_write(const char* p_filename, const klb_canvas_t* p_canvas)
{
#ifdef __KLB_NO_ZLIB__
    (void)p_filename;
    (void)p_canvas;
    return 1;
#else
    if (NULL == p_filename || NULL == p_canvas || NULL == p_canvas->p_addr)
    {
        return 1;
    }

    int w = p_canvas->rect.w;
    int h = p_canvas->rect.h;
    if (w <= 0 || h <= 0 || KLB_PNG_MAX_DIM < w || KLB_PNG_MAX_DIM < h)
    {
        return 1;
    }

    if (KLB_COLOR_FMT_ARGB8888 != p_canvas->color_fmt)
    {
        return 1;
    }

    if (p_canvas->pitch < (int64_t)w * 4)
    {
        return 1;
    }

    FILE* pf = fopen(p_filename, "wb");
    if (NULL == pf)
    {
        return 1;
    }

    uint8_t* p_raw = NULL;
    uint8_t* p_idat = NULL;

    // step1. ARGB8888 -> filter None RGBA
    int raw_len = 0;
    p_raw = pack_rgba_raw_klb_png(p_canvas, &raw_len);
    if (NULL == p_raw)
    {
        fclose(pf);
        return 1;
    }

    // step2. zlib deflate IDAT
    int idat_len = 0;
    p_idat = deflate_idat_klb_png(p_raw, raw_len, &idat_len);
    KLB_FREE(p_raw);
    if (NULL == p_idat)
    {
        fclose(pf);
        return 1;
    }

    // step3. 写 signature + IHDR + IDAT + IEND
    if (1 != fwrite(g_png_sig_klb_png, KLB_PNG_SIG_LEN, 1, pf))
    {
        goto err_write;
    }

    uint8_t ihdr[KLB_PNG_IHDR_LEN];
    write_be32_klb_png(ihdr, (uint32_t)w);
    write_be32_klb_png(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8;
    ihdr[9] = 6;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;

    if (!write_chunk_klb_png(pf, KLB_PNG_IHDR, ihdr, KLB_PNG_IHDR_LEN))
    {
        goto err_write;
    }

    if (!write_chunk_klb_png(pf, KLB_PNG_IDAT, p_idat, (uint32_t)idat_len))
    {
        goto err_write;
    }

    if (!write_chunk_klb_png(pf, KLB_PNG_IEND, NULL, 0))
    {
        goto err_write;
    }

    KLB_FREE(p_idat);
    fclose(pf);
    return 0;

err_write:
    KLB_FREE(p_idat);
    fclose(pf);
    return 1;
#endif
}

// end
