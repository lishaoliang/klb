// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"
#include "pfsmtd.h"

#include <stdint.h>
#include <string.h>

#define PFS_KLUA_MTD_UBI_DEFAULT_LEB_SIZE   (128u * 1024u)
#define PFS_KLUA_MTD_UBI_DEFAULT_LEB_COUNT  64u

static int pfs_klua_mtd_mkfs_one(lua_State *L, pfs_klua_blkio_sess_t *p_sess, int tbl_idx, int array_pos,
        int is_mkvol)
{
    pfs_fs_type_e fs_type = (pfs_fs_type_e)PFSMTD_SQUASHFS;
    const char *fs_str = pfs_klua_opts_string(L, tbl_idx, "fs", NULL);

    if (NULL == fs_str)
    {
        fs_str = pfs_klua_opts_string(L, tbl_idx, "type", "squashfs");
    }
    if (0 != pfs_klua_parse_mtd_fs_type(fs_str, &fs_type))
    {
        return PFS_EINVAL;
    }

    const char *label = NULL;
    const char *src_dir = NULL;
    if (0 != is_mkvol)
    {
        src_dir = pfs_klua_opts_string(L, tbl_idx, "src", NULL);
        if (NULL == src_dir || '\0' == src_dir[0])
        {
            return PFS_EINVAL;
        }
        label = pfs_klua_opts_string(L, tbl_idx, "label", NULL);
        if (NULL == label)
        {
            label = pfs_klua_opts_string(L, tbl_idx, "volume_id", NULL);
        }
    }
    else
    {
        label = pfs_klua_opts_string(L, tbl_idx, "label", NULL);
        if (NULL != label && '\0' != label[0])
        {
            return PFS_EINVAL;
        }
        label = pfs_klua_opts_string(L, tbl_idx, "volume_id", NULL);
        if (NULL != label && '\0' != label[0])
        {
            return PFS_EINVAL;
        }
    }

    int64_t part_offset;

    if (0 != pfs_klua_resolve_format_offset(L, tbl_idx, array_pos, &p_sess->blkio, &part_offset))
    {
        return PFS_EINVAL;
    }

    uint64_t vol_bytes;
    int have_vol;

    if (0 != pfs_klua_resolve_format_vol(L, tbl_idx, &p_sess->blkio, part_offset, &vol_bytes, &have_vol))
    {
        return PFS_EINVAL;
    }

    if (0 != is_mkvol)
    {
        pfsmtd_mkvol_opts_t mtd_opts = { 0 };

        mtd_opts.volume_id = label;
        mtd_opts.jffs2_erase_size = (uint32_t)pfs_klua_opts_integer(L, tbl_idx, "jffs2_erase_size", 0);
        return pfsmtd_mkvol(&p_sess->blkio, part_offset, vol_bytes, fs_type, src_dir, &mtd_opts);
    }

    return pfsmtd_mkfs(&p_sess->blkio, part_offset, vol_bytes, fs_type);
}

static int pfs_klua_mtd_mkfs_common(lua_State *L, int is_mkvol)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    int part_count = (opts_idx > 0 && pfs_klua_opts_is_part_array(L, opts_idx))
            ? pfs_klua_opts_part_array_count(L, opts_idx)
            : 1;

    if (part_count <= 0 || part_count > PFS_PART_ENTRY_MAX)
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    pfs_klua_blkio_sess_t sess;

    pfs_klua_blkio_sess_init(&sess);
    int rc = pfs_klua_blkio_sess_open_rdwr(&sess, path);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_rc(L, rc);
    }

    for (int i = 0; i < part_count; i++)
    {
        int last_rc;

        if (opts_idx > 0 && pfs_klua_opts_is_part_array(L, opts_idx))
        {
            lua_rawgeti(L, opts_idx, i + 1);
            int tbl_idx = lua_gettop(L);

            last_rc = pfs_klua_mtd_mkfs_one(L, &sess, tbl_idx, i + 1, is_mkvol);
            lua_pop(L, 1);
        }
        else if (opts_idx > 0)
        {
            last_rc = pfs_klua_mtd_mkfs_one(L, &sess, opts_idx, 1, is_mkvol);
        }
        else
        {
            lua_createtable(L, 0, 0);
            int tbl_idx = lua_gettop(L);

            last_rc = pfs_klua_mtd_mkfs_one(L, &sess, tbl_idx, 1, is_mkvol);
            lua_pop(L, 1);
        }

        if (PFS_OK != last_rc)
        {
            pfs_klua_blkio_sess_done(&sess);
            return pfs_klua_push_rc(L, last_rc);
        }
    }

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_rc(L, PFS_OK);
}

