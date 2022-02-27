#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbmem/klb_mem.h"
#include <stdlib.h>
#include <assert.h>


static int klua_ksys_exit(lua_State* L)
{
    klua_env_exit(klua_env_get_by_L(L));
    return 0;
}


//////////////////////////////////////////////////////////////////////////

int klua_open_ksys(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "exit",           klua_ksys_exit },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
