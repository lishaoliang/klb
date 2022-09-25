#ifndef __SDL_EXTENSION_H__
#define __SDL_EXTENSION_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct sdl_extension_t_ sdl_extension_t;

int klua_ex_register_sdl(klua_env_t* p_env);

sdl_extension_t* klua_ex_get_sdl(klua_env_t* p_env);
sdl_extension_t* klua_ex_get_sdl_by_L(lua_State* L);


int kluaex_sdl_open_wnd(sdl_extension_t* p_ex);
int kluaex_sdl_close_wnd(sdl_extension_t* p_ex);


#ifdef __cplusplus
}
#endif

#endif // __SDL_EXTENSION_H__
//end
