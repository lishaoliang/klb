// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

// http-flv
int klua_open_khttp_flv(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}

//end
