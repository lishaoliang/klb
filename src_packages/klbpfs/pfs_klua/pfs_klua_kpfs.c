// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua_ex.h"
#include "pfs_klua.h"
#include "pfs_klua_internal.h"
#include "pfs_klua_util.h"
#include "pfs_disk.h"
#include "pfs.h"
#include "klua/klua.h"

#include <string.h>

static int pfs_klua_kpfs_version(lua_State *L)
{
    lua_pushstring(L, "0.1");
    return 1;
}

static bool pfs_klua_mount_is_mode_array(lua_State *L, int idx)
{
    if (0 == lua_istable(L, idx))
    {
        return false;
    }

    lua_rawgeti(L, idx, 1);
    if (0 == lua_isstring(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    lua_pop(L, 1);
    return true;
}

static int pfs_klua_kpfs_mount(lua_State *L)
{
    const char *p_name = luaL_checkstring(L, 1);
    const char *p_image_path = luaL_checkstring(L, 2);
    pfs_disk_mount_opts_t opts;

    pfs_klua_mount_opts_init(&opts);
    int top = lua_gettop(L);
    int mode_idx = 0;
    int extra_idx = 0;

    if (top >= 3 && 0 == lua_isnoneornil(L, 3))
    {
        if (0 != lua_isstring(L, 3))
        {
            mode_idx = 3;
        }
        else if (pfs_klua_mount_is_mode_array(L, 3))
        {
            mode_idx = 3;
        }
        else if (0 != lua_istable(L, 3))
        {
            extra_idx = 3;
        }
        else
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
    }

    if (top >= 4 && 0 != lua_istable(L, 4))
    {
        extra_idx = 4;
    }

    if (mode_idx > 0)
    {
        if (0 != pfs_klua_mount_opts_from_lua(L, mode_idx, &opts))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
    }

    if (extra_idx > 0)
    {
        if (0 != pfs_klua_parse_mount_extra_opts(L, extra_idx, &opts))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
    }

    pfs_klua_ex_t *p_ex = pfs_klua_ex_get_by_L(L);
    int rc = pfs_klua_ex_mount(p_ex, p_name, p_image_path, &opts);

    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_kpfs_remount(lua_State *L)
{
    const char *p_name = luaL_checkstring(L, 1);
    pfs_klua_ex_t *p_ex = pfs_klua_ex_get_by_L(L);
    int top = lua_gettop(L);

    if (top < 2)
    {
        int rc = pfs_klua_ex_remount(p_ex, p_name, -1, PFS_KLUA_MNT_STYLE_R);

        return pfs_klua_push_rc(L, rc);
    }

    if (0 != lua_isstring(L, 2))
    {
        int style;

        if (0 != pfs_klua_parse_mount_mode_lua(L, 2, &style))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        int rc = pfs_klua_ex_remount(p_ex, p_name, -1, style);

        return pfs_klua_push_rc(L, rc);
    }

    if (0 == lua_istable(L, 2))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    if (pfs_klua_remount_is_single_item(L, 2))
    {
        int lua_idx = (int)pfs_klua_opts_integer(L, 2, "index", 1);
        int c_idx = pfs_klua_lua_part_to_c(lua_idx);

        if (c_idx < 0)
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }

        lua_getfield(L, 2, "mode");
        int style;

        if (0 != pfs_klua_parse_mount_mode_lua(L, -1, &style))
        {
            lua_pop(L, 1);
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        lua_pop(L, 1);

        int rc = pfs_klua_ex_remount(p_ex, p_name, c_idx, style);

        return pfs_klua_push_rc(L, rc);
    }

    int n = pfs_klua_remount_item_count(L, 2);

    if (n <= 0)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int ok_count = 0;
    int tried = 0;
    int last_fail = PFS_OK;

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, 2, i);
        if (0 == lua_istable(L, -1))
        {
            lua_pop(L, 1);
            break;
        }

        int lua_idx = (int)pfs_klua_opts_integer(L, -1, "index", 0);

        if (lua_idx < 1)
        {
            lua_pop(L, 1);
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }

        int c_idx = pfs_klua_lua_part_to_c(lua_idx);

        if (c_idx < 0)
        {
            lua_pop(L, 1);
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }

        lua_getfield(L, -1, "mode");
        int style;

        if (0 != pfs_klua_parse_mount_mode_lua(L, -1, &style))
        {
            lua_pop(L, 2);
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        lua_pop(L, 2);

        tried++;
        int rc = pfs_klua_ex_remount(p_ex, p_name, c_idx, style);

        if (PFS_OK == rc)
        {
            ok_count++;
        }
        else
        {
            last_fail = rc;
        }
    }

    if (0 == tried)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }
    if (0 == ok_count)
    {
        return pfs_klua_push_rc(L, (PFS_OK != last_fail) ? last_fail : PFS_EOPNOTSUPP);
    }

    return pfs_klua_push_rc(L, PFS_OK);
}

static int pfs_klua_kpfs_umount(lua_State *L)
{
    const char *p_name = luaL_checkstring(L, 1);
    pfs_klua_ex_t *p_ex = pfs_klua_ex_get_by_L(L);
    int top = lua_gettop(L);

    if (top < 2)
    {
        int rc = pfs_klua_ex_umount(p_ex, p_name, -1);

        return pfs_klua_push_rc(L, rc);
    }

    if (0 != lua_isinteger(L, 2) || 0 != lua_isnumber(L, 2))
    {
        int c_idx = pfs_klua_lua_part_to_c((int)lua_tointeger(L, 2));

        if (c_idx < 0)
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        int rc = pfs_klua_ex_umount(p_ex, p_name, c_idx);

        return pfs_klua_push_rc(L, rc);
    }

    if (0 == lua_istable(L, 2))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int ok_count = 0;
    int tried = 0;
    int last_fail = PFS_OK;

    for (int i = 1; i <= PFS_PART_ENTRY_MAX; i++)
    {
        lua_rawgeti(L, 2, i);
        if (0 == lua_isinteger(L, -1) && 0 == lua_isnumber(L, -1))
        {
            lua_pop(L, 1);
            break;
        }

        int c_idx = pfs_klua_lua_part_to_c((int)lua_tointeger(L, -1));

        lua_pop(L, 1);
        if (c_idx < 0)
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }

        tried++;
        int rc = pfs_klua_ex_umount(p_ex, p_name, c_idx);

        if (PFS_OK == rc)
        {
            ok_count++;
        }
        else
        {
            last_fail = rc;
        }
    }

    if (0 == tried)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }
    if (0 == ok_count)
    {
        return pfs_klua_push_rc(L, (PFS_OK != last_fail) ? last_fail : PFS_EOPNOTSUPP);
    }

    return pfs_klua_push_rc(L, PFS_OK);
}

