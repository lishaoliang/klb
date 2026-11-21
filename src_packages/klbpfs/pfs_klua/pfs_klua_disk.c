// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"

int pfs_klua_open_kpfs_disk(lua_State *L)
{
    static const luaL_Reg s_lib[] = {
        { "mkpt",       pfs_klua_lua_mkpt },
        { "mkfs",       pfs_klua_lua_mkfs },
        { "mkvol",      pfs_klua_lua_mkvol },
        { NULL,         NULL }
    };

    luaL_newlib(L, s_lib);
    return 1;
}

// end
