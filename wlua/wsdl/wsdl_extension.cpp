#include "wsdl_extension.h"
#include "SDL.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_canvas.h"
#include "klua/klua_gui.h"
#include "klbutil/klb_log.h"
#include "ft_raster.h"
#include "wsdl_images.h"
#include <assert.h>

#define WSDL_EXTENSION      "WSDL_EXTENSION"

#define WSDL_TMP_MAX        1024
#define WSDL_FONT_MAX       256


typedef struct wsdl_extension_t_
{
    klua_env_t*         p_env;

    SDL_Window*         p_window;
    SDL_Renderer*       p_render;

    SDL_Surface*        p_surface;
    SDL_Texture*        p_texture;

    SDL_Texture*        p_tex_text;

    klb_canvas_t        canvas;

    ft_raster_t*        p_ft;           // 字体
    wsdl_images_t*      p_imgs;         // 图片

    klb_gui_t*          p_gui;          // gui

    sds                 base_path;      // 执行文件 路径

    bool                refresh;
    bool                open;
}wsdl_extension_t;

//////////////////////////////////////////////////////////////////////////

static void wsdl_extension_init(wsdl_extension_t* p_ex, const char* p_font_path)
{
    // 基础路径
    char* p_base_path = SDL_GetBasePath();
    p_ex->base_path = sdsnew(p_base_path);
    KLB_FREE_BY(p_base_path, SDL_free);

    // 图片
    p_ex->p_imgs = wsdl_images_create(p_ex->p_render);

    // 字体
    sds font_path = sdsnew(p_base_path);
    font_path = sdscat(font_path, p_font_path);
    p_ex->p_ft = ft_raster_create(font_path);
    KLB_FREE_BY(font_path, sdsfree);
}

static void wsdl_extension_quit(wsdl_extension_t* p_ex)
{
    KLB_FREE_BY(p_ex->p_ft, ft_raster_destroy);
    KLB_FREE_BY(p_ex->p_imgs, wsdl_images_destroy);
    KLB_FREE_BY(p_ex->base_path, sdsfree);
}

void* wsdl_extension_create(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = KLB_MALLOCZ(wsdl_extension_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->refresh = false;
    p_ex->open = false;

    return p_ex;
}

void wsdl_extension_destroy(void* ptr)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;

    kluaex_wsdl_close_wnd(p_ex);

    KLB_FREE(p_ex);
}

