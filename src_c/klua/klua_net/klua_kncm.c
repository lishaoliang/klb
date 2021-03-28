#include "klua/klua.h"
#include "klbutil/klb_log.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

int klua_open_kncm(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        //{ "connect",    klua_khttp_connect },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
