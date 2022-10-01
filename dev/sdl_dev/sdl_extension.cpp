#include "sdl_extension.h"
#include "SDL.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_canvas.h"
#include "klua/klua_gui.h"
#include "klbutil/klb_log.h"
#include <assert.h>

#define SDL_EXTENSION       "SDL_EXTENSION"

typedef struct sdl_extension_t_
{
    klua_env_t*         p_env;

    SDL_Window*         p_window;
    SDL_Renderer*       p_render;

    SDL_Surface*        p_surface;
    SDL_Texture*        p_texture;

    klb_canvas_t        canvas;

    klb_gui_t*          p_gui;

    bool                refresh;
    bool                open;
}sdl_extension_t;


//////////////////////////////////////////////////////////////////////////


static volatile int g_init_count = 0;

static void sdl_dev_init()
{
    if (0 == g_init_count)
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    }

    g_init_count++;
}

static void sdl_dev_quit()
{
    g_init_count--;
    if (g_init_count <= 0)
    {
        SDL_Quit();
    }
}

//////////////////////////////////////////////////////////////////////////

void* sdl_extension_create(klua_env_t* p_env)
{
    sdl_dev_init();

    sdl_extension_t* p_ex = KLB_MALLOCZ(sdl_extension_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->refresh = false;
    p_ex->open = false;

    return p_ex;
}

void sdl_extension_destroy(void* ptr)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)ptr;

    KLB_FREE(p_ex);

    sdl_dev_quit();
}

int sdl_extension_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)ptr;

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
                done = 1;
            }
        }
        
        if (event.type == SDL_MOUSEMOTION && NULL != p_ex->p_gui)
        {
            klb_gui_push(p_ex->p_gui, KLB_WM_MOUSEMOVE, event.motion.x, event.motion.y, 0, 0, 0, 0);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && NULL != p_ex->p_gui)
        {
            if (1 == event.button.clicks)
            {
                klb_gui_push(p_ex->p_gui, KLB_WM_LBUTTONDOWN, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
            else if(2 == event.button.clicks)
            {
                klb_gui_push(p_ex->p_gui, KLB_WM_LBUTTONDBLCLK, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
        }
    }

    if (p_ex->refresh)
    {
        SDL_RenderCopy(p_ex->p_render, p_ex->p_texture, NULL, NULL);
        SDL_RenderPresent(p_ex->p_render);

        p_ex->refresh = false;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_sdl(klua_env_t* p_env)
{
    // 注册
    klua_env_extension_t ex = { 0 };

    ex.cb_create = sdl_extension_create;
    ex.cb_destroy = sdl_extension_destroy;
    ex.cb_loop_once = sdl_extension_loop_once;

    klua_env_register_extension(p_env, SDL_EXTENSION, &ex);

    return 0;
}

sdl_extension_t* klua_ex_get_sdl(klua_env_t* p_env)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)klua_env_get_extension(p_env, SDL_EXTENSION);
    if (NULL == p_ex)
    {
        klua_ex_register_sdl(p_env);
        p_ex = (sdl_extension_t*)klua_env_get_extension(p_env, SDL_EXTENSION);
    }

    return p_ex;
}

sdl_extension_t* klua_ex_get_sdl_by_L(lua_State* L)
{
    return klua_ex_get_sdl(klua_env_get_by_L(L));
}

//////////////////////////////////////////////////////////////////////////

static int kluaex_sdl_canvas_lock(klb_canvas_t* p_canvas)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)p_canvas->p_obj;
    if (0 != SDL_LockTextureToSurface(p_ex->p_texture, NULL, &p_ex->p_surface))
    {
        return 1;
    }
    return 0;
}

static int kluaex_sdl_canvas_unlock(klb_canvas_t* p_canvas)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)p_canvas->p_obj;
    SDL_UnlockTexture(p_ex->p_texture);
    return 0;
}

static int kluaex_sdl_canvas_draw_fill(klb_canvas_t* p_canvas, int x, int y, int w, int h, uint32_t color)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)p_canvas->p_obj;
    SDL_Rect rect = { x, y, w, h };
    int ret = SDL_FillRect(p_ex->p_surface, &rect, color);
    return ret;
}


static int kluaex_sdl_canvas_refresh(klb_canvas_t* p_canvas, int x, int y, int w, int h)
{
    sdl_extension_t* p_ex = (sdl_extension_t*)p_canvas->p_obj;
    p_ex->refresh = true;
    return 0;
}

static void kluaex_sdl_init_canvas(sdl_extension_t* p_ex)
{
    p_ex->canvas.p_obj = p_ex;
    p_ex->canvas.color_fmt = KLB_COLOR_FMT_ARGB8888;

    if (0 == SDL_LockTextureToSurface(p_ex->p_texture, NULL, &p_ex->p_surface))
    {
        //KLB_LOG("SDL_LockTextureToSurface ok!,[%p], pixels:[%p],wh[%d,%d],pitch:[%d]\n", p_surface, p_surface->pixels, p_surface->w, p_surface->h, p_surface->pitch);
        //SDL_Rect rect = { 0, 0, 100, 100 };
        //SDL_FillRect(p_surface, &rect, KLB_ARGB8888(255, 20, 20, 200));

        p_ex->canvas.p_addr = (uint8_t*)p_ex->p_surface->pixels;
        p_ex->canvas.rect.w = p_ex->p_surface->w;
        p_ex->canvas.rect.h = p_ex->p_surface->h;
        p_ex->canvas.stride = p_ex->p_surface->pitch;
        p_ex->canvas.mem_len = (int64_t)p_ex->p_surface->pitch * p_ex->p_surface->h;

        SDL_Rect rect = { 0, 0, p_ex->p_surface->w, p_ex->p_surface->h };
        SDL_FillRect(p_ex->p_surface, &rect, KLB_ARGB8888(255, 0, 0, 0));
        SDL_UnlockTexture(p_ex->p_texture);
    }
    else
    {
        KLB_LOG("SDL_LockTextureToSurface error!\n");
    }

    p_ex->canvas.vtable.lock = kluaex_sdl_canvas_lock;
    p_ex->canvas.vtable.unlock = kluaex_sdl_canvas_unlock;
    p_ex->canvas.vtable.draw_fill = kluaex_sdl_canvas_draw_fill;
    p_ex->canvas.vtable.refresh = kluaex_sdl_canvas_refresh;
}

int kluaex_sdl_open_wnd(sdl_extension_t* p_ex, int w, int h, const char* p_title)
{

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
    
    p_ex->p_texture = SDL_CreateTexture(p_ex->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);


    // 初始化画布信息
    kluaex_sdl_init_canvas(p_ex);

    SDL_RenderCopy(p_ex->p_render, p_ex->p_texture, NULL, NULL);
    SDL_RenderPresent(p_ex->p_render);

    p_ex->p_gui = klua_gui_get(p_ex->p_env);
    klb_gui_attach_canvas(p_ex->p_gui, &p_ex->canvas);

    p_ex->refresh = true;
    p_ex->open = true;

    return 0;
}

int kluaex_sdl_close_wnd(sdl_extension_t* p_ex)
{
    p_ex->open = false;

    KLB_FREE_BY(p_ex->p_texture, SDL_DestroyTexture);
    KLB_FREE_BY(p_ex->p_surface, SDL_FreeSurface);
    KLB_FREE_BY(p_ex->p_render, SDL_DestroyRenderer);
    KLB_FREE_BY(p_ex->p_window, SDL_DestroyWindow);

    return 0;
}
