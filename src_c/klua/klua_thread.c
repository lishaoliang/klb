#include "klua/klua.h"


int klua_open_kthread(lua_State* L)
{
    static luaL_Reg kthread_lib[] =
    {
        { NULL,     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, kthread_lib);

    return 1;
}
