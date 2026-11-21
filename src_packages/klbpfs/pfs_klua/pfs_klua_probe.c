// Doc Encode : UTF-8 BOM, Unix(LF)
#include "pfs_klua.h"
#include "pfs_klua_internal.h"
#include "pfs_klua_blkio.h"
#include "pfs_klua_util.h"
#include "pfsmtd.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

static void pfs_klua_probe_push_part_entry(lua_State *L, int c_part_index, const pfs_part_entry_t *p_entry,
        int64_t part_offset, int sector_size)
{
    lua_pushinteger(L, c_part_index + 1);
    lua_setfield(L, -2, "index");
    lua_pushinteger(L, (lua_Integer)part_offset);
    lua_setfield(L, -2, "part_offset");
    lua_pushinteger(L, (lua_Integer)p_entry->start_lba);
    lua_setfield(L, -2, "start_lba");
    lua_pushinteger(L, (lua_Integer)p_entry->size_lba);
    lua_setfield(L, -2, "size_lba");
    lua_pushinteger(L, (int)p_entry->mbr_sys_ind);
    lua_setfield(L, -2, "mbr_sys_ind");
    lua_pushboolean(L, p_entry->bootable ? 1 : 0);
    lua_setfield(L, -2, "bootable");
    (void)sector_size;
}

static int64_t pfs_klua_probe_part_offset(const pfs_part_entry_t *p_entry, int sector_size)
{
    if (NULL == p_entry || sector_size <= 0)
    {
        return -1;
    }
    if (p_entry->start_lba > (uint64_t)INT64_MAX / (uint64_t)sector_size)
    {
        return -1;
    }
    return (int64_t)(p_entry->start_lba * (uint64_t)sector_size);
}

static int pfs_klua_probe_fs_at(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_fs_type_e *p_fs_type,
        int *p_mount_ok, int *p_mount_err)
{
    if (NULL == p_blkio || part_offset < 0 || NULL == p_fs_type || NULL == p_mount_ok || NULL == p_mount_err)
    {
        return PFS_EINVAL;
    }

    *p_mount_ok = 0;
    *p_mount_err = PFS_OK;
    *p_fs_type = PFS_NONE;

    pfs_ctx_t *p_ctx = pfs_ctx_create();
    if (NULL == p_ctx)
    {
        *p_mount_err = PFS_ENOMEM;
        return PFS_ENOMEM;
    }
    PFSMTD_REGISTER_FS_ALL(p_ctx);

    int mount_mode = PFS_MNT_RDONLY;
    int rc = pfs_ctx_mount(p_ctx, p_blkio, part_offset, mount_mode);
    if (PFS_OK != rc)
    {
        mount_mode = PFS_MNT_RDWR;
        rc = pfs_ctx_mount(p_ctx, p_blkio, part_offset, mount_mode);
    }
    if (PFS_OK != rc)
    {
        *p_mount_err = rc;
        pfs_ctx_destroy(p_ctx);
        return rc;
    }

    *p_mount_ok = 1;
    *p_fs_type = pfs_ctx_fs_type(p_ctx);
    (void)pfs_ctx_umount(p_ctx);
    pfs_ctx_destroy(p_ctx);
    return PFS_OK;
}

static void pfs_klua_probe_push_fs_result(lua_State *L, pfs_fs_type_e fs_type, int mount_ok, int mount_err)
{
    if (0 != mount_ok)
    {
        lua_pushstring(L, pfs_klua_fs_type_name(fs_type));
    }
    else
    {
        lua_pushstring(L, "");
    }
    lua_setfield(L, -2, "fs_type");
    lua_pushboolean(L, mount_ok ? 1 : 0);
    lua_setfield(L, -2, "mount_ok");
    lua_pushinteger(L, mount_ok ? PFS_OK : mount_err);
    lua_setfield(L, -2, "mount_err");
}

static void pfs_klua_probe_push_part_table(lua_State *L, const pfs_part_table_t *p_tbl,
        pfs_blkio_t *p_blkio, int probe_fs)
{
    if (NULL == p_tbl)
    {
        return;
    }

    int sector_size = pfs_blkio_logical_sector_size(p_blkio);
    if (sector_size <= 0)
    {
        sector_size = 512;
    }

    lua_pushstring(L, pfs_klua_part_scheme_str(p_tbl->scheme));
    lua_setfield(L, -2, "scheme");
    lua_pushinteger(L, p_tbl->count);
    lua_setfield(L, -2, "part_count");

    lua_createtable(L, p_tbl->count, 0);
    for (int i = 0; i < p_tbl->count; i++)
    {
        int64_t part_offset = pfs_klua_probe_part_offset(&p_tbl->entries[i], sector_size);
        pfs_fs_type_e fs_type = PFS_NONE;
        int mount_ok = 0;
        int mount_err = PFS_OK;

        lua_createtable(L, 0, 12);
        pfs_klua_probe_push_part_entry(L, i, &p_tbl->entries[i], part_offset, sector_size);
        if (0 != probe_fs)
        {
            if (part_offset >= 0 && 0 != p_tbl->entries[i].size_lba)
            {
                (void)pfs_klua_probe_fs_at(p_blkio, part_offset, &fs_type, &mount_ok, &mount_err);
            }
            pfs_klua_probe_push_fs_result(L, fs_type, mount_ok, mount_err);
        }
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "parts");
}

