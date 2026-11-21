// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbrtp/klbrtp.h"
#include "klbnet/klb_netcode.h"
#include "klua/klua.h"


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_krtp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


static int klua_krtp_packetize(lua_State* L)
{
    return push_err_klua_krtp(L, KLB_NETCODE_EOPNOTSUPP);
}


int klua_open_krtp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "packetize",  klua_krtp_packetize },
        { NULL,         NULL }
    };

    luaL_newlib(L, lib);
    return 1;
}


// end
