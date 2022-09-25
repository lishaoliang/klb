#ifndef __SDL_DEV_H__
#define __SDL_DEV_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(_WIN32)
#if defined(__SDL_DEV_DLL__)
#define SDL_DEV_API __declspec(dllexport)
#elif defined(__SDL_DEV_USE_DLL__)
#define SDL_DEV_API __declspec(dllimport)
#else
#define SDL_DEV_API
#endif
#else
#ifdef __cplusplus
#define SDL_DEV_API extern "C"
#else
#define SDL_DEV_API extern
#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

SDL_DEV_API int luaopen_sdl_dev(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __SDL_DEV_H__
//end
