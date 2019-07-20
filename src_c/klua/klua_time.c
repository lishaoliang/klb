#include "klua/klua.h"
#include "time/klb_time.h"


static int klua_ktime_sleep(lua_State* L)
{
    return 0;
}

int klua_pre_open_ktime(lua_State* L)
{
    static luaL_Reg ktime_lib[] =
    {
        { "sleep",  klua_ktime_sleep },

        { NULL,     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, ktime_lib);

    return 1;
}
