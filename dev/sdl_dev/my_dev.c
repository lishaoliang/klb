#include "my_dev.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbutil/klb_color.h"
#include "sdl_canvas.h"
#include "klbgui/klb_gui.h"
#include <assert.h>

static int my_dev_thread(void* p_obj, volatile int* p_run);


my_dev_t* my_dev_create()
{
    my_dev_t* p_dev = KLB_MALLOC(my_dev_t, 1, 0);
    KLB_MEMSET(p_dev, 0, sizeof(my_dev_t));

    p_dev->init_thread_ok = false;
    p_dev->p_thread = klb_thread_create(my_dev_thread, p_dev, -1, "sdl dev window");

    while (!p_dev->init_thread_ok)
    {
        SDL_Delay(1);
    }

    return p_dev;
}

void my_dev_destroy(my_dev_t* p_dev)
{
    KLB_FREE_BY(p_dev->p_thread, klb_thread_destroy);
    KLB_FREE(p_dev);
}

int my_dev_start(my_dev_t* p_dev)
{
    return 0;
}

void my_dev_stop(my_dev_t* p_dev)
{

}

//////////////////////////////////////////////////////////////////////////


static int test_my_dlgs_btn_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        KLB_LOG("klb_button_on_command,[%d],[%d,%d]\n", msg, p_pt1->x, p_pt1->y);
        break;
    default:
        break;
    }

    return 0;
}

static void init_my_dlgs(klb_gui_t* p_gui)
{
    klb_gui_append(p_gui, "kdialog", "/home", 0, 0, 960, 540, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn1", 10, 10, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn2", 10, 40, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn3", 10, 70, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn4", 10, 100, 100, 24, NULL);

    klb_gui_bind_command(p_gui, "/home/btn4", test_my_dlgs_btn_on_command, NULL);
}

static void my_dev_push_msg(my_dev_t* p_dev, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
{
    if (NULL != p_dev->cb_msg)
    {
        p_dev->cb_msg(msg, x1, y1, x2, y2, lparam, wparam);
    }
}

static int my_dev_thread(void* p_obj, volatile int* p_run)
{
    KLB_LOG("start thread,name:[sdl dev window]\n");
    my_dev_t* p_dev = (my_dev_t*)p_obj;

    p_dev->p_window = SDL_CreateWindow("sld dev window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 960, 540, SDL_WINDOW_SHOWN);
    assert(NULL != p_dev->p_window);
    p_dev->p_renderer = SDL_CreateRenderer(p_dev->p_window, -1, 0);
    p_dev->p_texture = SDL_CreateTexture(p_dev->p_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 960, 540);

    p_dev->p_sdl_canvas = sdl_canvas_create(p_dev->p_texture);

    SDL_RenderCopy(p_dev->p_renderer, p_dev->p_texture, NULL, NULL);
    SDL_RenderPresent(p_dev->p_renderer);

    //klb_gui_t* p_gui = klb_gui_create(p_sdl_canvas->p_canvas);
    //init_my_dlgs(p_gui);

    //klb_gui_start(p_gui);

    p_dev->init_thread_ok = true;

    int done = 0;
    while (*p_run)
    {
        bool sleep = true;
        SDL_Event event;
        while (SDL_PollEvent(&event) > 0) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    done = 1;
                }
            }
            if (event.type == SDL_MOUSEMOTION)
            {
                my_dev_push_msg(p_dev, KLB_WM_MOUSEMOVE, event.motion.x, event.motion.y, 0, 0, 0, 0);
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (1 == event.button.clicks)
                {
                    my_dev_push_msg(p_dev, KLB_WM_LBUTTONDOWN, event.motion.x, event.motion.y, 0, 0, 0, 0);
                }
            }

            sleep = false;
        }

        if (sdl_canvas_need_refresh(p_dev->p_sdl_canvas))
        {
            SDL_RenderCopy(p_dev->p_renderer, p_dev->p_texture, NULL, NULL);
            SDL_RenderPresent(p_dev->p_renderer);

            sdl_canvas_set_refresh_ok(p_dev->p_sdl_canvas);
        }

        if (sleep)
        {
            SDL_Delay(1);
        }
    }


    //klb_gui_stop(p_gui);
    //KLB_FREE_BY(p_gui, klb_gui_destroy);

    KLB_FREE_BY(p_dev->p_sdl_canvas, sdl_canvas_destroy);

    KLB_FREE_BY(p_dev->p_texture, SDL_DestroyTexture);
    KLB_FREE_BY(p_dev->p_suface, SDL_FreeSurface);
    KLB_FREE_BY(p_dev->p_renderer, SDL_DestroyRenderer);
    KLB_FREE_BY(p_dev->p_window, SDL_DestroyWindow);

    KLB_LOG("stop thread,name:[sdl dev window]\n");
    return 0;
}
