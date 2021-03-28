#include "klua/klua.h"

int klua_open_kmcache(lua_State* L)
{
    static luaL_Reg lib [] =
    {
        { NULL,     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
