// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
//////////////////////////////////////////////////////////////////////////
// 扩展包: "kpa_rtsp"
// 
//////////////////////////////////////////////////////////////////////////
#include "klb_type.h"
#include "klua/klua.h"
#include <stdlib.h>
#include <assert.h>

#include "kpa_rtsp/kpa_rtsp_client.h"



KLB_API int klua_open_kpa_rtsp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",        kpa_rtsp_client_connect },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // rtsp client meta
    kpa_rtsp_client_createmeta(L);

    return 1;
}