static int pfs_klua_probe_run(lua_State *L, int probe_fs, int first_part_only)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
    }

    int64_t offset = (int64_t)pfs_klua_opts_integer(L, opts_idx, "offset", 0);

    pfs_klua_blkio_sess_t sess;

    pfs_klua_blkio_sess_init(&sess);
    int rc = pfs_klua_blkio_sess_open_rdonly(&sess, path);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_info_rc_fail(L, rc);
    }

    lua_createtable(L, 0, 8);
    pfs_klua_push_blkio_container(L, path, &sess.blkio);

    pfs_part_table_t part_tbl;

    rc = pfs_part_probe(&sess.blkio, &part_tbl);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        lua_pop(L, 1);
        return pfs_klua_push_info_rc_fail(L, rc);
    }

    if (part_tbl.count > 0)
    {
        if (0 != first_part_only && part_tbl.count > 1)
        {
            part_tbl.count = 1;
        }
        pfs_klua_probe_push_part_table(L, &part_tbl, &sess.blkio, probe_fs);
        if (0 != probe_fs)
        {
            lua_createtable(L, 0, 0);
            lua_setfield(L, -2, "bare_fs");
        }
    }
    else
    {
        pfs_fs_type_e fs_type = PFS_NONE;
        int mount_ok = 0;
        int mount_err = PFS_OK;

        lua_pushstring(L, "none");
        lua_setfield(L, -2, "scheme");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "part_count");
        lua_createtable(L, 0, 0);
        lua_setfield(L, -2, "parts");
        if (0 != probe_fs)
        {
            (void)pfs_klua_probe_fs_at(&sess.blkio, offset, &fs_type, &mount_ok, &mount_err);
            lua_createtable(L, 0, 4);
            lua_pushinteger(L, 1);
            lua_setfield(L, -2, "index");
            lua_pushinteger(L, offset);
            lua_setfield(L, -2, "part_offset");
            pfs_klua_probe_push_fs_result(L, fs_type, mount_ok, mount_err);
            lua_setfield(L, -2, "bare_fs");
        }
    }

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_info_rc_ok(L);
}

static int pfs_klua_probe_all(lua_State *L)
{
    return pfs_klua_probe_run(L, 1, 0);
}

static int pfs_klua_probe_part(lua_State *L)
{
    const char *path = NULL;
    int opts_idx = 0;

    if (0 != pfs_klua_parse_path_opts(L, &path, &opts_idx))
    {
        return pfs_klua_push_info_rc_fail(L, PFS_EINVAL);
    }

    pfs_klua_blkio_sess_t sess;

    pfs_klua_blkio_sess_init(&sess);
    int rc = pfs_klua_blkio_sess_open_rdonly(&sess, path);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_info_rc_fail(L, rc);
    }

    pfs_part_table_t part_tbl;

    rc = pfs_part_probe(&sess.blkio, &part_tbl);
    if (PFS_OK != rc)
    {
        pfs_klua_blkio_sess_done(&sess);
        return pfs_klua_push_info_rc_fail(L, rc);
    }

    lua_createtable(L, 0, 6);
    pfs_klua_push_blkio_container(L, path, &sess.blkio);
    pfs_klua_probe_push_part_table(L, &part_tbl, &sess.blkio, 0);

    pfs_klua_blkio_sess_done(&sess);
    return pfs_klua_push_info_rc_ok(L);
}

static int pfs_klua_probe_fs(lua_State *L)
{
    return pfs_klua_probe_run(L, 1, 1);
}

int pfs_klua_open_kpfs_probe(lua_State *L)
{
    static const luaL_Reg s_lib[] = {
        { "all",        pfs_klua_probe_all },
        { "part",       pfs_klua_probe_part },
        { "fs",         pfs_klua_probe_fs },
        { NULL,         NULL }
    };

    luaL_newlib(L, s_lib);
    return 1;
}

// end
