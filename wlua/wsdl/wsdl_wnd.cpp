#include "wsdl_wnd.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define WSDL_FONT_MAX       256


wsdl_wnd_t* wsdl_wnd_create()
{
    wsdl_wnd_t* p_wnd = KLB_MALLOCZ(wsdl_wnd_t, 1, 0);

    p_wnd->draw_color = KLB_ARGB8888(255, 10, 10, 10);
    p_wnd->font_h = 24;

    p_wnd->open = false;

    return p_wnd;
}

void wsdl_wnd_destroy(wsdl_wnd_t* p_wnd)
{
    // 关闭窗口
    wsdl_wnd_close(p_wnd);

    KLB_FREE(p_wnd);
}

int wsdl_wnd_open(wsdl_wnd_t* p_wnd, klb_gui_t* p_gui, int w, int h, const char* p_title)
{

#if 0
    p_wnd->p_window = SDL_CreateWindow(p_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w/*960*/, h/*540*/, SDL_WINDOW_SHOWN);
    assert(NULL != p_wnd->p_window);
    p_wnd->p_render = SDL_CreateRenderer(p_wnd->p_window, -1, 0);
#else
    SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_SHOWN, &p_wnd->p_window, &p_wnd->p_render);
    assert(NULL != p_wnd->p_window);
    assert(NULL != p_wnd->p_render);

    SDL_SetWindowTitle(p_wnd->p_window, p_title);
#endif

    p_wnd->p_tex_text = SDL_CreateTexture(p_wnd->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, WSDL_FONT_MAX);
    SDL_SetTextureBlendMode(p_wnd->p_tex_text, SDL_BLENDMODE_BLEND);

    p_wnd->p_tex_ui = SDL_CreateTexture(p_wnd->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET/*SDL_TEXTUREACCESS_STREAMING*/, w, h);

    p_wnd->open = true;

    return 0;
}

void wsdl_wnd_close(wsdl_wnd_t* p_wnd)
{
    if (p_wnd->open)
    {
        KLB_FREE_BY(p_wnd->p_tex_text, SDL_DestroyTexture);
        KLB_FREE_BY(p_wnd->p_tex_ui, SDL_DestroyTexture);
        KLB_FREE_BY(p_wnd->p_render, SDL_DestroyRenderer);
        KLB_FREE_BY(p_wnd->p_window, SDL_DestroyWindow);

        p_wnd->open = false;
    }
}

SDL_Renderer* wsdl_wnd_get_render(wsdl_wnd_t* p_wnd)
{
    return p_wnd->p_render;
}

void wsdl_wnd_render_present(wsdl_wnd_t* p_wnd)
{
    if (p_wnd->open)
    {
        SDL_RenderPresent(p_wnd->p_render);
    }
}

void wsdl_wnd_refresh(wsdl_wnd_t* p_wnd)
{
    SDL_SetRenderTarget(p_wnd->p_render, NULL);
    SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_ui, NULL, NULL);
    SDL_RenderPresent(p_wnd->p_render);
}

//////////////////////////////////////////////////////////////////////////

/// @brief 设置绘制颜色
int wsdl_wnd_set_draw_color(wsdl_wnd_t* p_wnd, uint32_t color)
{
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color)& 0xFF;

    SDL_SetRenderDrawColor(p_wnd->p_render, r, g, b, a);

    p_wnd->draw_color = color;

    return 0;
}

/// @brief 获取绘制颜色
/// @return uint32_t ARGB8888颜色值
uint32_t wsdl_wnd_get_draw_color(wsdl_wnd_t* p_wnd)
{
    uint8_t a = 0, r = 0, g = 0, b = 0;

    SDL_GetRenderDrawColor(p_wnd->p_render, &r, &g, &b, &a);

    return KLB_ARGB8888(a, r, g, b);
}

/// @brief 设置字体高度
int wsdl_wnd_set_font_height(wsdl_wnd_t* p_wnd, int h)
{
    p_wnd->font_h = h;
    return 0;
}

/// @brief 获取字体高度
int wsdl_wnd_get_font_height(wsdl_wnd_t* p_wnd)
{
    return p_wnd->font_h;
}

/// @brief 使用单色清空屏幕
int wsdl_wnd_draw_clear(wsdl_wnd_t* p_wnd)
{
    SDL_SetRenderTarget(p_wnd->p_render, p_wnd->p_tex_ui);
    SDL_RenderClear(p_wnd->p_render);

    return 0;
}