int wsdl_extension_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)ptr;

    if (!p_ex->open)
    {
        return 0;
    }

    int done = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) 
    {
        if (event.type == SDL_QUIT) 
        {
            done = 1;
        }

        if (event.type == SDL_KEYDOWN) 
        {
            if (event.key.keysym.sym == SDLK_ESCAPE) 
            {
                //done = 1;
            }
        }

        if (SDL_WINDOWEVENT == event.type)
        {
            if (SDL_WINDOWEVENT_EXPOSED == event.window.event)
            {
                SDL_RenderPresent(p_ex->p_render);
            }
        }
        
        if (event.type == SDL_MOUSEMOTION && NULL != p_ex->p_gui)
        {
            klb_gui_push_msg(p_ex->p_gui, KLB_WM_MOUSEMOVE, event.motion.x, event.motion.y, 0, 0, 0, 0);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && NULL != p_ex->p_gui)
        {
            if (1 == event.button.button)
            {
                klb_gui_push_msg(p_ex->p_gui, KLB_WM_LBUTTONDOWN, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
            else if(3 == event.button.button)
            {
                klb_gui_push_msg(p_ex->p_gui, KLB_WM_LBUTTONDBLCLK, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
        }
    }

    if (p_ex->refresh)
    {
        SDL_SetRenderTarget(p_ex->p_render, NULL);
        SDL_RenderCopy(p_ex->p_render, p_ex->p_texture, NULL, NULL);
        SDL_RenderPresent(p_ex->p_render);

        p_ex->refresh = false;
    }

    if (0 != done)
    {
        klua_env_exit(p_ex->p_env);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int kluaex_register_wsdl(klua_env_t* p_env)
{
    // 注册
    klua_env_extension_t ex = { 0 };

    ex.cb_create = wsdl_extension_create;
    ex.cb_destroy = wsdl_extension_destroy;
    ex.cb_loop_once = wsdl_extension_loop_once;

    klua_env_register_extension(p_env, WSDL_EXTENSION, &ex);

    return 0;
}

wsdl_extension_t* kluaex_get_wsdl(klua_env_t* p_env)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    if (NULL == p_ex)
    {
        kluaex_register_wsdl(p_env);
        p_ex = (wsdl_extension_t*)klua_env_get_extension(p_env, WSDL_EXTENSION);
    }

    return p_ex;
}

wsdl_extension_t* kluaex_get_wsdl_by_L(lua_State* L)
{
    return kluaex_get_wsdl(klua_env_get_by_L(L));
}

//////////////////////////////////////////////////////////////////////////


/// @brief 设置绘制颜色
int kluaex_sdl_canvas_set_draw_color(klb_canvas_t* p_canvas, uint32_t color)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = (color) & 0xFF;

    SDL_SetRenderDrawColor(p_ex->p_render, r, g, b, a);

    p_canvas->draw_color = color;

    return 0;
}

/// @brief 获取绘制颜色
/// @return uint32_t ARGB8888颜色值
static uint32_t kluaex_sdl_canvas_get_draw_color(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    uint8_t a = 0, r = 0, g = 0, b = 0;

    SDL_GetRenderDrawColor(p_ex->p_render, &r, &g, &b, &a);

    return KLB_ARGB8888(a, r, g, b);
}

/// @brief 设置字体高度
static int kluaex_sdl_canvas_set_font_height(klb_canvas_t* p_canvas, int h)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    p_canvas->font_h = h;
    return 0;
}

/// @brief 获取字体高度
static int kluaex_sdl_canvas_get_font_height(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return p_canvas->font_h;
}

/// @brief 加载图片
int kluaex_sdl_canvas_load_image(klb_canvas_t* p_canvas, const char* p_path, int* p_w, int* p_h)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    sds path = sdsnew(p_ex->base_path);
    path = sdscat(path, p_path);

    int ret = wsdl_images_load(p_ex->p_imgs, p_ex->p_render, p_path, path);

    KLB_FREE_BY(path, sdsfree);
    return ret;
}

/// @brief 使用单色清空屏幕
static int kluaex_sdl_canvas_draw_clear(klb_canvas_t* p_canvas)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);
    SDL_RenderClear(p_ex->p_render);

    return 0;
}

/// @brief 绘制点
static int kluaex_sdl_canvas_draw_point(klb_canvas_t* p_canvas, int x, int y)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return SDL_RenderDrawPoint(p_ex->p_render, x, y);
}

/// @brief 绘制多个点
static int kluaex_sdl_canvas_draw_points(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 绘制线段
static int kluaex_sdl_canvas_draw_line(klb_canvas_t* p_canvas, int x1, int y1, int x2, int y2)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return SDL_RenderDrawLine(p_ex->p_render, x1, y1, x2, y2);
}

/// @brief 绘制多个线段
static int kluaex_sdl_canvas_draw_lines(klb_canvas_t* p_canvas, const klb_point_t* p_points, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 绘制空心矩形
static int kluaex_sdl_canvas_draw_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    SDL_Rect rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };

    SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);
    SDL_RenderDrawRect(p_ex->p_render, &rect);

    return 0;
}

/// @brief 绘制多个空心矩形
static int kluaex_sdl_canvas_draw_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return 0;
}

/// @brief 使用单色填充绘制单个区域
static int kluaex_sdl_canvas_draw_fill_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    SDL_Rect rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };

    SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);
    SDL_RenderFillRect(p_ex->p_render, &rect);

    return 0;
}

/// @brief 使用单色填充多个区域
static int kluaex_sdl_canvas_draw_fill_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    return 0;
}

