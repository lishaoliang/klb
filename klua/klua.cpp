#include <stdio.h>
#include <string.h>
#include "klua/klua.h"
#include "klbbase/klb_base.h"


// vld - debug
#if defined(WIN32) && defined(_DEBUG)
#include "vld.h"
#endif // 


static int klua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);

    return 0;
}

int main(int argc, char** argv)
{
    klb_base_init(NULL);

    int ret = klua_main(argc, argv, klua_openlibs);

    klb_base_quit();
    return ret;
}
