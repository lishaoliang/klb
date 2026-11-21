// Doc Encode : UTF-8 BOM, Unix(LF)
// kbasecode: require("kbasecode") — 仅 strerror (常量见 klbcore.klbcode.basecode)
#include "klua/klua.h"
#include "klbbase/klb_basecode.h"


/// @brief kbasecode.strerror(code) -> string
static int klua_kbasecode_strerror(lua_State* L)
{
    int code = (int)luaL_checkinteger(L, 1);

    lua_pushstring(L, klb_basecode_strerror(code));
    return 1;
}


int klua_open_kbasecode(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "strerror",   klua_kbasecode_strerror },

        { NULL,         NULL }
    };

    luaL_newlib(L, lib);

    return 1;
}

// end
