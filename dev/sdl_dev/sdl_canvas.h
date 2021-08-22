#ifndef __SDL_CANVAS_H__
#define __SDL_CANVAS_H__

#include "klb_type.h"
#include "klbutil/klb_canvas.h"
#include "SDL.h"

typedef struct sdl_canvas_t_
{
    klb_canvas_t*   p_canvas;

    SDL_Texture*    p_texture;
    SDL_Surface*    p_surface;

    bool            refresh;
}sdl_canvas_t;


sdl_canvas_t* sdl_canvas_create(SDL_Texture* p_texture);
void sdl_canvas_destroy(sdl_canvas_t* p_sdl);

bool sdl_canvas_need_refresh(sdl_canvas_t* p_sdl);
void sdl_canvas_set_refresh_ok(sdl_canvas_t* p_sdl);

#endif // __SDL_CANVAS_H__
//end
