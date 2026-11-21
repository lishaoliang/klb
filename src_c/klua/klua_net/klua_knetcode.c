// Doc Encode : UTF-8 BOM, Unix(LF)
// knetcode: require("knetcode") — klb_netcode_e 查询
#include "klua/klua.h"
#include "klbnet/klb_netcode.h"


/// @brief knetcode.strerror(code) -> string
static int klua_knetcode_strerror(lua_State* L)
{
    int code = (int)luaL_checkinteger(L, 1);

    lua_pushstring(L, klb_netcode_strerror(code));
    return 1;
}


int klua_open_knetcode(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "strerror",   klua_knetcode_strerror },

        { NULL,         NULL }
    };

    luaL_newlib(L, lib);

    return 1;
}

// end
