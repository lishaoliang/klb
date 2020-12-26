#ifndef __MY_DEV_H__
#define __MY_DEV_H__

#include "klb_type.h"
#include "platform/klb_thread.h"
#include "sdl_canvas.h"
#include "mnp/klb_mnp_dev.h"
#include "SDL.h"

typedef struct my_dev_t_
{
    SDL_Window*         p_window;
    SDL_Renderer*       p_renderer;

    SDL_Surface*        p_suface;
    SDL_Texture*        p_texture;

    klb_thread_t*       p_thread;

    sdl_canvas_t*       p_sdl_canvas;
    klb_mnp_msg_cb      cb_msg;

    bool                init_thread_ok;
}my_dev_t;


my_dev_t* my_dev_create();
void my_dev_destroy(my_dev_t* p_dev);

int my_dev_start(my_dev_t* p_dev);
void my_dev_stop(my_dev_t* p_dev);


#endif // __MY_DEV_H__
//end