static int kluaex_sdl_canvas_draw_text(klb_canvas_t* p_canvas, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    SDL_Surface* p_surface = NULL;

    if (0 == SDL_LockTextureToSurface(p_ex->p_tex_text, NULL, &p_surface))
    {
        SDL_Rect src_rect = { 0, 0, p_rect->w, p_rect->h };
        SDL_FillRect(p_surface, &src_rect, KLB_ARGB8888(0, 0, 0, 0));

        ft_raster_pixels_t raster = { 0 };

        raster.p_pixels = (uint8_t*)p_surface->pixels;
        raster.pitch = p_surface->pitch;
        raster.w = p_surface->w;
        raster.h = p_surface->h;
        raster.color_fmt = KLB_COLOR_FMT_ARGB8888;

        ft_raster_text(p_ex->p_ft, &raster, 0, 0, p_rect->w, p_rect->h, p_utf8, utf8_len, p_canvas->draw_color, p_canvas->font_h);

        SDL_UnlockTexture(p_ex->p_tex_text);

        // 
        SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);

        SDL_Rect dst_rect = { p_rect->x, p_rect->y, p_rect->w, p_rect->h };
        SDL_RenderCopy(p_ex->p_render, p_ex->p_tex_text, &src_rect, &dst_rect);
        SDL_RenderPresent(p_ex->p_render);
    }
    return 0;
}

int kluaex_sdl_canvas_draw_image(klb_canvas_t* p_canvas, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;

    SDL_Texture* p_tex = wsdl_images_find(p_ex->p_imgs, p_ex->p_render, p_path);

    if (NULL == p_tex)
    {
        return 1;
    }

    SDL_Rect src_rect = { 0, 0, p_dst_rect->w, p_dst_rect->h };
    SDL_Rect dst_rect = { p_dst_rect->x, p_dst_rect->y, p_dst_rect->w, p_dst_rect->h };

    SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);
    SDL_RenderCopy(p_ex->p_render, p_tex, NULL/*&src_rect*/, &dst_rect);
    SDL_RenderPresent(p_ex->p_render);

    return 0;
}

static int kluaex_sdl_canvas_refresh_rect(klb_canvas_t* p_canvas, const klb_rect_t* p_rect)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    p_ex->refresh = true;
    return 0;
}

int kluaex_sdl_canvas_refresh_rects(klb_canvas_t* p_canvas, const klb_rect_t* p_rects, int count)
{
    wsdl_extension_t* p_ex = (wsdl_extension_t*)p_canvas->p_obj;
    p_ex->refresh = true;
    return 0;
}

static void kluaex_sdl_init_canvas(wsdl_extension_t* p_ex)
{
    p_ex->canvas.p_obj = p_ex;
    p_ex->canvas.color_fmt = KLB_COLOR_FMT_ARGB8888;

    //if (0 == SDL_LockTextureToSurface(p_ex->p_texture, NULL, &p_ex->p_surface))
    //{
    //    //KLB_LOG("SDL_LockTextureToSurface ok!,[%p], pixels:[%p],wh[%d,%d],pitch:[%d]\n", p_surface, p_surface->pixels, p_surface->w, p_surface->h, p_surface->pitch);
    //    //SDL_Rect rect = { 0, 0, 100, 100 };
    //    //SDL_FillRect(p_surface, &rect, KLB_ARGB8888(255, 20, 20, 200));
    //    p_ex->canvas.p_addr = (uint8_t*)p_ex->p_surface->pixels;
    //    p_ex->canvas.rect.w = p_ex->p_surface->w;
    //    p_ex->canvas.rect.h = p_ex->p_surface->h;
    //    p_ex->canvas.pitch = p_ex->p_surface->pitch;
    //    p_ex->canvas.mem_len = (int64_t)p_ex->p_surface->pitch * p_ex->p_surface->h;
    //    SDL_Rect rect = { 0, 0, p_ex->p_surface->w, p_ex->p_surface->h };
    //    SDL_FillRect(p_ex->p_surface, &rect, KLB_ARGB8888(255, 0, 0, 0));
    //    SDL_UnlockTexture(p_ex->p_texture);
    //}
    //else
    //{
    //    KLB_LOG("SDL_LockTextureToSurface error!\n");
    //}

    p_ex->canvas.vtable.set_draw_color = kluaex_sdl_canvas_set_draw_color;
    p_ex->canvas.vtable.get_draw_color = kluaex_sdl_canvas_get_draw_color;
    p_ex->canvas.vtable.set_font_height = kluaex_sdl_canvas_set_font_height;
    p_ex->canvas.vtable.get_font_height = kluaex_sdl_canvas_get_font_height;
    p_ex->canvas.vtable.load_image = kluaex_sdl_canvas_load_image;
    p_ex->canvas.vtable.draw_clear = kluaex_sdl_canvas_draw_clear;
    p_ex->canvas.vtable.draw_point = kluaex_sdl_canvas_draw_point;
    p_ex->canvas.vtable.draw_points = kluaex_sdl_canvas_draw_points;
    p_ex->canvas.vtable.draw_line = kluaex_sdl_canvas_draw_line;
    p_ex->canvas.vtable.draw_lines = kluaex_sdl_canvas_draw_lines;
    p_ex->canvas.vtable.draw_rect = kluaex_sdl_canvas_draw_rect;
    p_ex->canvas.vtable.draw_rects = kluaex_sdl_canvas_draw_rects;
    p_ex->canvas.vtable.draw_fill_rect = kluaex_sdl_canvas_draw_fill_rect;
    p_ex->canvas.vtable.draw_fill_rects = kluaex_sdl_canvas_refresh_rects;
    p_ex->canvas.vtable.draw_text = kluaex_sdl_canvas_draw_text;
    p_ex->canvas.vtable.draw_image = kluaex_sdl_canvas_draw_image;
    p_ex->canvas.vtable.refresh_rect = kluaex_sdl_canvas_refresh_rect;
    p_ex->canvas.vtable.refresh_rects = kluaex_sdl_canvas_refresh_rects;
}