/// @brief 绘制点
int wsdl_wnd_draw_point(wsdl_wnd_t* p_wnd, int x, int y)
{
    return SDL_RenderDrawPoint(p_wnd->p_render, x, y);
}

/// @brief 绘制多个点
int wsdl_wnd_draw_points(wsdl_wnd_t* p_wnd, const klb_point_t* p_points, int count)
{
    return 0;
}

/// @brief 绘制线段
int wsdl_wnd_draw_line(wsdl_wnd_t* p_wnd, int x1, int y1, int x2, int y2)
{
    return SDL_RenderDrawLine(p_wnd->p_render, x1, y1, x2, y2);
}

/// @brief 绘制多个线段
int wsdl_wnd_draw_lines(wsdl_wnd_t* p_wnd, const klb_point_t* p_points, int count)
{
    return 0;
}

/// @brief 绘制空心矩形
int wsdl_wnd_draw_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect)
{
    SDL_Rect rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };

    SDL_SetRenderTarget(p_wnd->p_render, p_wnd->p_tex_ui);
    SDL_RenderDrawRect(p_wnd->p_render, &rect);

    return 0;
}

/// @brief 绘制多个空心矩形
int wsdl_wnd_draw_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count)
{
    return 0;
}

/// @brief 使用单色填充绘制单个区域
int wsdl_wnd_draw_fill_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect)
{
    SDL_Rect rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };

    SDL_SetRenderTarget(p_wnd->p_render, p_wnd->p_tex_ui);
    SDL_RenderFillRect(p_wnd->p_render, &rect);

    return 0;
}

/// @brief 使用单色填充多个区域
int wsdl_wnd_draw_fill_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count)
{
    return 0;
}

int wsdl_wnd_draw_text(wsdl_wnd_t* p_wnd, ft_raster_t* p_ft, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len)
{
    SDL_Surface* p_surface = NULL;

    if (0 == SDL_LockTextureToSurface(p_wnd->p_tex_text, NULL, &p_surface))
    {
        SDL_Rect src_rect = { 0, 0, p_rect->w, p_rect->h };
        SDL_FillRect(p_surface, &src_rect, KLB_ARGB8888(0, 0, 0, 0));

        ft_raster_pixels_t raster = { 0 };

        raster.p_pixels = (uint8_t*)p_surface->pixels;
        raster.pitch = p_surface->pitch;
        raster.w = p_surface->w;
        raster.h = p_surface->h;
        raster.color_fmt = KLB_COLOR_FMT_ARGB8888;

        ft_raster_text(p_ft, &raster, 0, 0, p_rect->w, p_rect->h, p_utf8, utf8_len, p_wnd->draw_color, p_wnd->font_h);

        SDL_UnlockTexture(p_wnd->p_tex_text);

        // 
        SDL_SetRenderTarget(p_wnd->p_render, p_wnd->p_tex_ui);

        SDL_Rect dst_rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };
        SDL_RenderCopy(p_wnd->p_render, p_wnd->p_tex_text, &src_rect, &dst_rect);
        SDL_RenderPresent(p_wnd->p_render);
    }

    return 0;
}

int wsdl_wnd_draw_image(wsdl_wnd_t* p_wnd, wsdl_images_t* p_images, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    SDL_Texture* p_tex = wsdl_images_find(p_images, p_wnd->p_render, p_path);

    if (NULL == p_tex)
    {
        return 1;
    }

    SDL_Rect src_rect = { 0, 0, p_dst_rect->w, p_dst_rect->h };
    SDL_Rect dst_rect = { p_dst_rect->x, p_dst_rect->y, p_dst_rect->w, p_dst_rect->h };

    SDL_SetRenderTarget(p_wnd->p_render, p_wnd->p_tex_ui);
    SDL_RenderCopy(p_wnd->p_render, p_tex, NULL/*&src_rect*/, &dst_rect);
    SDL_RenderPresent(p_wnd->p_render);

    return 0;
}

int wsdl_wnd_refresh_rect(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rect)
{
    return 0;
}

int wsdl_wnd_refresh_rects(wsdl_wnd_t* p_wnd, const klb_rect_t* p_rects, int count)
{

    return 0;
}


//////////////////////////////////////////////////////////////////////////

