#include <stdio.h>
#include <string.h>
#include "klua/klua.h"
#include "klbbase/klb_base.h"
#include "klbapp/klb_app.h"


// vld - debug
#if defined(WIN32) && defined(_DEBUG)
#include "vld.h"
#endif // 


// 所有 lua环境 预加载 库
static int klua_openlibs(lua_State* L)
{
    klua_loadlib_all(L);

    return 0;
}

int main(int argc, char** argv)
{
    klb_base_init(NULL);

    int ret = klb_app_main(argc, argv, klua_openlibs);

    klb_base_quit();
    return ret;
}
