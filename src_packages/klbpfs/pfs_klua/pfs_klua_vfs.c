// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"
#include "klua/klua.h"

int pfs_klua_open_kpfs_vfs(lua_State *L)
{
    static const luaL_Reg s_lib[] = {
        { "open",           pfs_klua_file_open },
        { "opendir",        pfs_klua_dir_opendir },
        { NULL,             NULL }
    };

    pfs_klua_dir_setup(L);
    pfs_klua_file_setup(L);
    luaL_newlib(L, s_lib);
    pfs_klua_vfs_path_register(L);
    return 1;
}

// end
