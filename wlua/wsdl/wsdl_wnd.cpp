#include "wsdl_wnd.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define WSDL_FONT_MAX       256


wsdl_wnd_t* wsdl_wnd_create()
{
    wsdl_wnd_t* p_wnd = KLB_MALLOCZ(wsdl_wnd_t, 1, 0);

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

    p_wnd->p_texture = SDL_CreateTexture(p_wnd->p_render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET/*SDL_TEXTUREACCESS_STREAMING*/, w, h);

    p_wnd->open = true;

    return 0;
}

void wsdl_wnd_close(wsdl_wnd_t* p_wnd)
{
    if (p_wnd->open)
    {
        KLB_FREE_BY(p_wnd->p_tex_text, SDL_DestroyTexture);
        KLB_FREE_BY(p_wnd->p_texture, SDL_DestroyTexture);
        KLB_FREE_BY(p_wnd->p_render, SDL_DestroyRenderer);
        KLB_FREE_BY(p_wnd->p_window, SDL_DestroyWindow);

        p_wnd->open = false;
    }
}
