#include "SDL.h"
#include "klua/klua.h"
#include "klbbase/klb_base.h"

#include "wsdl.h"

#if defined(WIN32) && defined(DEBUG_VLD)
#include "vld.h"
#endif

static int wlua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);

    klua_loadlib(L, luaopen_wsdl, "wsdl");

    return 0;
}

int main(int argc, char *argv[])
{
    klb_base_init(NULL);
    SDL_Init(SDL_INIT_EVERYTHING);

    int ret = klua_main(argc, argv, wlua_openlibs);

    SDL_Quit();
    klb_base_quit();

    return ret;
}