int kluaex_wsdl_open_wnd(wsdl_extension_t* p_ex, const char* p_font_path, int w, int h, const char* p_title)
{
    // 初始化
    wsdl_extension_init(p_ex, p_font_path);

#if 0
    p_ex->p_window = SDL_CreateWindow(p_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w/*960*/, h/*540*/, SDL_WINDOW_SHOWN);
    assert(NULL != p_ex->p_window);
    p_ex->p_renderer = SDL_CreateRenderer(p_ex->p_window, -1, 0);
#else
    SDL_CreateWindowAndRenderer(w, h, SDL_WINDOW_SHOWN, &p_ex->p_window, &p_ex->p_render);
    assert(NULL != p_ex->p_window);
    assert(NULL != p_ex->p_render);

    SDL_SetWindowTitle(p_ex->p_window, p_title);
#endif
    
    p_ex->p_tex_text = SDL_CreateTexture(p_ex->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, WSDL_FONT_MAX);
    SDL_SetTextureBlendMode(p_ex->p_tex_text, SDL_BLENDMODE_BLEND);

    p_ex->p_texture = SDL_CreateTexture(p_ex->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET/*SDL_TEXTUREACCESS_STREAMING*/, w, h);

    // 初始化画布信息
    kluaex_sdl_init_canvas(p_ex);

    SDL_SetRenderTarget(p_ex->p_render, p_ex->p_texture);
    SDL_SetRenderDrawColor(p_ex->p_render, 10, 10, 10, 255);
    SDL_RenderClear(p_ex->p_render);

    SDL_SetRenderTarget(p_ex->p_render, NULL);
    SDL_RenderCopy(p_ex->p_render, p_ex->p_texture, NULL, NULL);
    SDL_RenderPresent(p_ex->p_render);

    p_ex->p_gui = klua_gui_get(p_ex->p_env);
    klb_gui_attach_canvas(p_ex->p_gui, &p_ex->canvas);

    p_ex->refresh = true;
    p_ex->open = true;

    return 0;
}

int kluaex_wsdl_close_wnd(wsdl_extension_t* p_ex)
{
    if (p_ex->open)
    {
        p_ex->open = false;

        KLB_FREE_BY(p_ex->p_tex_text, SDL_DestroyTexture);
        KLB_FREE_BY(p_ex->p_texture, SDL_DestroyTexture);
        KLB_FREE_BY(p_ex->p_render, SDL_DestroyRenderer);
        KLB_FREE_BY(p_ex->p_window, SDL_DestroyWindow);

        wsdl_extension_quit(p_ex);
    }

    return 0;
}
