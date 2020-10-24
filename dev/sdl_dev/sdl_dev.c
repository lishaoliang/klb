#include "sdl_dev.h"
#include "SDL.h"
#include "mem/klb_mem.h"
#include "my_dev.h"
#include <assert.h>


static my_dev_t* g_my_dev = NULL;


//////////////////////////////////////////////////////////////////////////

static uint32_t my_version()
{
    return 10101;
}

static const char* my_project()
{
    return "emulator";
}


static int my_init(const char* p_json_param)
{
    assert(NULL == g_my_dev);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    g_my_dev = my_dev_create();
    assert(NULL != g_my_dev);

    return 0;
}

static int my_start(const char* p_json_param)
{
    my_dev_start(g_my_dev);

    return 0;
}


static void my_stop()
{
    my_dev_stop(g_my_dev);
}

static void my_quit()
{
    KLB_FREE_BY(g_my_dev, my_dev_destroy);

    SDL_Quit();
}


static klb_canvas_t* my_get_canvas(int index)
{
    if (NULL != g_my_dev && g_my_dev->init_thread_ok)
    {
        return g_my_dev->p_sdl_canvas->p_canvas;
    }

    return NULL;
}

static void my_set_msg_receiver(int index, klb_mnp_msg_cb cb_msg)
{
    if (NULL != g_my_dev && g_my_dev->init_thread_ok)
    {
        g_my_dev->cb_msg = cb_msg;
    }
}

//////////////////////////////////////////////////////////////////////////

SDL_DEV_API int klb_mnp_dev_open(uint32_t version, void* p_interface, int interface_size)
{
    klb_mnp_dev_interface_t* p_dev = (klb_mnp_dev_interface_t*)p_interface;

    p_dev->version = my_version;
    p_dev->project = my_project;

    p_dev->init = my_init;
    p_dev->start = my_start;
    p_dev->stop = my_stop;
    p_dev->quit = my_quit;

    p_dev->get_canvas = my_get_canvas;
    p_dev->set_msg_receiver = my_set_msg_receiver;

    return 0;
}
