#include "ft_raster.h"
#include "SDL.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"
#include <math.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H


// 32位数26.6, 前26位整数部分; 后6位小数部分
#define FT_int32(i32_)      ((i32_) << 6)

// dpi
#define FT_dpi              72

//utf8 -> unicode
#define FT_utf8_to_unicode(UTF8_, UNICODE_, LEN_) \
{ \
    int nnn = 0; \
    unsigned char uft8_ch = UTF8_[nnn++]; \
    if((uft8_ch & 0x80) == 0) \
    { \
        UNICODE_ = uft8_ch; \
        LEN_ = 1; \
    } \
    else if((uft8_ch & 0xe0) == 0xe0) \
    { \
        UNICODE_ = (uft8_ch & 0x1F) << 12; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 3; \
    } \
    else \
    { \
        UNICODE_ = (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 2; \
    } \
}

typedef struct ft_raster_t_
{
    FT_Library      library;
    FT_Face         face;

    FT_GlyphSlot    slot;
}ft_raster_t;


ft_raster_t* ft_raster_create(const char* p_font_path)
{
    ft_raster_t* p_ft = KLB_MALLOCZ(ft_raster_t, 1, 0);

    FT_Error err = FT_Init_FreeType(&p_ft->library);                /* initialize library */
    assert(0 == err);

    err = FT_New_Face(p_ft->library, p_font_path, 0, &p_ft->face);  /* create face object */
    assert(0 == err);

    p_ft->slot = p_ft->face->glyph;

    return p_ft;
}

void ft_raster_destroy(ft_raster_t* p_ft)
{
    assert(NULL != p_ft);

    KLB_FREE_BY(p_ft->face, FT_Done_Face);
    KLB_FREE_BY(p_ft->library, FT_Done_FreeType);
    KLB_FREE(p_ft);
}

static void draw_bitmap(ft_raster_pixels_t* p_raster, FT_Bitmap* bitmap, FT_Int x, FT_Int y, uint32_t color)
{
    FT_Int  i, j, p, q;
    FT_Int  x_max = x + bitmap->width;
    FT_Int  y_max = y + bitmap->rows;

    /* for simplicity, we assume that `bitmap->pixel_mode' */
    /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

    for (i = x, p = 0; i < x_max; i++, p++)
    {
        for (j = y, q = 0; j < y_max; j++, q++)
        {
            if (i < 0 || j < 0 ||
                i >= p_raster->w || j >= p_raster->h)
            {
                continue;
            }

            uint32_t v = bitmap->buffer[q * bitmap->width + p];
            if (0 != v)
            {
                uint32_t r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = (color) & 0xFF;

                r = r * v / 255;
                g = g * v / 255;
                b = b * v / 255;

                uint32_t argb = KLB_ARGB8888(255, r, g, b);
                // [j][i]
                uint8_t* ptr = p_raster->p_pixels + p_raster->pitch * j + i * 4;
                
                memcpy(ptr, &argb, sizeof(uint32_t));
            }
        }
    }
}

int ft_raster_text(ft_raster_t* p_ft, ft_raster_pixels_t* p_raster, int x, int y, int w, int h, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
{
    FT_GlyphSlot slot = p_ft->face->glyph;

    /* use font_h pt at 72dpi */
    int error = FT_Set_Char_Size(p_ft->face, FT_int32(font_h), FT_int32(font_h), FT_dpi, FT_dpi);     /* set character size */

    FT_Matrix   matrix;         /* transformation matrix */

    double angle = 0.0;
    matrix.xx = (FT_Fixed)(SDL_cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed)(-SDL_sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed)(SDL_sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed)(SDL_cos(angle) * 0x10000L);

    // freetype 采用的英文基准线
    int offset_y = 2;
    if (28 < font_h)
    {
        offset_y = 4;
    }

    FT_Vector   pen;            /* untransformed origin  */

    /* the pen position in 26.6 cartesian space coordinates; */
    /* start at (x,y+h) relative to the upper left corner  */
    pen.x = FT_int32(x);
    pen.y = FT_int32(p_raster->h - (y + h) + offset_y);

    for (int i = 0; i < utf8_len; )
    {
        /* set transformation */
        FT_Set_Transform(p_ft->face, &matrix, &pen);

        uint32_t ch = 0;
        int len = 0;
        FT_utf8_to_unicode((p_utf8 + i), ch, len);

        if (utf8_len < i + len)
        {
            break;
        }

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(p_ft->face, ch, FT_LOAD_RENDER);
        if (error)
        {
            continue;                 /* ignore errors */
        }

        /* now, draw to our target surface (convert position) */
        draw_bitmap(p_raster, &slot->bitmap, slot->bitmap_left, p_raster->h - slot->bitmap_top, color);

        /* increment pen position */
        pen.x += slot->advance.x;
        pen.y += slot->advance.y;

        i += len;
    }

    return 0;
}