static int pfs_klua_mtd_mkfs(lua_State *L)
{
    return pfs_klua_mtd_mkfs_common(L, 0);
}

static int pfs_klua_mtd_mkvol(lua_State *L)
{
    return pfs_klua_mtd_mkfs_common(L, 1);
}

static int pfs_klua_mtd_ubi_create(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;
    int rc = pfs_klua_parse_opts_table(L, &path, &opts_idx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_klua_opts_require_force(L, opts_idx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    uint32_t leb_size = PFS_KLUA_MTD_UBI_DEFAULT_LEB_SIZE;
    uint32_t leb_count = PFS_KLUA_MTD_UBI_DEFAULT_LEB_COUNT;

    if (lua_getfield(L, opts_idx, "leb_size") != LUA_TNIL)
    {
        lua_Integer v = lua_tointeger(L, -1);

        lua_pop(L, 1);
        if (v < 512 || 0 != (v % 512))
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        leb_size = (uint32_t)v;
    }
    else
    {
        lua_pop(L, 1);
    }

    if (lua_getfield(L, opts_idx, "leb_count") != LUA_TNIL)
    {
        lua_Integer v = lua_tointeger(L, -1);

        lua_pop(L, 1);
        if (v < 1)
        {
            return pfs_klua_push_rc(L, PFS_EINVAL);
        }
        leb_count = (uint32_t)v;
    }
    else
    {
        lua_pop(L, 1);
    }

    rc = pfsmtd_ubi_create(path, leb_size, leb_count);
    return pfs_klua_push_rc(L, rc);
}

static int pfs_klua_mtd_raw_create(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;
    int rc = pfs_klua_parse_opts_table(L, &path, &opts_idx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    rc = pfs_klua_opts_require_force(L, opts_idx);
    if (PFS_OK != rc)
    {
        return pfs_klua_push_rc(L, rc);
    }

    uint64_t nbytes = 0;
    int have_size = 0;

    if (0 != pfs_klua_parse_size_bytes(L, opts_idx, "size", &nbytes, &have_size))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }
    if (0 == have_size || 0 == nbytes || 0 != (nbytes % 512ull))
    {
        return pfs_klua_push_rc(L, PFS_EINVAL);
    }

    rc = pfsmtd_raw_create(path, nbytes);
    return pfs_klua_push_rc(L, rc);
}

int pfs_klua_open_kpfs_mtd(lua_State *L)
{
    static const luaL_Reg s_ubi[] = {
        { "create",     pfs_klua_mtd_ubi_create },
        { NULL,         NULL }
    };

    static const luaL_Reg s_raw[] = {
        { "create",     pfs_klua_mtd_raw_create },
        { NULL,         NULL }
    };

    static const luaL_Reg s_lib[] = {
        { "mkfs",       pfs_klua_mtd_mkfs },
        { "mkvol",      pfs_klua_mtd_mkvol },
        { NULL,         NULL }
    };

    luaL_newlib(L, s_lib);
    luaL_newlib(L, s_ubi);
    lua_setfield(L, -2, "ubi");
    luaL_newlib(L, s_raw);
    lua_setfield(L, -2, "raw");
    return 1;
}

// end
