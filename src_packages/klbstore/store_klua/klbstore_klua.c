// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbstore/klbstore.h"
#include "klbstore/core/klb_store_core.h"
#include "klua/klua.h"

#include <stdio.h>
#include <string.h>


static int klua_kstore_available(lua_State* L)
{
    lua_pushboolean(L, 1);
    return 1;
}


static int klua_kstore_valid_id(lua_State* L)
{
    const char* p_id = NULL;

    p_id = luaL_checkstring(L, 1);
    lua_pushboolean(L, klb_store_core_valid_id(p_id) ? 1 : 0);

    return 1;
}


static int klua_kstore_rel_path(lua_State* L)
{
    char buf[KLB_STORE_PATH_MAX];
    const char* p_id = NULL;
    int rc = 0;

    p_id = luaL_checkstring(L, 1);
    rc = klb_store_core_rel_path(p_id, buf, (int)sizeof(buf));

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushinteger(L, rc);
        return 2;
    }

    lua_pushstring(L, buf);
    return 1;
}


static int klua_kstore_abs_path(lua_State* L)
{
    klb_store_t store;
    char buf[KLB_STORE_PATH_MAX];
    const char* p_root = NULL;
    const char* p_id = NULL;
    int rc = 0;

    p_root = luaL_checkstring(L, 1);
    p_id = luaL_checkstring(L, 2);

    if ((int)strlen(p_root) >= KLB_STORE_PATH_MAX)
    {
        lua_pushnil(L);
        lua_pushinteger(L, KLB_BASECODE_EINVAL);
        return 2;
    }

    memset(&store, 0, sizeof(store));
    snprintf(store.root, sizeof(store.root), "%s", p_root);

    rc = klb_store_core_abs_path(&store, p_id, buf, (int)sizeof(buf));

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushinteger(L, rc);
        return 2;
    }

    lua_pushstring(L, buf);
    return 1;
}


int klua_open_kstore(lua_State* L)
{
    static luaL_Reg lib[] = {
        { "available",  klua_kstore_available },
        { "valid_id",   klua_kstore_valid_id },
        { "rel_path",   klua_kstore_rel_path },
        { "abs_path",   klua_kstore_abs_path },
        { NULL, NULL }
    };

    luaL_newlib(L, lib);
    return 1;
}

// end