typedef struct pfs_klua_mount_info_all_ctx_t {
    lua_State *L;
    int mounts_idx;
    int i;
} pfs_klua_mount_info_all_ctx_t;

static void pfs_klua_mount_info_push_disk_cb(const char *p_key, int key_len, pfs_disk_t *p_disk, void *p_ctx)
{
    pfs_klua_mount_info_all_ctx_t *p_all = (pfs_klua_mount_info_all_ctx_t *)p_ctx;

    pfs_klua_push_mount_disk(p_all->L, p_key, key_len, p_disk);
    lua_rawseti(p_all->L, p_all->mounts_idx, p_all->i);
    p_all->i++;
}

static int pfs_klua_kpfs_mount_info(lua_State *L)
{
    pfs_klua_ex_t *p_ex = pfs_klua_ex_get_by_L(L);
    int top = lua_gettop(L);

    if (top == 0)
    {
        pfs_klua_mount_info_all_ctx_t all_ctx;

        lua_createtable(L, 0, 1);
        lua_createtable(L, pfs_klua_ex_disk_count(p_ex), 0);
        all_ctx.L = L;
        all_ctx.mounts_idx = lua_gettop(L);
        all_ctx.i = 1;
        pfs_klua_ex_foreach_disk(p_ex, pfs_klua_mount_info_push_disk_cb, &all_ctx);
        lua_setfield(L, -2, "mounts");
        return pfs_klua_push_info_rc_ok(L);
    }

    if (0 == lua_isstring(L, 1))
    {
        return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
    }

    const char *p_name = lua_tostring(L, 1);

    if (!pfs_klua_mount_name_ok(p_name))
    {
        return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
    }

    int name_len = (int)strlen(p_name);
    pfs_disk_t *p_disk = pfs_klua_ex_get_disk(p_ex, p_name, name_len);

    if (NULL == p_disk)
    {
        return pfs_klua_push_info_rc_fail(L, PFS_ENOENT);
    }

    if (top >= 2 && 0 == lua_isnoneornil(L, 2))
    {
        if (0 == lua_isinteger(L, 2) && 0 == lua_isnumber(L, 2))
        {
            return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
        }

        int lua_idx = (int)lua_tointeger(L, 2);

        if (lua_idx < 1)
        {
            return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
        }

        const pfs_disk_part_t *p_part = pfs_disk_part_info(p_disk, lua_idx);

        if (NULL == p_part)
        {
            return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
        }

        pfs_klua_push_mount_part_item(L, lua_idx, p_part);
        return pfs_klua_push_info_rc_ok(L);
    }

    pfs_klua_push_mount_disk(L, p_name, name_len, p_disk);
    return pfs_klua_push_info_rc_ok(L);
}

int pfs_klua_open_kpfs(lua_State *L)
{
    static const luaL_Reg s_lib[] = {
        { "version",        pfs_klua_kpfs_version },

        { "mount",          pfs_klua_kpfs_mount },
        { "remount",        pfs_klua_kpfs_remount },
        { "umount",         pfs_klua_kpfs_umount },
        { "mount_info",     pfs_klua_kpfs_mount_info },

        { NULL,             NULL }
    };

    luaL_newlib(L, s_lib);
    return 1;
}

// end
