// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
//////////////////////////////////////////////////////////////////////////
// 扩展包: "kpa_mnp"
// 
//////////////////////////////////////////////////////////////////////////
#include "klb_type.h"
#include "klua/klua.h"
#include <stdlib.h>
#include <assert.h>


KLB_API int klua_open_kpa_ws(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
